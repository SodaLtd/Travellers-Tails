#pragma once

#include "ofMain.h"
#include "ttNode.h"

class ttWorld;

class ttTagCloud : public ofNode {
public:
	class ttTag : public ofNode {
	public:
		ttTag();
		~ttTag();
		//
		//
		//
		void setup( string tag, float latitude, float longitude, float radius, float speed );
		void update();
		//
		// ofNode methods
		//
		void customDraw() override;
		//
		//
		//
		string m_tag;
		float m_latitude;
		float m_longitude;
		float m_orientation;
		float m_speed;
		float m_radius;
		bool m_visible;
		bool m_connected;
		float m_width;
	};
	//
	//
	//
	ttTagCloud();
	~ttTagCloud();
	//
	//
	//
	void setup( shared_ptr<ttWorld> world );
	void update();
	void drawTags( vector< shared_ptr< ttNode > >& connected_items );
	//
	// ofNode methods
	//
	//void customDraw() override;
	//
	//
	//
	void addTag( string tag );
	void clear() { m_tags.clear(); };
protected:
	vector< ttTag >					m_tags;
	shared_ptr<ttWorld>				m_world;
};