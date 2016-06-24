#pragma once
#include "sodaDistanceFieldFont.h"


class ttTextRenderer {
public:
	enum class HAlign : char {
		Left,
		Centre,
		Right
	};
	enum class VAlign : char {
		Top,
		Centre,
		Bottom
	};
	class ttTextLayout {
	public:
		vector< string >	m_lines;
		float				m_size;
		float				m_line_height;
		HAlign				m_h_align;
		VAlign				m_v_align;
		float				m_width;
		float				m_height;
		//
		//
		//
		bool match( float width, float size, HAlign h_align, VAlign v_align ) {
			return m_width == width && m_size == size && m_h_align == h_align && m_v_align == v_align; 
		}
	};
	//
	//
	//
	bool setup( string font_file, float letter_spacing = 1. );
	//
	// 
	//
	float drawText( string str, float x, float y, float size, HAlign h_align=HAlign::Left);
	ofRectangle drawTextInRect( string str, ofRectangle rect, float size, HAlign h_align=HAlign::Left, VAlign v_align=VAlign::Top, bool measure=false );
	float getTextWidth( string str, float size );
	//
	//
	//
	static ttTextRenderer shared;
protected:
	//
	//
	//
	vector< string > splitString( string str, float size, float max_width );
	ofRectangle drawLayout( ttTextLayout& layout, ofRectangle& rect, bool measure = false );
	//
	//
	//
	sodaDistanceFieldFont		m_font;
	map< string, ttTextLayout > m_layout_cache;
};