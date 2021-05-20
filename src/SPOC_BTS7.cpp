
#include "SPOC_BTS7.h"
#include <SPI.h>  // include the SPI library

#define PRINTBIN(Num) for (uint32_t t = (1UL<< (sizeof(Num)*8)-1); t; t >>= 1) Serial.write(Num  & t ? '1' : '0'); // Prints a binary number with leading zeros (Automatic Handling)



SPOC_BTS7::SPOC_BTS7(byte out0Pin, byte out1Pin, byte out2Pin, byte out3Pin, byte csnPin, byte isPin)
{
  _out0Pin = out0Pin;
  _out1Pin = out1Pin;
  _out2Pin = out2Pin;
  _out3Pin = out3Pin;
  _csnPin = csnPin;
  _isPin = isPin;

}

void SPOC_BTS7::begin() 
{
	SPI.begin();
	pinMode(_out0Pin, OUTPUT);
	pinMode(_out1Pin, OUTPUT);
	pinMode(_out2Pin, OUTPUT);
	pinMode(_out3Pin, OUTPUT);

	digitalWrite(_out0Pin, LOW);
	digitalWrite(_out1Pin, LOW);
	digitalWrite(_out2Pin, LOW);
	digitalWrite(_out3Pin, LOW);

	pinMode(_csnPin, OUTPUT);
	pinMode(_isPin, INPUT);

	// Response is always the result of executing the "next" command
	writeSPI(0b11110001, "set DCR.MUX");
	writeSPI(0b11100100, "set HWCR.COL to 1 for PWM");
	_channelStatus = 0b10000000;	
}




// MSBFIRST, SPI_MODE_1 is required. Speed can be adjusted
// speed set at 100000 for testing - works well with the 'scope set at 200ms
SPISettings settingsA(100000, MSBFIRST, SPI_MODE1);
uint8_t stat;

uint8_t SPOC_BTS7::writeSPI(byte address, String description) {

  SPI.beginTransaction(settingsA);

  digitalWrite(_csnPin, LOW);
  //delay(1);
  //stat = SPI.transfer(readStdDiag);

  stat = SPI.transfer(address);
  PRINTBIN(stat);
  Serial.println();
  //delay(1);
  digitalWrite(_csnPin, HIGH);
  Serial.print(description + " ");
  PRINTBIN(address);
  Serial.print(" ");
  SPI.endTransaction();
  return stat;
}



void SPOC_BTS7::channelOn(byte channel, bool highCurrent) {
	// todo - set high current/low current
	bitSet(_channelStatus, channel);
	
	// set KRC (KILIS range)
	// todo: set SWR to 0
	
	
	if (highCurrent) {
		bitClear(_krcAddressSetting, channel);
	} else {
		bitSet(_krcAddressSetting, channel);
	}
	writeSPI(_krcAddressSetting, "Set KRC");
	
	switch (channel) {
		case 0:		
			digitalWrite(_out0Pin, HIGH);
			break;
		case 1:		
			digitalWrite(_out1Pin, HIGH);
			break;
		case 2:		
			digitalWrite(_out2Pin, HIGH);
			break;
		case 3:		
			digitalWrite(_out3Pin, HIGH);
			break;
	}

	writeSPI(_channelStatus, "Channel" + String(channel) + " on");
	writeSPI(0b00000000, "Read out");
}

void SPOC_BTS7::channelOff(byte channel) {

	bitClear(_channelStatus, channel);
	switch (channel) {
		case 0:      
		  digitalWrite(_out0Pin, LOW);
		  break;
		case 1:      
		  digitalWrite(_out1Pin, LOW);
		  break;
		case 2:      
		  digitalWrite(_out2Pin, LOW);
		  break;
		case 3:      
		  digitalWrite(_out3Pin, LOW);
		  break;
	}

	writeSPI(_channelStatus, "Channel" + String(channel) + " off");
	writeSPI(0b00000000, "Read out");
	
}

