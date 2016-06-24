#pragma once

#include "ofMain.h"

class ttHeadupDisplay {
public:
	ttHeadupDisplay() {};
	~ttHeadupDisplay() {};
	//
	//
	//
	void setup();
	void update();
	void draw();
	//
	//
	//
	void clearJourney() { m_journey_images.clear(); };
	//
	//
	//
	void setLabel( string name, string value = "" );
	//
	// bodge for cabinet
	//
	ofImage* getIcon( string name );
	//
	//
	//
	static ttHeadupDisplay shared;
protected:
	float					m_scale;
	ofRectangle				m_background;
	vector< ofRectangle >	m_thumbnails;
	map< string, string >	m_labels;
	map< string, ofImage >	m_icons;
	map< string, shared_ptr< ofImage > > m_journey_images;
};
