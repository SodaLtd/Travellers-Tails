#pragma once

#include "ofMain.h"
//
//
//
#include "ft2build.h"
#include "freetype2/freetype/freetype.h"
#include "freetype2/freetype/ftglyph.h"
#include "freetype2/freetype/ftoutln.h"
#include "freetype2/freetype/fttrigon.h"

#ifdef TARGET_LINUX
#include <fontconfig/fontconfig.h>
#endif

/*
#include <vector>

#include "ofPoint.h"
#include "ofRectangle.h"
#include "ofConstants.h"
#include "ofPath.h"
#include "ofTexture.h"
#include "ofMesh.h"
#include "ofTrueTypeFont.h"
*/
class sodaDistanceFieldFont{

public:


	sodaDistanceFieldFont();
	virtual ~sodaDistanceFieldFont();
	
	//set the default dpi for all typefaces.
	static void setGlobalDpi(int newDpi);
			
	// 			-- default (without dpi), non-full char set, 96 dpi:
	bool 		loadFont(string filename, bool _bFullCharacterSet=false, float simplifyAmt=0.3, int dpi=0);
	
	bool		isLoaded();
	bool		isAntiAliased();
	bool		hasFullCharacterSet();

	float		getDefaultFontSize();
    int         getSize();
    float       getLineHeight();
  	void 		setLineHeight(float height);
	float 		getLetterSpacing();
	void 		setLetterSpacing(float spacing);
	float 		getSpaceSize();
	void 		setSpaceSize(float size);
	float 		stringWidth(string s, float size);
	float 		stringHeight(string s, float size);
	float 		stringHeight(string s, float size, float maxWidth );
	
	ofRectangle getStringBoundingBox(string s, float x, float y, float size);
	
	void 		drawString(string s, float x, float y, float size, float maxWidth = 0);
	
	//			get the num chars in the loaded char set
	int			getNumCharacters();	
	
	ofMesh & getStringMesh(string s, float x, float y, float size, float maxWidth = 0);
	ofTexture & getFontTexture();

	void bind();
	void unbind();

	ofTextEncoding getEncoding() const;
	void setEncoding(ofTextEncoding encoding);

protected:
	bool			bLoadedOk;
	bool 			bFullCharacterSet;
	int 			nCharacters;
	
	float 			lineHeight;
	float			letterSpacing;
	float			spaceSize;
	bool			useKerning;

	vector<charProps> 	cps;			// properties for each character

	int				fontSize;
	float 			simplifyAmt;
	int 			dpi;

	void 			drawChar(int c, float x, float y, float size);
	void			createStringMesh(string c, float x, float y, float size,float maxWidth = 0);
	void			generateDistanceField( FT_Face& face, int i, ofPixels& p );
	int				border;//, visibleBorder;
	string			filename;

	ofTexture texAtlas;
	bool binded;
	ofMesh stringQuads;

	static ofShader shader;
private:
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
	friend void ofUnloadAllFontTextures();
	friend void ofReloadAllFontTextures();
#endif

	void write( string& filename );
	bool read( string& filename );

	GLint blend_src, blend_dst;
	GLboolean blend_enabled;
	GLboolean texture_2d_enabled;

	ofTextEncoding encoding;
	void		unloadTextures();
	void		reloadTextures();
	static bool	initLibraries();
	static void finishLibraries();

	friend void ofExitCallback();
};


