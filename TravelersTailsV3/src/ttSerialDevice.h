#pragma once

#include "ofMain.h"

class ttSerialDevice {
public:
	ttSerialDevice();
	~ttSerialDevice();
	//
	//
	//
	bool setup( int device = 0 );
	bool setup( string port );
	void update();
	//
	//
	//
	bool isConnected() { return m_connection.isInitialized(); };
	//
	//
	//
	static map< string, string > listDevices( int min_port = 1, int max_port = 6 );
protected:
	ofSerial m_connection;
	//
	//
	//
	ostringstream m_buffer;
	virtual void processBuffer() = 0;
};