#pragma once

#include "ofMain.h"

class ttFan {
public:
	ttFan();
	~ttFan();
	//
	//
	//
	bool setup( string port = "COM4" );
	void setSpeed( float speed ); // 0 to 1
protected:
	ofSerial	m_connection;
	int			m_speed;
};