#ifndef _IFont_H_
#define _IFont_H_

#include "GUIbase.h"
#include "IFont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef _DEBUG
#	pragma comment(lib, "freetype.lib")
//#	pragma comment(lib, "freetype26d.lib")
#else
#	pragma comment(lib, "freetype.lib")
//#	pragma comment(lib, "freetype26.lib")
#endif

#define SXF_VERSION 2
#define SXF_MAGICK 'DFXS'

namespace gui
{
	// basePath/fonts/name.ttf // ttf font source file
	// basePath/fonts/name_<size>.sxf // sxf font cache file

	class IFontManager;
	class CFont: public IFont
	{
	public:
		~CFont();

		void load(IFontManager *pFontManager, CTextureManager *pTextureManager, const WCHAR *szFont, UINT size, STYLE style, int iBlurRadius = 0, float fScale = 1.0f);
		void release();

		void save();

		struct CharDesc
		{
			UINT id;
			short x;
			short y;
			float width;
			float height;
			float tx;
			float ty;
			float twidth;
			float theight;
			float xoffset;
			float yoffset;
			float xadvantage;
			short texid;
			CharDesc():id(0), xadvantage(0)
			{
			}
		};

		struct vertex
		{
			float3_t Pos;
			float2_t Tex;
			//DWORD color;
		};


		const CharDesc* getChar(UINT id);


		IGXTexture2D* getAPITexture(UINT i) override;

		void getStringMetrics(const StringW &str, UINT *width, UINT *height, UINT *vertexCount, UINT *indexCount, UINT *strCount, char_rects *pcr = NULL);

		void buildString(const StringW &str, UINT decoration, TEXT_ALIGN textAlign, IGXRenderBuffer **ppVertexBuffer, IGXIndexBuffer **ppIndexBuffer, UINT *vertexCount, UINT *indexCount, UINT * lineIndexCount, UINT iAreaWidth = 0, UINT iFirstShift = 0, UINT * pStrWidth = NULL);
	protected:

		struct SXFheader
		{
			SXFheader():version(SXF_VERSION), magick(SXF_MAGICK)
			{
			}
			DWORD magick;
			int version;
			int charCount;
			int size;
			int texCount;
		};

		struct symRange
		{
			UINT start;
			UINT end;
		};

		struct chardata
		{
			int w;
			int h;
			int x;
			int y;
			bool ins;
			int c;
			byte * data;
			int like;
			int xo;
			int yo;
			int xa;
			bool operator<(const CFont::chardata & a)
			{
				return(h > a.h);
			}
			bool operator>(const CFont::chardata & a)
			{
				return(h < a.h);
			}
		};

		bool place(Array<chardata> &list, int width, int height);

		IFontManager *m_pFontManager = NULL;
		CTextureManager *m_pTextureManager = NULL;

		FT_Face m_pFTfontFace = NULL;

		SXFheader m_header;
		Array<CharDesc> m_chars;

		STYLE m_style;
		bool m_bEmulateBold;
		bool m_bEmulateItalic;

		StringW m_szFontName;
		StringW m_szFontChars;

		UINT m_iFontSize;

		Array<IGXTexture2D*> m_vpTextures;
		Array<byte*> m_ppTextures;

		bool m_bHasBeenChanged = false;

		void buildFont();

		void loadChar(UINT id);

		void loadFTfontFace();
		void freeFTfontFace();

		void generateBase();
		void regen();
		void addChar(WCHAR c, bool full = true);

		bool CFont::imageEqual(const chardata &a, const chardata &b);

		static const symRange* getSymRange(UINT sym);

		int m_iBlurRadius = 0;

		float m_fScale = 1.0f;
		void applyScale(CharDesc *pCd);
	};



	typedef const CFont::CharDesc * CPFCHAR;

	class IFontManager
	{
	public:
		IFontManager(const WCHAR *szResourceDir, CTextureManager *pTextureManager);
		~IFontManager();

		CFont* getFont(const WCHAR *szName, UINT size, CFont::STYLE style = CFont::STYLE_NONE, int iBlurRadius = 0, float fScale = 1.0f);
		FT_Library requestFT();
		
		const StringW& getResourceDir()
		{
			return(m_wsResourceDir);
		}

		void getFontIndexBuffer(UINT uChars, IGXIndexBuffer **ppIndexBuffer);

	protected:
		StringW m_wsResourceDir; 
		CTextureManager *m_pTextureManager;

		AssotiativeArray<StringW, CFont> m_mFonts;

		FT_Library m_pFT;

		UINT m_uFontIBChars = 0;
		IGXIndexBuffer *m_pFontIB = NULL;
	};

};

#endif
