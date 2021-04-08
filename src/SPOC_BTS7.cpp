
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

	writeSPI(readStdDiag, "readStdDiag");
	// Response is always the result of executing the "next" command
	_stdDiagResponse = writeSPI(0b11110001, "set DCR.MUX");
	writeSPI(0b11100100, "set HWCR.COL to 1 for PWM");
	writeSPI(readWrnDiag, "readWrnDiag");
	_channelStatus = 0b10000000;	
}




// MSBFIRST, SPI_MODE_1 is required. Speed can be adjusted
// speed set at 100000 for testing - works well with the 'scope set at 200ms
SPISettings settingsA(100000, MSBFIRST, SPI_MODE1);
uint8_t stat;

uint8_t SPOC_BTS7::writeSPI(byte address, String description) {

  SPI.beginTransaction(settingsA);

  digitalWrite(_csnPin, LOW);
  delay(1);
  //stat = SPI.transfer(readStdDiag);

  stat = SPI.transfer(address);
  PRINTBIN(stat);
  Serial.println();
  delay(1);
  digitalWrite(_csnPin, HIGH);
  Serial.print(description + " ");
  PRINTBIN(address);
  Serial.print(" ");
  SPI.endTransaction();
  delay(1);
  if (bitRead(stat, 6) == 0 && bitRead(stat, 7) == 0) {
	  // we have a stdDiag message
	  //Serial.print("stdiag response");
	  //PRINTBIN(stat);
	  _stdDiagResponse = stat;
  }
  return stat;
}



void SPOC_BTS7::channelOn(byte channel, bool highCurrent) {
	// todo - set high current/low current
	bitSet(_channelStatus, channel);
	setDcrMuxChannel(channel);
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
	writeSPI(readWrnDiag, "readWrnDiag");
	return writeSPI(readStdDiag, "readStdDiag");
}

void SPOC_BTS7::setDcrMuxChannel(byte channel) {
	byte address = 0;
	switch(channel) {
		case 0:
			address = 0b11110000;
			break;
		case 1:
			address = 0b11110001;
			break;
		case 2:
			address = 0b11110010;
			break;
		case 3:
			address = 0b11110100;
			break;		
	}
	
	if (!address == _dcrMuxSetting) {
		writeSPI(address, "set DCR.MUX channel " + String(channel));
	}
	// Store the current DCR mux settings so as to not bother setting them again if they haven't changed
	_dcrMuxSetting = address;
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
	pinMode(olDetPin, OUTPUT);
	digitalWrite(olDetPin, HIGH);
	delay(1);
	setDcrMuxChannel(channel);
	getStdDiag();
	digitalWrite(olDetPin, LOW);
	//delay(1);
	if (bitRead(_channelStatus, channel) == 0) {
		// it's off
		//SBM (bit 1) is 1 in stddiag if OL_DET is HIGH, opt is off and we do not have open load!
		return !(bitRead(_stdDiagResponse, 1) == 1);
	} else {
		return (readCurrent(channel) < 5);
	}
	
	return false;
}


bool SPOC_BTS7::isShortedToVs(byte olDetPin, byte channel) {
	pinMode(olDetPin, OUTPUT);
	digitalWrite(olDetPin, LOW);
	delay(1);
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

// 5W bulb is 26, 10W is 50, therefore (approx) 21W is 65, 55W is 276 and 60W is 300
int SPOC_BTS7::readCurrent(byte channel) {
	setDcrMuxChannel(channel);
	delay(1);
	return analogRead(_isPin);	
}


SPOC_BTS7::~SPOC_BTS7() 
{
	
}