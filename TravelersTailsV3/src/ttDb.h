//
//  ttDb.h
//  MPEG7_test
//
//  Created by Jonathan Jones-morris on 20/01/2015.
//
//
#pragma once

#include "ofMain.h"
#include "ttUtil.h"

class ttDbDomColour {
public:
    ttDbDomColour() {
        x = y = z = d = 0.0;
    }
    ttDbDomColour( float _x, float _y, float _z, float _d ) {
        x = _x;
        y = _y;
        z = _z;
        d = _d;
    }
	void setColour() {
		float xyz[ 3 ] = { x, y, z };
		float rgb[ 3 ];
		ttUtil::XYZtoRGB(xyz,rgb);
		ofSetColor((int)rgb[ 0 ], (int)rgb[ 1 ], (int)rgb[ 2 ]);
	}
    float x;
    float y;
    float z;
    float d;
};
class ttDbEntry {
public:
    //
    // primary id
    //
    string m_primary_id;
    //
    // source id
    //
    string m_source_id;
	//
	//
	//
	string m_source;
    //
    //
    //
    string m_name;
    //
    // colour descriptors
    //
    vector< ttDbDomColour >  m_colours;
    //
    // texture descriptors
    //
    char                        m_edge_histogram[ 80 ];
    //
    // tags ( from excel )
    //
    vector< string >            m_tags;
    //
    //
    //
    string                      m_image;
	//
	//
	//
	string mediaPath();
    //
    //
    //
    float colourDistance( ttDbEntry* other );
    float textureDistance( ttDbEntry* other );
    float tagDistance( ttDbEntry* other );
	//
	//
	//
	ofColor	getDominantColour();
};

class ttDbMatchResult {
public:
    ttDbMatchResult() {};
    ttDbMatchResult( string seed, ttDbEntry* entry, int type, float distance ) {
        m_seed      = seed;
        m_entry     = entry;
        m_type      = type;
        m_distance  = distance;
    }
    ttDbMatchResult( const ttDbMatchResult& other ) {
        m_seed      = other.m_seed;
        m_entry     = other.m_entry;
        m_type      = other.m_type;
        m_distance  = other.m_distance;
	}
	string toString();
    string          m_seed;
    ttDbEntry*		m_entry;
    int             m_type;
    float           m_distance;
};

bool sortMatchesAscending( const ttDbMatchResult& a, const ttDbMatchResult&b );

class ttDb {
public:
    enum MatchType {
        MATCH_COLOURS    = 1,
        MATCH_TEXTURES   = 32,
        MATCH_TAGS       = 64
    };
    //
    //
    //
    ttDb();
    virtual ~ttDb();
    //
    //
    //
    bool setup( string filename = "data.json" );
    //
    //
    //
    bool read( string filename = "data.json" );
    bool write( string filename = "data.json" );
    //
    //
    //
    void add( ttDbEntry* entry ) {
        m_entries[ entry->m_primary_id ] = entry;
    }
    void remove( string primary_id ) {
        m_entries.erase(primary_id);
    }
	int count(string source="") {
		if ( source.length() == 0 ) return m_entries.size();
		int count = 0;
		for ( auto& entry : m_entries ) {
			if ( entry.second->m_source == source ) {
				count++;
			}
		}
		return count;
	}
    ttDbEntry* get( int i, string source="" ) {
        if ( i >= 0 && i < count(source) ) {
			//
			// index entire collection
			//
			if ( source.length() == 0 ) {
				map< string, ttDbEntry* >::iterator it = m_entries.begin();
				advance( it, i );
				return it->second;
			}
			// 
			// index entries from specified source
			//
			int index = 0;
			for ( auto& entry : m_entries ) {
				if ( entry.second->m_source == source ) {
					if ( index == i ) return entry.second;
					index++;
				}
			}
        }
        return nullptr;
    }
	ttDbEntry* getRandom( string source = "" ) {
		int index = ofRandom(0,count(source)-1);
		return get(index,source);
	}
    ttDbEntry* find( string primary_id ) {
        map< string, ttDbEntry* >::iterator it = m_entries.find(primary_id);
        return it != m_entries.end() ? it->second : nullptr;
    }
    void clear() {
        map< string, ttDbEntry* >::iterator it = m_entries.begin();
        for ( ; it != m_entries.end(); it++ ) {
            delete it->second;
        }
        m_entries.clear();
    }
    vector<ttDbMatchResult> match( string seed_id, MatchType match_type, string source = "" ) {
        vector<ttDbMatchResult> result;
        ttDbEntry* seed = find( seed_id );
        if ( seed ) {
            float max_distance = 0;
            float min_distance = numeric_limits<float>::max();
            map< string, ttDbEntry* >::iterator it = m_entries.begin();
            for ( ; it != m_entries.end(); it++ ) {
				if ( it->first != seed_id && ( source.length() == 0 || source == it->second->m_source ) ) {
                    ttDbEntry* other = it->second;
                    float distance = -1;
                    switch ( match_type ) {
                        case MATCH_COLOURS :
                            distance = seed->colourDistance(other);
                            break;
                        case MATCH_TEXTURES :
                            distance = seed->textureDistance(other);
                            break;
                        case MATCH_TAGS :
                            distance = seed->tagDistance(other);
                            break;
                    }
					if ( distance >= 0 ) {
                        if ( distance > max_distance ) max_distance = distance;
                        if ( distance < min_distance ) min_distance = distance;
						result.push_back( ttDbMatchResult( seed_id, other, match_type, distance ) );
					}
                }
            }
            //
            // normalize distance
            //
            for ( auto& match : result ) {
				if ( max_distance > min_distance ) {
					match.m_distance = ( match.m_distance - min_distance ) / ( max_distance - min_distance );
				} else {
					match.m_distance = 0.5;
				}
            }
            //
            // sort by distance
            //
            ofSort(result, sortMatchesAscending);
        }
        return result;
    }
	static ttDb shared;
protected:
    string m_filename;
    map< string, ttDbEntry* > m_entries;
};