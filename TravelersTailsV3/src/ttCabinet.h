#pragma once

#include "ofMain.h"
#include "ofxAnimatableFloat.h"
#include "ttTextRenderer.h"

class ttCabinetItem {
public:
	void setEntry( shared_ptr< ofImage > image, string name );
	void update(float dt);
	void draw();
	//
	//
	//
	void fitRestingBounds( ofRectangle& max_bounds );
	float commitBounds( float left, float v_center ); // returns width
	//
	//
	//
	void select( bool select = true );
	void zoom( bool zoom = true );

	bool isSelected() { return m_selected; };
	bool isZoomed() { return m_zoomed; };

	inline float getScale() { return m_scale; };
	string getName() { return m_name; };
protected:
	shared_ptr< ofImage >	m_image;
	string					m_name;
	ofRectangle				m_natural_bounds;
	ofRectangle				m_resting_bounds;
	float					m_selected_scale;
	float					m_zoom_scale;
	//
	//
	//
	ofRectangle				m_zoom_bounds;
	//
	//
	//

	//
	//
	//
	ofxAnimatableFloat		m_scale;
	//
	//
	//
	bool					m_selected;
	bool					m_zoomed;
};

class ttCabinet {
public:
	ttCabinet();
	~ttCabinet();
	//
	//
	//
	void setup();
	void update();
	void draw();
	//
	//
	//
	void start();
	void end();
	//
	//
	//
	void select( int direction );
	void redButton();
	void greenButton();
private:
	ttTextRenderer					m_font;
	ofImage							m_background;
	ofImage							m_foreground;
	ofxAnimatableFloat				m_position_animation;
	ofxAnimatableFloat				m_selection_animation;
	int								m_selection;
	bool							m_zoomed;
	vector< shared_ptr< ttCabinetItem > > m_items;
};