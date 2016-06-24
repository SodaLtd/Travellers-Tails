#pragma once

#include "ofMain.h"
#include "ttCluster.h"
/*
#include "ofxRipples.h"
#include "ofxFire.h"
*/
#include "ttTagCloud.h"

#include <thread>
#include <mutex>

class ttWorldItem {
public:
	ttWorldItem();
	virtual ~ttWorldItem();
	//
	//
	//
	bool setup( ofVec2f& position, string filename );
	void update();
	void draw();
};

class ttWorld {
public:
	ttWorld();
	virtual ~ttWorld();
	//
	//
	//
	bool setup( float radius = 300. );
	void update();
	void draw();
	//
	//
	//
	void addCluster( shared_ptr< ttCluster > cluster );
	void removeCluster( string id );
	void removeCluster( shared_ptr< ttCluster > cluster );
	shared_ptr<ttCluster> findCluster( float lat, float lon, float tolerance = 50. );
	void navigateToNearestCluster( float duration = 0. );
	void navigateToCluster( shared_ptr< ttCluster > cluster, float duration = 0. );
	void navigateTo( float lat, float lon, float duration = 0. );
	shared_ptr<ttCluster> getCluster( int i );
	shared_ptr<ttCluster> getNearestCluster();
	int clusterCount();
	bool hasJourneyCluster();
	bool hasSelectedItem();
	void sinkAll();
	bool isOnAutopilot() { return m_autopilot; };
	int visibleClusterCount();
	bool hasVisibleClusters() { return visibleClusterCount() > 0; };
	//
	//
	//
	ofNode& getRootNode() { return m_sphere; };
	//
	//
	//
	shared_ptr< ttNode > findNode( string id );
	//
	// navigation
	//
	void accelerate( float speed ) { m_velocity += speed; }
	void turn( float speed ) { m_rotational_velocity += speed; }
	void setVelocity( float velocity ) { m_velocity = velocity; }; 
	float getVelocity() { return m_velocity; };
	void setRotationalVelocity( float velocity ) { m_rotational_velocity = velocity; }; 
	float getRotationalVelocity() { return m_rotational_velocity; };
	float getMinVelocity();
	float getMaxVelocity();
	float getMinRotationalVelocity();
	float getMaxRotationalVelocity();
	ofVec3f getCurrentPosition() { return m_current_position; };
	float getCurrentHeading() { return m_heading; };
	void getCameraVectors( ofVec3f& position, ofVec3f& forward, ofVec3f& right, ofVec3f& up );
	float getCameraAltitude();
	ofVec3f getCameraRay() { return m_cameras[ 0 ].getLookAtDir(); };
	float getCameraFov() { return m_cameras[ 0 ].getFov(); };
	ofNode& getPOVCamera() { return m_cameras[ 0 ]; };
	//
	//
	//
	void toggleCamera() { m_active_camera = !m_active_camera; };
	void zoomOverhead( float direction ) { m_overhead_offset += 4.0 * direction; };
	void setLock( bool lock = true ) { m_locked = lock; };
	void sinkLock() { if ( m_lock_cluster ) m_lock_cluster->fall(); };
	void selectItem();
	//
	//
	//
	static ofQuaternion getOrientationFromLatLon( float lat, float lon );
	static ofVec3f getPositionFromLatLon( float lat, float lon, float radius );
	static ofVec3f getLookAtFromHeading( float lat, float lon, float radius, float heading );
	static void clampLatLon( float& lat, float& lon );
	static void clampLatLon( ofVec3f& p ) { clampLatLon( p.x, p.y ); };
	static ofVec3f pointAtBearingAndDistance( float lat, float lon, float bearing, float distance, float radius );
	static ofVec3f ttWorld::latLonAtRotatation( ofQuaternion rotation, float radius );
protected:
	//
	//
	//
	float					m_scale;
	//
	//
	//
	float					m_radius;
	ofSpherePrimitive		m_sphere;
	ofSpherePrimitive		m_sky_dome;
	//
	//
	//
	int			m_active_camera;
	float		m_overhead_offset;
	float		m_pitch;
	float		m_roll;
	ofCamera	m_cameras[2];
	ofVec3f		m_current_position;
	//
	//
	//
	bool				m_autopilot;
	ofxAnimatableFloat	m_autopilot_progress;
	ofQuaternion		m_autopilot_start;
	ofQuaternion		m_autopilot_destination;
	//
	//
	//
	ofLight		m_sun;
	//
	//
	//
	ofVec3f		m_position;				// latitude and longitude
	float		m_velocity;				// speed in degrees / second ? 
	float		m_heading;				// heading 0 to 360
	float		m_rotational_velocity;	// rotation about position
	float		m_last_time;
	//
	//
	//
	ofVec3f		m_camera_target;
	//
	//
	//
	mutex								m_cluster_mutex;
	vector< shared_ptr< ttCluster > >	m_clusters;

	vector< ofVec3f	>					m_history;
	//
	//
	//
	bool					m_locked;
	shared_ptr< ttCluster > m_lock_cluster;
	//
	//
	//
	ofShader				m_shader;
	//
	//
	//
	ttTagCloud				m_tag_cloud;
};