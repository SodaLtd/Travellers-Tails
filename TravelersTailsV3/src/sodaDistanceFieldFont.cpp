#include "sodaDistanceFieldFont.h"
//--------------------------


#include <algorithm>

#include "ofUtils.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "Poco/TextConverter.h"
#include "Poco/UTF8Encoding.h"
#include "Poco/Latin1Encoding.h"
#include "Poco/Latin9Encoding.h"

static bool printVectorInfo = false;
static int ttfGlobalDpi = 96;
static bool librariesInitialized = false;
static FT_Library library;

const float k_default_fontsize = 32.0;
//const float k_default_fontsize = 64.0;
const int k_default_spread = 16;
//--------------------------------------------------------
void sodaDistanceFieldFont::setGlobalDpi(int newDpi){
	ttfGlobalDpi = newDpi;
}

//--------------------------------------------------------

#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
	#include <set>
	static set<sodaDistanceFieldFont*> & all_fonts(){
		static set<sodaDistanceFieldFont*> *all_fonts = new set<sodaDistanceFieldFont*>;
		return *all_fonts;
	}

	void ofUnloadAllFontTextures(){
		set<sodaDistanceFieldFont*>::iterator it;
		for(it=all_fonts().begin();it!=all_fonts().end();it++){
			(*it)->unloadTextures();
		}
	}

	void ofReloadAllFontTextures(){
		set<sodaDistanceFieldFont*>::iterator it;
		for(it=all_fonts().begin();it!=all_fonts().end();it++){
			(*it)->reloadTextures();
		}
	}

#endif

static bool compare_cps(const charProps & c1, const charProps & c2){
	if(c1.tH == c2.tH) return c1.tW > c2.tW;
	else return c1.tH > c2.tH;
}


#ifdef TARGET_OSX
static string osxFontPathByName( string fontname ){
	CFStringRef targetName = CFStringCreateWithCString(NULL, fontname.c_str(), kCFStringEncodingUTF8);
	CTFontDescriptorRef targetDescriptor = CTFontDescriptorCreateWithNameAndSize(targetName, 0.0);
	CFURLRef targetURL = (CFURLRef) CTFontDescriptorCopyAttribute(targetDescriptor, kCTFontURLAttribute);
	string fontPath = "";
	
	if(targetURL) {
		UInt8 buffer[PATH_MAX];
		CFURLGetFileSystemRepresentation(targetURL, true, buffer, PATH_MAX);
		fontPath = string((char *)buffer);
		CFRelease(targetURL);
	}
	
	CFRelease(targetName);
	CFRelease(targetDescriptor);

	return fontPath;
}
#endif

#ifdef TARGET_WIN32
#include <map>
// font font face -> file name name mapping
static map<string, string> fonts_table;
// read font linking information from registry, and store in std::map
static void initWindows(){
	LONG l_ret;

	const wchar_t *Fonts = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

	HKEY key_ft;
	l_ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, Fonts, 0, KEY_QUERY_VALUE, &key_ft);
	if (l_ret != ERROR_SUCCESS){
	    ofLogError("sodaDistanceFieldFont") << "initWindows(): couldn't find fonts registery key";
        return;
	}

	DWORD value_count;
	DWORD max_data_len;
	wchar_t value_name[2048];
	BYTE *value_data;


	// get font_file_name -> font_face mapping from the "Fonts" registry key

	l_ret = RegQueryInfoKeyW(key_ft, NULL, NULL, NULL, NULL, NULL, NULL, &value_count, NULL, &max_data_len, NULL, NULL);
	if(l_ret != ERROR_SUCCESS){
	    ofLogError("sodaDistanceFieldFont") << "initWindows(): couldn't query registery for fonts";
        return;
	}

	// no font installed
	if (value_count == 0){
	    ofLogError("sodaDistanceFieldFont") << "initWindows(): couldn't find any fonts in registery";
        return;
	}

	// max_data_len is in BYTE
	value_data = static_cast<BYTE *>(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, max_data_len));
	if(value_data == NULL) return;

	char value_name_char[2048];
	char value_data_char[2048];
	/*char ppidl[2048];
	char fontsPath[2048];
    SHGetKnownFolderIDList(FOLDERID_Fonts, 0, NULL, &ppidl);
    SHGetPathFromIDList(ppidl,&fontsPath);*/
    string fontsDir = getenv ("windir");
    fontsDir += "\\Fonts\\";
	for (DWORD i = 0; i < value_count; ++i)
	{
			DWORD name_len = 2048;
			DWORD data_len = max_data_len;

			l_ret = RegEnumValueW(key_ft, i, value_name, &name_len, NULL, NULL, value_data, &data_len);
			if(l_ret != ERROR_SUCCESS){
			     ofLogError("sodaDistanceFieldFont") << "initWindows(): couldn't read registry key for font type";
			     continue;
			}

            wcstombs(value_name_char,value_name,2048);
			wcstombs(value_data_char,reinterpret_cast<wchar_t *>(value_data),2048);
			string curr_face = value_name_char;
			string font_file = value_data_char;
			curr_face = curr_face.substr(0, curr_face.find('(') - 1);
			fonts_table[curr_face] = fontsDir + font_file;
	}


	HeapFree(GetProcessHeap(), 0, value_data);

	l_ret = RegCloseKey(key_ft);
}


static string winFontPathByName( string fontname ){
    if(fonts_table.find(fontname)!=fonts_table.end()){
        return fonts_table[fontname];
    }
    for(map<string,string>::iterator it = fonts_table.begin(); it!=fonts_table.end(); it++){
        if(ofIsStringInString(ofToLower(it->first),ofToLower(fontname))) return it->second;
    }
    return "";
}
#endif

