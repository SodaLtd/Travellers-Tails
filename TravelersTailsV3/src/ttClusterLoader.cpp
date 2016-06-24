//
//  ttClusterLoader.cpp
//  TravellersTailsV2
//
//  Created by Jonathan Jones-morris on 30/04/2015.
//
//

#include "ttClusterLoader.h"
#include "ttCluster.h"
#include "ofApp.h"

const int k_max_journey_length = 6;
//const int k_max_clusters = 2;
const int k_max_clusters = 4;
const int k_max_cluster_size = 8;

const float k_base_cluster_distance = 5.6;
const float k_min_cluster_distance = 3.7;
const float k_max_cluster_distance = 5.;
const float k_max_cluster_sweep = 80.;

ttClusterLoader::ttClusterLoader() {
    
}
ttClusterLoader::~ttClusterLoader() {
	clearPendingLoads();
	waitForThread();
	m_world = nullptr;
}
//
//
//
void ttClusterLoader::setup( shared_ptr<ttWorld> world ) {
    //
    //
    //
    m_world = world;
    //
    //
    //
    m_last_load = 0;
    startThread();
}
//
//
//
bool containsMatch( vector< ttDbMatchResult >& matches, string target_id ) {
    for ( auto& current : matches ) {
        if ( current.m_entry->m_primary_id == target_id ) return true;
    }
    return false;
}

bool containsMatch( deque< vector<ttDbMatchResult> >& matches, string target_id ) {
    for ( auto& current : matches ) {
        if ( containsMatch(current,target_id) ) return true;
    }
    return false;
}

void ttClusterLoader::loadCluster( string seed_id, int match, string collection ) {
    //
    // match entry
    //
    float start_time = ofGetElapsedTimef();
	vector< ttDbMatchResult > tag_result = ttDb::shared.match( seed_id, ttDb::MATCH_TAGS, collection.length() > 0 ? collection : "" );
    vector< ttDbMatchResult > colour_result = ttDb::shared.match( seed_id, ttDb::MATCH_COLOURS, collection.length() > 0 ? collection : "" );
    vector< ttDbMatchResult > texture_result = ttDb::shared.match( seed_id, ttDb::MATCH_TEXTURES, collection.length() > 0 ? collection : "" );
    printf( "search took : %fs\n", ( ofGetElapsedTimef() - start_time ) );
 	//
	// 
	//
#ifdef _DEBUG
	printf( "locking pending clusters\n" );
#endif
	lock();
   //
    // add top entries
    //
	int max_total = ( k_max_clusters * k_max_cluster_size ) / 3;
	int count = 0;
	vector< ttDbMatchResult > matches;
	//
	// take top n from each search
	//
	if ( match & ttDb::MATCH_TAGS ) {
		for ( int i = 0; count < max_total && i < tag_result.size(); i++ ) {
			if ( !containsMatch( m_pending_loads, tag_result[ i ].m_entry->m_primary_id ) && 
				!containsMatch( m_journey, tag_result[ i ].m_entry->m_primary_id ) &&
				!m_world->findNode(tag_result[ i ].m_entry->m_primary_id)) {
				matches.push_back(tag_result[ i ]);
				count++;
			}
		}
	}
	if ( match & ttDb::MATCH_COLOURS ) {
		for ( int i = 0; count < max_total*2 && i < colour_result.size(); i++ ) {
			if ( !containsMatch( m_pending_loads, colour_result[ i ].m_entry->m_primary_id) && 
				!containsMatch( matches, colour_result[ i ].m_entry->m_primary_id ) &&
				!containsMatch( m_journey, colour_result[ i ].m_entry->m_primary_id ) &&
				!m_world->findNode(colour_result[ i ].m_entry->m_primary_id) ) {
				matches.push_back(colour_result[ i ]);
				count++;
			}
		}
	}
	if ( match & ttDb::MATCH_TEXTURES ) {
		for ( int i = 0; count < max_total*3 && i < texture_result.size(); i++ ) {
			if ( !containsMatch( m_pending_loads, texture_result[ i ].m_entry->m_primary_id) && 
				!containsMatch( matches, texture_result[ i ].m_entry->m_primary_id ) &&
				!containsMatch( m_journey, texture_result[ i ].m_entry->m_primary_id ) &&
				!m_world->findNode(texture_result[ i ].m_entry->m_primary_id) ) {
				matches.push_back(texture_result[ i ]);
				count++;
			}
		}
	}
	printf( "total matches: %d\n", matches.size() );
	//
	// shuffle
	//
#ifdef _DEBUG
	printf( "shuffling matches\n" );
#endif
    ofRandomize( matches );
	//
	//
	//
#ifdef _DEBUG
	printf( "getting camera vectors\n" );
#endif
	m_heading_sweep = ofRandom( 180., 360. );
	m_heading_offset = 0.0;
	m_distance_offset = k_base_cluster_distance;
	m_world->getCameraVectors(m_seed_position, m_seed_forward, m_seed_right, m_seed_up);
	m_seed_heading = m_world->getCurrentHeading();
	m_seed_orientation = m_world->getRootNode().getGlobalOrientation();
	//
	//
	//

	/*
	max_total = min( (int)matches.size(), max_total );
	if ( matches.size() > max_total ) {
		matches.resize(max_total);
	}
	m_pending_loads.push_back(matches);
	*/
#ifdef _DEBUG
	printf( "creating match clusters\n" );
#endif
	vector< ttDbMatchResult > cluster;
	for ( ttDbMatchResult& match : matches ) {
		cluster.push_back( match );
		if ( cluster.size() == k_max_cluster_size ) {
			m_pending_loads.push_back(cluster);
			cluster.clear();
			if ( m_pending_loads.size() >= k_max_clusters ) break;
		}
	}
	if ( m_pending_loads.size() < k_max_clusters && cluster.size() > 2 ) { // catch the last few
		m_pending_loads.push_back(cluster);
	}
	m_total_loads = m_pending_loads.size();
#ifdef _DEBUG
	printf( "%d clusters pending load\n", m_total_loads );
#endif
    unlock();
}

