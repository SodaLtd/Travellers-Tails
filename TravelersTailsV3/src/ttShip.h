#pragma once

#include "ofMain.h"
#include "ttSerialDevice.h"

class ttShip : public ttSerialDevice {
public:
	ttShip();
	~ttShip();
	//
	//
	//
	void setLED( bool red, bool green ); 
protected:
	void processBuffer() override;
};