#ifdef TARGET_LINUX
static string linuxFontPathByName(string fontname){
	string filename;
	FcPattern * pattern = FcNameParse((const FcChar8*)fontname.c_str());
	FcBool ret = FcConfigSubstitute(0,pattern,FcMatchPattern);
	if(!ret){
		ofLogError() << "linuxFontPathByName(): couldn't find font file or system font with name \"" << fontname << "\"";
		return "";
	}
	FcDefaultSubstitute(pattern);
	FcResult result;
	FcPattern * fontMatch=NULL;
	fontMatch = FcFontMatch(0,pattern,&result);

	if(!fontMatch){
		ofLogError() << "linuxFontPathByName(): couldn't match font file or system font with name \"" << fontname << "\"";
		return "";
	}
	FcChar8	*file;
	if (FcPatternGetString (fontMatch, FC_FILE, 0, &file) == FcResultMatch){
		filename = (const char*)file;
	}else{
		ofLogError() << "linuxFontPathByName(): couldn't find font match for \"" << fontname << "\"";
		return "";
	}
	return filename;
}
#endif

bool sodaDistanceFieldFont::initLibraries(){
	if(!librariesInitialized){
	    FT_Error err;
	    err = FT_Init_FreeType( &library );

	    if (err){
			ofLogError("sodaDistanceFieldFont") << "loadFont(): couldn't initialize Freetype lib: FT_Error " << err;
			return false;
		}
#ifdef TARGET_LINUX
		FcBool result = FcInit();
		if(!result){
			return false;
		}
#endif
#ifdef TARGET_WIN32
		initWindows();
#endif
		librariesInitialized = true;
	}
    return true;
}

void sodaDistanceFieldFont::finishLibraries(){
	if(librariesInitialized){
#ifdef TARGET_LINUX
		//FcFini();
#endif
		FT_Done_FreeType(library);
	}
}

//------------------------------------------------------------------
ofShader sodaDistanceFieldFont::shader;

sodaDistanceFieldFont::sodaDistanceFieldFont(){
	bLoadedOk		= false;
	#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
		all_fonts().insert(this);
	#endif
	//cps				= NULL;
	letterSpacing = 1;
	spaceSize = 1;
	useKerning = false;
	// 3 pixel border around the glyph
	// We show 2 pixels of this, so that blending looks good.
	// 1 pixels is hidden because we don't want to see the real edge of the texture

	border			= 3;
	//visibleBorder	= 2;
	stringQuads.setMode(OF_PRIMITIVE_TRIANGLES);
	binded = false;
}

//------------------------------------------------------------------
sodaDistanceFieldFont::~sodaDistanceFieldFont(){

	if (bLoadedOk){
		unloadTextures();
	}

	#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
		all_fonts().erase(this);
	#endif
}

void sodaDistanceFieldFont::unloadTextures(){
	if(!bLoadedOk) return;
	texAtlas.clear();
	bLoadedOk = false;
}

void sodaDistanceFieldFont::reloadTextures(){
	loadFont(filename, bFullCharacterSet, simplifyAmt, dpi);
}

static bool loadFontFace(string fontname, int _fontSize, FT_Face & face, string & filename){
	filename = ofToDataPath(fontname,true);
	ofFile fontFile(filename,ofFile::Reference);
	int fontID = 0;
	if(!fontFile.exists()){
#ifdef TARGET_LINUX
		filename = linuxFontPathByName(fontname);
#elif defined(TARGET_OSX)
		if(fontname==OF_TTF_SANS){
			fontname = "Helvetica Neue";
			fontID = 4;
		}else if(fontname==OF_TTF_SERIF){
			fontname = "Times New Roman";
		}else if(fontname==OF_TTF_MONO){
			fontname = "Menlo Regular";
		}
		filename = osxFontPathByName(fontname);
#elif defined(TARGET_WIN32)
		if(fontname==OF_TTF_SANS){
			fontname = "Arial";
		}else if(fontname==OF_TTF_SERIF){
			fontname = "Times New Roman";
		}else if(fontname==OF_TTF_MONO){
			fontname = "Courier New";
		}
        filename = winFontPathByName(fontname);
#endif
		if(filename == "" ){
			ofLogError("sodaDistanceFieldFont") << "loadFontFace(): couldn't find font \"" << fontname << "\"";
			return false;
		}
		ofLogVerbose("sodaDistanceFieldFont") << "loadFontFace(): \"" << fontname << "\" not a file in data loading system font from \"" << filename << "\"";
	}
	FT_Error err;
	err = FT_New_Face( library, filename.c_str(), fontID, &face );
	if (err) {
		// simple error table in lieu of full table (see fterrors.h)
		string errorString = "unknown freetype";
		if(err == 1) errorString = "INVALID FILENAME";
		ofLogError("sodaDistanceFieldFont") << "loadFontFace(): couldn't create new face for \"" << fontname << "\": FT_Error " << err << " " << errorString;
		return false;
	}

	return true;
}

void copyFTBitmapToPixels( FT_Bitmap& bitmap, ofPixels& pixels, bool antialias = false ) {
	// Allocate Memory For The Texture Data.
	pixels.allocate(bitmap.width, bitmap.rows, 2);

	//-------------------------------- clear data:
	pixels.set(0,255); // every luminance pixel = 255
	pixels.set(1,0);


	if (antialias == true){
		ofPixels bitmapPixels;
		bitmapPixels.setFromExternalPixels(bitmap.buffer,bitmap.width,bitmap.rows,1);
		pixels.setChannel(1,bitmapPixels);
	} else {
		//-----------------------------------
		// true type packs monochrome info in a
		// 1-bit format, hella funky
		// here we unpack it:
		unsigned char *src =  bitmap.buffer;
		for(int j=0; j <bitmap.rows;j++) {
			unsigned char b=0;
			unsigned char *bptr =  src;
			for(int k=0; k < bitmap.width ; k++){
				pixels[2*(k+j*bitmap.width)] = 255;

				if (k%8==0){
					b = (*bptr++);
				}


				pixels[2*(k+j*bitmap.width) + 1] = b&0x80 ? 255 : 0;
				b <<= 1;
			}
			src += bitmap.pitch;
		}
		//-----------------------------------
	}

}

