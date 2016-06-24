#pragma once

#include "ofMain.h"
#include "ttWorld.h"
#include "ttController.h"
#include "ttShip.h"
#include "ttCabinet.h"

class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();
	void exit();
	//
	//
	//
	void greenButton();
	void redButton();
	void setPhase( string phase );
	string getPhase() { return m_phase; };
	void startJourney();
	void saveJourney();
	void restart();
	//
	//
	//
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
		
protected:
	shared_ptr<ttWorld>			m_world;
	bool						m_use_controller;
	ttController				m_controller;
	ttShip						m_ship;
	ttCabinet					m_cabinet;
	//
	//
	//
	float						m_last_action;
	float						m_timeout;
	string						m_timeout_phase;
	float						m_last_select;
	string						m_timeout_text;
	ofVec3f						m_current_euler;
	map< int, bool >			m_keydown;
	//
	//
	//
	float						m_last_gesture;
	bool						m_in_gesture;
	int							m_gesture_direction;
	float						m_swipe_position;
	float						m_swipe_velocity;						
	//
	//
	//
	string						m_phase;
	map< string, string >		m_phase_content; // image:filename or text:string
	//
	//
	//
	float						m_headup_font_size;
	ofTrueTypeFont				m_headup_font;
	//
	//
	//
	float						m_fullscreen_timer;
	//
	//
	//
	float						m_scale;
	//
	//
	//
	bool						m_test_mode;
	float						m_next_test;
};
