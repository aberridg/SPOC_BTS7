
#include "SPOC_BTS7.h"
#include <SPI.h>  // include the SPI library

#define PRINTBIN(Num) for (uint32_t t = (1UL<< (sizeof(Num)*8)-1); t; t >>= 1) Serial.write(Num  & t ? '1' : '0'); // Prints a binary number with leading zeros (Automatic Handling)



SPOC_BTS7::SPOC_BTS7(byte out0Pin, byte out1Pin, byte out2Pin, byte out3Pin, byte csnPin, byte isPin, byte olDetPin)
{
  _out0Pin = out0Pin;
  _out1Pin = out1Pin;
  _out2Pin = out2Pin;
  _out3Pin = out3Pin;
  _csnPin = csnPin;
  _isPin = isPin;
  _olDetPin = olDetPin;

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
// 3Mhz seems too fast for reliable comms. 2Mhz works well and should be plenty fast enough
SPISettings settingsA(2000000, MSBFIRST, SPI_MODE1);
uint8_t stat;

uint8_t SPOC_BTS7::writeSPI(byte address, String description) {

  SPI.beginTransaction(settingsA);

  digitalWrite(_csnPin, LOW);
  //delay(1);
  //stat = SPI.transfer(readStdDiag);

  stat = SPI.transfer(address);
  if (debugMode) PRINTBIN(stat);
  if (debugMode) Serial.println();
  //delay(1);
  digitalWrite(_csnPin, HIGH);
  if (debugMode) Serial.print(description + " ");
  if (debugMode) PRINTBIN(address);
  if (debugMode) Serial.print(" ");
  SPI.endTransaction();
  return stat;
}

void SPOC_BTS7::setChannels(byte channels, byte currentSettings) {	
	for (int i = 0; i < 4 ; i++) {
		if (bitRead(channels, i)) {
			channelOn(i, bitRead(currentSettings, i));
		} else {
			channelOff(i);
		}
	}
}

void SPOC_BTS7::events() {
	if (_sinceCheck < 25) return;
	
	// check the current channel every time this function is called	
	if (isOpenLoad(_checkChannel)) {
		bitSet(_openLoadStatus, _checkChannel);
	} else {
		bitClear(_openLoadStatus, _checkChannel);
	}
	if (isOverCurrent(_checkChannel)) {
		bitSet(_overCurrentStatus, _checkChannel);
	} else {
		bitClear(_overCurrentStatus, _checkChannel);
	}
	if (isError(_checkChannel)) {
		bitSet(_errorStatus, _checkChannel);
	} else {
		bitClear(_errorStatus, _checkChannel);
	}
	
	_checkChannel = (_checkChannel++) % 4;
	_sinceCheck = 0;
}


void SPOC_BTS7::channelOn(byte channel, bool lowCurrent) {
	bitSet(_channelStatus, channel);
	
	// set DCR.SWR to 0 (so we can set KRC) and DCR.MUX to current sense verification mode
	_dcrMuxSetting = 0b11110101;
	writeSPI(_dcrMuxSetting, "DCR.SWR=0, current sense verification");
	
	// set KRC (KILIS range) and OCR (overcurrent range)
	if (lowCurrent) {
		bitSet(_krcAddressSetting, channel);
		bitSet(_ocrSetting, channel);
	} else {
		bitClear(_krcAddressSetting, channel);
		bitClear(_ocrSetting, channel);
	}
	writeSPI(_ocrSetting, "Set OCR - overcurrent");
	writeSPI(_krcAddressSetting, "Set KRC - KILIS range");
	
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

byte SPOC_BTS7::getWrnDiag() {	
	writeSPI(readWrnDiag, "readWrnDiag");
	return writeSPI(readStdDiag, "readStdDiag");	
}

byte SPOC_BTS7::getErrDiag() {
	writeSPI(readErrDiag, "readErrDiag");
	return writeSPI(readStdDiag, "readStdDiag");	
}

void SPOC_BTS7::setDcrMuxChannel(byte channel) {
	byte address = 0b11110000 | channel;
	writeSPI(address, "set DCR.MUX channel " + String(channel));	
	_dcrMuxSetting = address;
}

bool SPOC_BTS7::isChannelOn(byte channel) {
	return bitRead(_channelStatus, channel);
}

bool SPOC_BTS7::isError(byte channel) {
	return bitRead(getErrDiag(), channel);
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

bool SPOC_BTS7::isOpenLoad(byte channel) {
	if (isChannelOn(channel)) {
		return readCurrent(channel) < 0.05;
	} else {
		setDcrMuxChannel(channel);
		pinMode(_olDetPin, OUTPUT);
		digitalWrite(_olDetPin, HIGH);	
		
		// SBM - switch bypass monitor
		byte sbm = bitRead(getStdDiag(), 1);
		digitalWrite(_olDetPin, LOW);
		return !sbm;
	}
}

// Note - can only detect if shorted to VS if channel is off!
bool SPOC_BTS7::isShortedToVs(byte channel) {
	pinMode(_olDetPin, OUTPUT);
	digitalWrite(_olDetPin, LOW);
	//delay(1);
	setDcrMuxChannel(channel);
	
	//delay(1);
	if (bitRead(_channelStatus, channel) == 0) {
		// it's off
		//SBM (bit 1) is 1 in stddiag if OL_DET is HIGH, opt is off and we do not have open load!
		return !(bitRead(getStdDiag(), 1));
	}
	return  false;
}

bool SPOC_BTS7::isOverCurrent(byte channel) {
	_warningStatus = getWrnDiag();
	return bitRead(_warningStatus, channel) || readCurrent(channel) > _overCurrentSettings[channel];
}

/* Read the current of a channel. 
 * Note that results are not very accurate when PWM is being used.
 * This is because the current sense is sometimes performed when the channel is off. Can't 
 * really check the "on" register for this and only read when it is on, as timings won't permit it!
 * And this wouldn't work anyway as we need the average current
 */

double SPOC_BTS7::readCurrent(byte channel) {
	setDcrMuxChannel(channel);

	double raw = analogRead(_isPin); 
	// If we are using PWM for this channel, average several readings together...
	if (_pwmBrightnesses[channel] < 255) {
		for (byte i = 0; i < 19; i++) {
			raw += analogRead(_isPin); 
			delay(1);
		}
		raw = raw/20;
	}
	double voltage = (raw / 1024.0) * 3.3;
	if (debugMode) Serial.println("Current sense voltage: " + String(voltage, 6));
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

void SPOC_BTS7::setSoftOverCurrentSettings(byte channel, byte currentAmps) {
	_overCurrentSettings[channel] = currentAmps;
}

byte SPOC_BTS7::getAllChannelStatus() {
	return _channelStatus;
}
byte SPOC_BTS7::getAllChannelOpenLoadStatus() {
	return 	_openLoadStatus;
	
}
byte SPOC_BTS7::getAllChannelOverCurrentStatus() {
	return _overCurrentStatus;
}

byte SPOC_BTS7::getAllChannelErrorStatus() {
	return _errorStatus;
}


SPOC_BTS7::~SPOC_BTS7() 
{
	
}