//-----------------------------------------------------------
bool sodaDistanceFieldFont::loadFont(string _filename, bool _bFullCharacterSet, float _simplifyAmt, int _dpi) {

	sodaDistanceFieldFont::initLibraries();
	if ( !shader.isLoaded() ) {
        #ifdef TARGET_OPENGLES
            shader.load( "shaders_gles/distancefield" );
        #else
		if ( ofIsGLProgrammableRenderer() ) {
            shader.load( "shaders_gl3/distancefield" );
		} else {
			shader.load( "shaders/distancefield" );
		}
        #endif
	}

	//------------------------------------------------
	if (bLoadedOk == true){

		// we've already been loaded, try to clean up :
		unloadTextures();
	}
	//------------------------------------------------
	string sdff_filename = _filename + ".sdff";
	if ( read( sdff_filename ) ) {

	} else {
		if( _dpi == 0 ){
			_dpi = ttfGlobalDpi;
		}


		bool bAntiAliased	= false;
		bLoadedOk 			= false;
		bFullCharacterSet 	= _bFullCharacterSet;
		fontSize			= k_default_fontsize; 
		simplifyAmt			= _simplifyAmt;
		dpi 				= _dpi;

		//--------------- load the library and typeface


		FT_Face face;
		if(!loadFontFace(_filename,fontSize,face,filename)){
			return false;
		} 
		FT_Set_Char_Size( face, fontSize << 6, fontSize << 6, dpi, dpi);
		lineHeight = fontSize * 1.43f;

		//------------------------------------------------------
		//kerning would be great to support:
		//ofLogNotice("sodaDistanceFieldFont") << "FT_HAS_KERNING ? " <<  FT_HAS_KERNING(face);
		//------------------------------------------------------
		useKerning = FT_HAS_KERNING(face);

		nCharacters = (bFullCharacterSet ? 256 : 128) - NUM_CHARACTER_TO_START;

		//--------------- initialize character info and textures
		cps.resize(nCharacters);

		vector<ofPixels> expanded_data(nCharacters);

		long areaSum=0;
		FT_Error err;


		//--------------------- load each char -----------------------
		for (int i = 0 ; i < nCharacters; i++){

			//------------------------------------------ anti aliased or not:
			int glyph = (unsigned char)(i+NUM_CHARACTER_TO_START);
			if (glyph == 0xA4) glyph = 0x20AC; // hack to load the euro sign, all codes in 8859-15 match with utf-32 except for this one
			err = FT_Load_Glyph( face, FT_Get_Char_Index( face, glyph ), FT_LOAD_DEFAULT );
			if(err){
				ofLogError("sodaDistanceFieldFont") << "loadFont(): FT_Load_Glyph failed for char " << i << ": FT_Error " << err;

			}

			// JONS: antialiasing should happen in post processing FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (bAntiAliased == true) FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			else FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);

			//------------------------------------------
			FT_Bitmap& bitmap= face->glyph->bitmap;


			// prepare the texture:
			/*int width  = ofNextPow2( bitmap.width + border*2 );
			int height = ofNextPow2( bitmap.rows  + border*2 );


			// ------------------------- this is fixing a bug with small type
			// ------------------------- appearantly, opengl has trouble with
			// ------------------------- width or height textures of 1, so we
			// ------------------------- we just set it to 2...
			if (width == 1) width = 2;
			if (height == 1) height = 2;*/



			// -------------------------
			// info about the character:
			cps[i].character		= i;
			cps[i].height 			= face->glyph->bitmap_top;
			cps[i].width 			= face->glyph->bitmap.width;
			cps[i].setWidth 		= face->glyph->advance.x >> 6;
			cps[i].topExtent 		= face->glyph->bitmap.rows;
			cps[i].leftExtent		= face->glyph->bitmap_left;

			int width  = cps[i].width;
			int height = bitmap.rows;


			cps[i].tW				= width;
			cps[i].tH				= height;



			GLint fheight	= cps[i].height;
			GLint bwidth	= cps[i].width;
			GLint top		= cps[i].topExtent - cps[i].height;
			GLint lextent	= cps[i].leftExtent;

			GLfloat	corr, stretch;

			//this accounts for the fact that we are showing 2*visibleBorder extra pixels
			//so we make the size of each char that many pixels bigger
			stretch = 0;//(float)(visibleBorder * 2);

			corr	= (float)(( (fontSize - fheight) + top) - fontSize);

			cps[i].x1		= lextent + bwidth + stretch;
			cps[i].y1		= fheight + corr + stretch;
			cps[i].x2		= (float) lextent;
			cps[i].y2		= -top + corr;

			// Allocate Memory For The Texture Data.
			expanded_data[i].allocate(width, height, 2);
			//-------------------------------- clear data:
			expanded_data[i].set(0,255); // every luminance pixel = 255
			expanded_data[i].set(1,0);

			areaSum += (cps[i].width+border*2)*(cps[i].height+border*2);
		}

		vector<charProps> sortedCopy = cps;
		sort(sortedCopy.begin(),sortedCopy.end(),&compare_cps);

		// pack in a texture, algorithm to calculate min w/h from
		// http://upcommons.upc.edu/pfc/bitstream/2099.1/7720/1/TesiMasterJonas.pdf
		//ofLogNotice("sodaDistanceFieldFont") << "loadFont(): areaSum: " << areaSum

		bool packed = false;
		float alpha = logf(areaSum)*1.44269;

		int w;
		int h;
		while(!packed){
			w = pow(2,floor((alpha/2.f) + 0.5)); // there doesn't seem to be a round in cmath for windows.
			//w = pow(2,round(alpha/2.f));
			h = w;//pow(2,round(alpha - round(alpha/2.f)));
			int x=0;
			int y=0;
			int maxRowHeight = sortedCopy[0].tH + border*2;
			for(int i=0;i<(int)cps.size();i++){
				if(x+sortedCopy[i].tW + border*2>w){
					x = 0;
					y += maxRowHeight;
					maxRowHeight = sortedCopy[i].tH + border*2;
					if(y + maxRowHeight > h){
						alpha++;
						break;
					}
				}
				x+= sortedCopy[i].tW + border*2;
				if(i==(int)cps.size()-1) packed = true;
			}

		}

		ofPixels atlasPixelsLuminanceAlpha;
		atlasPixelsLuminanceAlpha.allocate(w,h,2);
		atlasPixelsLuminanceAlpha.set(0,255);
		atlasPixelsLuminanceAlpha.set(1,0);

		int x=0;
		int y=0;
		int maxRowHeight = sortedCopy[0].tH + border*2;
		for(int i=0;i<(int)cps.size();i++){
			ofPixels & charPixels = expanded_data[sortedCopy[i].character];

			if(x+sortedCopy[i].tW + border*2>w){
				x = 0;
				y += maxRowHeight;
				maxRowHeight = sortedCopy[i].tH + border*2;
			}

			cps[sortedCopy[i].character].t2		= float(x + border)/float(w);
			cps[sortedCopy[i].character].v2		= float(y + border)/float(h);
			cps[sortedCopy[i].character].t1		= float(cps[sortedCopy[i].character].tW + x + border)/float(w);
			cps[sortedCopy[i].character].v1		= float(cps[sortedCopy[i].character].tH + y + border)/float(h);

			// JONS
			generateDistanceField( face, sortedCopy[i].character, charPixels );

			charPixels.pasteInto(atlasPixelsLuminanceAlpha,x+border,y+border);
			x+= sortedCopy[i].tW + border*2;
		}

		ofPixels atlasPixels;
		atlasPixels.allocate(atlasPixelsLuminanceAlpha.getWidth(),atlasPixelsLuminanceAlpha.getHeight(),4);
		atlasPixels.setChannel(0,atlasPixelsLuminanceAlpha.getChannel(0));
		atlasPixels.setChannel(1,atlasPixelsLuminanceAlpha.getChannel(0));
		atlasPixels.setChannel(2,atlasPixelsLuminanceAlpha.getChannel(0));
		atlasPixels.setChannel(3,atlasPixelsLuminanceAlpha.getChannel(1));
		texAtlas.allocate(atlasPixels,false);
		texAtlas.setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
		texAtlas.loadData(atlasPixels);
		// ------------- close the library and typeface
		FT_Done_Face(face);

		//
		// save sdf file
		//
		write( sdff_filename );
	}
  	bLoadedOk = true;
	return true;
}

