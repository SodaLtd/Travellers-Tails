#pragma once

#include "ofMain.h"
#include "ttSerialDevice.h"

class ttController : public ttSerialDevice {
public:
	ttController();
	~ttController();
	//
	//
	//
	ofVec3f getEuler() { return m_euler; };
protected:
	//
	//
	//
	ofVec3f	m_euler;
	ofVec3f m_gyro;
	//
	//
	//
	ofVec3f	m_euler_max;
	ofVec3f	m_euler_min;
	ofVec3f m_gyro_max;
	ofVec3f m_gyro_min;
	//
	//
	//
	void processBuffer() override;
	void updateMinMax();
};