string ttClusterLoader::getRandomId(string source) {
	int count = ttDb::shared.count(source);
	while(true) {
		int index = ofRandom(count);
		ttDbEntry* entry = ttDb::shared.get(index,source);
		if ( entry ) {
			return entry->m_primary_id;
		}
	}
	return "";
}

ttDbEntry* ttClusterLoader::getEntry( string id ) {
	return ttDb::shared.find(id);
}
string ttClusterLoader::getMediaPath( string id ) {
    ttDbEntry* entry = ttDb::shared.find(id);
    if ( entry ) {
        return "images/" + entry->mediaPath();
    }
    return "";
}

void ttClusterLoader::clearPendingLoads() {
    lock();
	m_pending_loads.clear();
    unlock();
}
//
//
//
void ttClusterLoader::clearJourney() {
	m_journey.clear();
	m_journey_match_times.clear();
}
void ttClusterLoader::startJourney( string seed_id ) {
	m_journey_seed = seed_id;
	clearJourney();
	m_journey_start_time = ofGetElapsedTimef();
}
void ttClusterLoader::addToJourney( ttDbMatchResult match ) {
	m_journey.push_back( match );
	m_journey_match_times.push_back(ofGetElapsedTimef());
}
string ttClusterLoader::getJourneyDescription() {
	ostringstream os;
	os << "{\n";
	os << "\t\"start\":" << ofToString( m_journey_start_time ) << ",";
	os << "\t\"entries\": [\n";
	for ( int i = 0 ; i < m_journey.size(); i++ ) {
		os << "\t\t{\n";
		os << "\t\t\t\"time\":" << ofToString( m_journey_match_times[ i ] ) << ",\n";
		os << m_journey[ i ].toString();
		os << "\t\t}";
		if ( i < m_journey.size() - 1 ) {
			os << ",\n";
		} else {
			os << "\n";
		}
	}
	os << "\t]";
	os << "},\n";
	return os.str();
}
void ttClusterLoader::logJourney() {
	ostringstream filename;
	filename << "logs/" << ofGetYear() << ofGetMonth() << ofGetDay() << ".log";
	ofFile log;
	if ( log.open(ofToDataPath(filename.str()),ofFile::Append) ) {
		string description = getJourneyDescription();
		log.write(description.c_str(),description.length());
		log.flush();
		log.close();
	}
}

