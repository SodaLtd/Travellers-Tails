#include "ttTextRenderer.h"

ttTextRenderer ttTextRenderer::shared;

bool ttTextRenderer::setup( string file_name, float letter_spacing ) {
	if ( m_font.loadFont(file_name) ) {
		m_font.setLetterSpacing( letter_spacing );
		return true;
	}
	return false;
}

float ttTextRenderer::getTextWidth( string str, float size ) {
	return m_font.stringWidth( str, size );
}

float ttTextRenderer::drawText( string str, float x, float y, float size, HAlign h_align) {
	float x_ref = x;
	if ( h_align == HAlign::Centre ) {
		x_ref = x - m_font.stringWidth( str, size ) / 2.f; 
	} else if ( h_align == HAlign::Right ) {
		x_ref = x - m_font.stringWidth( str, size );
	}
	m_font.drawString( str, x_ref, y, size );
	return m_font.stringHeight("W",size) * 1.5; 
}

ofRectangle ttTextRenderer::drawTextInRect( string str, ofRectangle rect, float size, HAlign h_align, VAlign v_align, bool measure ) {
	//
	// get layout
	//
	if ( !m_layout_cache[ str ].match( rect.width, size, h_align, v_align ) ) {
		//
		// create new cache entry
		//
		m_layout_cache[ str ].m_size		= size;
		m_layout_cache[ str ].m_line_height	= m_font.stringHeight("W",size) * 1.5; 
		m_layout_cache[ str ].m_h_align		= h_align;
		m_layout_cache[ str ].m_v_align		= v_align;
		m_layout_cache[ str ].m_lines		= splitString( str, size, rect.width );
		m_layout_cache[ str ].m_width		= rect.width;
		m_layout_cache[ str ].m_height		= m_layout_cache[ str ].m_lines.size() * m_layout_cache[ str ].m_line_height;
	}
	//
	// draw layout
	//
	ofRectangle calculated_bounds = drawLayout( m_layout_cache[ str ], rect, measure );
	//
	// update layout
	//
	m_layout_cache[ str ].m_width = calculated_bounds.width;
	return calculated_bounds;
}

vector< string > ttTextRenderer::splitString( string str, float size, float max_width ) {
	vector< string > lines;
	if ( ( str.find("\n") == string::npos && m_font.stringWidth( str, size ) < max_width ) || ( str.find(".") == string::npos && str.find(",") == string::npos && str.find(" ") == string::npos ) ) { // fits or can't be split
		lines.push_back(str);
	} else {
		string out_str = str;
		while( out_str.length() > 0 ) {
			int start = 0;
			int end = out_str.length() - 1;
			string line = start > end ? out_str.substr(start,end) : out_str;
			float width = m_font.stringWidth( line, size ); 
			while( width >= max_width ) {
				end--;
				unsigned char c = out_str.at(end);
				while( !( isspace( c ) || ispunct( c ) ) && end > start ) {
					end--;
					c = out_str.at(end);
				}
				line = out_str.substr(start,end);
				width = m_font.stringWidth( line, size ); 
			}
			/*
			if ( line == str ) {
				lines.push_back( line );
				return lines;
			}
			*/
			//
			// trim ends
			//
			while( line.size() > 0 && ( line.at( 0 ) == ' ' || line.at( 0 ) == ' \n' ) ) line.erase( 0, 1 );
			while( line.size() > 0 && ( line.at( line.size() - 1 ) == ' ' || line.at( line.size() - 1 ) == '\n' ) ) line.erase( line.size() - 1, 1 );
			//
			// process newlines
			//
			vector< string > newlines = ofSplitString(line,"\n",false,true);
			//
			// add to lines
			//
			for ( auto& l : newlines ) {
				lines.push_back( l );
			}
			if ( end + 1 >= out_str.length() ) break;
			out_str = out_str.substr(end);
		}
	}
	return lines;
}
ofRectangle ttTextRenderer::drawLayout(ttTextLayout& layout, ofRectangle& rect, bool measure) {
	float x_ref = rect.x;
	if ( layout.m_h_align == HAlign::Centre ) {
		x_ref += rect.width / 2.f;
	} else if ( layout.m_h_align == HAlign::Right ) {
		x_ref += rect.width;
	}
	float y = rect.getTop() + layout.m_line_height; // default to vertical top alignment
	if ( layout.m_v_align == VAlign::Centre ) {
		if ( layout.m_lines.size() <= 1 ) {
			y = ( rect.getCenter().y + layout.m_line_height / 2.f );
		} else {
			y = ( rect.getCenter().y - layout.m_height / 2.f ) + layout.m_line_height;
		}
	} else if ( layout.m_v_align == VAlign::Bottom ) {
		y = ( rect.getBottom() - layout.m_height ) + layout.m_line_height;
	}
	float min_y = y - layout.m_line_height;
	float min_x = numeric_limits<float>::max();
	float max_x = numeric_limits<float>::min();
	
	for ( auto& line : layout.m_lines ) {
		float x = x_ref;
		float width = m_font.stringWidth( line, layout.m_size );
		if ( layout.m_h_align == HAlign::Centre ) {
			x -= width / 2.f;
		} else if ( layout.m_h_align == HAlign::Right ) {
			x -= width;
		}
		if ( x < min_x ) min_x = x;
		if ( x + width > max_x ) max_x = x + width;
		if ( !measure ) {
			/*
			if ( layout.m_height > rect.height && y + layout.m_line_height > rect.getBottom() ) {
				string ellipsis = line + "...";
				m_font.drawString( ellipsis, x, y, layout.m_size );
			} else {
				m_font.drawString( line, x, y, layout.m_size );
			}
			*/
			m_font.drawString( line, x, y, layout.m_size );
		}
		y += layout.m_line_height;
	}
	y -= layout.m_line_height;
	return ofRectangle( min_x, min_y, max_x - min_x, y - min_y );
}

