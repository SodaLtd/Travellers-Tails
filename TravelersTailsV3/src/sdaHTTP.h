#pragma once

#include "ofMain.h"

namespace sdaHTTP {
	//
	//
	//
	string builduri( const string& protocol, string& host, unsigned short port, const string& path, const map<string,string>& parameters );
	//
	// sync calls / string response
	// return is the HTTP status code or internal error ( should probably throw exception for this )
	//
	int get( const string& url, string& response );	
	int post( const string& url, const string& data, string& response );
	int put( const string& url, const string& data, string& response );	
	int del( const string& url, string& response );
	//
	// sync calls / stream response
	// return is the HTTP status code
	//
	int get( const string& url, ostream& response );	
	int post( const string& url, const string& data, ostream& response );
	int put( const string& url, const string& data, ostream& response );	
	int del( const string& url, ostream& response );
	//
	// async calls / string response
	//
	//
	// callback is a function with signature
	// void callback( string response, int status )
	//
	template< class T >
	string get( const string& url, T callback );	
	template< class T >
	string post( const string& url, const string& data, T callback );
	template< class T >
	string put( const string& url, const string& data,  T callback );	
	template< class T >
	string del( const string& url, T callback );
	//
	// async calls / stream response
	//
	//
	// callback is a function with signature
	// void callback( istream& response, int status )
	//
	template< class T >
	string get( const string& url, ostream& response, T callback );	
	template< class T >
	string post( const string& url, const string& data, ostream& response, T callback );
	template< class T >
	string put( const string& url, const string& data, ostream& response, T callback );	
	template< class T >
	string del( const string& url, ostream& response, T callback );
	//
	//
	//
	void stop( string id );
	void stopAll();
};