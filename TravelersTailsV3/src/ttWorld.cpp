#include "ttWorld.h"
#include "ttDb.h"
#include "ttFog.h"
#include "ttClusterLoader.h"
#include "ttTextRenderer.h"
#include "ofApp.h"

//#define DEFAULT_CLUSTERS 1
//#define LOCK_POSITION 1

const float		k_friction = 0.99;
const ofVec3f	k_center( 0., 0., 1. );

const ofVec3f	k_up( 0., 0., 1. );
const ofVec3f	k_forward( 0., 1., 0. );
const ofVec3f	k_right( 1., 0., 0. );

//const float		k_v_offset = 4.;
const float		k_v_offset = 6.;
const float		k_max_velocity = 0.1;//0.05;
const float		k_max_rotational_velocity = 1.0;//0.5;

const float		k_max_pitch = 10.;
const float		k_max_roll = 20.;

const float		k_target_frame_time = 1./60.;

ttWorld::ttWorld() {

}
ttWorld::~ttWorld() {

}
//
//
//
bool ttWorld::setup( float radius ) {
	m_scale = ofGetWidth() / 3840.f; // UI scale
	m_radius = radius;
	m_sphere.enableNormals();
	m_sphere.enableTextures();
	m_sphere.set( radius, 360 );
	//m_sky_dome.enableNormals();
	m_sky_dome.enableTextures();
	m_sky_dome.set( radius*1.5, 360 );
	/*
	m_sky_dome.set( radius*10., 360, OF_PRIMITIVE_TRIANGLES );
	ofMesh* sky_mesh = m_sky_dome.getMeshPtr();
	vector< ofIndexType >& indices = sky_mesh->getIndices();
	for ( int i = 0; i < indices.size(); i += 3 ) {
		ofIndexType temp = indices[ i ];
		indices[ i ] = indices[ i + 2 ];
		indices[ i + 2 ] = temp;
	}
	*/
	//
	//
	//
	m_position.set( 0, 0, radius );
	m_velocity = 0.;
	m_rotational_velocity = 0.;
	m_heading = 0.;
	m_overhead_offset = ( k_v_offset * 15 );
	m_pitch = 0.;
	m_roll = 0.;
	//
	//
	//
	m_autopilot = false;
	//
	//
	//
	m_active_camera = 0;
	m_cameras[ 0 ].setNearClip(0.001);
	m_cameras[ 0 ].setFarClip(14000.);
	//
	//
	//
	shared_ptr<ttWorld> world = shared_ptr<ttWorld>(this);
	m_tag_cloud.setup(world);
	//
	//
	//
#ifdef DEFAULT_CLUSTERS
	//int count = 16;
	int cluster_count = 8;
	float bearing = 0.;
	float distance = 8.;
	float increment = 360. / cluster_count;
	for ( int i = 0; i < cluster_count; i++ ) {
		shared_ptr< ttCluster > cluster = shared_ptr< ttCluster >( new ttCluster );
		ofVec3f position = pointAtBearingAndDistance( 0., 0., bearing, distance,  m_radius + k_v_offset );
		cluster->setup( world, ofToString( ofRandom( 5000 ), 0 ), position );
		int node_count = ofRandom( 6, 9 );
		for ( int j = 0; j < node_count; j++ ) {
			shared_ptr< ttNode > node = shared_ptr< ttNode >( new ttNode );
			ttDbEntry* entry = ttDb::shared.getRandom();
			while( findNode(entry->m_primary_id) ) entry = ttDb::shared.getRandom();
			node->setup( entry->m_primary_id );
			node->setShowImage(false);
			cluster->addItem( node );
		}
		m_clusters.push_back(cluster);
		bearing += 45.;
		distance += cluster->getRadius() * 1.5;
	}
#endif
	//
	//
	//
	ofSetSmoothLighting(true);
	ofVec3f sun_position = getPositionFromLatLon(0,0, m_radius + k_v_offset * 4);
	m_sun.setPosition(sun_position);
    m_sun.setDiffuseColor( ofFloatColor(1.f, 1.f, .0f) );
    m_sun.setSpecularColor( ofFloatColor(1.f, 1.f, 1.f));
	//
	//
	//
	m_locked = false;
	m_lock_cluster = nullptr;
	//
	//
	//
	m_last_time = ofGetElapsedTimef();
	//
	//
	//
	return true;
}

