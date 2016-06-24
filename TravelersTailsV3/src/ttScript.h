#pragma once
#include "ofMain.h"
#include "ofxJSONElement.h"
#include "ofxAnimatableFloat.h"

class ttScriptItem {
public:
	ttScriptItem() {};
	ttScriptItem( const ttScriptItem& other ) {

	}
	string	m_name;
	string	m_type;
	string	m_content;
	float	m_duration; // 0 = permanent, > 0 = disappear after m_duration seconds
	int		m_repeat;	// this is repeats per sesssion 
	//
	//
	//
	int		m_repeat_count;
};

class ttScript {
public:
	ttScript() {};
	~ttScript() {};
	//
	//
	//
	bool setup( Json::Value& json );
	void update();
	void draw();
	//
	//
	//
	void startSession( string phase );
	void setPhase( string phase );
	bool hasPhase( string phase ) { return m_items.find(phase) != m_items.end(); };
	//
	//
	//
	static ttScript shared;
protected:
	map< string, ttScriptItem > m_items;
	string						m_phase;
	ofxAnimatableFloat			m_phase_animation;
	float						m_phase_start_time;
	bool						m_active_phase;
	shared_ptr<ofImage>			m_phase_image;
	ofRectangle					m_phase_text_bounds;
	ofRectangle					m_phase_bounds;
	ttScriptItem				m_phase_item;
	float						m_scale;
};
