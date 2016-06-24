#include "ttCabinet.h"
#include "ttClusterLoader.h"
#include "ttHeadupDisplay.h"
#include "ofApp.h"

const float k_frame_width	= 3840.;
const float k_frame_height	= 2160.;
const float k_margin_left	= 167.;
const float k_margin_right	= 180.;
const float k_margin_top	= 362.;
const float k_margin_bottom = 135.;
const float k_v_centre		= 1196.;
const float k_item_spacing	= 135.;
const float k_shadow_dim	= k_item_spacing * .75;

const ofRectangle k_max_zoom_bounds( k_margin_left + k_item_spacing, k_margin_top + k_item_spacing, k_frame_width - (k_margin_left+k_margin_right+k_item_spacing*2.), k_frame_height - (k_margin_top+k_margin_bottom+k_item_spacing*2.) );
const ofRectangle k_title_bounds( 305., 83., 2154., 170. );
const ofRectangle k_red_indicator_bounds( 2719., 95., 120., 120. );
const ofRectangle k_green_indicator_bounds( 3003., 95., 120., 120. );
const ofRectangle k_red_label_bounds( 2680., 234., 198., 62. );
const ofRectangle k_green_label_bounds( 2956., 232., 202., 62. );

const ofColor k_label_background(187,173,82);

const float k_label_font_size = 36.;

void ttCabinetItem::setEntry( shared_ptr< ofImage > image, string name ) {
	m_image = image;
	m_natural_bounds.set(0,0,m_image->getWidth(),m_image->getHeight());
	m_selected = m_zoomed = false;
	m_name = name;
}
void ttCabinetItem::update(float dt) {
	m_scale.update(dt);
}
void ttCabinetItem::draw() {
	//
	// scale
	//
	float current_scale = m_scale;
	ofRectangle current_bounds = m_resting_bounds;
	current_bounds.scaleFromCenter(current_scale);
	//
	// fit within frame
	//
	if ( current_bounds.getLeft() < k_max_zoom_bounds.getLeft() ) current_bounds.x = k_max_zoom_bounds.getLeft();
	if ( current_bounds.getRight() > k_max_zoom_bounds.getRight() ) current_bounds.x = k_max_zoom_bounds.getRight() - current_bounds.width;
	if ( current_bounds.getTop() < k_max_zoom_bounds.getTop() ) current_bounds.y = k_max_zoom_bounds.getTop();
	if ( current_bounds.getBottom() > k_max_zoom_bounds.getBottom() ) current_bounds.y = k_max_zoom_bounds.getBottom() - current_bounds.height;
	//
	//
	//
	if ( current_scale > 1.f ) {
		//
		// draw shadow
		//
		ofPushStyle();
		ofRectangle shadow_bounds = current_bounds;
		float shadow_scale = 1. + ( .5 * ( current_scale / m_zoom_scale ) );
		shadow_bounds.width += k_shadow_dim * shadow_scale;
		shadow_bounds.height += k_shadow_dim * shadow_scale;
		ofSetColor(ofColor::black,64);
		ofFill();
		ofRect(shadow_bounds);
		ofPopStyle();
	}
	//m_image->setAnchorPercent(0,0);
	m_image->draw(current_bounds);
	//m_image->setAnchorPercent(.5,.5);
	/*
	if ( m_zoomed ) {
		ofPushStyle();
		ofSetColor(ofColor::red);
		ofNoFill();
		ofRect( m_zoom_bounds );
		ofPopStyle();
	}
	*/
}
void ttCabinetItem::fitRestingBounds( ofRectangle& target_bounds ) {
	m_resting_bounds = m_natural_bounds;
	m_resting_bounds.scaleTo(target_bounds); 
}
float ttCabinetItem::commitBounds( float left, float v_center ) {
	//
	// offset resting bounds
	//
	m_resting_bounds.x = left;
	m_resting_bounds.y = v_center - m_resting_bounds.height / 2.;
	//
	// calculate selected bounds
	//
	/*
	ofRectangle selected_bounds = m_resting_bounds;
	selected_bounds.scaleFromCenter((k_item_spacing+m_resting_bounds.width)/m_resting_bounds.width);
	m_selected_scale = selected_bounds.width / m_resting_bounds.width;
	*/
	m_selected_scale = (k_item_spacing+m_resting_bounds.width)/m_resting_bounds.width;
	//
	// calculate zoomed bounds
	//
	m_zoom_bounds = m_resting_bounds;
	m_zoom_bounds.scaleTo(k_max_zoom_bounds);
	m_zoom_scale = min( k_max_zoom_bounds.width / m_resting_bounds.width, k_max_zoom_bounds.height / m_resting_bounds.height );
	printf( "hscale: %f vscale: %f zoom_scale: %f\n", m_zoom_bounds.width / m_resting_bounds.width, m_zoom_bounds.height / m_resting_bounds.height, m_zoom_scale );
	//
	// default to resting bounds
	//
	m_scale.setDuration(0.5);
	m_scale.setCurve(OBJECT_DROP);
	m_scale.reset( 1. );
	//
	//
	//
	return m_resting_bounds.width;
}
void ttCabinetItem::select( bool select ) {
	if ( select != m_selected ) {
		m_selected = select;
		if ( m_selected ) {
			m_scale.animateTo( m_selected_scale );
		} else {
			m_scale.animateTo(1.);
		}
	}
	if ( !m_selected ) m_zoomed = false;
}
void ttCabinetItem::zoom( bool zoom ) {
	if ( zoom != m_zoomed ) {
		m_zoomed = zoom;
		if ( m_zoomed ) {
			m_scale.animateTo( m_zoom_scale );
			printf( "zoom scale: %f\n", m_zoom_scale );
		} else {
			m_scale.animateTo( m_selected ? m_selected_scale : 1. );
		}
	}
}