ofTextEncoding sodaDistanceFieldFont::getEncoding() const {
	return encoding;
}

void sodaDistanceFieldFont::setEncoding(ofTextEncoding _encoding) {
	encoding = _encoding;
}

//-----------------------------------------------------------
bool sodaDistanceFieldFont::isLoaded() {
	return bLoadedOk;
}

//-----------------------------------------------------------
bool sodaDistanceFieldFont::isAntiAliased() {
	return true;
}

//-----------------------------------------------------------
bool sodaDistanceFieldFont::hasFullCharacterSet() {
	return bFullCharacterSet;
}

//-----------------------------------------------------------
float sodaDistanceFieldFont::getDefaultFontSize() {
	return k_default_fontsize;
}

int sodaDistanceFieldFont::getSize() {
	return fontSize;
}

//-----------------------------------------------------------
void sodaDistanceFieldFont::setLineHeight(float _newLineHeight) {
	lineHeight = _newLineHeight;
}

//-----------------------------------------------------------
float sodaDistanceFieldFont::getLineHeight(){
	return lineHeight;
}

//-----------------------------------------------------------
void sodaDistanceFieldFont::setLetterSpacing(float _newletterSpacing) {
	letterSpacing = _newletterSpacing;
}

//-----------------------------------------------------------
float sodaDistanceFieldFont::getLetterSpacing(){
	return letterSpacing;
}

//-----------------------------------------------------------
void sodaDistanceFieldFont::setSpaceSize(float _newspaceSize) {
	spaceSize = _newspaceSize;
}

//-----------------------------------------------------------
float sodaDistanceFieldFont::getSpaceSize(){
	return spaceSize;
}

//-----------------------------------------------------------
void sodaDistanceFieldFont::drawChar(int c, float x, float y, float scale) {

	if (c >= nCharacters){
		//ofLogError("sodaDistanceFieldFont") << "drawChar(): char " << c + NUM_CHARACTER_TO_START << " not allocated: line " << __LINE__ << " in " << __FILE__;
		return;
	}

	GLfloat	x1, y1, x2, y2;
	GLfloat t1, v1, t2, v2;
	t2		= cps[c].t2;
	v2		= cps[c].v2;
	t1		= cps[c].t1;
	v1		= cps[c].v1;

	x1		= (cps[c].x1*scale)+x;
	y1		= cps[c].y1;
	x2		= (cps[c].x2*scale)+x;
	y2		= cps[c].y2;

	int firstIndex = stringQuads.getVertices().size();

	if(!ofIsVFlipped()){
       y1 *= -scale;
       y2 *= -scale;  
	} else {
       y1 *= scale;
       y2 *= scale;  
	}

    y1 += y;
    y2 += y; 

	stringQuads.addVertex(ofVec3f(x1,y1));
	stringQuads.addVertex(ofVec3f(x2,y1));
	stringQuads.addVertex(ofVec3f(x2,y2));
	stringQuads.addVertex(ofVec3f(x1,y2));

	stringQuads.addTexCoord(ofVec2f(t1,v1));
	stringQuads.addTexCoord(ofVec2f(t2,v1));
	stringQuads.addTexCoord(ofVec2f(t2,v2));
	stringQuads.addTexCoord(ofVec2f(t1,v2));

	stringQuads.addIndex(firstIndex);
	stringQuads.addIndex(firstIndex+1);
	stringQuads.addIndex(firstIndex+2);
	stringQuads.addIndex(firstIndex+2);
	stringQuads.addIndex(firstIndex+3);
	stringQuads.addIndex(firstIndex);
}



