//
//  ttDb.cpp
//  MPEG7_test
//
//  Created by Jonathan Jones-morris on 20/01/2015.
//
//

#include "ttDb.h"
#include "ofxJSONElement.h"
#include "ttUtil.h"

string ttDbMatchResult::toString() {
	ostringstream os;
	os << "\"match\":" << m_entry->m_primary_id << "\",\n";
	os << "\"seed\":\"" << m_seed << "\",\n";
	// TODO: utility method to convert link type to text 
	os << "\"match type\":\"" << ( m_type == ttDb::MATCH_TAGS ? "tags" : m_type == ttDb::MATCH_COLOURS ? "colours" : "textures" );
	os << "\",\n";
	os << "\"distance\":" << ofToString( m_distance ) << "\n";
	return os.str();
}

float ttDbEntry::colourDistance( ttDbEntry* other ) {
    bool first = true;
    float distance = numeric_limits<float>::max();
    int n_seed_colour = m_colours.size();
    for ( int i = 0; i < n_seed_colour; i++ ) {
        float min_distance = numeric_limits<float>::max(); // TODO: this should be threshold
        int closest_other_colour = -1;
        int n_other_colour = other->m_colours.size();
        for ( int j = 0; j < n_other_colour; j++ ) {
            float dx = other->m_colours[ j ].x - m_colours[ i ].x;
            float dy = other->m_colours[ j ].y - m_colours[ i ].y;
            float dz = other->m_colours[ j ].z - m_colours[ i ].z;
            float candidate_distance = ( dx * dx ) + ( dy * dy ) + ( dz * dz );
            if ( candidate_distance < min_distance ) {
                min_distance = candidate_distance;
                closest_other_colour = j;
            }
        }
        float factored_distance = numeric_limits<float>::max() * m_colours[ i ].d; // factor by dominance in seed image TODO: this should be threshold
        if ( closest_other_colour > 0 ) {
            factored_distance = min_distance * ( ( 1.0 - m_colours[ i ].d ) * ( 1.0 - other->m_colours[ closest_other_colour ].d ) ); // factor by dominance in both images
        }
        if ( first ) {
            distance = factored_distance;
            first = false;
        } else {
            distance += factored_distance;
        }
    }
    return distance;
}

float ttDbEntry::textureDistance(ttDbEntry*other) {
    char* query = this->m_edge_histogram;
    char* test = other->m_edge_histogram;
    
    int size = 80;
    
    float dist = 0.0f;
    int i, j;
    for( i = 0; i < size; i++ ){
        dist += (float)abs(query[i] - test[i]);
    }
    
    // global: 5 directions 4x4 = 16 blocks => 80
    int gvalues1[5];
    memset(gvalues1, 0, 5*sizeof(int));
    
    for( j = 0; j < 80; j += 5 ) {
        for( i = 0; i < 5; i++) {
            gvalues1[i] += query[ j + i ];
        }
    }
    for( i = 0; i < 5; i++) {
        gvalues1[i] = gvalues1[i]*5.0/16;
    }
    
    int gvalues2[5];
    memset(gvalues2, 0, 5*sizeof(int));
    for( j = 0; j < 80; j += 5 ) {
        for( i = 0; i < 5; i++) {
            gvalues2[i] += test[ j + i ];
        }
    }
    for( i = 0; i < 5; i++) {
        gvalues2[i] = gvalues2[i]*5.0f/16;
    }
    
    // add to distance
    for( i = 0; i < 5; i++) {
        dist += abs( gvalues1[i] - gvalues2[i] );
    }
    
    return dist;
}

float ttDbEntry::tagDistance( ttDbEntry* other ) {
	if( m_tags.size() == 0 || other->m_tags.size() == 0 ) return -1;
    float distance = 100.f;
    int n_tags = m_tags.size();
	float dec = distance / (float)n_tags;
	int matches = 0;
	for ( auto& current_tag : m_tags ) {
		for ( auto& other_tag : other->m_tags ) {
			if ( other_tag == current_tag ) {
				matches++;
				break;
			}
		}
	}
	if ( matches == 0 ) return -1.f;
    return distance - ( dec * matches );
}


string ttDbEntry::mediaPath() {
	return m_source + "/" + m_image;
}
//
//
//
ofColor	ttDbEntry::getDominantColour() {
	if ( m_colours.size() == 0 ) return ofColor::white;
	float xyz[ 3 ];
	float max_d = numeric_limits<float>::min();
	for ( auto& colour : m_colours ) {
		if ( colour.d > max_d ) {
			max_d = colour.d;
			xyz[ 0 ] = colour.x;
			xyz[ 1 ] = colour.y;
			xyz[ 2 ] = colour.z;
		}
	}
	float rgb[ 3 ];
	ttUtil::XYZtoRGB(xyz,rgb);
	ofVec3f colour(rgb[ 0 ], rgb[ 1 ], rgb[ 2 ]);
	colour.normalize();
	colour *= 255;
	return ofColor( colour.x, colour.y, colour.z );
}

bool sortMatchesAscending( const ttDbMatchResult& a, const ttDbMatchResult&b ) {
    return (a.m_distance < b.m_distance);
}

ttDb ttDb::shared;