void ttWorld::update() {
	float dt = ofGetElapsedTimef() - m_last_time;
	float time_scale = dt / k_target_frame_time;
	//
	//
	//
	m_velocity = ofClamp( m_velocity, -k_max_velocity, k_max_velocity );
	m_rotational_velocity = ofClamp( m_rotational_velocity, -k_max_rotational_velocity, k_max_rotational_velocity );
	float velocity = m_velocity * time_scale;
	float rotational_velocity = m_rotational_velocity * time_scale;
	//
	// TODO: notching
	//
	if ( m_locked ) {
		ofVec3f ray = m_cameras[ 0 ].getLookAtDir();
		vector< ofVec3f >& view_positions = m_lock_cluster->getItemViewPoisitions();
		float min_angle = numeric_limits<float>::max();
		for ( auto& position : view_positions ) {
			ofVec3f norm = position.getNormalized();
			float angle = norm.angle( ray );
			ofVec3f cross = norm.cross( ray );
			float dot = k_up.dot( cross );
			if ( dot < 0 ) angle = -angle;
			if ( fabs(angle) < fabs(min_angle) ) {
				min_angle = angle;
			}
		}
		//printf( "min angle=%f\n", min_angle );
		if ( false ) {//fabs( min_angle ) > 0. ) {
			m_heading = ofWrapDegrees( m_heading + rotational_velocity + ( -min_angle * ( .9 / (  min_angle * min_angle ) ) ), 0., 360. );
		} else {
			m_heading = ofWrapDegrees( m_heading + rotational_velocity, 0, 360);
		}
	} else {
		m_heading = ofWrapDegrees( m_heading + rotational_velocity, 0, 360);
	}
	//
	// rotate frame around origin
	//
	ofVec3f position = k_up * ( m_radius + k_v_offset );
	ofVec3f right = k_right.getRotated(m_heading,ofVec3f::zero(),k_up);
	ofVec3f forward = k_forward.getRotated(m_heading,ofVec3f::zero(),k_up);
	//
	// rotate sphere around right
	//
	if ( m_autopilot ) {
		ofQuaternion current;
		current.slerp(m_autopilot_progress.val(),m_autopilot_start,m_autopilot_destination);
		m_sphere.setGlobalOrientation(current);
		m_autopilot = m_autopilot_progress.isAnimating();
		m_autopilot_progress.update(1./ofGetFrameRate());
	} else if ( !m_locked ) {
#ifndef LOCK_POSITION
		
		ofQuaternion current = m_sphere.getGlobalOrientation();
		ofQuaternion movement;
		movement.makeRotate(velocity,right);
		current *= movement;
		m_sphere.setGlobalOrientation(current);
		
#endif
	}
	//
	// position firstperson camera
	//
	m_cameras[ 0 ].setPosition(position);
	m_cameras[ 0 ].lookAt(position+forward,k_up);
	//
	//
	//
	if ( m_locked ) {
		//
		// raycast
		//
		ofVec3f ray = m_cameras[ 0 ].getLookAtDir();
		shared_ptr< ttNode > selected = m_lock_cluster->hitTest(ray);
		if ( selected ) {
			//printf( "%s\n", selected->getId().c_str() ); 
		}
		m_velocity = 0;
	} else {
		//
		// set pitch and roll
		//
		float target_roll = ofMap( m_rotational_velocity, getMinRotationalVelocity(), getMaxRotationalVelocity(), -k_max_roll, k_max_roll, true );
		m_roll = ofLerp(m_roll,target_roll,0.01);
		m_cameras[0].roll( m_roll );
		float target_pitch = ofMap( m_velocity, getMinVelocity(), getMaxVelocity(), -k_max_pitch, k_max_pitch, true );
		m_pitch = ofLerp(m_pitch,target_pitch,0.01);
		m_cameras[0].tilt( m_pitch );
	}
	//
	// position overhead camera
	//
	position.z += m_overhead_offset;
	m_cameras[ 1 ].setPosition(position);
	m_cameras[ 1 ].lookAt(ofVec3f::zero(),forward);
	//
	// apply friction
	//
	m_velocity *= k_friction;
	m_rotational_velocity *= k_friction;
	//
	//
	//
	m_cluster_mutex.lock();
	//
	// remove all finished clusters
	//
	for ( int i = 0; i < m_clusters.size(); ) {
		if ( m_clusters[ i ]->isAwaitingRemoval() ) {
			m_clusters.erase(m_clusters.begin()+i);
		} else {
			i++;
		}
	}
	//
	//
	//
	for ( auto& cluster : m_clusters ) {
		cluster->update();
		if ( cluster->isSelected() && m_lock_cluster != cluster ) {
			if ( !m_locked ) {
				//
				// change lock status, tell app
				// TODO: this should probably be through notifications or callbacks and should perhaps be called from setLock
				//
				((ofApp*)ofGetAppPtr())->setPhase( cluster->isJourney() ? "reveal" : "select" );
			}
			setLock();
			m_lock_cluster = cluster;
			m_velocity = m_rotational_velocity = m_pitch = m_roll = 0.;
			navigateToCluster(m_lock_cluster,1.);
			//
			// get tags from cluster and create tag cloud
			//
			m_tag_cloud.clear();
			if ( m_lock_cluster->isJourney() ) {
				vector< string > all_tags = m_lock_cluster->getAllTags();//m_lock_cluster->getCommonTags();
				for ( auto& tag : all_tags ) {
					m_tag_cloud.addTag(tag);
				}
			}
		}
	}
	m_cluster_mutex.unlock();
	//
	//
	//
	m_current_position = latLonAtRotatation(m_sphere.getGlobalOrientation(), m_radius + k_v_offset );
	//
	//
	//
	if ( m_locked && m_lock_cluster && m_lock_cluster->isJourney() ) m_tag_cloud.update();
	//
	//
	//
	m_last_time = ofGetElapsedTimef();
}
void ttWorld::draw() {
	//
	// setup common fog shader parameters
	//
	shared_ptr< ttFog > fog = ttFog::shared();
	ofVec3f camera_position = m_cameras[0].getGlobalPosition();
	ofVec3f light_position = camera_position + ( m_cameras[ 0 ].getUpDir() * 50.0 ) + ( m_cameras[ 0 ].getLookAtDir() * 100 );//100;//camera_position * 2;
	ofColor fog_colour = ofGetBackground();
	fog->begin();
	fog->setUniform3f( "eye_position", camera_position.x, camera_position.y, camera_position.z );
	fog->setUniform3f( "light_position", light_position.x, light_position.y, light_position.z );
	fog->setUniform3f( "fog_colour", fog_colour.r / 255., fog_colour.g / 255., fog_colour.b / 255. );
	//fog->setUniform3f("difuse_colour",1.0,0.54,0.0);
	fog->setUniform3f("difuse_colour",1.0,0.54,0.0);
	fog->setUniform3f("rim_colour",0.9,0.9,0.9);
	fog->end();
	//
	// render scene
	//
	ofPushStyle();
	ofEnableDepthTest();
	ofEnableAntiAliasing();
	ofEnableSmoothing();
	ofSetCircleResolution(40);
	m_cameras[m_active_camera].begin();
	//
	// draw sphere
	//
	if ( !(  m_locked && m_lock_cluster && m_lock_cluster->isJourney() ) ) { // don't draw sphere during reveal
		//
		//
		//
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);

		fog->begin();
		fog->setUniform1f("time",ofGetElapsedTimef());
		fog->setUniform1i("mode",2);	// sky mode
		ofDisableDepthTest();
		m_sky_dome.draw();
		ofEnableDepthTest();

		fog->setUniform1i("mode",1);	// sea mode
		
		m_sphere.draw();

		fog->setUniform1i("mode",0);
		fog->end();
		//glDisable(GL_CULL_FACE);
	}
	//
	// draw camera position
	//
	ofSetColor(255);
	if ( m_active_camera == 1 ) {
		ofSetColor(ofColor::orangeRed);
		ofLine( m_cameras[0].getPosition(), m_camera_target );
		ofDrawBox(m_cameras[0].getPosition(), 1. );
		ofSetColor(ofColor::aliceBlue);
		ofDrawSphere( light_position, 4. );
	}
	//
	// draw clusters
	//
	ofEnableNormalizedTexCoords();
	m_cluster_mutex.lock();
	for ( auto& cluster : m_clusters ) {
		cluster->draw();
		if ( m_locked ) {
			fog->begin();
			if ( cluster->getId() == m_lock_cluster->getId() ) {
				fog->setUniform3f("difuse_colour",0.0,0.0,0.0);
				fog->setUniform3f("rim_colour",0.0,0.0,0.0);
			} else {
				fog->setUniform3f("difuse_colour",1.0,0.54,0.0);
				fog->setUniform3f("rim_colour",0.9,0.9,0.9);
			}
			fog->end();
		}
		cluster->drawItems();
	}
	m_cluster_mutex.unlock();
	ofDisableNormalizedTexCoords();
	//
	//
	//
	
	if ( m_locked && m_lock_cluster && m_lock_cluster->isJourney() ) {
		vector< shared_ptr< ttNode > > visible_items = m_lock_cluster->getVisibleItems();
		m_tag_cloud.drawTags(visible_items);
	}
	ofDisableDepthTest();
	//
	//
	//
	m_cameras[m_active_camera].end();
	// ofDisableDepthTest();
	//
	// headup display
	// TODO: should consider moving this into a separate class which is rendered at app level, this is in progress
	//
	if ( m_locked && m_lock_cluster ) {
		if ( m_lock_cluster->isJourney() ) {
			string source = m_lock_cluster->getSelectedSource();
			string credit = "In the collection of the ";
			if ( source == "cook" ) {
				credit += "Cook Museum";
			} else if ( source == "grant" ) {
				credit += "Grant Museum";
			} else if ( source == "horniman" ) {
				credit += "Horniman Museum";
			} else if ( source == "hunterian" ) {
				credit += "Hunterian Museum";
			} else if ( source == "nmm" ) {
				credit += "National Maritime Museum";
			}
			//
			// draw title
			//
			string label = m_lock_cluster->getSelectedLabel();
			if ( credit.length() > 0 ) {
				if ( label.length() > 0 ) label += "\n";
				label += credit;
			}
			if ( label.length() > 0 ) {
				/*
				float label_width = ( 3840. / 3. ) * 2.;
				ofRectangle bounds( 0, 0, label_width*m_scale, 2160*m_scale );
				*/
				float label_width = ofGetWidth() - ( m_scale * 72. * 2. );
				ofRectangle bounds( ( m_scale * 72. ), 0, label_width, 0 );
				bounds = ttTextRenderer::shared.drawTextInRect(label, bounds, 72.*m_scale, ttTextRenderer::HAlign::Left, ttTextRenderer::VAlign::Top, true);
				bounds.y = ofGetHeight() - ( bounds.height + m_scale * 72. );

				ofSetColor( 200, 188 );
				ofRect( 0, bounds.y - ( m_scale * 72. ), bounds.width + ( m_scale * 72. * 2. ), bounds.height + ( m_scale * 72. * 2. ) ); 
				ofSetColor( 0, 188 );
				ttTextRenderer::shared.drawTextInRect(label, bounds, 72.*m_scale, ttTextRenderer::HAlign::Left, ttTextRenderer::VAlign::Top);
				/*
				bounds.y = ofGetHeight() - ( 48. + bounds.height );
				bounds.x = (ofGetWidth()-bounds.width) / 2.;
				bounds.width = label_width*m_scale;
				ttTextRenderer::shared.drawTextInRect(label,bounds, 72.*m_scale);//, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Top);
				*/
			}
			//
			//
			//
			/*
			string source = m_lock_cluster->getSelectedSource();
			if ( source.length() > 0 ) {
				shared_ptr< ofImage > logo = ttCache::shared.load("ui_images/" + source + ".png");
				if ( logo ) {
					ofSetColor(255,188);
					float scale = 1.0*m_scale; // TODO: export to settings
					float w = logo->getWidth()*scale;
					float h = logo->getHeight()*scale;
					float x = (ofGetWidth()-w*.5)-24.*m_scale;
					float y = (h*.5)+24.*m_scale;
					logo->draw(x,y,w,h);
				} else {
					ofLogError() << "Unable to load image for source : " << source;
				}
			}
			*/
		}
	}
	/*
	ofSetColor(255);
	string status = 
		"fps: " + ofToString( ofGetFrameRate(), 2. ) + 
		" position: [" + ofToString( m_current_position.x, 2 ) + "," + ofToString( m_current_position.y, 2 ) + "," + ofToString( m_current_position.z, 2 ) + "]" +
		" heading: " + ofToString( m_heading, 2. ) + 
		" velocity: " + ofToString( m_velocity, 2 ) +
		" rotational velocity: " + ofToString( m_rotational_velocity, 2 );
	ofDrawBitmapStringHighlight( status, 20, 20 );
	*/
	ofPopStyle();
}
float ttWorld::getMinVelocity() {
	return -k_max_velocity;
}
float ttWorld::getMaxVelocity() {
	return k_max_velocity;
}
float ttWorld::getMinRotationalVelocity() {
	return -k_max_rotational_velocity;
}
float ttWorld::getMaxRotationalVelocity() {
	return k_max_rotational_velocity;
}
void ttWorld::getCameraVectors( ofVec3f& position, ofVec3f& forward, ofVec3f& right, ofVec3f& up ) {
	position = k_up * ( m_radius + k_v_offset );
	right = k_right.getRotated(m_heading,ofVec3f::zero(),k_up);
	forward = k_forward.getRotated(m_heading,ofVec3f::zero(),k_up);
	up = k_up;
}
float ttWorld::getCameraAltitude() {
	return m_radius + k_v_offset;
}
//
//
//
void ttWorld::selectItem() {
	if ( m_locked && m_lock_cluster ) {
		if ( m_lock_cluster->isJourney() ) {
			//
			// start new journey
			//
			sinkAll();
			string seed_id = ttClusterLoader::shared()->getRandomId("cook");
			ttClusterLoader::shared()->startJourney(seed_id);
			ttClusterLoader::shared()->loadCluster(seed_id);
		} else {
			//
			// use current selection as seed for new cluster
			// TODO: should perhaps move this up from cluster to here or even app level
			//
			m_lock_cluster->selectItem();
		}
		setLock(false);
	}
}

