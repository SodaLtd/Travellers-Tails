#include "ttHeadupDisplay.h"
#include "ttClusterLoader.h"
#include "ttTextRenderer.h"

const float k_headup_font_size = 56.;
const float k_headup_alpha = 0.7;
const float k_headup_alpha_255 = k_headup_alpha * 255.;
const ofColor k_background_colour( 246, 228, 98, k_headup_alpha_255 );
const ofColor k_thumbnail_background_colour( 255, 255, 255, k_headup_alpha_255 );
const float k_thumbnail_dim = 200.;
const float k_thumbnail_padding = 30.;

const ofRectangle k_thumbnail_label_bounds( k_thumbnail_padding, 260, 1350, 90 );

const ofRectangle k_ship_stern_left_arrow_bounds( 1440., k_thumbnail_padding, 272, 264 );
const ofRectangle k_ship_stern_right_arrow_bounds( 1712., k_thumbnail_padding, 272, 264 );
const ofRectangle k_ship_stern_bounds( 1602., k_thumbnail_padding, 220, 251 );
const ofRectangle k_ship_stern_left_arrow_label_bounds( 1440., 260, 272, 90 );
const ofRectangle k_ship_stern_right_arrow_label_bounds( 1712., 260, 272, 90 );

const ofRectangle k_ship_side_left_arrow_bounds( 1984., k_thumbnail_padding, 272, 264 );
const ofRectangle k_ship_side_right_arrow_bounds( 2698., k_thumbnail_padding, 272, 264 );
const ofRectangle k_ship_side_bounds( 2146., k_thumbnail_padding, 663, 166 );
const ofRectangle k_ship_side_left_arrow_label_bounds( 1984., 260, 272, 90 );
const ofRectangle k_ship_side_right_arrow_label_bounds( 2698., 260, 272, 90 );

const ofRectangle k_red_button_bounds( 3119., k_thumbnail_padding, 201, 200 );
const ofRectangle k_red_button_label_bounds( 3069, 260, 301, 90 );
const ofRectangle k_green_button_bounds( 3485., k_thumbnail_padding, 201, 200 );
const ofRectangle k_green_button_label_bounds( 3435, 260, 301, 90 );

ttHeadupDisplay ttHeadupDisplay::shared;

