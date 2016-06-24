#include "ofApp.h"
#include "ofxJSONElement.h"
#include "ttDb.h"
#include "ttFog.h"
#include "ttClusterLoader.h"
#include "ttCache.h"
#include "ttTextRenderer.h"
#include "ttScript.h"
#include "ttSerialDevice.h"
#include "sdaHTTP.h"
#include "ttHeadupDisplay.h"

#include <thread>

class chained {
public:
	chained() {
		m_thread = nullptr;
		m_running = false;
	}
	virtual ~chained() {
		stop();
	}
	chained& operator +=(function<void()>f) {
		lock_guard< mutex > lock(m_queue_guard);
		m_queue.push(f);
		return *this;
	}
	bool is_running() { return m_running; }
	void start() {
		m_running = true;
		m_thread = new thread([this]() {
			while(m_running) {
				m_queue_guard.lock();
				if ( m_queue.size() > 0 ) {
					function<void()>f = m_queue.front();
					m_queue.pop();
					m_queue_guard.unlock();
					f();
				} else {
					m_queue_guard.unlock();
					break;
				}
			}
			m_thread->detach();
			delete m_thread;
			m_thread = nullptr;
		});
	}
	void stop() {
		m_running = false;
		if ( m_thread ) {
			m_thread->join();
			delete m_thread;
			m_thread = nullptr;
		}
	}
protected:	
	mutex						m_queue_guard;
	queue< function<void()> >	m_queue;
	thread*						m_thread;
	bool						m_running;
};

chained chained_test;

