#include "ttSerialDevice.h"

ttSerialDevice::ttSerialDevice() {

}
ttSerialDevice::~ttSerialDevice() {
	if ( m_connection.isInitialized() ) {
		m_connection.close();
	}
}
//
//
//
bool ttSerialDevice::setup( int device ) {
	map< string, string > devices = listDevices();
	if ( device >= 0 && device < devices.size() ) {
		map< string, string >::iterator it = devices.begin();
		for ( int i = 0; i  < device; i++ ) it++;
		return setup( it->second );
	}
	return false;
}
bool ttSerialDevice::setup( string port ) {
	m_connection.setup( port, 9600 );
	if ( !m_connection.isInitialized() ) {
		ofLogError("ttSerialDevice::setup") << "unable to initialise connection";
	}
	return m_connection.isInitialized();
}
void ttSerialDevice::update() {
	if ( m_connection.isInitialized() ) {
		int available = m_connection.available();
		if ( available > 0 ) {
			unsigned char* buffer = new unsigned char [ available + 1 ];
			buffer[ available ] = 0;
			m_connection.readBytes(buffer,available);
			//
			//
			//
			bool partial_message = m_buffer.tellp() > 0;
			for ( int i = 0; i < available; i++ ) {
				if ( partial_message ) {
					if ( buffer[ i ] == ']' ) { // end message
						processBuffer();
						return;
					} else {
						m_buffer << buffer[ i ];
					}
				} else if ( buffer[ i ] == '[' ) { // start of message
					partial_message = true;
				}
			}
		}
	}
}
map< string, string > ttSerialDevice::listDevices( int min_port, int max_port ) {
	map< string, string > devices;
	ofSerial connection;
	for ( int i = min_port; i <= max_port; i++ ) {
		string port = "COM" + ofToString( i );
		connection.setup( port, 9600 );
		if ( connection.isInitialized() ) {
			connection.writeByte('i');
			float start = ofGetElapsedTimef();
			bool done = false;
			bool message_body = false;
			ostringstream str_buffer;
			while( !done && ofGetElapsedTimef() - start < 4. ) {
				int available = connection.available();
				if ( available > 0 ) {
					char c = connection.readByte();
					if ( message_body ) {
						if ( c == ']' ) { // end message
							vector<string> parts = ofSplitString(str_buffer.str(),",");
							str_buffer.clear();
							str_buffer.str("");
							message_body = false;
							if ( parts.size() >= 2 ) {
								string command = parts[ 0 ];
								if ( command == "id" ) {
									string name = parts[ 1 ];
									devices[ name ] = port;
									done = true;
								}
							} 
						} else {
							str_buffer << c;
						}
					} else if ( c == '[' ) { // start of message
						message_body = true;
					}
				}
			}
			connection.close();
		}
	}
	return devices;
}