//-----------------------------------------------------------
float sodaDistanceFieldFont::stringWidth(string c, float size) {
    ofRectangle rect = getStringBoundingBox(c, 0,0,size);
    return rect.width;
}


ofRectangle sodaDistanceFieldFont::getStringBoundingBox(string c, float x, float y, float size){
    float scale = size/k_default_fontsize;
    ofRectangle myRect;

    if (!bLoadedOk){
    	ofLogError("sodaDistanceFieldFont") << "getStringBoundingBox(): font not allocated";
    	return myRect;
    }

	GLint		index	= 0;
	GLfloat		xoffset	= 0;
	GLfloat		yoffset	= 0;
    int         len     = (int)c.length();
    float       minx    = -1;
    float       miny    = -1;
    float       maxx    = -1;
    float       maxy    = -1;

    if ( len < 1 || cps.empty() ){
        myRect.x        = 0;
        myRect.y        = 0;
        myRect.width    = 0;
        myRect.height   = 0;
        return myRect;
    }

    bool bFirstCharacter = true;
	int previous = -1;
	while(index < len){
		int cy = (unsigned char)c[index] - NUM_CHARACTER_TO_START;
 	    if (cy < nCharacters){ 			// full char set or not?
	       if (c[index] == '\n') {
				yoffset += lineHeight*scale;
				xoffset = 0 ; //reset X Pos back to zero
	      } else if (c[index] == ' ') {
	     		int cy = (int)'p' - NUM_CHARACTER_TO_START;
				xoffset += ( cps[cy].setWidth * letterSpacing * spaceSize ) * scale;
				 // zach - this is a bug to fix -- for now, we don't currently deal with ' ' in calculating string bounding box
		  } else if(cy > -1){
                GLint height	= cps[cy].height*scale;
            	GLint bwidth	= (cps[cy].width * letterSpacing)*scale;
            	GLint top		= (cps[cy].topExtent - cps[cy].height)*scale;
            	GLint lextent	= cps[cy].leftExtent*scale;
            	float	x1, y1, x2, y2, corr, stretch;
            	stretch = 0;//(float)visibleBorder * 2;
				corr = (float)(((fontSize - height) + top) - fontSize);
				x1		= (x + xoffset + lextent + bwidth + stretch);
            	y1		= (y + yoffset + height + corr + stretch);
            	x2		= (x + xoffset + lextent);
            	y2		= (y + yoffset + -top + corr);
				if ( useKerning  && previous >= 0 ) {
					FT_Vector  delta;
					//FT_Get_Kerning( face, previous, glyph_index, FT_KERNING_DEFAULT, &delta );
					xoffset += (cps[cy].setWidth * letterSpacing)*scale;
					previous = cy;
				} else {
					xoffset += (cps[cy].setWidth * letterSpacing)*scale;
				}
				if (bFirstCharacter == true){
                    minx = x2;
                    miny = y2;
                    maxx = x1;
                    maxy = y1;
                    bFirstCharacter = false;
                } else {
                    if (x2 < minx) minx = x2;
                    if (y2 < miny) miny = y2;
                    if (x1 > maxx) maxx = x1;
                    if (y1 > maxy) maxy = y1;
            }
		  }
	  	}
    	index++;
    }

    myRect.x        = minx;
    myRect.y        = miny;
    myRect.width    = maxx-minx;
    myRect.height   = maxy-miny;
    return myRect;
}

//-----------------------------------------------------------
float sodaDistanceFieldFont::stringHeight(string c, float size) {
    ofRectangle rect = getStringBoundingBox(c, 0,0,size);
    return rect.height;
}

float sodaDistanceFieldFont::stringHeight(string c, float size, float maxWidth ) {
	GLint		index	= 0;
	GLfloat		X		= 0;
	GLfloat		Y		= 0;
	int newLineDirection		= 1;
    float scale = size / k_default_fontsize;
    
	if(!ofIsVFlipped()){
		// this would align multiline texts to the last line when vflip is disabled
		//int lines = ofStringTimesInString(c,"\n");
		//Y = lines*lineHeight;
		newLineDirection = -1;
	}
    
	int len = (int)c.length();
    
	while(index < len){
		int cy = (unsigned char)c[index] - NUM_CHARACTER_TO_START;
        float charWidth = c[index] == ' ' || c[index] == '\n' ? 0.0 : ( cps[cy].setWidth * letterSpacing )  * scale;
		if (cy < nCharacters){ 			// full char set or not?
            if (c[index] == '\n' || ( maxWidth > 0 && X + charWidth > maxWidth ) ) {
                
				Y += ( lineHeight*newLineDirection ) * scale;
				X = 0 ; //reset X Pos back to zero
                
            }else if (c[index] == ' ') {
                int cy = (int)'p' - NUM_CHARACTER_TO_START;
                X += ( cps[cy].setWidth * letterSpacing * spaceSize ) * scale;
            } else if(cy > -1){
				X += charWidth;
            }
		}
		index++;
	}
    
    return Y + ( lineHeight*newLineDirection ) * scale;
}