ttCabinet::ttCabinet() {
}
ttCabinet::~ttCabinet() {

}
//
//
//
void ttCabinet::setup() {
	m_font.setup("fonts/times.ttf", 0.9);
	m_background.loadImage("ui_images/hornimanbackpanel.png");
	m_foreground.loadImage("ui_images/hornimanframe.png");
	m_selection = -1;
	m_zoomed = false;
}
void ttCabinet::update() {
	float dt = 2./ofGetFrameRate();
	//
	// update cabinet animation
	//
	m_position_animation.update(dt);
	//
	// update item animation
	//
	for ( auto& item : m_items ) {
		item->update(dt);
	}
}
bool sort_by_scale (shared_ptr< ttCabinetItem > a, shared_ptr< ttCabinetItem > b ) { 
	return (a->getScale()<b->getScale());
}

void ttCabinet::draw() {
	float scale = ofGetWidth() / k_frame_width;
	ofPushMatrix();
	ofPushStyle();
	ofSetColor(255);
	ofTranslate(m_position_animation,0.);
	ofScale( scale, scale );
	//
	//
	//
	m_background.draw(0,0);
	//
	//
	//
	string title = "Your Collection";
	vector< shared_ptr< ttCabinetItem > > scale_sorted = m_items;
	sort(scale_sorted.begin(),scale_sorted.end(),sort_by_scale);
	for ( auto& item : scale_sorted ) {
		item->draw();
		if ( item->isZoomed() ) {
			title = item->getName();
		}
	}
	//
	//
	//
	m_foreground.draw(0,0);
	//
	// draw title
	//
	if ( title.length() > 0 ) {
		ofPushStyle();
		ofSetColor( 0 );
		/*
		if ( title.length() > 256 ) {
			title.resize(256);
			title += "...";
		}
		*/
		float font_size = 64.;
		ofRectangle title_bounds;
		while( font_size > 16. ) {
			title_bounds = m_font.drawTextInRect( title, k_title_bounds, font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre, true );
			if ( title_bounds.height <= k_title_bounds.height ) {
				break;
			} else {
				font_size -= 8.;
			}
		}
		if ( font_size <= 16. ) {
			title.resize( 128 );
			title += "...";
			m_font.drawTextInRect( title, k_title_bounds, 32., ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
		} else {
			m_font.drawTextInRect( title, k_title_bounds, font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
		}
		/*
		ofNoFill();
		ofSetColor( ofColor::red );
		ofRect( k_title_bounds );
		ofSetColor( ofColor::green );
		ofRect( title_bounds );
		*/
		ofPopStyle();
	}
	//
	// draw buttons
	//
	string app_phase = ((ofApp*)ofGetAppPtr())->getPhase();
	bool restart = app_phase == "confirm_restart";
	bool timeout = app_phase == "timeout";
	ofImage* gray_button = ttHeadupDisplay::shared.getIcon("gray_button");
	ofImage* red_button = ttHeadupDisplay::shared.getIcon("red_button");
	ofImage* green_button = ttHeadupDisplay::shared.getIcon("green_button");
	if ( red_button ) red_button->draw(k_red_indicator_bounds);
	if ( green_button ) green_button->draw(k_green_indicator_bounds);
	ofPushStyle();
	ofSetColor( k_label_background );
	ofRect( k_red_label_bounds );
	ofRect( k_green_label_bounds );
	ofSetColor( 0 );
	if ( restart ) {
		m_font.drawTextInRect( "YES", k_red_label_bounds, k_label_font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
		m_font.drawTextInRect( "NO", k_green_label_bounds, k_label_font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
	} else if ( timeout ) {
		m_font.drawTextInRect( "NO", k_red_label_bounds, k_label_font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
		m_font.drawTextInRect( "YES", k_green_label_bounds, k_label_font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
	} else {
		m_font.drawTextInRect( "RESTART", k_red_label_bounds, k_label_font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
		m_font.drawTextInRect( m_zoomed ? "SHRINK" : "ZOOM", k_green_label_bounds, k_label_font_size, ttTextRenderer::HAlign::Centre, ttTextRenderer::VAlign::Centre );
	}
	/*
	ofSetColor( ofColor::red );
	ofNoFill();
	ofRect( k_red_label_bounds );
	ofRect( k_green_label_bounds );
	*/
	ofPopStyle();
	//
	//
	//
	ofPopStyle();
	ofPopMatrix();

}
//
//
//
void ttCabinet::start() {
	//
	//
	//
	m_selection = -1;
	m_zoomed = 0;
	m_items.clear();
	m_position_animation.animateFromTo(-k_frame_width,0.);

	//
	// load images
	//
	vector< ttDbMatchResult > journey = ttClusterLoader::shared()->getJourney();
	for ( auto& journey_item : journey ) {
		string image_path = "images/" + journey_item.m_entry->mediaPath();
		shared_ptr< ofImage > image = ttCache::shared.load(image_path);
		if ( image ) {
			shared_ptr<ttCabinetItem> item(new ttCabinetItem);
			string name = journey_item.m_entry->m_name;
			string source = journey_item.m_entry->m_source;
			if ( source == "cook" ) {
				name += ": Cook Museum";
			} else if ( source == "grant" ) {
				name += ": Grant Museum";
			} else if ( source == "horniman" ) {
				name += ": Horniman Museum";
			} else if ( source == "hunterian" ) {
				name += ": Hunterian Museum";
			} else if ( source == "nmm" ) {
				name += ": National Maritime Museum";
			}

			item->setEntry( image, name );
			m_items.push_back(item);
		} else {
			ofLogError("ttCabinet::start") << " Unable to load image '" << image_path << "'";
		}
	}
	//
	// layout items
	//
	float total_spacing = k_item_spacing * ( m_items.size() + 1 );
	float max_width = (k_frame_width - (k_margin_left+k_margin_right+total_spacing))/m_items.size();
	float max_height = k_frame_height - (k_margin_top+k_margin_bottom+k_item_spacing*2.);
	ofRectangle max_bounds( 0, 0, max_width, max_height );
	for ( auto& item : m_items ) {
		item->fitRestingBounds(max_bounds);
	}
	float left = k_margin_left + k_item_spacing;
	float v_centre = k_margin_top + k_item_spacing + max_height / 2.;
	for ( auto& item : m_items ) {
		left += item->commitBounds(left,v_centre)+k_item_spacing;
	}
	m_items[ 0 ]->select();
	//
	// start cabinet entry animation
	//
}
void ttCabinet::end() {
	//
	// start cabinet exit animation
	//
}
//
//
//
void ttCabinet::select( int direction ) {
	if ( m_zoomed ) return;
	int selection = ofWrap( m_selection + direction, 0, m_items.size() );
	if ( selection != m_selection ) {
		printf( "selection: %d\n", selection );
		m_selection = selection;
		for ( int i = 0; i < m_items.size(); i++ ) {
			m_items[ i ]->select( i == m_selection );
		}
	}
}
void ttCabinet::redButton() {
	//
	// exit cabinet
	//
	((ofApp*)ofGetAppPtr())->setPhase("confirm_restart");
}
void ttCabinet::greenButton() {
	//
	// toggle zoom
	//
	if ( m_zoomed ) {
		for ( auto& item : m_items ) {
			if ( item->isZoomed() ) {
				item->zoom(false);
			}
		}
		m_zoomed = false;
	} else {
		for ( auto& item : m_items ) {
			if ( item->isSelected() ) {
				m_zoomed = true;
				item->zoom();
			} else {
				item->zoom(false);
			}
		}
	}
}