void ttClusterLoader::loadJourney() {
	m_world->sinkAll();
	lock();
	m_pending_loads.clear();
	m_pending_loads.push_back(m_journey);
	unlock();
}
int ttClusterLoader::getMaxJourneyLength() {
	return k_max_journey_length;
}
//
//
//
void ttClusterLoader::threadedFunction() {
    while (isThreadRunning()) {
        if ( ofGetElapsedTimef() - m_last_load > 0.1 ) {
#ifdef _DEBUG
			printf( "waiting for cluster lock\n" );
#endif
			lock();
            if ( m_pending_loads.size() > 0 ) {
#ifdef _DEBUG
				printf( "loading cluster %d of %d\n", ( int ) ( m_total_loads - m_pending_loads.size() ) + 1, m_total_loads );
#endif               
				vector< ttDbMatchResult > entries = m_pending_loads.front();
				m_pending_loads.pop_front();
				unlock();
				
				shared_ptr< ttCluster > cluster = shared_ptr< ttCluster >( new ttCluster );
				ofQuaternion rotation;
				rotation.makeRotate(m_heading_offset,m_seed_up);
				ofQuaternion movement;
				//movement.makeRotate(5.+m_distance_offset,m_seed_right);
				movement.makeRotate(m_distance_offset,m_seed_right);
				ofQuaternion transform = m_seed_orientation * rotation * movement;
				ofVec3f position = m_seed_position * transform.inverse();
				cluster->setup( m_world, ofToString( ofRandom( 5000 ), 0 ), position, transform );

				for ( auto& match : entries ) {
#ifdef _DEBUG
					printf( "loading node %s\n", match.m_entry->m_primary_id.c_str() );
#endif // _DEBUG

					shared_ptr< ttNode > node = shared_ptr< ttNode >( new ttNode );
					if ( node->setup( match.m_entry->m_primary_id ) ) {
#ifdef _DEBUG
						printf( "adding node %s\n", match.m_entry->m_primary_id.c_str() );
#endif
						node->setShowImage(false);
						node->m_match = match;
						cluster->addItem( node );
					} else {
						ofLogError( "ttClusterLoader::threadedFunction" ) << "unable to load node : " << match.m_entry->m_primary_id;
					}
				}
				if ( cluster->itemCount() > 4 ) {
#ifdef _DEBUG
					printf( "adding cluster\n" );
#endif
					m_world->addCluster( cluster );
					cluster->rise();
					if ( getJourneyLength() >= k_max_journey_length ) { // assume this is the journey cluster, should probably make this more explicit
						cluster->setJourney();
						m_world->navigateToCluster(cluster,2.0);
						logJourney();
					}
					//m_distance_offset += ofRandom( 5., 6.); // TODO: these should be progressive
					m_distance_offset += ofRandom( k_min_cluster_distance, k_max_cluster_distance); // TODO: these should be progressive
					m_heading_offset += ofRandom(-k_max_cluster_sweep, k_max_cluster_sweep);
					
					//m_heading_offset += m_heading_sweep/m_total_loads;
				} else {
					ofLogError( "ttClusterLoader::threadedFunction" ) << "empty cluster";
				}
				
			} else {
				unlock();
			}
            m_last_load = ofGetElapsedTimef();
        }
    }
}
//
// singleton
//
shared_ptr<ttClusterLoader>	ttClusterLoader::s_shared = nullptr;
shared_ptr<ttClusterLoader>	ttClusterLoader::shared() {
	if ( s_shared == nullptr ) {
		s_shared = shared_ptr< ttClusterLoader >( new ttClusterLoader );
	}
	return s_shared;
}

