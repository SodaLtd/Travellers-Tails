#include "sdaHTTP.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Thread.h"
#include "Poco/Runnable.h"
#include "Poco/UUIDGenerator.h"
#include "Poco/StreamCopier.h"
#include <algorithm>
#include <functional>

using namespace sdaHTTP;

class sdaHTTPAsyncWrapper : public Poco::Runnable {
public:
	sdaHTTPAsyncWrapper() {
		m_function = nullptr;
	}
	//
	//
	//
	void run() {
		if ( m_function) m_function();
	}
	function<void(void)> m_function;
};
const string sdaHTTP_no_data = "";
static Poco::UUIDGenerator g_sdaHTTP_udid_generator;
static map< string, sdaHTTPAsyncWrapper > g_sdaHTTP_async_requests;
static shared_ptr<Poco::ThreadPool> g_sdaHTTP_async_pool = nullptr;
static shared_ptr<Poco::ThreadPool> sdaHTTP_async_pool() {
	if ( !g_sdaHTTP_async_pool ) {
		g_sdaHTTP_async_pool = shared_ptr<Poco::ThreadPool>( new Poco::ThreadPool( 2, 32 ) );
	}
	return g_sdaHTTP_async_pool;
}

template<class T>
void sendrequest( const string& method, const string& url, const string& data, T callback ){
	try {
		//
		//
		//
		Poco::URI uri(url);
		//
		//
		//
		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";
		//
		//
		//
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		Poco::Net::HTTPRequest request(method, path, Poco::Net::HTTPMessage::HTTP_1_1);
		Poco::Net::HTTPResponse response;
		//
		//
		//
		ostream& os = session.sendRequest(request);
		if ( data.length() > 0 ) {
			istringstream data_is(data);
			copy(istream_iterator<char>(data_is),
			istream_iterator<char>(),
			ostream_iterator<char>(os));
		}
		istream& rs = session.receiveResponse(response);
		callback(rs,response.getStatus());
	} catch( Poco::Exception& e ) {
		ofLogError("sdaHTTP::sendrequest") << e.what() << " : " << e.message();
	}
}
string responsetostring(istream& response_is) {
	if ( response_is.good() ) {
		ostringstream response_os;
		copy(istream_iterator<char>(response_is),
		istream_iterator<char>(),
		ostream_iterator<char>(response_os));
		return response_os.str();
	}
	return sdaHTTP_no_data;
}
//
//
//
string sdaHTTP::builduri( const string& protocol, string& host, unsigned short port, const string& path, const map<string,string>& parameters ) {
	Poco::URI uri;
	uri.setScheme(protocol);
	uri.setHost(host);
	uri.setPort(port);
	uri.setPath(path.length()>0?path:"/");
	if ( parameters.size() > 0 ) {
		string querystring;
		char prefix = 0;
		for ( auto& parameter : parameters ) {
			if ( prefix ) querystring += prefix;
			querystring += parameter.first;
			querystring += '=';
			querystring += parameter.second;
			prefix = '&';
		}
		uri.setQuery(querystring);
	}
	return uri.toString();
}
//
// sync calls / string response
// return is the HTTP status code or internal error ( should probably throw exception for this )
//
int sdaHTTP::get( const string& url, string& response ) {
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_GET, url, sdaHTTP_no_data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyToString(response_is,response);
		status = response_status;
	});
	return status;
}
int sdaHTTP::post( const string& url, const string& data, string& response ) {
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_POST, url, data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyToString(response_is,response);
		status = response_status;
	});
	return status;
}
int sdaHTTP::put( const string& url, const string& data, string& response ){
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_PUT, url, data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyToString(response_is,response);
		status = response_status;
	});
	return status;
}	
int sdaHTTP::del( const string& url, string& response ){
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_DELETE, url, sdaHTTP_no_data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyToString(response_is,response);
		status = response_status;
	});
	return status;

}
//
// sync calls / stream response
// return is the HTTP status code
//
int sdaHTTP::get( const string& url, ostream& response ){
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_GET, url, sdaHTTP_no_data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyStream(response_is, response);
		status = response_status;
	});
	return status;
}	
int sdaHTTP::post( const string& url, const string& data, ostream& response ){
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_POST, url, data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyStream(response_is, response);
		status = response_status;
	});
	return status;
}
int sdaHTTP::put( const string& url, const string& data, ostream& response ){
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_PUT, url, data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyStream(response_is, response);
		status = response_status;
	});
	return status;
}	
int sdaHTTP::del( const string& url, ostream& response ){
	int status;
 	sendrequest( Poco::Net::HTTPRequest::HTTP_DELETE, url, sdaHTTP_no_data, [&response,&status]( istream& response_is, int response_status ) {
		Poco::StreamCopier::copyStream(response_is, response);
		status = response_status;
	});
	return status;
}
//
// async calls / string response
//
//
// callback is a function with signature
// void callback( string response, int status )
//
template< class T >
string sdaHTTP::get( const string& url, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [&url,callback]() {
		string response;
		int status = get(url,response);
		callback(response,status);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}	
template< class T >
string sdaHTTP::post( const string& url, const string& data, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,data,callback]() {
		string response;
		int status = post(url,data,response);
		callback(response,status);
		g_sdaHTTP_async_requests.erase(id);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}
template< class T >
string sdaHTTP::put( const string& url, const string& data,  T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,data,callback]() {
		string response;
		int status = put(url,data,response);
		callback(response,status);
		g_sdaHTTP_async_requests.erase(id);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}
template< class T >
string sdaHTTP::del( const string& url, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,callback]() {
		string response;
		int status = del(url,response);
		callback(response,status);
		g_sdaHTTP_async_requests.erase(id);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}
//
// async calls / stream response
//
//
// callback is a function with signature
// void callback( string id, int status )
//
template< class T >
string sdaHTTP::get( const string& url, ostream& response, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,&response,callback]() {
		int status = get(url,response);
		g_sdaHTTP_async_requests.erase(id);
		callback(id,status);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}
template< class T >
string sdaHTTP::post( const string& url, const string& data, ostream& response, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,data,&response,callback]() {
		int status = post(url,data,response);
		g_sdaHTTP_async_requests.erase(id);
		callback(id,status);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}
template< class T >
string sdaHTTP::put( const string& url, const string& data, ostream& response, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,data,&response,callback]() {
		int status = put(url,data,response);
		g_sdaHTTP_async_requests.erase(id);
		callback(id,status);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id]);
	return id;
}
template< class T >
string sdaHTTP::del( const string& url, ostream& response, T callback ){
	string id = g_sdaHTTP_udid_generator.create().toString();
	g_sdaHTTP_async_requests[id].m_function = [id,url,&response,callback]() {
		int status = del(url,response);
		g_sdaHTTP_async_requests.erase(id);
		callback(id,status);
	};
	sdaHTTP_async_pool()->start(g_sdaHTTP_async_requests[id],id);
	return id;
}

//
//
//
// TODO: curently no way of stopping single async calls
void stop( string id ) {
	if ( g_sdaHTTP_async_requests.find(id) != g_sdaHTTP_async_requests.end() ) {

	}
}
void stopAll() {
	sdaHTTP_async_pool()->stopAll();
	g_sdaHTTP_async_requests.clear();
}

