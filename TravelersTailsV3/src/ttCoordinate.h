#pragma once

#include "ofMain.h"

class ttCoordinate {
public:
	ttCoordinate() { lat = lon = 0.; };
	ttCoordinate( float _lat, float _lon ) { lat = _lat; lon = _lon; };
	//
	//
	//
	void set( float _lat, float _lon ) { lat = _lat; lon = _lon; };
	//
	//
	//
	// Operator overloading for ofVec3f
    //
    ofVec3f  operator+( const ttCoordinate& pnt ) const;
    ofVec3f& operator+=( const ttCoordinate& pnt );
    ofVec3f  operator-( const ttCoordinate& vec ) const;
    ofVec3f& operator-=( const ttCoordinate& vec );
    ofVec3f  operator*( const ttCoordinate& vec ) const;
    ofVec3f& operator*=( const ttCoordinate& vec );
    ofVec3f  operator/( const ttCoordinate& vec ) const;
    ofVec3f& operator/=( const ttCoordinate& vec );
    ofVec3f  operator-() const;
	//
	// Operator overloading for ofVec3f
    //
    ofVec3f  operator+( const ofVec3f& pnt ) const;
    ofVec3f& operator+=( const ofVec3f& pnt );
    ofVec3f  operator-( const ofVec3f& vec ) const;
    ofVec3f& operator-=( const ofVec3f& vec );
    ofVec3f  operator*( const ofVec3f& vec ) const;
    ofVec3f& operator*=( const ofVec3f& vec );
    ofVec3f  operator/( const ofVec3f& vec ) const;
    ofVec3f& operator/=( const ofVec3f& vec );
    ofVec3f  operator-() const;
	
    //operator overloading for float
    //
	//    void 	  operator=( const float f);
    ofVec3f  operator+( const float f ) const;
    ofVec3f& operator+=( const float f );
 	ofVec3f  operator-( const float f ) const;
    ofVec3f& operator-=( const float f );
    ofVec3f  operator*( const float f ) const;
    ofVec3f& operator*=( const float f );
    ofVec3f  operator/( const float f ) const;
    ofVec3f& operator/=( const float f );

protected:
	float lat;
	float lon;
};