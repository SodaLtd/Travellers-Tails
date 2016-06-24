#pragma once

#include "ofMain.h"
#include "ofxParametricSurface.h"
#include "ofxAnimatableFloat.h"

class ttLeaf {
public:
	enum morph_target  {
		PLANE,
		LEAF
	};
	//
	//
	//
	ttLeaf();
	~ttLeaf();
	//
	//
	//
	bool setup( string filename, float max_width, float max_height );
	void update();
	void draw();
	//
	//
	//
	float getWidth() { return m_width; };
	float getHeight() { return m_height; };
	//
	//
	//
	void morph( morph_target target, float time = 1., float delay = 0. );
	//
	// ofxParametricSurface overrides
	//
	ofPoint valueForPoint(float u,float v);
    ofVec2f texCoordForPoint(float u,float v,ofPoint value);
	//
	//
	//
	ofVec3f getNearestConnectionPoint( ofVec3f p );
	vector< ofVec3f > getConnectionPoints() { return m_connection_points; };
	//
	//
	//
protected:
	void generatePlaneControlPoints();
	void generateLeafControlPoints();
	void createMesh();
	float bezierBlend(int k, double mu, int n);
	//
	//
	//
	shared_ptr< ofImage >				m_image;
	float								m_scale;
	float								m_width;
	float								m_height;
	map< pair< float, float >, map< pair< int, int >, float > >	m_blending; // cached blending values, indexed by m_blending[ u ][ v ][ i ][ j ]
	ofVec3f								m_plane_cp[ 5 ][ 5 ];
	ofVec3f								m_leaf_cp[ 5 ][ 5 ];
	vector<ofVec3f>						m_plane_verticies;
	vector<ofVec3f>						m_leaf_verticies;
	ofVboMesh							m_mesh;
	ofxAnimatableFloat					m_morph;
	bool								m_morphing;
	morph_target						m_current_target;
	//
	//
	//
	ofVec3f								m_top_left;
	ofVec3f								m_top_right;
	ofVec3f								m_bottom_left;
	ofVec3f								m_bottom_right;
	vector< ofVec3f >					m_connection_points;

};