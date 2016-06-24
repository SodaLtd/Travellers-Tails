#include "ttShip.h"

ttShip::ttShip() {
}
ttShip::~ttShip() {
}
//
//
//
void ttShip::setLED( bool red, bool green ) {
	if ( m_connection.isInitialized() ) {
		string command = red ? "r" : "f";
		m_connection.writeBytes(( unsigned char * ) command.c_str(), command.length() );
		command = green ? "g" : "b";
		m_connection.writeBytes(( unsigned char * ) command.c_str(), command.length() );
	}
}
void ttShip::processBuffer() {
	string command = m_buffer.str();
	m_buffer.clear();
	m_buffer.str("");
	//printf( "button: %s\n", command.c_str() );
	if ( command == "RED" ) {
		ofNotifyKeyPressed('[');
	} else if ( command == "GREEN" ) {
		ofNotifyKeyPressed(']');
	}
}
