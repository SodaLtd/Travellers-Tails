#include "ttNode.h"
#include "ttWorld.h"
#include "ttDb.h"
#include "ttCache.h"

int node_count = 0;
ttNode::ttNode() {
	node_count++;
	//printf( "new %d nodes\n", node_count );
}
ttNode::~ttNode() {
	node_count--;
	//printf( "delete %d nodes\n", node_count );
}
//
//
//
bool ttNode::setup( string id ) {
	//
	//
	//
	m_id = id;
	m_visible = true;
	//
	// load database entry
	//
#ifdef _DEBUG
	printf( "ttNode::setup : finding entry %s\n", id.c_str() );
#endif
	ttDbEntry* entry = ttDb::shared.find(m_id);
	if ( entry ) {
		string media_path = "images/" + entry->mediaPath();
#ifdef _DEBUG
		printf( "ttNode::setup : creating leaf %s\n", media_path.c_str() );
#endif
		if ( m_leaf.setup(media_path,16.,16.) ) {
			m_dim.set( m_leaf.getWidth(), m_leaf.getHeight() );
			m_label = entry->m_name;
			m_tags.insert( m_tags.end(), entry->m_tags.begin(), entry->m_tags.end() );
			/*
			for ( auto& tag : entry->m_tags ) {
				m_tags.push_back(tag);
			}
			*/
			for ( auto& colour : entry->m_colours ) {
				m_colours.push_back( ttUtil::XYZtoColor( colour.x, colour.y, colour.z ) );
			}
			m_source = entry->m_source;
			return true;
		} else {
			ofLogError("ttNode::setup") << "Unable to load node " << media_path;
		}
	} else {
		m_dim.set( ofRandom(4.0,8.0), ofRandom(4.0,8.0), ofRandom(4.0,8.0));
		ofLogError("ttNode::setup") << "no database entry for " << m_id;
	}	
	return false;
}
void ttNode::update() {
	m_leaf.update();
}
void ttNode::customDraw() {
	if ( m_visible ) {
		ofPushStyle();
		ofSetColor(255);
		m_leaf.draw();
		ofPopStyle();
	}
}
//
//
//
bool ttNode::hasTag( string& tag ) {
	return find( m_tags.begin(), m_tags.end(), tag ) != m_tags.end();
}
bool ttNode::hasColour( ofColor& colour ) {
	return find( m_colours.begin(), m_colours.end(), colour ) != m_colours.end();
}
vector< ofVec3f > ttNode::getConnectionPoints() {
	/*
	ofVec3f position			= getGlobalPosition();
	ofQuaternion orientation	= getGlobalOrientation();

	float width = m_leaf.getWidth();
	float height = m_leaf.getHeight();
	ofVec3f top_left( -( width / 2. ) + position.x, height + position.y, position.z );
	ofVec3f top_right( ( width / 2. ) + position.x, height + position.y, position.z );
	ofVec3f bottom_left( -( width / 2. ) + position.x, position.y, position.z );
	ofVec3f bottom_right( ( width / 2. ) + position.x, position.y, position.z );
	//ofMatrix4x4 transform = getGlobalTransformMatrix();
	vector< ofVec3f > connection_points;
	connection_points.push_back( top_left );
	connection_points.push_back( top_right );
	connection_points.push_back( bottom_left );
	connection_points.push_back( bottom_right );
	
	printf( "position[%f,%f,%f] top_left=[%f,%f,%f] top_right=[%f,%f,%f] bottom_left=[%f,%f,%f] bottom_right[%f %f %f]\n",
		position.x, position.y, position.z,
		connection_points[ 0 ].x, connection_points[ 0 ].y, connection_points[ 0 ].z, 
		connection_points[ 1 ].x, connection_points[ 1 ].y, connection_points[ 1 ].z, 
		connection_points[ 2 ].x, connection_points[ 2 ].y, connection_points[ 2 ].z, 
		connection_points[ 3 ].x, connection_points[ 3 ].y, connection_points[ 3 ].z ); 
	*/
	ofMatrix4x4 transform = getGlobalTransformMatrix();
	vector< ofVec3f > connection_points = m_leaf.getConnectionPoints();
	for ( int i = 0; i < connection_points.size(); i++ ) {
		connection_points[ i ] = connection_points[ i ] * transform;
	}
	/*
	ofVec3f position = getGlobalPosition();
	printf( "position[%f,%f,%f] top_left=[%f,%f,%f] top_right=[%f,%f,%f] bottom_left=[%f,%f,%f] bottom_right[%f %f %f]\n",
		position.x, position.y, position.z,
		connection_points[ 0 ].x, connection_points[ 0 ].y, connection_points[ 0 ].z, 
		connection_points[ 1 ].x, connection_points[ 1 ].y, connection_points[ 1 ].z, 
		connection_points[ 2 ].x, connection_points[ 2 ].y, connection_points[ 2 ].z, 
		connection_points[ 3 ].x, connection_points[ 3 ].y, connection_points[ 3 ].z ); 
	*/
	return connection_points;
}

ofVec3f ttNode::getNearestConnectionPoint( ofVec3f p ) {
	float min_dist = numeric_limits<float>::max();
	int nearest = -1;
	vector< ofVec3f > connection_points = getConnectionPoints();
	for ( int i = 0; i < connection_points.size(); i++ ) {
		float distance = p.distance(connection_points[i]);
		if ( distance < min_dist ) {
			min_dist = distance;
			nearest = i;
		}
	}
	return nearest >= 0 ? connection_points[ nearest ] : ofVec3f();
}