void sodaDistanceFieldFont::createStringMesh(string c, float x, float y, float scale, float maxWidth){
	GLint		index	= 0;
	GLfloat		X		= x;
	GLfloat		Y		= y;
	int newLineDirection		= 1;

	if(!ofIsVFlipped()){
		// this would align multiline texts to the last line when vflip is disabled
		//int lines = ofStringTimesInString(c,"\n");
		//Y = lines*lineHeight;
		newLineDirection = -1;
	}

	int len = (int)c.length();
    float maxY = x + maxWidth;
	while(index < len){
		int cy = (unsigned char)c[index] - NUM_CHARACTER_TO_START;
		float charWidth = c[index] == ' ' || c[index] == '\n' || cy >= cps.size() ? 0.0 : ( cps[cy].setWidth * letterSpacing )  * scale;
		if (cy < nCharacters){ 			// full char set or not?
		  if (c[index] == '\n' || ( maxWidth > 0 && X + charWidth > maxY ) ) {

				Y += ( lineHeight*newLineDirection ) * scale;
				X = x ; //reset X Pos back to zero

		  }else if (c[index] == ' ') {
				 int cy = (int)'p' - NUM_CHARACTER_TO_START;
				 X += ( cps[cy].setWidth * letterSpacing * spaceSize ) * scale;
		  } else if(cy > -1){
				drawChar(cy, X, Y, scale);
				X += charWidth;
		  }
		}
		index++;
	}
}
/*
const float k_outside = 10000.0f;

class grid {
public:
	grid( int width = 0, int height = 0 ) {
		m_width = width;
		m_height = height;
		if ( m_width > 0 && m_height > 0 ) {
			allocate( width, height );
		}
	}
	//
	//
	//
	void allocate( int width, int height ) {
		m_width = width;
		m_height = height;
		m_grid.resize( width * height );
		set( ofVec2f(0,0) );
	}
	void set( ofVec2f& value ) {
		for ( int i = 0; i < m_grid.size(); i++ ) {
			m_grid[ i ] =  value;
		}
	}
	void set( int i, ofVec2f& value ) {
		if ( i >= 0 && i < m_grid.size() ) {
			m_grid[ i ] = value;
		}
	}
	void set( int x, int y, ofVec2f& value ) {
		set( ( y * m_width ) + x, value );
	}
	ofVec2f& get( int i ) {
		if ( i < 0 || i >= m_grid.size() ) {
			return ofVec2f( k_outside, k_outside );
		} 
		return m_grid[ i ];
	}
	ofVec2f& get( int x, int y ) {
		if ( x < 0 || x >= m_width || y < 0 || y >= m_height ) {
			return ofVec2f( k_outside, k_outside );
		}
		return get( ( y * m_width ) + x );
	}
	void compare( ofVec2f& cell, int x, int y, int offsetX, int offsetY ) {
		ofVec2f other( get(x+offsetX,y+offsetY) );

		other.x += offsetX;
		other.y += offsetY;

		if ( other.lengthSquared() < cell.lengthSquared() ) {
			cell = other;
		}
	}
	void propogate() {
		ofVec2f p;
 
		// pass 0
		for (int y=0; y < m_height; y++) {
 
			for (int x=0; x < m_width; x++) {
				p = get(x, y);
				compare( p, x, y, -1,  0 );
				compare( p, x, y,  0, -1 );
				compare( p, x, y, -1, -1 );
				compare( p, x, y,  1, -1 );
				set( x, y, p );
			}
 
			for (int x=m_width-1;x>=0;x--) {
				p = get(x, y);
				compare( p, x, y, 1,  0 );
				set( x, y, p);
			}
 
		}
 
		// pass 1
		for ( int y=m_height-1; y>=0; y--) {
			for ( int x=m_width-1;x>=0;x--) {
				p = get(x, y);
				compare( p, x, y,  1,  0 );
				compare( p, x, y,  0,  1 );
				compare( p, x, y, -1,  1 );
				compare( p, x, y,  1,  1 );
 
				set( x, y, p);
			}
 
			for (int x=0; x<m_width; x++) {
				p = get(x, y);
				compare( p, x, y, -1,  0 );
				set( x, y, p);
			}
 
		}

	}

	int m_width;
	int m_height;
	vector<ofVec2f> m_grid;
};
*/
 unsigned char get_SDF_radial(
		ofPixels& fontmap,
		int w, int h,
		int x, int y,
		int max_radius )
{
	//	hideous brute force method
	float d2 = max_radius*max_radius+1.0;
	int index = fontmap.getPixelIndex(x,y);
	unsigned char v = fontmap.getPixels()[ index + 1 ];

	for( int radius = 1; (radius <= max_radius) && (radius*radius < d2); ++radius ) {
		int line, lo, hi;
		//	north
		line = y - radius;
		if( (line >= 0) && (line < h) )
		{
			lo = x - radius;
			hi = x + radius;
			if( lo < 0 ) { lo = 0; }
			if( hi >= w ) { hi = w-1; }
			int idx = fontmap.getPixelIndex(lo,line);
			for( int i = lo; i <= hi; ++i )
			{
				//	check this pixel
				if( fontmap.getPixels()[idx+1] != v )
				{
					float nx = i - x;
					float ny = line - y;
					float nd2 = nx*nx+ny*ny;
					if( nd2 < d2 )
					{
						d2 = nd2;
					}
				}
				//	move on
				idx = fontmap.getPixelIndex(i,line);
			}
		}
		//	south
		line = y + radius;
		if( (line >= 0) && (line < h) )
		{
			lo = x - radius;
			hi = x + radius;
			if( lo < 0 ) { lo = 0; }
			if( hi >= w ) { hi = w-1; }
			int idx = fontmap.getPixelIndex(lo,line);
			for( int i = lo; i <= hi; ++i )
			{
				//	check this pixel
				if( fontmap.getPixels()[idx+1] != v )
				{
					float nx = i - x;
					float ny = line - y;
					float nd2 = nx*nx+ny*ny;
					if( nd2 < d2 )
					{
						d2 = nd2;
					}
				}
				//	move on
				idx = fontmap.getPixelIndex(i,line);
			}
		}
		//	west
		line = x - radius;
		if( (line >= 0) && (line < w) )
		{
			lo = y - radius + 1;
			hi = y + radius - 1;
			if( lo < 0 ) { lo = 0; }
			if( hi >= h ) { hi = h-1; }
			int idx = fontmap.getPixelIndex(line,lo);
			for( int i = lo; i <= hi; ++i )
			{
				//	check this pixel
				if( fontmap.getPixels()[idx+1] != v )
				{
					float nx = line - x;
					float ny = i - y;
					float nd2 = nx*nx+ny*ny;
					if( nd2 < d2 )
					{
						d2 = nd2;
					}
				}
				//	move on
				idx = fontmap.getPixelIndex(line,i);
			}
		}
		//	east
		line = x + radius;
		if( (line >= 0) && (line < w) )
		{
			lo = y - radius + 1;
			hi = y + radius - 1;
			if( lo < 0 ) { lo = 0; }
			if( hi >= h ) { hi = h-1; }
			int idx = fontmap.getPixelIndex(line,lo);
			for( int i = lo; i <= hi; ++i )
			{
				//	check this pixel
				if( fontmap.getPixels()[idx+1] != v )
				{
					float nx = line - x;
					float ny = i - y;
					float nd2 = nx*nx+ny*ny;
					if( nd2 < d2 )
					{
						d2 = nd2;
					}
				}
				//	move on
				idx = fontmap.getPixelIndex(line,i);
			}
		}
	}
	d2 = sqrtf( d2 );
	if( v==0 )
	{
		d2 = -d2;
	}
	d2 *= 127.5 / max_radius;
	d2 += 127.5;
	if( d2 < 0.0 ) d2 = 0.0;
	if( d2 > 255.0 ) d2 = 255.0;
	return (unsigned char)(d2 + 0.5);
}

