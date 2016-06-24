#include "ttSurface.h"

ttSurface::ttSurface() {
	
}
ttSurface::~ttSurface() {

}
void ttSurface::setup( map< string, float > param, function<ofPoint(map< string, float >&,float,float)> func) {
	m_param = param;
	m_func = func;
	float u_step = (PI+PI)/30.;
	float v_step = 27. / 40.;
	ofxParametricSurface::setup(-PI,PI,-2,25,u_step,v_step);
}
//
//
//
ofPoint ttSurface::valueForPoint(float u,float v) {
	return m_func( m_param, u, v );
}
ofVec2f ttSurface::texCoordForPoint(float u,float v,ofPoint value) {
        float x = ofMap(u,getUMin(),getUMax(),0,1);
        float y = ofMap(v,getVMin(),getVMax(),0,1);
        return ofVec2f(x,y);
}


ofVec2f ttSurface::backTexCoordForPoint(float u,float v,ofPoint value) {
        float x = ofMap(u,getUMin(),getUMax(),1,0);
        float y = ofMap(v,getVMin(),getVMax(),1,0);
        return ofVec2f(x,y);
}