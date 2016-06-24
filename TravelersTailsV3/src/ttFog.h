#pragma once

#include "ofMain.h"

class ttFog : public ofShader {
public:
	ttFog();
	~ttFog();
	//
	//
	//
	void setup();
	//
	//
	//
	static shared_ptr< ttFog > shared();
protected:
	static shared_ptr< ttFog > s_shared;

};