void sodaDistanceFieldFont::generateDistanceField( FT_Face& face, int c, ofPixels& p ) {
	//
	// render hires version
	//
	FT_Set_Char_Size( face, fontSize << 6, fontSize << 6, dpi * ( k_default_spread * 2 ), dpi * ( k_default_spread * 2 ) );
	//------------------------------------------ anti aliased or not:
	int glyph = (unsigned char)(c+NUM_CHARACTER_TO_START);
	if (glyph == 0xA4) glyph = 0x20AC; // hack to load the euro sign, all codes in 8859-15 match with utf-32 except for this one
	FT_Error err = FT_Load_Glyph( face, FT_Get_Char_Index( face, glyph ), FT_LOAD_DEFAULT );
    if(err){
		ofLogError("sodaDistanceFieldFont") << "loadFont(): FT_Load_Glyph failed for char " << c << ": FT_Error " << err;

	}

	// JONS: antialiasing should happen in post processing FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	if (false) FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
	else FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);

	//------------------------------------------
	FT_Bitmap& bitmap= face->glyph->bitmap;
	ofPixels temp;
	copyFTBitmapToPixels( bitmap, temp, false );

	//
	// copy to buffer
	//
	int width = p.getWidth();
	int height = p.getHeight();
	/*
	int swidth = temp.getWidth() + ( k_default_spread * 4 );
	int sheight = temp.getHeight() + ( k_default_spread * 4 );
	*/
	int swidth = temp.getWidth() + ( k_default_spread * 8 );
	int sheight = temp.getHeight() + ( k_default_spread * 8 );
	ofPixels smooth;
	smooth.allocate( swidth , sheight, 2 );
	smooth.set(0,255);
	smooth.set(1,0);
	temp.pasteInto( smooth, k_default_spread * 2, k_default_spread * 2 );

	float scalex = ( float ) ( swidth - ( k_default_spread * 4 ) ) / ( float ) width;
	float scaley = ( float ) ( sheight - ( k_default_spread * 4 ) ) / ( float ) height;
	/*
	ofPixels distances;
	distances.allocate( width, height, 2 );
	distances.set(0,255);
	distances.set(1,0);
	*/
	float saw = swidth - ( k_default_spread * 4 );
	float sah = sheight - ( k_default_spread * 4 );
	for ( int y = 0; y < height; y++ ) {
		for ( int x = 0; x < width; x++ ) {
			//unsigned char distance = get_SDF_radial(smooth,width,height,x+k_default_spread,y+k_default_spread, k_default_spread );
			unsigned char distance = get_SDF_radial(smooth,
				swidth,sheight,
				ceilf(x*scalex + ( k_default_spread * 2 )+1), 
				ceilf(y*scaley + ( k_default_spread * 2 )+1),
				2*k_default_spread);
				/*
				x*k_default_spread + ( k_default_spread >> 1 ), 
				y*k_default_spread + ( k_default_spread >> 1 ),
				2*k_default_spread);
				*/

			int index = p.getPixelIndex( x, y );
			p.getPixels()[ index ] =
			p.getPixels()[ index + 1 ] = distance;
			/*
			if ( x == 0 || x == width - 1 || y == 0 || y == height - 1 ) {
				p.getPixels()[ index ] = 
				p.getPixels()[ index + 1 ] = 128;
			}
			*/
		}
	}
	//p.setFromPixels( distances.getPixels(), width, height, 2 );
	FT_Set_Char_Size( face, fontSize << 6, fontSize << 6, dpi, dpi);
}

ofMesh & sodaDistanceFieldFont::getStringMesh(string c, float x, float y, float size, float maxWidth){
	stringQuads.clear();
	createStringMesh(c,x,y,size/k_default_fontsize,maxWidth);
	return stringQuads;
}

