#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	int width = 3840/2;
	int height = 2160/2;
#ifdef _DEBUG
	ofSetupOpenGL(width,height,OF_WINDOW);			// <-------- setup the GL context
#else
	ofSetupOpenGL(width,height,OF_FULLSCREEN);			// <-------- setup the GL context
#endif

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