void ttHeadupDisplay::setup() {
	//
	//
	//
	m_scale = ofGetWidth() / 3840.;
	//
	//
	//
	m_background.set( 0, 0, 3840., 350. );
	//
	//
	//
	m_thumbnails.resize( 6 );
	float x = k_thumbnail_padding;
	for ( auto& thumbnail : m_thumbnails ) {
		thumbnail.set( x, k_thumbnail_padding, k_thumbnail_dim, k_thumbnail_dim );
		x += k_thumbnail_dim + k_thumbnail_padding;
	}
	//
	//
	//
	m_icons[ "ship_stern" ].loadImage("ui_images/stern view ship.png");
	m_icons[ "ship_side" ].loadImage("ui_images/port view ship.png");
	m_icons[ "left_arrow" ].loadImage("ui_images/anti clock arrow.png");
	m_icons[ "right_arrow" ].loadImage("ui_images/clock arrow.png");
	m_icons[ "red_button" ].loadImage("ui_images/red button.png");
	m_icons[ "green_button" ].loadImage("ui_images/green button.png");
	m_icons[ "gray_button" ].loadImage("ui_images/gray button.png");
	//
	//
	//


}
void ttHeadupDisplay::update() {

}
void ttHeadupDisplay::draw() {
	m_scale = ofGetWidth() / 3840.;
	ofPushMatrix();
	ofScale( m_scale, m_scale );
	ofPushStyle();
	//
	//
	//
	ofSetColor( k_background_colour );
	ofRect( m_background );
	//
	//
	//
	vector< ttDbMatchResult > journey = ttClusterLoader::shared()->getJourney();
	ofSetColor( k_thumbnail_background_colour );
	for ( int i = 0; i < m_thumbnails.size(); i++ ) {
		ofRect( m_thumbnails[ i ] );
		if ( i < journey.size() ) {
			string image_path = "images/" + journey[ i ].m_entry->mediaPath();
			shared_ptr< ofImage > image = m_journey_images.find(image_path) != m_journey_images.end() ? m_journey_images[ image_path ] : ttCache::shared.load(image_path);
			m_journey_images[ image_path ] = image;
			if ( image ) {
				float image_scale = min( k_thumbnail_dim / image->getWidth(), k_thumbnail_dim / image->getHeight() );
				float image_width = image->getWidth() * image_scale;
				float h_margin = ( k_thumbnail_dim - image_width ) / 2.;
				float image_height = image->getHeight() * image_scale;
				float v_margin = ( k_thumbnail_dim - image_height ) / 2.;
				image->draw( m_thumbnails[ i ].x + h_margin, m_thumbnails[ i ].y + v_margin, image_width, image_height );
			} else {
				ofLogError("ttHeadupDisplay::draw") << " Unable to load image '" << image_path << "'";
			}
		}
	}
	//
	//
	//
	string thumbnail_label = "YOUR COLLECTION : " + ofToString( journey.size() ) + " of 6";
	ofPushStyle();
	ofSetColor(76);
	ttTextRenderer::shared.drawTextInRect( thumbnail_label, k_thumbnail_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
	ofPopStyle();
	//
	//
	//
	m_icons[ "ship_stern" ].draw( k_ship_stern_bounds );
	if ( m_labels[ "ship_roll_left" ].length() > 0 ) {
		m_icons[ "left_arrow" ].draw( k_ship_stern_left_arrow_bounds );
		ofPushStyle();
		ofSetColor(76);
		ttTextRenderer::shared.drawTextInRect( m_labels[ "ship_roll_left" ], k_ship_stern_left_arrow_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
		ofPopStyle();
		
	}
	if ( m_labels[ "ship_roll_right" ].length() > 0 ) {
		m_icons[ "right_arrow" ].draw( k_ship_stern_right_arrow_bounds );
		ofPushStyle();
		ofSetColor(76);
		ttTextRenderer::shared.drawTextInRect( m_labels[ "ship_roll_right" ], k_ship_stern_right_arrow_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
		ofPopStyle();
	}
	m_icons[ "ship_side" ].draw( k_ship_side_bounds );
	if ( m_labels[ "ship_pitch_forwards" ].length() > 0 ) {
		m_icons[ "left_arrow" ].draw( k_ship_side_left_arrow_bounds );
		ofPushStyle();
		ofSetColor(76);
		ttTextRenderer::shared.drawTextInRect( m_labels[ "ship_pitch_forwards" ], k_ship_side_left_arrow_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
		ofPopStyle();
	}
	if ( m_labels[ "ship_pitch_backwards" ].length() > 0 ) {
		m_icons[ "right_arrow" ].draw( k_ship_side_right_arrow_bounds );
		ofPushStyle();
		ofSetColor(76);
		ttTextRenderer::shared.drawTextInRect( m_labels[ "ship_pitch_backwards" ], k_ship_side_right_arrow_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
		ofPopStyle();
	}
	if ( m_labels[ "red_button" ].length() > 0 ) {
		m_icons[ "red_button" ].draw( k_red_button_bounds );
		ofPushStyle();
		ofSetColor(76);
		ttTextRenderer::shared.drawTextInRect( m_labels[ "red_button" ], k_red_button_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
		ofPopStyle();
	} else {
		m_icons[ "gray_button" ].draw( k_red_button_bounds );
	}
	if ( m_labels[ "green_button" ].length() > 0 ) {
		m_icons[ "green_button" ].draw( k_green_button_bounds );
		ofPushStyle();
		ofSetColor(76);
		ttTextRenderer::shared.drawTextInRect( m_labels[ "green_button" ], k_green_button_label_bounds, k_headup_font_size, ttTextRenderer::HAlign::Centre );
		ofPopStyle();
	} else {
		m_icons[ "gray_button" ].draw( k_green_button_bounds );
	}
	ofPopStyle();
	ofPopMatrix();
}

void ttHeadupDisplay::setLabel( string name, string value ) {
	m_labels[ name ] = value;
}
//
// bodge for cabinet
//
ofImage* ttHeadupDisplay::getIcon( string name ) {
	if ( m_icons.find(name) != m_icons.end() ) {
		return &m_icons[ name ];
	}
	return nullptr;
}

