
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
	digitalWrite(_csnPin, HIGH);
	writeSPI(readStdDiag, "readStdDiag");
	writeSPI(0b11110001, "set DCR.MUX");
	writeSPI(0b11100100, "set HWCR.COL to 1 for PWM");
	writeSPI(readWrnDiag, "readWrnDiag");
	_channelStatus = 0;	
}




// MSBFIRST, SPI_MODE_1 is required. Speed can be adjusted
// speed set at 100000 for testing - works well with the 'scope set at 200ms
SPISettings settingsA(100000, MSBFIRST, SPI_MODE1);
uint8_t stat;

uint8_t SPOC_BTS7::writeSPI(byte address, String description) {

  SPI.beginTransaction(settingsA);

  digitalWrite(_csnPin, LOW);
  delay(10);
  //stat = SPI.transfer(readStdDiag);

  stat = SPI.transfer(address);
  PRINTBIN(stat);
  Serial.println();
  delay(10);
  digitalWrite(_csnPin, HIGH);
  Serial.print(description + " ");
  PRINTBIN(address);
  Serial.print(" ");
  SPI.endTransaction();
  delay(10);
  return stat;
}



void SPOC_BTS7::channelOn(byte channel, bool highCurrent) {


  switch (channel) {
	bitSet(_channelStatus, channel);
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

bool isOpenLoad(byte channel) {
	
	
	
}


SPOC_BTS7::~SPOC_BTS7() 
{
	
}