byte SPOC_BTS7::getStdDiag() {	
	writeSPI(readStdDiag, "readStdDiag");
	return writeSPI(readWrnDiag, "readWrnDiag");	
}

void SPOC_BTS7::setDcrMuxChannel(byte channel) {
	byte address = 0b11110000 | channel;
	if (!address == _dcrMuxSetting) {
		writeSPI(address, "set DCR.MUX channel " + String(channel));
	}
	// Store the current DCR mux settings so as to not bother setting them again if they haven't changed
	_dcrMuxSetting = address;
}

bool SPOC_BTS7::isChannelOn(byte channel) {
	return bitRead(_channelStatus, channel);
}

void SPOC_BTS7::setPwm(byte channel, byte dutyCycle) {
	switch (channel) {
		case 0:
			analogWrite(_out0Pin, dutyCycle);
			break;
		case 1:
			analogWrite(_out1Pin, dutyCycle);
			break;
		case 2:
			analogWrite(_out2Pin, dutyCycle);
			break;
		case 3:
			analogWrite(_out3Pin, dutyCycle);
			break;
	}
}

bool SPOC_BTS7::isOpenLoad(byte olDetPin, byte channel) {
	if (isChannelOn(channel)) {
		return readCurrent(channel) < 0.05;
	} else {
			
		pinMode(olDetPin, OUTPUT);
		digitalWrite(olDetPin, HIGH);	
		
		// SBM - switch bypass monitor
		byte sbm = bitRead(getStdDiag(), 1);
		digitalWrite(olDetPin, LOW);
		return !sbm;
	}
}


bool SPOC_BTS7::isShortedToVs(byte olDetPin, byte channel) {
	pinMode(olDetPin, OUTPUT);
	digitalWrite(olDetPin, LOW);
	//delay(1);
	setDcrMuxChannel(channel);
	getStdDiag();
	//delay(1);
	if (bitRead(_channelStatus, channel) == 0) {
		// it's off
		//SBM (bit 1) is 1 in stddiag if OL_DET is HIGH, opt is off and we do not have open load!
		return !(bitRead(_stdDiagResponse, 1) == 1);
	} else {
		return (readCurrent(channel) < 5);
	}
	digitalWrite(olDetPin, LOW);
	
}

/* Read the current of a channel. 
 * Note that results are not very accurate when PWM is being used.
 * This is because the current sense is sometimes performed when the channel is off. Can't 
 * really check the "on" register for this and only read when it is on, as timings won't permit it!
 */

double SPOC_BTS7::readCurrent(byte channel) {
	setDcrMuxChannel(channel);
	//delay(20);
	
	// take 10 readings for an average. Need it when using PWM;
	// todo - figure out a method of not needing a delay for this - don't want to introduce latency
	double raw = 0; 
	
	for (byte i = 0; i <10; i++) {
		raw += analogRead(_isPin);
		delay(1);
	}
	
	raw = raw/10;
	double voltage = (raw / 1024.0) * 3.3;
	Serial.println("Current sense voltage: " + String(voltage, 6));
	// In low range KILIS mode. Channel 0 (65W)
	//5W bulb draws approx 420mA and gives 0.24V from ADC
	// Therefore 1mA, or 0.001A is 5.714e-04 V
	// Therefore current (in Amps) = v / 5.71e-04 * 1000
	// todo: high KILIS range!!!
	double current = 0;
	if (channel == 0 || channel == 3) { // 65W BTS72220-4ESA
		// In low range KILIS mode. Channel 0 (65W)
		// 5W bulb draws approx 420mA and gives 0.24V from ADC
		// Therefore 1mA, or 0.001A is 5.714e-04 V
		// Therefore current (in Amps) = v / 5.71e-04 * 1000
		current = voltage / 0.5714;
	} else {
		// 42W channels, low range
		// 5W bulb 0.52V from ADC
		// 1A is 1.22767V
		current = voltage / 1.22767;
	}
	
	return current;	
}


SPOC_BTS7::~SPOC_BTS7() 
{
	
}