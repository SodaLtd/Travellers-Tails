#include "ttTagCloud.h"
#include "ttTextRenderer.h"
#include "ttWorld.h"

const float k_tag_animation_speed = 0.1;
const float k_tag_fontsize = 2.;

ttTagCloud::ttTag::ttTag() {

}
ttTagCloud::ttTag::~ttTag() {

}
//
//
//
void ttTagCloud::ttTag::setup( string tag, float latitude, float longitude, float radius, float speed ) {
	m_tag		= tag;
	m_latitude	= latitude;
	m_longitude	= longitude;
	m_orientation = 0.;
	m_radius	= radius;
	m_speed		= speed;
	m_visible	= true;
	m_connected = false;
	m_width		= ttTextRenderer::shared.getTextWidth( m_tag, k_tag_fontsize );
}

void ttTagCloud::ttTag::update() {
	m_longitude = ofWrapDegrees(m_longitude+m_speed);

	ofVec3f right( 1., 0., 0. );
	ofVec3f up( 0., -1., 0. );
	ofVec3f position( 0., 0., m_radius );
	position.rotate(m_latitude, ofVec3f::zero(), right );
	position.rotate(m_longitude, ofVec3f::zero(), up );
	setPosition( position );
	//
	//
	//
	m_orientation =  180.-m_longitude;
	setOrientation( ofQuaternion(m_orientation, ofVec3f(0.,1.,0.) ) );
}

void ttTagCloud::ttTag::customDraw() {
	if ( m_visible ) {
		ofPushStyle();
		ofSetColor(0.,0.,0.,m_connected?200.:64);
		/*
		string tag = "#longitude ";
		tag += ofToString(m_longitude);
		tag += " : ";
		tag += ofToString(m_orientation);
		*/
		ttTextRenderer::shared.drawText( m_tag, m_connected?.25:0., 0., k_tag_fontsize, ttTextRenderer::HAlign::Left);
		if ( m_connected ) {
			ofPushStyle();
			ofSetColor(214.,214.,214.);
			ofSphere( 0.,0., 0.25 );
			ofPopStyle();
		}
		ofPopStyle();
	}
}

//
//
//
ttTagCloud::ttTagCloud() {
}
ttTagCloud::~ttTagCloud() {

}
//
//
//
void ttTagCloud::setup( shared_ptr<ttWorld> world ) {
	m_world = world;
}
void ttTagCloud::update() {
	setPosition( m_world->getPOVCamera().getGlobalPosition() );
	ofVec3f position = getGlobalPosition();
	ofVec3f target = position + ofVec3f( 0., 1., 0. );
	lookAt( target, position.normalized() );

	ofVec3f camera( 0, 0, m_world->getCameraAltitude());
	ofVec3f camera_ray = m_world->getCameraRay().normalized();
	float camera_fov = m_world->getCameraFov(); 
	for ( ttTag& tag : m_tags ) {
		tag.update();
		ofVec3f direction = ( tag.getGlobalPosition() - camera ).normalized();
		float angle = camera_ray.angle(direction);
		tag.m_visible = abs( angle ) <= camera_fov;
	}
}
void ttTagCloud::drawTags(vector< shared_ptr< ttNode > >& connected_items) {
	ofPushStyle();
	ofSetColor(0,0,0,127);
	for ( ttTag& tag : m_tags ) {
		if ( tag.m_visible ) {
			tag.m_connected = false;
			ofPushStyle();
			ofSetLineWidth( 2. );
			ofVec3f tag_position = tag.getGlobalPosition();
			
			for ( auto& item : connected_items ) {
				if ( item->hasTag( tag.m_tag ) ) {
					tag.m_connected = true;
					ofVec3f connection = item->getNearestConnectionPoint(tag_position);
					ofPushStyle();
					ofSetColor(0,0,0,84);
					ofLine( connection, tag_position );
					ofSetColor(214.,214.,214.);
					ofSphere( connection, 0.5 );
					ofPopStyle();
				}
			}
			ofPopStyle();
			tag.draw();
		}
	}
	ofSetColor( ofColor::black );
	ofSetLineWidth( 2 );
	ofLine(ofVec3f::zero(), ofVec3f( 0, 49., 0 ) );
	ofPopStyle();
}
//
//
//
void ttTagCloud::addTag( string tag ) {
	float fov =  m_world->getCameraFov() / ( 1920. / 1080. ); // view port aspectratio
	m_tags.emplace_back();
	m_tags.back().setup( tag, ofRandom( -fov/2., fov/2. ), ofRandom( -180., 180. ), 49., ( m_tags.size() % 2 ? -1. : 1. ) * k_tag_animation_speed * ofRandom(0.5,1.5) );
	m_tags.back().setParent( *this );
}
