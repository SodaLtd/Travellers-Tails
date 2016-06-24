#include "ttCluster.h"
#include "ttWorld.h"
#include "ttFog.h"
#include "ttClusterLoader.h"
#include "ttTextRenderer.h"

int cluster_count = 0;

const float k_base_z_offset = 0.25;

ttCluster::ttCluster() {
	cluster_count++;
	//printf( "new %d clusters\n", cluster_count );
}
ttCluster::~ttCluster() {
	cluster_count--;
	//printf( "delete %d clusters\n", cluster_count );
}
//
//
//
bool ttCluster::setup( shared_ptr<ttWorld> world, string id, ofVec3f latlon ) {
	m_world = world;
	m_id = id;
	m_latlon = latlon;
	//
	// move and rotate into position
	//
	m_position = ttWorld::getPositionFromLatLon(latlon.x,latlon.y,latlon.z);
	printf( "new cluster @ [%f,%f,%f]\n", m_position.x, m_position.y, m_position.z );
	setGlobalPosition(m_position);
	ofQuaternion orientation = ttWorld::getOrientationFromLatLon(latlon.x,latlon.y);
	setGlobalOrientation(orientation);
	/*
	//
	// add random local orientation offset
	//
	m_heading = ofRandomf()*180.;
	ofQuaternion local_rotation;
	local_rotation.makeRotate(m_heading,position);
	setGlobalOrientation(orientation*local_rotation);
	*/
	//setParent( parent, true );
	setParent( m_world->getRootNode() );
	m_selected = false;
	m_selected_item = nullptr;
	m_radius = 50.;
	m_radius_animation.setDuration(1.);
	m_radius_animation.animateTo(0.);
	m_vertical_offset.setDuration(1.);
	//
	//
	//
	m_journey = false;
	return true;
}
bool ttCluster::setup( shared_ptr<ttWorld> world, string id, ofVec3f position, ofQuaternion orientation ) {
	m_world = world;
	m_id = id;
	m_latlon = ttWorld::latLonAtRotatation(orientation.inverse(),position.z);
	//
	// move and rotate into position
	//
	m_position = position;
	printf( "new cluster @ [%f,%f,%f]\n", m_position.x, m_position.y, m_position.z );
	setGlobalPosition(m_position);
	ofQuaternion global_orientation = ttWorld::getOrientationFromLatLon(m_latlon.x,m_latlon.y);
	setGlobalOrientation(global_orientation);
	//
	//
	//
	setParent(  m_world->getRootNode(), true );
	//
	//
	//
	m_selected = false;
	m_selected_item = nullptr;
	m_radius = 50.;
	m_radius_animation.setDuration(1.);
	m_radius_animation.animateTo(0.);
	m_vertical_offset.setDuration(1.);
	//
	//
	//
	m_journey = false;

	return true;
}

void ttCluster::update() {
	//
	// 
	//
	ofVec3f camera( 0, 0, m_world->getCameraAltitude());
	ofVec3f position = getGlobalPosition();
	float distance = camera.distance(position);
	//
	// visibility
	//
	ofVec3f ray = m_world->getCameraRay().normalized();
	ofVec3f direction = ( position - camera ).normalized();
	float angle = ray.angle(direction);
	m_visible = distance < 300. && ( abs( angle ) <= m_world->getCameraFov() );
	//
	// selection
	//
	if ( m_journey || ( ( m_visible || m_world->isOnAutopilot() ) && !( isAwaitingRemoval() || isFalling() ) ) ) { 
		if (  distance < m_radius * .5 ) {
			if ( !m_selected ) {
				m_selected = true;
				m_default_colour = m_items[ 0 ]->getColour();
				for ( auto& item : m_items ) {
					item->setShowImage();
				}
				m_radius_animation.animateTo(1.);
			}
		} else if ( m_selected ) {
			m_selected = false;
			for ( auto& item : m_items ) {
				item->setShowImage(false);
			}
			m_radius_animation.animateTo(0.);
		}
	}
	//
	// apply vertical offset
	//
	ofVec3f offset = m_position.normalized() * m_vertical_offset.getCurrentValue();
	if ( offset.length() > 0 ) {
		setPosition( m_position + offset );
	}
	if ( m_selected ) {
		//
		// rotate items to face camera
		//
		ofVec3f target = getGlobalPosition();
		for ( auto& item : m_items ) {
			ofVec3f up = item->getGlobalPosition().getNormalized();
			item->lookAt(target,up);
		}
	}
	float dt = 1./ofGetFrameRate();
	//if ( !m_selected ) rotate(dt,0,0.,1.);
	float amplitude = 10.;
	float phase = ofGetElapsedTimef();
	float phase_incr = PI / m_items.size();
	for ( int i = 0; i < m_items.size(); i++ ) {
		if ( m_radius_animation.isAnimating() ) {
			//
			// TODO: should lerp from current position
			//
			float radius_animation = m_radius_animation;
			float z_offset = 10. * sin(PI*radius_animation);
			ofVec3f position( m_view_position[ i ].x * radius_animation, m_view_position[ i ].y * radius_animation, m_view_position[ i ].z + z_offset );
			m_items[ i ]->setPosition(position);
		} else if ( !m_selected ) {
			float offset = sin(phase)*amplitude;
			ofVec3f p(0,0,offset);
			//m_items[ i ]->setPosition(p);
			phase += phase_incr;
		}
		m_items[ i ]->update();
		direction = ( m_items[ i ]->getGlobalPosition() - camera ).normalized();
		angle = ray.angle(direction);
		m_items[ i ]->setVisible( abs( angle ) <= m_world->getCameraFov());
	}
	//
	//
	//
	//if ( !m_selected ) rotate(1,getPosition().normalized());
	//
	// update animations
	//
	m_radius_animation.update(dt);
	m_vertical_offset.update(dt);

}
//
//
//
void ttCluster::addItem( shared_ptr<ttNode> node ) {
	node->setParent(*this);
	m_items.push_back(node);
	updateLayout();
}
void ttCluster::removeItem( string id ) {
	vector< shared_ptr< ttNode > >::iterator it = m_items.begin();
	for ( ; it != m_items.end(); ++it ) {
		if ( it->get()->getId() == id ) {
			m_items.erase(it);
			updateLayout();
			break;
		}
	}
}
shared_ptr< ttNode > ttCluster::findItem( string id ) {
	for ( auto& item : m_items ) {
		if ( item->getId() == id ) return item;
	}
	return nullptr;
}
vector< shared_ptr< ttNode > > ttCluster::getVisibleItems() {
	vector< shared_ptr< ttNode > > visible_items;
	for ( auto item : m_items ) {
		if ( item->isVisible() ) visible_items.push_back(item);
	}
	return visible_items;
}

