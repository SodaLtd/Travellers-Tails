#include "ttFan.h"

ttFan::ttFan() {

}
ttFan::~ttFan() {
	if ( m_connection.isInitialized() ) {
		m_connection.close();
	}
}
//
//
//
bool ttFan::setup( string port ) {
	m_speed = -1;
	m_connection.setup( port, 9600 );
	if ( !m_connection.isInitialized() ) {
		ofLogError("ttFan::setup") << "unable to initialise connection";
	}
	return m_connection.isInitialized();
}
void ttFan::setSpeed( float speed ) {
	if ( m_connection.isInitialized() ) {
		int value = ofMap( speed, 0., 1., 0, 255, true );
		if ( value != m_speed ) {
			m_speed = value;
			string command = ofToString( m_speed ) + "\n";
			m_connection.writeBytes(( unsigned char * ) command.c_str(), command.length() );
		}
	}
}
