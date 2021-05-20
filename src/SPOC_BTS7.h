#ifndef SPOC_BTS7xxxx_h
#define SPOC_BTS7xxxx_h

#include "Arduino.h"

#define readStdDiag 0b00000010
#define readWrnDiag 0b00000001
#define readErrDiag 0b00000011

class SPOC_BTS7
{
  public:

    SPOC_BTS7(byte out0Pin, byte out1Pin, byte out2Pin, byte out3Pin, byte csnPin, byte isPin);
    ~SPOC_BTS7();
	
	void begin();

    
    void channelOn(byte channel, bool highCurrent);
    void channelOff(byte channel);
    void setPwm(byte channel, byte dutyCycle);
	byte getStdDiag();
	
	bool isOpenLoad(byte olDetPin, byte channel);
	/* Todo */
    
	bool isChannelOn(byte channel);
	
    bool isShortCircuit(byte olDetPin, byte channel);
	bool isShortedToVs(byte olDetPin, byte channel);
    bool isLatchedOff();
	double readCurrent(byte channel);

  private:
	void setDcrMuxChannel(byte channel);
    byte _out0Pin, _out1Pin, _out2Pin, _out3Pin, _csnPin, _isPin;
    uint8_t writeSPI(byte address, String description);
    byte _channelStatus;
	
	byte _stdDiagResponse;	
	byte _dcrMuxSetting;
	byte _krcAddressSetting = 0b11010000;

};

#endif
