#pragma once

#include "ofMain.h"

class ttCache {
public:
	ttCache();
	~ttCache();
	//
	//
	//
	void setup( unsigned long memory_limit = 200 );
	void update();
	//
	//
	//
	shared_ptr< ofImage > load( string filepath );
	//
	//
	//
	static ttCache shared;
protected:
	unsigned long m_memory_limit;
	unsigned long m_memory_usage;
	map< string, shared_ptr< ofImage > > m_images;
	ofMutex m_lock;
};