ttDb::ttDb() {
    
}
ttDb::~ttDb() {
    
}
//
//
//
bool ttDb::setup( string filename ) {
    if ( filename.length() > 0 ) {
        m_filename = ofToDataPath(filename);
        return read( filename );
    }
    return true;
}
//
//
//
bool ttDb::read( string filename ) {
    
    ofxJSONElement json;
    if ( json.open( filename ) ) {
        m_entries.clear();
        vector<string> members = json.getMemberNames();
        for ( int i = 0; i < members.size(); i++ ) {
            ttDbEntry* entry = new ttDbEntry;
            try {
                //
                // primary id
                //
                entry->m_primary_id = members[ i ];
				if ( json[ entry->m_primary_id ].isMember( "source_id" ) ) {
					entry->m_source_id = json[ entry->m_primary_id ][ "source_id" ].asString();
				} else {
					entry->m_source_id = entry->m_primary_id;
				}
                //
                // source
                //
                entry->m_source = json[ entry->m_primary_id ][ "source" ].asString();
                //
                // name
                //
                entry->m_name = json[ entry->m_primary_id ][ "name" ].asString();
                //
                //
                //
                entry->m_image = json[ entry->m_primary_id ][ "image" ].asString();
                //
                // colour descriptors
                //
                if ( json[ entry->m_primary_id ].isMember("colours") ) {
                    entry->m_colours.resize(json[ entry->m_primary_id ][ "colours" ].size());
                    for ( int colour = 0; colour < json[ entry->m_primary_id ][ "colours" ].size(); colour++ ) {
                        entry->m_colours[ colour ].x = json[ entry->m_primary_id ][ "colours" ][ colour ][ "x" ].asFloat();
                        entry->m_colours[ colour ].y = json[ entry->m_primary_id ][ "colours" ][ colour ][ "y" ].asFloat();
                        entry->m_colours[ colour ].z = json[ entry->m_primary_id ][ "colours" ][ colour ][ "z" ].asFloat();
                        entry->m_colours[ colour ].d = json[ entry->m_primary_id ][ "colours" ][ colour ][ "d" ].asFloat();
                    }
                }
                //
                // texture descriptors
                //
                if ( json[ entry->m_primary_id ].isMember("texture") ) {
                    for ( int descriptor = 0; descriptor < 80; descriptor++ ) {
                        entry->m_edge_histogram[ descriptor ] = ( char ) json[ entry->m_primary_id ][ "texture" ][ descriptor ].asInt();
                    }
                }
                //
                // tags
                //
                for ( int tag = 0; tag < json[ entry->m_primary_id ][ "tags" ].size(); tag++ ) {
                    entry->m_tags.push_back(json[ entry->m_primary_id ][ "tags" ][ tag ].asString());
                }
                //
                //
                //
                m_entries[ entry->m_primary_id ] = entry;
            } catch( ... ) {
                ofLogError("ttDb::read") << "Problems reading entry : " << members[ i ];
                delete entry;
            }
           
        }
        return true;
    }
    
    return false;
}
bool ttDb::write( string filename ) {
	//
	// backup
	//
	if ( ofFile::doesFileExist(filename,true) ) {
		string backup = ofFilePath::getBaseName(filename) + "-";
		backup += ofToString(ofGetYear(),4,'0');
		backup += ofToString(ofGetMonth(),2,'0');
		backup += ofToString(ofGetDay(),2,'0');
		backup += "." + ofFilePath::getFileExt(filename);
		ofFile::moveFromTo(filename,backup);
	}
	//
	//
	//
    static const Json::StaticString name("name");
    static const Json::StaticString source_id("source_id");
    static const Json::StaticString source("source");
    static const Json::StaticString image("image");
    static const Json::StaticString colours("colours");
    static const Json::StaticString texture("texture");
    static const Json::StaticString tags("tags");
    static const Json::StaticString x("x");
    static const Json::StaticString y("y");
    static const Json::StaticString z("z");
    static const Json::StaticString d("d");
    //
    //
    //
    ofxJSONElement json;
    map< string, ttDbEntry* >::iterator it = m_entries.begin();
    for ( ; it != m_entries.end(); it++ ) {
        ttDbEntry* entry = it->second;
        //
        // build entry JSON object
        //
        Json::Value entry_json;
        entry_json[name]		= entry->m_name;
        entry_json[source_id]	= entry->m_source_id;
        entry_json[source]		= entry->m_source;
        entry_json[image]		= entry->m_image;

        Json::Value colour_json( Json::arrayValue );
        colour_json.resize(entry->m_colours.size());
        for ( int colour = 0; colour < entry->m_colours.size(); colour++ ) {
            colour_json[ colour ][ x ] = entry->m_colours[ colour ].x;
            colour_json[ colour ][ y ] = entry->m_colours[ colour ].y;
            colour_json[ colour ][ z ] = entry->m_colours[ colour ].z;
            colour_json[ colour ][ d ] = entry->m_colours[ colour ].d;
        }
        entry_json[ colours ] = colour_json;
        
        Json::Value texture_json( Json::arrayValue );
        texture_json.resize(80);
        for ( int descriptor = 0; descriptor < 80; descriptor++ ) {
            texture_json[ descriptor ] = entry->m_edge_histogram[ descriptor ];
        }
        entry_json[ texture ] = texture_json;
        
        Json::Value tag_json( Json::arrayValue );
        tag_json.resize(entry->m_tags.size());
        for ( int tag = 0; tag < entry->m_tags.size(); tag++ ) {
            tag_json[ tag ] = entry->m_tags[ tag ];
        }
        entry_json[ tags ] = tag_json;
        
        Json::StaticString primary_id(entry->m_primary_id.c_str());
        json[ primary_id ] = entry_json;
    }
    return json.save(filename,true);
}