//
//
//
void ttWorld::addCluster( shared_ptr< ttCluster > cluster ) {
	lock_guard<mutex> guard(m_cluster_mutex);
	m_clusters.push_back( cluster );
}
void ttWorld::removeCluster( string id ) {
	lock_guard<mutex> guard(m_cluster_mutex);
	vector< shared_ptr< ttCluster > >::iterator it = m_clusters.begin();
    for ( ; it != m_clusters.end(); ++it ) {
        if ( it->get()->getId() == id ) {
            m_clusters.erase(it);
            break;
        }
    }
}
void ttWorld::removeCluster( shared_ptr< ttCluster > cluster ) {
	removeCluster(cluster->getId());
}
shared_ptr<ttCluster> ttWorld::findCluster( float lat, float lon, float tolerance ) {
	ofVec3f position( lat, lon, m_radius + k_v_offset );
	shared_ptr<ttCluster> closest = nullptr;
	float min_distance = numeric_limits<float>::max();
	printf( "position [%f,%f]\n", position.x, position.y );
	for ( auto cluster : m_clusters ) {
		ofVec3f cluster_position = cluster->getLatLon();
		float distance = cluster_position.distance( position );
		if ( distance < tolerance && distance < min_distance ) {
			printf( "distance: %f\n", distance );
			min_distance = distance;
			closest = cluster;
		}
	}
	return closest;
}
void ttWorld::navigateToCluster( shared_ptr< ttCluster > cluster, float duration ) {
	ofVec3f latlon = cluster->getLatLon();
	navigateTo( latlon.x, latlon.y, duration );
}
void ttWorld::navigateToNearestCluster( float duration ) {
	shared_ptr< ttCluster > cluster = getNearestCluster();
	if ( cluster ) {
		navigateToCluster(cluster,duration); 
	}
}

