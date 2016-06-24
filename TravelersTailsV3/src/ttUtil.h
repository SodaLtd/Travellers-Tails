#pragma once

#include "ofMain.h"
#include "ttCache.h"

class ttUtil {
public:
	static void RGBtoXYZ( float* rgb, float*xyz ) {
		ofVec3f rgb_v( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ] );
		ofVec3f xyz_v = RGBtoXYZ( rgb_v );
		xyz[ 0 ] = xyz_v.x;
		xyz[ 1 ] = xyz_v.y;
		xyz[ 2 ] = xyz_v.z;
    }
	static void XYZtoRGB( float*xyz, float* rgb ) {
		ofVec3f xyz_v( xyz[ 0 ], xyz[ 1 ], xyz[ 2 ] );
		ofVec3f rgb_v = XYZtoRGB( xyz_v );
		rgb[ 0 ] = rgb_v.x;
		rgb[ 1 ] = rgb_v.y;
		rgb[ 2 ] = rgb_v.z;
	}
	static ofColor XYZtoColor( float*xyz ) {
		ofVec3f xyz_v( xyz[ 0 ], xyz[ 1 ], xyz[ 2 ] );
		ofVec3f rgb_v = XYZtoRGB( xyz_v );
		return ofColor( rgb_v.x, rgb_v.y, rgb_v.z );
	}
	static ofColor XYZtoColor( float x, float y, float z ) {
		float xyz[ 3 ] = {
			x, y, z 
		};
		return XYZtoColor( xyz );
	}
	static ofVec3f RGBtoXYZ( ofVec3f rgb ) {
		const ofMatrix4x4 matrix( 
			/* row 0 */		0.412453,	0.357580,	0.180423,	0., 
			/* row 1 */		0.212671,	0.715160,	0.072169,	0.,
			/* row 2 */		0.019334,	0.119193,	0.950227,	0.,
			/* row 3 */		0.,			0.,			0.,			1. );
		return matrix * rgb;
	}
	static ofVec3f XYZtoRGB( ofVec3f xyz ) {
		const ofMatrix4x4 matrix( 
			/* row 0 */		3.240479,	-1.537150,	-0.498535,	0., 
			/* row 1 */		-0.969256,	1.875992,	0.041556,	0.,
			/* row 2 */		0.055648,	-0.204043,  1.057311,	0.,
			/* row 3 */		0.,			0.,			0.,			1. );
		return matrix * xyz;
	}
	//
	// string utils
	//

	static string trim(string str) {
		return trimFront(trimBack(str));
	}
	static string trimFront(string str) {
		size_t startpos = str.find_first_not_of(" \t\n\r");
		return (startpos == std::string::npos) ? "" : str.substr(startpos);
	}
	static string trimBack(string str) {
		size_t endpos = str.find_last_not_of(" \t\n\r");
		return (endpos == std::string::npos) ? "" : str.substr(0, endpos+1);
	}
	static void drawArrow( ofRectangle& rect, int direction ) {
		ofPushStyle();
		shared_ptr< ofImage > image;
		ofPoint p0, p1, p2;
		ofColor background;
		ofColor colour;
		ofPoint offset;
		if ( direction == 0 ) {
			p0.set( rect.getRight(), rect.getTop() );
			p1.set( rect.getLeft(), rect.getCenter().y );
			p2.set( rect.getRight(), rect.getBottom() );
			colour = ofColor( 171, 57, 11 );
			background = ofColor::red;
			image = ttCache::shared.load("ui_images/cross.png");
			offset.set( rect.width * 0.1f, 0 );
		} else {
			p0.set( rect.getLeft(), rect.getTop() );
			p1.set( rect.getRight(), rect.getCenter().y );
			p2.set( rect.getLeft(), rect.getBottom() );
			background = ofColor::green;
			colour = ofColor( 37, 144, 82 );
			image = ttCache::shared.load("ui_images/tick.png");
			offset.set( -rect.width * 0.1f, 0 );
		}
		ofSetColor( background, 188 );
		ofRect( rect );
		ofSetColor( colour );
		ofTriangle( p0, p1, p2 );
		float scale = ( rect.width * 0.65 ) / image->width; // TODO: dynamic margin
		ofSetColor( ofColor::white );
		image->draw( rect.getCenter() + offset, image->width * scale, image->height * scale );
		ofPopStyle();
	}
};
