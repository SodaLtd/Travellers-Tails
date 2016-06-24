#pragma once

#include "ofMain.h"
#include "ttSurface.h"
#include "ttLeaf.h"
#include "ttDb.h"

class ttNode : public ofNode {
public:
	ttNode();
	virtual ~ttNode();
	//
	//
	//
	bool setup( string id );
	void update();
	//
	//
	//
	string getId() { return m_id; };
	ofVec3f getDim() { return m_dim; };
	ofColor getColour() { return m_colour; };
	void setDim( ofVec3f dim ) { m_dim = dim; };
	void setColour( ofColor colour ) { m_colour = colour; };
	string getLabel() { return m_label; };
	vector< string > getTags() { return m_tags; };
	string getSource() { return m_source; };
	//
	//
	//
	bool hasTag( string& tag );
	bool hasColour( ofColor& color );
	vector< ofVec3f > getConnectionPoints();
	//
	//
	//
	void setShowImage(bool show=true) { m_leaf.morph(show?ttLeaf::PLANE:ttLeaf::LEAF);};
	//
	//
	//
	void setVisible(bool visible=true) { m_visible = visible; };
	bool isVisible() { return m_visible; };
	//
	//
	//
	ofVec3f getNearestConnectionPoint( ofVec3f p );
	//
	// ofNode methods
	//
	void customDraw() override;
	//
	// TODO: find a better place for this
	//
	ttDbMatchResult m_match;
	//
	//
	//
protected:
	//
	//
	///
	//
	//
	//
	string							m_id;
	ofVec3f							m_dim;
	ofColor							m_colour;
	ttLeaf							m_leaf;
	ofImage							m_image;
	string							m_label;
	vector< string >				m_tags;
	vector< ofColor >				m_colours;
	string							m_source;
	bool							m_visible;

};