void ttWorld::navigateTo( float lat, float lon, float duration ) {
	ofQuaternion rotation = ttWorld::getOrientationFromLatLon(lat,lon).inverse();
	if ( duration > 0. ) {
		m_autopilot_progress.reset(0.);
		m_autopilot_start = m_sphere.getGlobalOrientation();
		m_autopilot_destination = rotation;
		m_autopilot_progress.setDuration(duration);
		m_autopilot_progress.animateFromTo(0.,1.);
		m_autopilot = true;
	} else {
		m_sphere.setGlobalOrientation(rotation);
		m_autopilot = false;
	}
}
shared_ptr<ttCluster> ttWorld::getCluster( int i ) {
	if ( i < 0 || i >= m_clusters.size() ) return nullptr;
	return m_clusters[ i ];
}
shared_ptr<ttCluster> ttWorld::getNearestCluster() {
	float min_distance = numeric_limits< float >::max();
	shared_ptr<ttCluster> cluster = nullptr;
	ofVec3f position = k_up * ( m_radius + k_v_offset );
	for ( auto current : m_clusters ) {
		float distance = current->getGlobalPosition().distance(position);
		if ( distance < min_distance ) {
			min_distance = distance;
			cluster = current;
		}
	}
	return cluster;
}
int ttWorld::clusterCount() {
	return m_clusters.size();
}
bool ttWorld::hasJourneyCluster() {
	for ( auto& cluster : m_clusters ) {
		if ( cluster->isJourney() ) return true;
	}
	return false;
}
bool ttWorld::hasSelectedItem() {
	return m_locked && m_lock_cluster && m_lock_cluster->hasSelectedItem();
}
void ttWorld::sinkAll() {
	//ttClusterLoader::shared()->clearPendingLoads();
	m_cluster_mutex.lock();
	for ( auto& cluster : m_clusters ) {
		if ( !( cluster->isAwaitingRemoval() || cluster->isFalling() ) ) {
			cluster->fall();
		}
	}
	m_cluster_mutex.unlock();
}
int ttWorld::visibleClusterCount() {
	m_cluster_mutex.lock();
	int count = 0;
	for ( auto& cluster : m_clusters ) {
		if ( cluster->isVisible() && !( cluster->isAwaitingRemoval() || cluster->isFalling() ) ) {
			count++;
		}
	}
	m_cluster_mutex.unlock();
	return count;
}
//
//
//
shared_ptr< ttNode > ttWorld::findNode( string id ) {
	for ( shared_ptr< ttCluster > cluster : m_clusters ) {
		shared_ptr< ttNode > node = cluster->findItem(id);
		if ( node ) return node;
	}
	return nullptr;
}
//
//
//
ofQuaternion ttWorld::getOrientationFromLatLon( float lat, float lon ) {
	ofQuaternion latRot, lonRot;
	latRot.makeRotate(lat, 1, 0, 0);
	lonRot.makeRotate(lon, 0, 1, 0);
	return latRot * lonRot;
}

