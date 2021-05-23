#ifndef SPOC_BTS7xxxx_h
#define SPOC_BTS7xxxx_h

#include "Arduino.h"

#define readStdDiag 0b00000010
#define readWrnDiag 0b00000001
#define readErrDiag 0b00000011

class SPOC_BTS7
{
    using ChecksCompleteCallback = void (*)();
  public:

    SPOC_BTS7(byte out0Pin, byte out1Pin, byte out2Pin, byte out3Pin, byte csnPin, byte isPin, byte olDetPin);
    ~SPOC_BTS7();
	
	bool debugMode = false;
	
	void begin();
	
	void onChecksComplete(ChecksCompleteCallback callback) {
		_callback = callback;
	}
	
	void events();

    void setChannels(byte channels, byte currentSettings);
    void channelOn(byte channel, bool highCurrent);
    void channelOff(byte channel);
    void setPwm(byte channel, byte dutyCycle);
	void setSoftOverCurrentSettings(byte channel, byte currentAmps);
	byte getStdDiag();
	byte getWrnDiag();
	byte getErrDiag();
	
	bool isOpenLoad(byte channel);
	/* Todo */
    
	bool isChannelOn(byte channel);
	
    bool isOverCurrent(byte channel);
	bool isShortedToVs(byte channel);
	bool isError(byte channel);
    bool isLatchedOff();
	double readCurrent(byte channel);
	
	byte getAllChannelStatus();
	byte getAllChannelOpenLoadStatus();
	byte getAllChannelOverCurrentStatus();
	byte getAllChannelErrorStatus();

  private:
	void setDcrMuxChannel(byte channel);
    byte _out0Pin, _out1Pin, _out2Pin, _out3Pin, _csnPin, _isPin, _olDetPin;
    uint8_t writeSPI(byte address, String description);
    byte _channelStatus;
	
	byte _stdDiagResponse;	
	byte _dcrMuxSetting;
	byte _krcAddressSetting = 0b11010000;
	byte _ocrSetting = 0b11000000;
	ChecksCompleteCallback _callback;
	byte _checkChannel = 0;
	byte _openLoadStatus = 0;
	byte _overCurrentStatus = 0;
	elapsedMillis _sinceCheck = 0;
	byte _pwmBrightnesses[4] = {255, 255, 255, 255};
	byte _warningStatus = 0;
	byte _errorStatus = 0;
	byte _overCurrentSettings[4] = {7, 4, 4, 7};
	

};

#endif
