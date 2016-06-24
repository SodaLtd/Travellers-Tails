#include "ttCache.h"

#define _DEBUG 1

ttCache ttCache::shared;

ttCache::ttCache() {

}
ttCache::~ttCache() {

}
//
//
//
void ttCache::setup( unsigned long memory_limit ) {
	m_memory_limit = memory_limit;
	m_memory_usage = 0;
}
void ttCache::update() {
	ofScopedLock lock( m_lock );
	if ( m_memory_usage >= m_memory_limit ) {
#ifdef _DEBUG
		printf( "ttCache::update : memory limit exceeded : limit=%d usage=%d\n", m_memory_limit, m_memory_usage );
#endif
		unsigned long target = m_memory_usage - m_memory_limit;
		unsigned long reclaimed = 0;
		vector< string > to_remove;
		for ( auto& entry : m_images ) {
			if ( entry.second.use_count() <= 1 ) {
				unsigned long size = ( ( entry.second->width * entry.second->height * 4 ) / 1024 / 1024 );
				reclaimed += size;
				to_remove.push_back(entry.first);
#ifdef _DEBUG
				printf( "ttCache::update : preparing to remove image : %s\n", entry.first.c_str() );
#endif
			}
			if ( reclaimed >= target ) break;
		}
		for ( string filepath : to_remove ) {
#ifdef _DEBUG
			printf( "ttCache::update : removing image : %s\n", filepath.c_str() );
#endif
			m_images.erase(filepath);
		}
		m_memory_usage -= reclaimed;
#ifdef _DEBUG
		printf( "ttCache::update : limit=%d usage=%d reclaimed=%d\n", m_memory_limit, m_memory_usage, reclaimed );
#endif
	}
}
//
//
//
shared_ptr< ofImage > ttCache::load( string filepath ) {
	ofScopedLock lock( m_lock );
#ifdef _DEBUG
	printf( "ttCache::load : finding image : %s\n", filepath.c_str() );
#endif
	auto entry = m_images.find( filepath );
	if ( entry != m_images.end()) {
#ifdef _DEBUG
		printf( "ttCache::load : found image : %s\n", filepath.c_str() );
#endif
		return entry->second;
	} else {
#ifdef _DEBUG
		printf( "ttCache::load : loading image : %s\n", filepath.c_str() );
#endif
		shared_ptr< ofImage > image = shared_ptr<ofImage>( new ofImage );
		image->setUseTexture(ofThread::isMainThread());
		if( image->loadImage(filepath) ) {
#ifdef _DEBUG
			printf( "ttCache::load : image loaded : %s\n", filepath.c_str() );
#endif
			//image->setAnchorPercent(0.5, 0.5);
			unsigned long size = ( ( image->width * image->height * 4 ) / 1024 / 1024 );
			m_memory_usage += size;
			//update();
			m_images[ filepath ] = image;
			return image;
		} else {
#ifdef _DEBUG
			printf( "ttCache::load : unable to load image : %s\n", filepath.c_str() );
#endif
		}
	}
	return nullptr;
}