void ttCluster::removeItem( shared_ptr<ttNode> node ) {
	removeItem( node->getId() );
}
void ttCluster::drawItems() {
	if ( !m_selected && !m_visible ) {
		return;
	}
	for ( auto& item : m_items ) {
		item->draw();
	}
}
void ttCluster::rise() {
	m_vertical_offset.animateFromTo( -8, 0. );
}
void ttCluster::fall() {
	m_vertical_offset.animateFromTo( 0., -20. );
}
vector< string > ttCluster::getAllTags() {
	vector< string > all_tags;
	for ( auto item : m_items ) {
		vector< string > item_tags = item->getTags();
		for ( auto& tag : item_tags ) {
			if ( find( all_tags.begin(), all_tags.end(), tag ) == all_tags.end() ) { // ensure unique tags only
				all_tags.push_back(tag);
			}
		}
		///all_tags.insert(all_tags.end(),item_tags.begin(),item_tags.end());
	}
	return all_tags;
}

vector< string > ttCluster::getCommonTags() {
	map< string, int > tag_occurences;
	for ( auto item : m_items ) {
		vector< string > item_tags = item->getTags();
		for ( auto& tag : item_tags ) {
			tag_occurences[ tag ]++;
		}
	}
	vector< string > common_tags;
	for ( auto& tag : tag_occurences ) {
		if ( tag.second > 1 ) {
			common_tags.push_back( tag.first );
		}
	}
	return common_tags;
}

void ttCluster::selectItem() {
	if ( m_selected_item ) {
		ttClusterLoader::shared()->addToJourney(m_selected_item->m_match);
		if ( ttClusterLoader::shared()->getJourneyLength() >= ttClusterLoader::shared()->getMaxJourneyLength() ) {
			ttClusterLoader::shared()->loadJourney();
		} else {
			ttClusterLoader::shared()->loadCluster(m_selected_item->getId());
		}
		fall();
	}
}
shared_ptr< ttNode > ttCluster::hitTest( ofVec3f direction ) {
	ofVec3f position = getGlobalPosition();
	shared_ptr< ttNode > selected_item = nullptr;
	for ( int i = 0; i < m_items.size(); i++ ) {
		shared_ptr<ttNode> item = m_items[ i ];
		ofVec3f dim = item->getDim();
		ofVec3f item_position	= item->getGlobalPosition();
		float item_distance		= position.distance(item_position);
		ofVec3f item_direction	= m_view_position[ i ].normalized();
		float angle				= direction.angle(item_direction);
		float sweep				= dim.x * m_units_per_degree;
		if ( angle < sweep ) {
			ofVec3f target( 0., 0., k_base_z_offset - dim.y * 0.5 );
			float interp = ofLerp( 0.8, 0.0, ( angle / sweep ));
			ofVec3f p = m_view_position[ i ].interpolated( target, interp );
			p = item->getPosition().interpolated(p,0.1);
			item->setPosition(p);
			selected_item = item;
		} else {
			ofVec3f p = item->getPosition().interpolated(m_view_position[ i ],0.1);
			item->setPosition(p);
		}
	}
	if ( selected_item && selected_item != m_selected_item ) {
		m_selected_item = selected_item;
		m_selected_label = selected_item->getLabel();
		m_selected_tags = selected_item->getTags();
		m_selected_source = selected_item->getSource();
	}
	return m_selected_item;
}
//
// ofNode methods
//
void ttCluster::customDraw() {
	//ofDrawAxis(4.);
}

void ttCluster::updateLayout() {
	float total_width = 0.;
	for ( auto& item : m_items ) {
		total_width += item->getDim().x * 2.;
	}
	m_radius = ( total_width * 1.5/* * 2.0 */ ) / ( 2. * PI ); 
	float sweep_angle = 360.0;
	m_units_per_degree = sweep_angle / total_width;
	float angle = 30.;
	m_view_position.clear();
	ofVec3f target = getGlobalPosition();
	ofVec3f up( 0., 0, 1. );
	ofVec3f position( 0., m_radius, k_base_z_offset );//0. );
	for ( auto& item : m_items ) {
		ofVec3f item_dim = item->getDim();
		//
		// rotate into position
		//
		item->setPosition( position.getRotated( angle, ofVec3f::zero(), up ) );
		//
		// orientate towards center
		//
		ofVec3f item_position = item->getGlobalPosition();
		item->lookAt( target, item_position.normalized() );
		//
		// store position
		//
		m_view_position.push_back( item->getPosition() );
		angle += ( item_dim.x * 2. ) * m_units_per_degree;
	}
}
