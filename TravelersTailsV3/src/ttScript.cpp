#include "ttScript.h"
#include "ttCache.h"
#include "ttTextRenderer.h"

const float k_text_margin = 24;
const float k_text_size = 64.;

ttScript ttScript::shared;

bool ttScript::setup( Json::Value& json ) {
	for ( int i = 0; i < json.size(); i++ ) {
		string name = json[ i ][ "name" ].asString();
		m_items[ name ].m_name		= name;
		m_items[ name ].m_type		= json[ i ][ "type" ].asString();
		m_items[ name ].m_content	= json[ i ][ "content" ].asString();
		if ( m_items[ name ].m_type == "text" ) {
			ofStringReplace( m_items[ name ].m_content, "<br/>", "\n" );
		}
		m_items[ name ].m_duration	= json[ i ][ "duration" ].asFloat();
		m_items[ name ].m_repeat	= json[ i ][ "repeat" ].asInt();
	}
	m_active_phase = false;
	m_phase_animation.setDuration(1.);
	m_phase_animation.setCurve(AnimCurve::EASE_OUT);
	m_scale = ofGetWidth() / 3840.f;
	return true;
}
void ttScript::update() {
	if ( m_active_phase ) {
		if ( m_phase_item.m_duration > 0 && ofGetElapsedTimef() - m_phase_start_time > m_phase_item.m_duration ) {
			//m_phase_animation.animateTo(-m_phase_bounds.height);
			m_phase_animation.animateTo(-m_phase_bounds.width);
		}
		m_phase_animation.update(1./ofGetFrameRate());
	}
}
void ttScript::draw() {
	if ( m_active_phase ) {
		//
		// draw phase
		//
		//m_phase_bounds.y = m_phase_animation;
		m_phase_bounds.x = m_phase_animation;
		if ( m_phase_image ) {
			//m_phase_image->draw(m_phase_bounds.getCenter(),m_phase_bounds.width,m_phase_bounds.height);
			m_phase_image->draw(m_phase_bounds);//.getCenter(),m_phase_bounds.width,m_phase_bounds.height);
		} else {
			//ofSetColor( 43, 188 );
			//ofSetColor( 200, 188 );
			ofSetColor(246, 228, 98, 255.*.7 );
			ofRect(m_phase_bounds);
			ofSetColor(43);
			m_phase_text_bounds.x = m_phase_bounds.x + k_text_margin*m_scale;
			m_phase_text_bounds.y = m_phase_bounds.y + k_text_margin*m_scale;
			ttTextRenderer::shared.drawTextInRect(m_phase_item.m_content,m_phase_text_bounds,k_text_size*m_scale,ttTextRenderer::HAlign::Left,ttTextRenderer::VAlign::Top);
		}
	}
}
//
//
//
void ttScript::startSession( string phase ) {
	//
	// reset repeat count
	//
	for ( auto& item : m_items ) {
		item.second.m_repeat_count = 0;
	}
	//
	//
	//
	m_phase_image = nullptr;
	m_phase = "";
	setPhase( phase );
}
void ttScript::setPhase( string phase ) {
	if ( phase != m_phase ) {
		map< string, ttScriptItem >::iterator item = m_items.find(phase);
		if ( item != m_items.end() ) {
			if ( item->second.m_repeat == 0 || item->second.m_repeat_count < item->second.m_repeat ) {
				item->second.m_repeat_count++;
				m_phase_item = item->second;
				m_phase = phase;
				m_active_phase = true;
				//
				// calculate dimensions of phase content
				//
				if ( m_phase_item.m_type == "image" ) {
					ofSetColor(255);
					m_phase_image = ttCache::shared.load("ui_images/" + m_phase_item.m_content );
					if ( m_phase_image ) {
						m_phase_bounds.width = m_phase_image->width * m_scale;
						m_phase_bounds.height = m_phase_image->height * m_scale;
					} else {
						m_phase = "";
						m_active_phase = false;
						return;
					}
				} else {
					m_phase_image = nullptr;
					/*
					ofRectangle bounds( 0, 0, ( ofGetWidth() / 3. ) * 2., ofGetHeight() );
					m_phase_text_bounds = ttTextRenderer::shared.drawTextInRect(item->second.m_content,bounds,k_text_size*m_scale,ttTextRenderer::HAlign::Centre,ttTextRenderer::VAlign::Top,true);
					m_phase_bounds.width = m_phase_text_bounds.width + ( k_text_margin * 2. * m_scale );
					m_phase_bounds.height = m_phase_text_bounds.height + ( k_text_margin * 3 * m_scale );
					*/
					ofRectangle bounds( 0, 0, ofGetWidth(), ofGetHeight() );
					m_phase_text_bounds = ttTextRenderer::shared.drawTextInRect(item->second.m_content,bounds,k_text_size*m_scale,ttTextRenderer::HAlign::Left,ttTextRenderer::VAlign::Top,true);
					m_phase_bounds.width = m_phase_text_bounds.width + ( k_text_margin * 2. * m_scale );
					m_phase_bounds.height = m_phase_text_bounds.height + ( k_text_margin * 3 * m_scale );
				}
				//
				// start animator
				//
				/*
				m_phase_bounds.x = ( ofGetWidth() - m_phase_bounds.width ) / 2.;
				m_phase_bounds.y = -m_phase_bounds.height;
				m_phase_start_time = ofGetElapsedTimef();
				m_phase_animation.setDuration( m_phase_bounds.height / ofGetHeight() );
				m_phase_animation.animateFromTo(m_phase_bounds.y,0);
				*/
				m_phase_bounds.x = - m_phase_bounds.width;
				m_phase_bounds.y = ( ofGetHeight()-m_phase_bounds.height ) / 2.;
				m_phase_start_time = ofGetElapsedTimef();
				m_phase_animation.setDuration( m_phase_bounds.width / ofGetWidth() );
				m_phase_animation.animateFromTo(m_phase_bounds.x,0);
				return;
			}
		}
		m_phase = "";
		m_active_phase = false;
	}
}