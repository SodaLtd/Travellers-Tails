#include "ttLeaf.h"
#include "ttCache.h"
#include "ttFog.h"

const int k_cp_dim = 5;
const int k_mesh_resolution = 20;

ttLeaf::ttLeaf() {

}
ttLeaf::~ttLeaf() {

}
//
//
//
bool ttLeaf::setup( string filename, float max_width, float max_height ) {
	m_morph.reset(0.);
	m_morphing = false;
	m_current_target = PLANE;
#ifdef _DEBUG
	printf( "ttLeaf::setup : loading image : %s\n", filename.c_str() );
#endif
	m_image = ttCache::shared.load(filename);
	if ( m_image ) {
		float image_width = m_image->getWidth();
		float image_height = m_image->getHeight();
		m_scale = image_width > image_height ? max_width / image_width : max_height / image_height;
		m_width = image_width * m_scale;
		m_height = image_height * m_scale;
		//printf( "leaf generatePlaneControlPoints\n" );
#ifdef _DEBUG
	printf( "ttLeaf::setup : generating plane control points : width=%f height=%f\n", m_width, m_height );
#endif
		generatePlaneControlPoints();
		//printf( "leaf generateLeafControlPoints\n" );
#ifdef _DEBUG
	printf( "ttLeaf::setup : generating leaf control points : width=%f height=%f\n", m_width, m_height );
#endif
		generateLeafControlPoints();
		//printf( "leaf createMesh\n" );
#ifdef _DEBUG
	printf( "ttLeaf::setup : creating mesh\n" );
#endif
		createMesh();
		return true;
	}
	return false;
}
void ttLeaf::update() {
	if ( m_morphing ) {
		m_morph.update(1./ofGetFrameRate());
		m_morphing = m_morph.isOrWillBeAnimating();
		float morph = m_morph;
		for ( int i = 0; i < m_plane_verticies.size(); i++ ) {
			m_mesh.setVertex(i,m_plane_verticies[ i ].interpolated(m_leaf_verticies[ i ], morph));
		}
	}
}
void ttLeaf::draw() {
	ofPushMatrix();
	float morph = m_morph;
	//ofRotateX(135.*morph);
	//ofRotateX(-90.*morph);
	ofRotateX(-80.*morph);
	float z_offset = ofLerp(m_plane_verticies.back().z,m_leaf_verticies.back().z,morph);
	ofTranslate(0.,-z_offset,0.);
	shared_ptr<ttFog> fog = ttFog::shared();

	if ( m_image ) {
		if ( !m_image->isUsingTexture() ) { // ensure images loaded in another thread are loaded as textures
			m_image->setUseTexture(true);
			m_image->update();
		}
		m_image->bind();
		fog->begin();
		fog->setUniformTexture("texture", m_image->getTextureReference() , 1 );
		fog->setUniform1i("mode",0);
	}
	m_mesh.draw(OF_MESH_FILL);
	if ( m_image ) {
		fog->end();
		m_image->unbind();
	}
	/*
	ofPushStyle();
	ofSetColor(ofColor::red);
	ofCircle(m_top_left,1.);
	ofCircle(m_top_right,1.);
	ofCircle(m_bottom_left,1.);
	ofCircle(m_bottom_right,1.);
	ofPopStyle();
	*/
	//
	// update connection points 
	//
	ofMatrix4x4 rotation_m;
	rotation_m.makeRotationMatrix(-90.*morph,1.,0.,0.);
	ofMatrix4x4 translate_m;
	translate_m.makeTranslationMatrix(0.,-z_offset,0.);
	m_connection_points.clear();
	ofVec3f tp = m_top_left * rotation_m;
	tp = tp * translate_m;
	m_connection_points.push_back(tp);
	tp = m_top_right * rotation_m;
	tp = tp * translate_m;
	m_connection_points.push_back(tp);
	tp = m_bottom_right * rotation_m;
	tp = tp * translate_m;
	m_connection_points.push_back(tp);
	tp = m_bottom_left * rotation_m;
	tp = tp * translate_m;
	m_connection_points.push_back(tp);
	//
	//
	//
	ofPopMatrix();
}
//
// ofxParametricSurface overrides
//
ofPoint ttLeaf::valueForPoint(float u,float v) {

	ofVec3f p;
	map< pair< float, float >, map< pair< int, int >, float > >::iterator it = m_blending.find(pair<float,float>(u,v));
	if ( it != m_blending.end() ) {
		int limit = k_cp_dim - 1;
        for (int i=0;i<=limit;i++) {
            for (int j=0;j<=limit;j++) {
				p += m_plane_cp[ i ][ j ].interpolated( m_leaf_cp[ i ][ j ], m_morph ) * it->second[ pair<int,int>(i , j) ];
            }
        }
	} else {
		int limit = k_cp_dim - 1;
		pair< float, float > uv( u, v );
        for ( int i=0; i<=limit; i++ ) {
			float bi = bezierBlend(i,u,limit);
            for ( int j=0; j<=limit; j++ ) {
				float bj = bezierBlend(j,v,limit);
				float b = bi * bj;
				p += m_plane_cp[ i ][ j ].interpolated( m_leaf_cp[ i ][ j ], m_morph ) * b;
				m_blending[ uv ][ pair< int, int >( i, j ) ] = b;
            }
        }
	}
	return p;
}
//
//
//
void ttLeaf::morph( morph_target target, float time, float delay ) {
	if ( m_current_target != target ) {
		m_current_target = target;
		m_morph.setDuration(time);
		m_morph.animateToAfterDelay(target,delay);
		m_morphing = true;
	}
}

