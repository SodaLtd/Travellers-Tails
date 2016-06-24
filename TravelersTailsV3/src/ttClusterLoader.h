//
//  ttClusterLoader.h
//  TravellersTailsV2
//
//  Created by Jonathan Jones-morris on 30/04/2015.
//
//
#pragma once

#include "ofMain.h"
#include "ttDb.h"
#include "ttWorld.h"
//
// TODO: refactor to something more appropriate
//
class ttClusterLoader : public ofThread {
public:
    ttClusterLoader();
    ~ttClusterLoader();
    //
    //
    //
    void setup( shared_ptr<ttWorld> world );
    //
    //
    //
	void loadCluster( string seed_id, int match = ttDb::MATCH_COLOURS | ttDb::MATCH_TEXTURES | ttDb::MATCH_TAGS, string collection = "" );
    //
    //
    //
	string getRandomId(string source="");
	ttDbEntry* getEntry( string id );
    string getMediaPath( string id );
    void clearPendingLoads();
	bool hasPendingLoads() { return m_pending_loads.size() > 0; };
	int getPendingLoadsCount() { return m_pending_loads.size(); };
	//
	//
	//
	void clearJourney();
	void startJourney( string seed_id );
	void addToJourney( ttDbMatchResult match );
	int getJourneyLength() { return m_journey.size(); };
	int getMaxJourneyLength();
	string getJourneySeed() { return m_journey_seed; };
	vector< ttDbMatchResult > getJourney() { return m_journey; };
	string getJourneyDescription();
	void loadJourney();
	void logJourney();
    //
    //
    //
	static shared_ptr<ttClusterLoader> shared();
protected:
    void threadedFunction();
    //
    //
    //
    float									m_last_load;
    deque< vector< ttDbMatchResult > >		m_pending_loads;
	
	//
	//
	//
	ofQuaternion							m_seed_orientation;
	ofVec3f									m_seed_position;
	ofVec3f									m_seed_forward;
	ofVec3f									m_seed_right;
	ofVec3f									m_seed_up;
	float									m_seed_heading;
	float									m_heading_offset;
	float									m_distance_offset;
	int										m_total_loads;
	float									m_heading_sweep;
	//
	//
	//
	string							m_journey_seed;
	vector< ttDbMatchResult >		m_journey;
	float							m_journey_start_time;
	vector< float >					m_journey_match_times;
    //
    //
    //
    shared_ptr<ttWorld>				m_world;
	//
	//
	//
	static shared_ptr<ttClusterLoader>	s_shared;
};
