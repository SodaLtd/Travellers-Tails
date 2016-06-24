#include "ttController.h"

ttController::ttController() {

}
ttController::~ttController() {
	if ( m_connection.isInitialized() ) {
		m_connection.close();
	}
}
//
//
//
void ttController::processBuffer() {
	string command = m_buffer.str();
	vector< string > parts = ofSplitString( command, "," );
	if ( parts.size() == 4 ) {
		if ( parts[ 0 ] == "euler" ) {
			m_euler.x = ofFromString<float>( parts[ 1 ] );
			m_euler.y = ofFromString<float>( parts[ 2 ] );
			m_euler.z = ofFromString<float>( parts[ 3 ] );
			updateMinMax();
		} else if ( parts[ 0 ] == "gyro" ) {
			m_gyro.x = ofFromString<float>( parts[ 1 ] );
			m_gyro.y = ofFromString<float>( parts[ 2 ] );
			m_gyro.z = ofFromString<float>( parts[ 3 ] );
			updateMinMax();
		}
	}
	m_buffer.clear();
	m_buffer.str("");
}
void ttController::updateMinMax() {
	bool change = false;
	for ( int i = 0; i < 3; i++ ) {
		if ( m_euler[i] > m_euler_max[i] )  {
			m_euler_max[i] = m_euler[i]; 
			change = true;
		}
		if ( m_euler[i] < m_euler_min[i] ) {
			m_euler_min[i] = m_euler[i]; 
			change = true;
		}
		if ( m_gyro[i] > m_gyro_max[i] )  {
			m_gyro_max[i] = m_euler[i]; 
			change = true;
		}
		if ( m_gyro[i] < m_gyro_min[i] )  {
			m_gyro_min[i] = m_euler[i]; 
			change = true;
		}
	}
	if ( change ) {
		//printf( "euler max change [%f,%f,%f]", m_euler_max.x, m_euler_max.y, m_euler_max.z );
	}
}