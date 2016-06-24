#pragma once

#include "ofMain.h"
#include "ttNode.h"
#include "ofxAnimatableFloat.h"

class ttWorld;

class ttCluster : public ofNode {
public:
	ttCluster();
	virtual ~ttCluster();
	//
	//
	//
	bool setup( shared_ptr<ttWorld> world, string id, ofVec3f latlon );
	bool setup( shared_ptr<ttWorld> world, string id, ofVec3f position, ofQuaternion orientation );
	void update();
	//
	//
	//
	void addItem( shared_ptr<ttNode> node );
	void removeItem( shared_ptr<ttNode> node );
	void removeItem( string id );
	shared_ptr< ttNode > findItem( string id );
	int itemCount() { return m_items.size(); };
	vector< shared_ptr< ttNode > > getVisibleItems();
	//
	//
	//
	vector< ofVec3f >& getItemViewPoisitions() { return m_view_position; };
	vector< ofVec3f >& getItemSelectPoisitions() { return m_select_position; };
	//
	//
	//
	void select(bool select=true) { m_selected = select; };
	bool isSelected() { return m_selected; };
	//
	//
	//
	bool isFalling() { return m_vertical_offset.getTargetValue() < 0. && m_vertical_offset.isAnimating(); };
	bool isAwaitingRemoval() { return m_vertical_offset.getTargetValue() < 0. && m_vertical_offset.hasFinishedAnimating(); };
	//
	//
	//
	bool isJourney() { return m_journey; };
	void setJourney(bool journey=true) { m_journey = journey; };
	//
	//
	//
	bool isVisible() { return m_visible; };
	void setVisible( bool visible = true ) { m_visible = visible; };
	//
	//
	//
	string getSelectedLabel() { return m_selected_label; };
	vector< string > getSelectedTags() { return m_selected_tags; };
	string getSelectedSource() { return m_selected_source; };
	vector< string > getAllTags();
	vector< string > getCommonTags();
	//
	//
	//
	void selectItem();
	shared_ptr< ttNode > hitTest( ofVec3f direction ); 
	bool hasSelectedItem() { return m_selected_item != nullptr; };
	//
	//
	//
	void rise();
	void fall();
	//
	//
	//
	void drawItems();
	//
	//
	//
	string getId() { return m_id; };
	float getRadius() { return m_radius; };
	float getHeading() { return m_heading; };
	ofVec3f getLatLon() { return m_latlon; };
	//
	// ofNode methods
	//
	void customDraw() override;
	//
	//
	//
protected:
	//
	//
	//
	void updateLayout();
	//
	//
	//
	shared_ptr<ttWorld>				m_world;
	string m_id;
	ofVec3f m_latlon;
	ofVec3f m_position;
	vector< shared_ptr< ttNode > >	m_items;
	shared_ptr< ttNode >			m_selected_item;
	vector< shared_ptr< ttNode > >	m_deselected_items;
	vector< ofVec3f >				m_view_position;
	vector< ofVec3f >				m_select_position;
	//
	//
	//
	float	m_radius;
	float	m_units_per_degree;
	bool	m_selected;
	ofColor m_default_colour;
	float	m_heading;
	//
	//
	//
	bool				m_journey;
	string				m_selected_label;
	vector< string >	m_selected_tags;
	string				m_selected_source;
	//
	//
	//
	ofxAnimatableFloat	m_radius_animation;
	ofxAnimatableFloat	m_vertical_offset;
	//
	//
	//
	bool				m_visible;
};