ofVec3f ttWorld::getPositionFromLatLon( float lat, float lon, float radius ) {
	return getOrientationFromLatLon(lat,lon) * ( k_center * radius );
}

ofVec3f ttWorld::getLookAtFromHeading( float lat, float lon, float radius, float heading ) {
	ofVec3f direction	= k_forward.getRotated(ofWrapDegrees( heading ),k_center);
	ofVec3f target = getPositionFromLatLon(lat+direction.x,lon+direction.y, radius);

	return target;
}
void ttWorld::clampLatLon( float& lat, float& lon ) {
	lat = ofWrapDegrees( lat, -90., 90. );
	lon = ofWrapDegrees( lon, -180., 180. );
}
ofVec3f ttWorld::pointAtBearingAndDistance( float lat, float lon, float bearing, float distance, float radius ) {
	//
	//
	//
	float dist_ratio = distance / radius;
	float dist_ratio_cos = cos( dist_ratio );
	float dist_ratio_sin = sin( dist_ratio );
	//
	//
	//
	float bearing_rad = ofDegToRad(bearing);
	//
	//
	//
    float lat_rad = ofDegToRad(lat);
	float lon_rad = ofDegToRad(lon);
	//
	//
	//
    float lat_cos = cos(lat_rad);
    float lat_sin = sin(lat_rad);
	//
	//
	//
    float end_lat = asin((lat_sin * dist_ratio_cos) + (lat_cos * dist_ratio_sin * cos(bearing_rad)));
    float end_lon = lon_rad + atan2( sin(bearing_rad) * dist_ratio_sin * lat_cos, dist_ratio_cos - lat_sin * sin(end_lat));
	//
	//
	//
    return ofVec3f( ofRadToDeg( end_lat ), ofRadToDeg( end_lon ), radius );
}
ofVec3f ttWorld::latLonAtRotatation( ofQuaternion rotation, float radius ) {
	ofVec3f c = k_center * -1.;
	ofVec3f p = rotation * c;
	ofVec3f latlon( ofRadToDeg(asin(p.y)), ofRadToDeg(-atan2(p.z,p.x))-90, radius);
	if ( latlon.y < -180. ) latlon.y += 360.;
	return latlon;
}
