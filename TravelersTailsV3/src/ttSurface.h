#pragma once

#include "ofMain.h"
#include "ofxParametricSurface.h"

class ttSurface : public ofxParametricSurface {
public:
	ttSurface();
	~ttSurface();
	//
	//
	//
	void setup( map< string, float > param, function<ofPoint(map< string, float >&,float,float)> func);
	//
	//
	//
	virtual ofPoint valueForPoint(float u,float v) override;
    virtual ofVec2f texCoordForPoint(float u,float v,ofPoint value) override;
    virtual ofVec2f backTexCoordForPoint(float u,float v, ofPoint value) override;
protected:
	map< string, float >									m_param;
	function<ofPoint(map< string, float >&,float,float)>	m_func;
};