ofVec2f ttLeaf::texCoordForPoint(float u,float v,ofPoint value) {
	//return ofVec2f( u * m_width, v * m_height );
	return ofVec2f( u, v );
}

ofVec3f ttLeaf::getNearestConnectionPoint( ofVec3f p ) {
	float min_dist = numeric_limits<float>::max();
	int nearest = -1;
	for ( int i = 0; i < m_connection_points.size(); i++ ) {
		float distance = p.distance(m_connection_points[i]);
		if ( distance < min_dist ) {
			min_dist = distance;
			nearest = i;
		}
	}
	return nearest >= 0 ? m_connection_points[ nearest ] : ofVec3f();
}

void ttLeaf::generatePlaneControlPoints() {
	float base_x = -(m_width/2);
	float base_y = -m_height;
	int limit = k_cp_dim - 1;
	for ( int i = 0; i <= limit; i++ ) {
		for ( int j = 0; j <= limit; j++ ) {
            m_plane_cp[i][j].x = ofMap(i, 0, limit, base_x, base_x + m_width);
            m_plane_cp[i][j].y = ofMap(j, 0, limit, base_y, base_y + m_height);
            m_plane_cp[i][j].z = 0;
		}
	}
	m_top_left.set( base_x, base_y, 0. );
	m_top_right.set( base_x + m_width, base_y, 0. );
	m_bottom_left.set( base_x, base_y + m_height, 0. );
	m_bottom_right.set( base_x + m_width, base_y + m_height, 0. );
}
void ttLeaf::generateLeafControlPoints() {
		float half_width = m_width / 2.;
		float half_height = m_height / 2.;
		//
		// vertical positions
		//
		float base_y	= -m_height;
		float middle	= base_y + m_height * ofRandom( .25, .75 ); 
		float shoulder	= ofRandom( base_y, middle );
		float hip		= ofRandom( middle, m_height );
		//
		// widths
		//
		float middle_width0 = half_width * ofRandom( .5, 1. );
		float middle_width1 = middle_width0 * ofRandom( .25, .75 );
		float shoulder_width0 = middle_width0 * ofRandom( .25, 1. );
		float shoulder_width1 = shoulder_width0 * ofRandom( .25, .75 );
		float hip_width0 = middle_width0 * ofRandom( .25, 1. );
		float hip_width1 = hip_width0 * ofRandom( .25, .75 );
		//
		// displacement
		//
		float max_displacement = half_width;
		float top_displacement = ofRandom( -max_displacement, max_displacement); 
		float shoulder_displacement0 = ofRandom( -max_displacement, max_displacement); 
		float shoulder_displacement1 = ofRandom( -max_displacement, max_displacement); 
		float middle_displacement0 = ofRandom( -max_displacement, max_displacement); 
		float middle_displacement1 = ofRandom( -max_displacement, max_displacement); 
		float hip_displacement0 = ofRandom( -max_displacement, max_displacement); 
		float hip_displacement1 = ofRandom( -max_displacement, max_displacement); 
		float bottom_displacement = ofRandom( max_displacement, max_displacement); 
		//
		// move top and bottom into center
		//
		int limit = k_cp_dim - 1;
		for ( int x = 0; x <= limit; x++ ) {
			m_leaf_cp[ x ][ 0 ].x =
			m_leaf_cp[ x ][ limit ].x = 0;
			m_leaf_cp[ x ][ 0 ].y = base_y;
			m_leaf_cp[ x ][ limit ].y = 0;
			m_leaf_cp[ x ][ 0 ].z = top_displacement;
			m_leaf_cp[ x ][ limit ].z = bottom_displacement;
		}
		//
		// move other points into position
		//
		//
		// shoulder
		//
		int y = 1;
		m_leaf_cp[ 0 ][ y ].y = 
		m_leaf_cp[ limit ][ y ].y = shoulder;
		m_leaf_cp[ 0 ][ y ].x = -shoulder_width0;
		m_leaf_cp[ limit ][ y ].x = +shoulder_width0;
		m_leaf_cp[ 0 ][ y ].z = 
		m_leaf_cp[ limit ][ y ].z = shoulder_displacement0;
		m_leaf_cp[ 1 ][ y ].y = 
		m_leaf_cp[ limit - 1 ][ y ].y = shoulder;
		m_leaf_cp[ 1 ][ y ].x = -shoulder_width1;
		m_leaf_cp[ limit - 1 ][ y ].x = +shoulder_width1;
		m_leaf_cp[ 1 ][ y ].z = 
		m_leaf_cp[ limit - 1 ][ y ].z = shoulder_displacement1;
		//
		// middle
		//
		y = 2;
		m_leaf_cp[ 0 ][ y ].y = 
		m_leaf_cp[ limit ][ y ].y = middle;
		m_leaf_cp[ 0 ][ y ].x = -middle_width0;
		m_leaf_cp[ limit ][ y ].x = +middle_width0;
		m_leaf_cp[ 0 ][ y ].z = 
		m_leaf_cp[ limit ][ y ].z = middle_displacement0;
		m_leaf_cp[ 1 ][ y ].y = 
		m_leaf_cp[ limit - 1 ][ y ].y = middle;
		m_leaf_cp[ 1 ][ y ].x = -middle_width1;
		m_leaf_cp[ limit - 1 ][ y ].x = +middle_width1;
		m_leaf_cp[ 1 ][ y ].z = 
		m_leaf_cp[ limit - 1 ][ y ].z = middle_displacement1;
		//
		// hips
		//
		y = 3;
		m_leaf_cp[ 0 ][ y ].y = 
		m_leaf_cp[ limit ][ y ].y = hip;
		m_leaf_cp[ 0 ][ y ].x = -hip_width0;
		m_leaf_cp[ limit ][ y ].x = +hip_width0;
		m_leaf_cp[ 0 ][ y ].z = 
		m_leaf_cp[ limit ][ y ].z = hip_displacement0;
		m_leaf_cp[ 1 ][ y ].y = 
		m_leaf_cp[ limit - 1 ][ y ].y = hip;
		m_leaf_cp[ 1 ][ y ].x = -hip_width1;
		m_leaf_cp[ limit - 1 ][ y ].x = +hip_width1;
		m_leaf_cp[ 1 ][ y ].z = 
		m_leaf_cp[ limit - 1 ][ y ].z = hip_displacement1;		

}
void ttLeaf::createMesh() {
	//
	// create plane verticies and texture coords
	//
	m_morph.reset(PLANE);
	m_plane_verticies.clear();
	vector< ofVec2f > texcoords;
	float dim = k_mesh_resolution - 1;
	for ( int i = 0; i < k_mesh_resolution; i++ ) {
		float u = ( float ) i / dim;
		for ( int j = 0; j < k_mesh_resolution; j++ ) {
			float v = ( float ) j / dim;
			ofVec3f p = valueForPoint( u, v );
			m_plane_verticies.push_back(p);
			texcoords.push_back( ofVec2f(u*m_image->getWidth(),v*m_image->getHeight()) );
		}
	}
	//
	// create leaf verticies
	//
	m_morph.reset(LEAF);
	m_leaf_verticies.clear();
	for ( int i = 0; i < k_mesh_resolution; i++ ) {
		float u = ( float ) i / dim;
		for ( int j = 0; j < k_mesh_resolution; j++ ) {
			float v = ( float ) j / dim;
			ofVec3f p = valueForPoint( u, v );
			m_leaf_verticies.push_back(p);
		}
	}
	m_morph.reset(PLANE);
	//
	// create mesh
	//
	m_mesh = ofMesh::plane(m_width, m_height, k_mesh_resolution, k_mesh_resolution, OF_PRIMITIVE_TRIANGLES);
	vector< ofVec3f > mesh_verticies = m_mesh.getVertices();
	float texture_scale = 1./m_scale;
    for ( int i = 0; i < mesh_verticies.size(); i++) {
        m_mesh.setTexCoord(i, texcoords[ i ]);
		m_mesh.setVertex(i,m_plane_verticies[i]);
    }
}

float ttLeaf::bezierBlend(int k, double mu, int n) {
    int nn,kn,nkn;
    double blend=1;
    
    nn = n;
    kn = k;
    nkn = n - k;
    
    while (nn >= 1) {
        blend *= nn;
        nn--;
        if (kn > 1) {
            blend /= (double)kn;
            kn--;
        }
        if (nkn > 1) {
            blend /= (double)nkn;
            nkn--;
        }
    }

    if (k > 0)
        blend *= pow(mu,(double)k);
    if (n-k > 0)
        blend *= pow(1-mu,(double)(n-k));
    
    return(blend);

}