//--------------------------------------------------------------
void ofApp::setup(){
	/*
	int a = 0;
	chained_test += [&a]() { printf("f1() a=%d ", ++a ); };
	chained_test += [&a]() { printf("f2() a=%d ", ++a ); };
	chained_test += [&a]() { printf("f3() a=%d ", ++a ); };
	chained_test += [&a]() { printf("f4() a=%d ", ++a ); };
	chained_test += [&a]() { printf("f5() a=%d ", ++a ); };
	chained_test += [&a]() { printf("f6() a=%d ", ++a ); };
	chained_test.start();
	*/
	//
	// default settings
	//
	string controller_port = "COM3";
	string ship_port = "COM4";
	m_use_controller = false;
	m_timeout = 20.;
	m_timeout_text = "Do you want to continue?";
	m_headup_font_size = 18.;
	m_scale = ofGetWidth() / 3840.f;
	//
	//
	//
	m_test_mode = false;
	//
	//
	//
	int target_framerate = 60;
	bool vertical_sync = true;
	//
	// load settings
	//
	ofxJSONElement settings;
	if ( settings.open(ofToDataPath("settings.json")) ) {
		if ( settings.isMember( "target_framerate" ) ) {
			target_framerate = settings["target_framerate"].asInt();
		}
		if ( settings.isMember( "vertical_sync" ) ) {
			vertical_sync = settings["vertical_sync"].asBool();
		}
		if ( settings.isMember( "timeout" ) ) {
			m_timeout = settings[ "timeout" ].asFloat();
		}
		if ( settings.isMember( "timeout_text" ) ) {
			m_timeout_text = settings[ "timeout_text" ].asString();
		}
		if ( settings.isMember("controller_port") ) {
			controller_port = settings["controller_port"].asString();
		}
		if ( settings.isMember("ship_port") ) {
			ship_port = settings["ship_port"].asString();
		}
		if ( settings.isMember("use_controller") ) {
			m_use_controller = settings["use_controller"].asBool();
		}
		if ( settings.isMember("headup_fontsize") ) {
			m_headup_font_size = settings["headup_fontsize"].asFloat();
		}
	
		//
		// load phase content
		//
		if ( settings.isMember( "script" ) ) {
			ttScript::shared.setup(settings["script"]);
		}
	}
	//
	//
	//
	if ( target_framerate > 0 ) ofSetFrameRate( target_framerate );
	ofSetVerticalSync(vertical_sync);
	ofBackground(255);
	//ofBackground(96,96,128);
	//ofBackground(ofColor::orangeRed);
	//
	//
	//
	ttCache::shared.setup();
	//ttTextRenderer::shared.setup("fonts/DINAlternateBold.ttf");
	ttTextRenderer::shared.setup("fonts/DINCondensedBold.ttf");
	//m_headup_font.loadFont("fonts/DSEG14Classic-Bold.ttf", m_headup_font_size*m_scale);
	//
	// sync database
	//
	string db_json_data;
	if ( false ) {//sdaHTTP::get("http://travellerstails-soda.rhcloud.com/export",db_json_data) < 400 ) {
		//
		// update database
		//
		ofxJSONElement db_json;
		if ( db_json.parse(db_json_data) && db_json.isArray() ) {
			ttDb::shared.clear();
			for ( int i = 0; i < db_json.size(); i++ ) {
				ttDbEntry* entry = new ttDbEntry;
               //
                // primary id
                //
				entry->m_primary_id = db_json[ i ]["id"].asString();
				if ( db_json[ i ].isMember( "source_id" ) ) {
					entry->m_source_id = db_json[ i ][ "source_id" ].asString();
				} else {
					entry->m_source_id = entry->m_primary_id;
				}
                //
                // source
                //
                entry->m_source = db_json[ i ][ "source" ].asString();
                //
                // name
                //
                entry->m_name = db_json[ i ][ "name" ].asString();
                //
                //
                //
                entry->m_image = db_json[ i ][ "image" ].asString();
                //
                // colour descriptors
                //
                if ( db_json[ i ].isMember("colours") ) {
                    entry->m_colours.resize(db_json[ i ][ "colours" ].size());
                    for ( int colour = 0; colour < db_json[ i ][ "colours" ].size(); colour++ ) {
                        entry->m_colours[ colour ].x = db_json[ i ][ "colours" ][ colour ][ "x" ].asFloat();
                        entry->m_colours[ colour ].y = db_json[ i ][ "colours" ][ colour ][ "y" ].asFloat();
                        entry->m_colours[ colour ].z = db_json[ i ][ "colours" ][ colour ][ "z" ].asFloat();
                        entry->m_colours[ colour ].d = db_json[ i ][ "colours" ][ colour ][ "d" ].asFloat();
                    }
                }
                //
                // texture descriptors
                //
                if ( db_json[ i ].isMember("texture") ) {
                    for ( int descriptor = 0; descriptor < 80; descriptor++ ) {
                        entry->m_edge_histogram[ descriptor ] = ( char ) db_json[ i ][ "texture" ][ descriptor ].asInt();
                    }
                }
                //
                // tags
                //
                for ( int tag = 0; tag < db_json[ i ][ "tags" ].size(); tag++ ) {
                    entry->m_tags.push_back(db_json[ i ][ "tags" ][ tag ].asString());
                }
				//
				//
				//
				ttDb::shared.add(entry);
			}
			//
			// backup database
			//
			ostringstream backup;
			backup << "data-";
			backup << ofGetTimestampString();
			backup << ".json";
			ofFile::copyFromTo("data.json",backup.str(),true,true);
			//
			// write new data
			//
			ttDb::shared.write();
		}
	}
	ttDb::shared.setup();
	//
	// initialise input devices
	//
	map< string, string > devices = ttSerialDevice::listDevices();
	map< string, string >::iterator it = devices.begin();
	printf( "Devices\n" );
	for ( ; it != devices.end(); ++it ) {
		printf( "%s : %s\n", it->first.c_str(), it->second.c_str() );
	}
	it = devices.find("controller");
	if ( it != devices.end() ) {
		m_controller.setup(it->second);
	} else {
		ofLogError() << "orientation controller not found, using default port";
		if ( !m_controller.setup(controller_port) ) {
			ofLogError() << "orientation controller not found";
		}
	}
	it = devices.find("buttons");
	if ( it != devices.end() ) {
		m_ship.setup(it->second);
		m_ship.setLED(false,false);
	} else {
		ofLogError() << "button controller not found, using default port";
		if ( !m_ship.setup(ship_port) ) {
			ofLogError() << "button controller not found";
		}
	}
	//
	// create world
	// TODO: this should be a singleton
	//
	m_world = shared_ptr< ttWorld >( new ttWorld );
	m_world->setup( 1200. );
	//
	// initialise cluster loader
	//
	ttClusterLoader::shared()->setup( m_world );
	//
	//
	//
	ttHeadupDisplay::shared.setup();
	//
	//
	//
	m_cabinet.setup();
	//
	//
	//
	m_last_action = 0;
	m_in_gesture = false;
	m_last_gesture = 0.;
	m_swipe_position =
	m_swipe_velocity = 0.;
	/*
	m_phase = "attractor";
	ttScript::shared.startSession("attractor");
	*/
	setPhase( "attractor" );
	m_fullscreen_timer = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::update(){
	if ( m_test_mode && m_next_test < ofGetElapsedTimef() ) {
		if ( m_phase == "journey" ) { 
			greenButton();
		} else {
			if ( ofRandom( -100, 100 ) > 0 ) {
				greenButton();
			} else {
				redButton();
			}
		}
		m_next_test = ofGetElapsedTimef() + ofRandom(0.5, 11.0);
		m_last_action = ofGetElapsedTimef();
	}
#ifndef _DEBUG
	/*
	if ( ofGetElapsedTimef() - m_fullscreen_timer > 10. ) { 
		ofSetFullscreen(true);
		ofHideCursor();
		m_fullscreen_timer = numeric_limits<float>::max();
	}
	*/
#endif

	if ( ofGetElapsedTimef() - m_last_action > m_timeout && m_phase != "attractor" ) {
		if ( m_phase == "timeout" ) {
			m_world->sinkAll();
			m_world->setLock(false);
			m_timeout_phase = "";
			setPhase("attractor");
		} else {
			m_last_action = ofGetElapsedTimef();
			m_timeout_phase = m_phase;
			setPhase("timeout");
		}
		
	}
	//
	//
	//
	if ( m_controller.isConnected() ) {
		m_controller.update();
		ofVec3f euler = m_controller.getEuler();
		if ( euler != m_current_euler ) {
			bool update = euler.distance(m_current_euler) > 4.0;
			
			m_current_euler = euler;
			
			//
			// 
			//
			if ( update )  {
				m_last_action = ofGetElapsedTimef();
				if ( m_phase == "attractor" ) {
					m_phase = "instructions1";
					m_ship.setLED(false,true);
					ttScript::shared.startSession("instructions1");
					ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
					ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
					ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
					ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
					ttHeadupDisplay::shared.setLabel( "red_button", "" );
					ttHeadupDisplay::shared.setLabel( "green_button", "NEXT" );
				} 
			}
		}
		m_world->setVelocity(ofMap(euler.y,-90.,/*90.*/45.,m_world->getMaxVelocity(),/*m_world->getMinVelocity()*/0.,true));
		m_world->setRotationalVelocity(ofMap(euler.z,-90.,90.,m_world->getMinRotationalVelocity(),m_world->getMaxRotationalVelocity(),true));
		float swipe_position = ofLerp( m_swipe_position, ofMap(-euler.z,-90.,90.,-1.,1.,true), 0.5 );
		m_swipe_velocity = ( swipe_position - m_swipe_position ) / ( ofGetFrameRate() / 60. );
		m_swipe_position = swipe_position;
		if ( m_phase == "reveal" ) {
			if ( fabs( m_swipe_position ) > 0.5 && !m_in_gesture && ofGetElapsedTimef() - m_last_gesture > 0.75 ) {
				m_in_gesture = true;
				m_last_gesture = ofGetElapsedTimef();
				m_gesture_direction = m_swipe_position < 0. ? -1 : 1;
				printf( "swipe position: %f gesture direction : %d\n", m_swipe_position, m_gesture_direction );
			} else {
				if ( m_in_gesture ) {
					m_in_gesture = false;
					m_cabinet.select( m_gesture_direction );
					m_gesture_direction = 0;
				}
			}
		}
	} else {
		if ( m_phase == "attractor" && ofGetElapsedTimef() - m_last_action < m_timeout ) {
 			m_phase = "instructions1";
			m_ship.setLED(false,true);
			ttScript::shared.startSession("instructions1");
			ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
			ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
			ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
			ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
			ttHeadupDisplay::shared.setLabel( "red_button", "" );
			ttHeadupDisplay::shared.setLabel( "green_button", "NEXT" );
		}
		ofVec2f range( ( ofGetWidth() / 2. ), ( ofGetHeight() / 2. ) );
		ofVec3f controller_posn( ( ofGetMouseX() - range.x ) * -1., ofGetMouseY() - range.y );
		m_world->setVelocity(ofMap(controller_posn.y,-range.y,range.y,m_world->getMaxVelocity(),m_world->getMinVelocity(),true));
		m_world->setRotationalVelocity(ofMap(controller_posn.x,-range.x,range.x,m_world->getMinRotationalVelocity(),m_world->getMaxRotationalVelocity(),true));
		float swipe_position = ofLerp( m_swipe_position, ofMap(controller_posn.x*-1.,-range.x,range.x,-1.,1.,true), 0.5 );
		m_swipe_velocity = ( swipe_position - m_swipe_position ) / ( ofGetFrameRate() / 60. );
		m_swipe_position = swipe_position;
		//
		//
		//
		if ( m_phase == "reveal" ) {
			//printf( "swipe position: %f velocity : %f\n", m_swipe_position, m_swipe_velocity );
			if ( fabs( m_swipe_position ) > 0.5 && !m_in_gesture && ofGetElapsedTimef() - m_last_gesture > 0.5 ) {
				m_in_gesture = true;
				m_last_gesture = ofGetElapsedTimef();
				m_gesture_direction = m_swipe_position < 0. ? -1 : 1;
				printf( "swipe position: %f gesture direction : %d\n", m_swipe_position, m_gesture_direction );
			} else {
				if ( m_in_gesture ) {
					m_in_gesture = false;
					m_cabinet.select( m_gesture_direction );
					m_gesture_direction = 0;
				}
			}
		} else {
			if ( m_keydown[OF_KEY_UP]  ) m_world->accelerate(0.01);
			if ( m_keydown[OF_KEY_DOWN]  ) m_world->accelerate(-0.01);
			if ( m_keydown[OF_KEY_LEFT]  ) m_world->turn( 0.01 );
			if ( m_keydown[OF_KEY_RIGHT]  ) m_world->turn( -0.01 );
		}
	}
	if ( m_phase == "reveal" || m_timeout_phase == "reveal" || m_phase == "confirm_restart" ) {
		m_cabinet.update();
	} else {
		m_world->update();
	}
	//
	// update ship
	//
	m_ship.update();
	//
	//
	//
	ttScript::shared.update();
	ttCache::shared.update();
	ttHeadupDisplay::shared.update();
	//
	//
	//
	if ( m_phase == "journey" ) {
		if ( ofGetElapsedTimef() - m_last_select > 10. ) {
			if ( !ttClusterLoader::shared()->hasPendingLoads() && !m_world->hasVisibleClusters() ) { // TODO: move time to settings
				//
				// assume user is lost
				//
				printf( "select timeout\n" );
				ttClusterLoader::shared()->clearPendingLoads();
				m_world->sinkAll();
				m_world->setLock(false);
				string seed_id = ttClusterLoader::shared()->getRandomId();
				ttClusterLoader::shared()->loadCluster(seed_id);
				m_last_select = ofGetElapsedTimef();
			}
		}
	} else {
		m_last_select = ofGetElapsedTimef();
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	if ( m_phase == "reveal" || m_timeout_phase == "reveal" || m_phase == "confirm_restart" ) {
		m_cabinet.draw();
		ttScript::shared.draw();
	} else {
		m_world->draw();
		ttScript::shared.draw();
		if ( m_phase != "attractor" ) {
			ttHeadupDisplay::shared.draw();
		}
	}
}
void ofApp::exit() {
	m_ship.setLED( false, false );
}

//
//
//
const string k_instructions = "instructions";
void ofApp::greenButton() {
	if ( m_phase.find(k_instructions) != string::npos ) {
		if ( m_phase.length() == k_instructions.length() ) { 
			startJourney();	
		} else {
			string next_instructions = "instructions" + ofToString( ofToInt( m_phase.substr( k_instructions.length() ) ) + 1 );
			if ( ttScript::shared.hasPhase(next_instructions) ) {
				setPhase( next_instructions );
			} else {
				startJourney();		
			}
		}
	} else if ( m_phase == "journey" ) {
		if ( !m_world->hasJourneyCluster() ) {
			if ( m_world->clusterCount() > 0 ) {
				shared_ptr< ttCluster > cluster = m_world->getCluster( 0 );
				//if ( cluster ) m_world->navigateToCluster(cluster,1.);
				if ( cluster ) m_world->navigateToNearestCluster(1.);
			} else {
				string seed_id = ttClusterLoader::shared()->getRandomId();
				ttClusterLoader::shared()->loadCluster(seed_id);
			}
			m_last_select = ofGetElapsedTimef();
		}
	} else if ( m_phase == "select" ) {
		if ( m_world->hasSelectedItem() ) {
			ttClusterLoader::shared()->clearPendingLoads();
			m_world->selectItem();
			m_world->sinkAll();
			int journey_length = ttClusterLoader::shared()->getJourneyLength();
			if (  journey_length >= ttClusterLoader::shared()->getMaxJourneyLength() ) { 
				ttScript::shared.setPhase( "collectioncomplete" );
			} else if (  journey_length > 0 ) {
				m_ship.setLED(true,true);
			}
			ttHeadupDisplay::shared.setLabel( "ship_roll_left", "LEFT" );
			ttHeadupDisplay::shared.setLabel( "ship_roll_right", "RIGHT" );
			ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "ACCELERATE" );
			ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "BRAKE" );
			ttHeadupDisplay::shared.setLabel( "red_button", "RESTART" );
			ttHeadupDisplay::shared.setLabel( "green_button", "AUTOPILOT" );
			m_phase = "journey";
			m_ship.setLED(true,true);
			m_last_select = ofGetElapsedTimef();
		}
	} else if ( m_phase == "reveal" ) {
		m_cabinet.greenButton();
	} else if ( m_phase == "confirm_restart" ) {
		setPhase( "reveal");
	} else if ( m_phase == "timeout" ) {
		setPhase(m_timeout_phase);
	}
}
void ofApp::redButton() {
	
	if ( m_phase == "select" ) {
		setPhase( "journey" );
		m_world->sinkLock();
		m_world->setLock(false);
		if ( m_world->clusterCount() <= 1 ) { // only load new clusters if we have none left
			string seed_id = ttClusterLoader::shared()->getRandomId();
			ttClusterLoader::shared()->loadCluster(seed_id);
		}
		m_last_select = ofGetElapsedTimef();
	} else if ( m_phase == "journey" || m_phase == "timeout" ) {
		restart();
	} else if ( m_phase == "reveal" ) {
		m_cabinet.redButton();
		//setPhase( "confirm_restart" );
	} else if ( m_phase == "confirm_restart" ) {
		setPhase( "outro" );
		ttClusterLoader::shared()->clearPendingLoads();
		ttClusterLoader::shared()->clearJourney();
		ttHeadupDisplay::shared.clearJourney();
	} else if ( m_phase == "outro" ) {
		restart();
	}
}
void ofApp::setPhase( string phase ) {
	string previous_phase = m_phase;
	m_phase = phase;
	ttScript::shared.setPhase( phase );
	//
	// update LEDs
	//
	if ( phase == "attractor" ) {
		ttScript::shared.startSession("attractor");
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "" );
		ttHeadupDisplay::shared.setLabel( "green_button", "" );
		m_ship.setLED(false,false);
	} else if ( m_phase.find(k_instructions) != string::npos ) {
		m_ship.setLED(false,true);
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "" );
		ttHeadupDisplay::shared.setLabel( "green_button", "NEXT" );
	} else if ( phase == "journey" ) {
		m_last_select = ofGetElapsedTimef();
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "LEFT" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "RIGHT" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "ACCELERATE" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "BRAKE" );
		ttHeadupDisplay::shared.setLabel( "red_button", "RESTART" );
		ttHeadupDisplay::shared.setLabel( "green_button", "AUTOPILOT" );
		m_ship.setLED(true,true);
	} else if ( phase == "select" ) {
		m_last_select = ofGetElapsedTimef();
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "LEFT" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "RIGHT" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "LEAVE" );
		ttHeadupDisplay::shared.setLabel( "green_button", "SELECT" );
		m_ship.setLED(true,true);
	} else if ( phase == "reveal" ) {
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "LEFT" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "RIGHT" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "RESTART" );
		ttHeadupDisplay::shared.setLabel( "green_button", "" );
		m_ship.setLED(true,true);
		if ( previous_phase != "confirm_restart" && previous_phase != "timeout" ) m_cabinet.start();
	} else if ( phase == "outro" ) {
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "RESTART" );
		ttHeadupDisplay::shared.setLabel( "green_button", "" );
		m_ship.setLED(true,false);
	} else if ( phase == "confirm_restart" ) {
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "YES" );
		ttHeadupDisplay::shared.setLabel( "green_button", "NO" );
		m_ship.setLED(true,true);
	} else if ( phase == "timeout" ) {
		ttHeadupDisplay::shared.setLabel( "ship_roll_left", "" );
		ttHeadupDisplay::shared.setLabel( "ship_roll_right", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_forwards", "" );
		ttHeadupDisplay::shared.setLabel( "ship_pitch_backwards", "" );
		ttHeadupDisplay::shared.setLabel( "red_button", "RESTART" );
		ttHeadupDisplay::shared.setLabel( "green_button", "CONTINUE" );
		m_ship.setLED(true,true);
	}
}
void ofApp::startJourney() {
	m_world->sinkAll();
	m_world->navigateTo(0,0);
	string seed_id = ttClusterLoader::shared()->getRandomId();
	ttClusterLoader::shared()->startJourney(seed_id);
	ttClusterLoader::shared()->loadCluster(seed_id);
	m_world->setLock(false);
	setPhase( "journey" );
}