ofTexture & sodaDistanceFieldFont::getFontTexture(){
	return texAtlas;
}

//=====================================================================
void sodaDistanceFieldFont::drawString(string c, float x, float y, float size, float maxWidth) {
	
	/*glEnable(GL_BLEND);
	 glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	 texAtlas.draw(0,0);*/
	
	if(bFullCharacterSet && encoding==OF_ENCODING_UTF8){
		string o;
		Poco::TextConverter(Poco::UTF8Encoding(),Poco::Latin9Encoding()).convert(c,o);
		c=o;
	}
	
	if (!bLoadedOk){
		ofLogError("sodaDistanceFieldFont") << "drawString(): font not allocated: line " << __LINE__ << " in " << __FILE__;
		return;
	};
	
	bool alreadyBinded = binded;

	if(!alreadyBinded) bind();
	shader.setUniform1f( "spread", k_default_spread );
	shader.setUniform1f( "scale", size/k_default_fontsize );

	createStringMesh(c,x,y,size/k_default_fontsize,maxWidth);
	if(!alreadyBinded) unbind();

}

//-----------------------------------------------------------
void sodaDistanceFieldFont::bind(){
	if(!binded){
		/*
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.5f);
		*/

		shader.begin();
		shader.setUniformTexture( "distances", texAtlas, 0 );
		stringQuads.clear();

		binded = true;
	}
}

//-----------------------------------------------------------
void sodaDistanceFieldFont::unbind(){
	if(binded){

		stringQuads.drawFaces();
		//
		//
		//
		shader.end();
		//glDisable(GL_ALPHA_TEST);
		binded = false;
	}
}

//-----------------------------------------------------------
int sodaDistanceFieldFont::getNumCharacters() {
	return nCharacters;
}

const char k_sdff_sig[ 4 ] = { 'S', 'D', 'F', 'F' };

void sodaDistanceFieldFont::write( string& _filename ) {
	//
	// open file
	//
	ofstream os( ofToDataPath( _filename ).c_str(), ofstream::out | ofstream::binary );
	if ( os.is_open() ) {
		//
		// write signature
		//
		os.write( k_sdff_sig, 4 );
		//
		// write font metrics
		//
		os.write( ( char * ) &bFullCharacterSet, sizeof( bool ) );
		os.write( ( char * ) &nCharacters, sizeof( int ) );
		os.write( ( char * ) &lineHeight, sizeof( float ) );
		os.write( ( char * ) &letterSpacing, sizeof( float ) );
		os.write( ( char * ) &spaceSize, sizeof( float ) );
		os.write( ( char * ) &fontSize, sizeof( int ) );
		os.write( ( char * ) &simplifyAmt, sizeof( float ) );
		os.write( ( char * ) &dpi, sizeof( int ) );
		//
		// write character metrics
		//
		int count = cps.size();
		os.write( ( char * ) &count, sizeof( int ) );
		for ( int i = 0; i < count; i++ ) {
			os.write( ( char * ) &cps[ i ], sizeof( charProps ) );
		}
		//
		// write distance field
		//
		ofPixels pixels;
		texAtlas.texData.glTypeInternal = GL_RGBA; // TODO: this is to fix a bug in setting and use of internal format
		texAtlas.readToPixels(pixels);
		ofBuffer buffer;
		ofSaveImage ( pixels, buffer );
		count = buffer.size();
		os.write( ( char * ) &count, sizeof( int ) );
		buffer.writeTo( os );
		os.close();
	} else {
		ofLogError() << "sodaDistanceFieldFont::write - unable to open file " << ofToDataPath( _filename ) << endl;
	}
}

bool sodaDistanceFieldFont::read( string& _filename ) {
	//
	// TODO: error checking
	//
	ifstream ifs( ofToDataPath( _filename ).c_str(), ifstream::in | ifstream::binary );
	if ( ifs.is_open() ) {
		//
		// read signature
		//
		char sdff_sig[ 4 ];
		ifs.read( sdff_sig, 4 );
		if( memcmp( k_sdff_sig, sdff_sig, 4 ) != 0 ) {
			ofLogError() << "sodaDistanceFieldFont::read - invalid file signature in file " << ofToDataPath( _filename ) << endl;
			return false;
		}
		//
		// read font metrics
		//
		ifs.read( ( char * ) &bFullCharacterSet, sizeof( bool ) );
		ifs.read( ( char * ) &nCharacters, sizeof( int ) );
		ifs.read( ( char * ) &lineHeight, sizeof( float ) );
		ifs.read( ( char * ) &letterSpacing, sizeof( float ) );
		ifs.read( ( char * ) &spaceSize, sizeof( float ) );
		ifs.read( ( char * ) &fontSize, sizeof( int ) );
		ifs.read( ( char * ) &simplifyAmt, sizeof( float ) );
		ifs.read( ( char * ) &dpi, sizeof( int ) );
		//
		// read character metrics
		//
		cps.clear();
		int count;
		ifs.read( ( char * ) &count, sizeof( int ) );
		for ( int i = 0; i < count; i++ ) {
			charProps cp;
			ifs.read( ( char * ) &cp, sizeof( charProps ) );
			cps.push_back( cp );
		}
		//
		// read distance field
		//
		ifs.read( ( char * ) &count, sizeof( int ) );
		char* data = new char [ count ];
		ifs.read( data, count );
		ofBuffer buffer( data, count );
		ofPixels pixels;
		ofLoadImage( pixels, buffer );
		delete [] data;
		texAtlas.setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
		texAtlas.allocate( pixels, false );
		texAtlas.loadData( pixels );
		ifs.close();
		
		
		ofSaveImage( pixels, "testinfinput.png" );
		return true;
	} else {
		ofLogError() << "sodaDistanceFieldFont::read - unable to open file " << ofToDataPath( _filename ) << endl;
	}
	return false;
}