void ofApp::restart() {
	m_timeout_phase = "";
	ttClusterLoader::shared()->clearPendingLoads();
	ttClusterLoader::shared()->clearJourney();
	ttHeadupDisplay::shared.clearJourney();
	m_world->sinkAll();
	m_world->setLock(false);
	setPhase( "instructions1" );
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	m_keydown[key] = true;
	switch( key ) {
	case 't' : m_test_mode = !m_test_mode; m_next_test = ofGetElapsedTimef() + 0.5; break;
	case 'c' : m_world->toggleCamera(); break;
	case 'z' : m_world->zoomOverhead( 1 ); break;
	case 'x' : m_world->zoomOverhead( -1 ); break;
	case 'u' : m_world->setLock( false ); break;
	case 'y' : break; // load selection
	case 'f' : ofToggleFullscreen(); break;

	case ']' : greenButton(); break;
	case '[' : redButton(); break;
	case 's' : {
			string filename;
			filename = ofToString( ofGetElapsedTimeMillis() ) + "capture.png";
			ofSaveScreen(filename);
		}
		break;
	}
	if ( key >= 48 && key <= 57 ) {
		shared_ptr< ttCluster > cluster = m_world->getCluster( key - 48 );
		if ( cluster ) m_world->navigateToCluster(cluster,1.);
	}
	m_last_action = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	m_keydown[key] = false;
	m_last_action = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	m_last_action = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	m_last_action = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if ( button == 0 ) {
		greenButton();
	} else {
		redButton();
	}
	m_last_action = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	m_last_action = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
