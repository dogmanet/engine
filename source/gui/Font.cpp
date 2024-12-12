#include "Font.h"
#include <math.h>

#include FT_BITMAP_H

namespace gui
{

	void imageCopy(byte * dst, byte * src, int dstwidth, int dstheight, int srcwidth, int srcheight, int posX, int posY, int bpp)
	{
		for(int x = 0; x < srcwidth; x++)
		{
			for(int y = 0; y < srcheight; y++)
			{
				for(int c = 0; c < bpp; c++)
				{
					UINT db = (dstheight - posY - y) * dstwidth * bpp + (posX + x) * bpp + c;
					UINT sb = (srcheight - y - 1) * srcwidth * bpp + x * bpp + c;
					assert(db < (UINT)(dstwidth * dstheight * 4));
					assert(sb < (UINT)(srcwidth * srcheight * 4));
					dst[db] = max(src[sb], dst[db]);
					if((dstheight - posY - y) * dstwidth * bpp + (posX + x) * bpp + c >= dstwidth * dstheight * bpp)
					{
						printf("%d\n", (dstheight - posY - y) * dstwidth * bpp + (posX + x) * bpp + c);
					}
				}
			}
		}
	}
	
//##########################################################################

	void CFont::save()
	{
		if(!m_bHasBeenChanged)
		{
			return;
		}
		m_bHasBeenChanged = false;
		FILE * pF;
		StringW file = m_pFontManager->getResourceDir() + L"/fonts/" + m_szFontName + L"_" + StringW((int)m_iFontSize) + L"+" + StringW(m_style) + L"-" + StringW(m_iBlurRadius) + L".sxf";
		_wfopen_s(&pF, file.c_str(), L"wb");
		if(!pF)
		{
			printf(COLOR_LRED "[Error]: Unable to open \"%S\" file for writing\n" COLOR_RESET, file.c_str());
			return;
		}
		fwrite(&m_header, sizeof(SXFheader), 1, pF);
		UINT c = 0;
		for(UINT i = 0, l = m_chars.size(); i < l; i++)
		{
			if(m_chars[i].id != 0)
			{
				fwrite(&m_chars[i], sizeof(CharDesc), 1, pF);
				c++;
			}
		}
		uint32_t w, h;
		byte * data;
		for(UINT i = 0; i < m_vpTextures.size(); i++)
		{
			w = m_vpTextures[i]->getWidth();
			h = m_vpTextures[i]->getHeight();
			data = m_ppTextures[i];
			fwrite(&w, sizeof(w), 1, pF);
			fwrite(&h, sizeof(h), 1, pF);
			fwrite(data, sizeof(byte), w * h * 4, pF);
		}
		// TODO: write font textures
		fclose(pF);
	}

	CPFCHAR CFont::getChar(UINT id)
	{
		CPFCHAR ch = &m_chars[id];
		if(ch->id == 0)
		{
			loadChar(id);
			ch = &m_chars[id];
		}
		return(ch);
	}

	void CFont::loadFTfontFace()
	{
		if(!m_pFTfontFace)
		{
			StringW suf;
			switch(m_style)
			{
			case STYLE_BOLD:
				suf = L"bd";
				break;
			case STYLE_ITALIC:
				suf = L"i";
				break;
			case STYLE_BOLD_ITALIC:
				suf = L"bi";
				break;
			}
			StringW file = m_pFontManager->getResourceDir() + L"/fonts/" + m_szFontName + suf + L".ttf";
			String _f(file);
			
			if(FT_New_Face(m_pFontManager->requestFT(), _f.c_str(), 0, &m_pFTfontFace))
			{
				printf("Unable to load \"%s\" font\n", _f.c_str());
				if(m_style != STYLE_NONE)
				{
					file = m_pFontManager->getResourceDir() + L"/fonts/" + m_szFontName + L".ttf";
					_f = file;
					
					if(FT_New_Face(m_pFontManager->requestFT(), _f.c_str(), 0, &m_pFTfontFace))
					{
						printf("Unable to load \"%s\" font\n", _f.c_str());
					}
					else
					{
						m_bEmulateBold = 0 != (m_style & STYLE_BOLD);
						m_bEmulateItalic = 0 != (m_style & STYLE_ITALIC);
					}
				}
			}
			FT_Set_Pixel_Sizes(m_pFTfontFace, 0, m_iFontSize);
		}
	}

	void CFont::load(IFontManager *pFontManager, CTextureManager *pTextureManager, const WCHAR * szFont, UINT size, STYLE style, int iBlurRadius, float fScale)
	{
		if(size > 160)
		{
			size = 160;
		}
		m_fScale = fScale;
		m_pFontManager = pFontManager;
		m_pTextureManager = pTextureManager;
		m_style = style;
		freeFTfontFace();
		m_iFontSize = size;
		m_szFontName = szFont;
		m_bEmulateBold = false;
		m_bEmulateItalic = false;
		m_iBlurRadius = iBlurRadius;

		StringW file = m_pFontManager->getResourceDir() + L"/fonts/" + m_szFontName + L"_" + StringW((int)size) + L"+" + StringW((int)style) + L"-" + StringW(iBlurRadius) + L".sxf";

		FILE * pF;
		_wfopen_s(&pF, file.c_str(), L"rb");
		if(!pF)
		{
			printf(COLOR_YELLOW "[Warning]: Unable to open \"%s\" file to reading\nGenerating font\n" COLOR_RESET, String(file).c_str());
			m_header.size = size;
			generateBase();
			return;
		}

		fread(&m_header, sizeof(SXFheader), 1, pF);

		if(m_header.magick != SXF_MAGICK || m_header.version != SXF_VERSION || m_header.size != size)
		{
			m_header = SXFheader();
			fclose(pF);

			m_header.size = size;
			printf(COLOR_YELLOW "[Warning]: Invalid magick or version \"%s\"\nRegenerating font\n" COLOR_RESET, String(file).c_str());
			generateBase();
			return;
		}

		CharDesc cd;
		for(int i = 0; i < m_header.charCount; i++)
		{
			fread(&cd, sizeof(CharDesc), 1, pF);

			applyScale(&cd);

			m_chars[cd.id] = cd;
		}
		UINT w, h;
		byte * data;
		for(int i = 0; i < m_header.texCount; i++)
		{
			fread(&w, sizeof(w), 1, pF);
			fread(&h, sizeof(h), 1, pF);
			data = new byte[w * h * 4];
			fread(data, sizeof(byte), w * h * 4, pF);
			m_ppTextures.push_back(data);
			
			IGXTexture2D *pTex = GetGUI()->getDevice()->createTexture2D(w, h, 0, 0, GXFMT_A8B8G8R8, data);

			m_vpTextures.push_back(pTex);
		}
		//LoadFTfontFace();
		fclose(pF);
	}

	void CFont::generateBase()
	{
		static StringW s = L" qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=\\~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?йцукенгшщзхъфывапролджэячсмитьбюЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮёЁ	";
		for(UINT i = 0; i < s.length(); i++)
		{
			addChar(s[i]);
		}
		regen();
	}

	void CFont::regen()
	{
		//printf("_heapchk() = %d\n", _heapchk());
		for(UINT i = 0; i < m_vpTextures.size(); i++)
		{
			mem_release(m_vpTextures[i]);
		}
		for(UINT i = 0; i < m_ppTextures.size(); i++)
		{
			mem_delete_a(m_ppTextures[i]);
		}
		m_vpTextures.clear();
		m_ppTextures.clear();
		m_chars.clear();
		//TODO: Chear caches
		loadFTfontFace();
		if(!m_pFTfontFace)
		{
			return;
		}
		FT_GlyphSlot g = m_pFTfontFace->glyph;
		m_chars.clear();
		Array<chardata> list;
		//m_pFTfontFace->
		if(m_bEmulateItalic)
		{
			FT_Matrix m = {
				0x10000L, 0x04000L,
				0, 0x10000L
			};

			FT_Set_Transform(m_pFTfontFace, &m, NULL);
		}
		for(UINT i = 0; i < m_szFontChars.length(); ++i)
		{
			if(m_szFontChars[i] != L'\t')
			{
				if(FT_Load_Char(m_pFTfontFace, m_szFontChars[i], FT_LOAD_RENDER | (m_iFontSize > 13 ? FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT/* | FT_LOAD_TARGET_LCD*/ : FT_LOAD_TARGET_MONO)))
					//if(FT_Load_Char(m_pFTfontFace, m_szFontChars[i], FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT))
				{
					continue;
				}
			}
			else
			{
				if(FT_Load_Char(m_pFTfontFace, L' ', FT_LOAD_RENDER))
				{
					continue;
				}
				//g->bitmap.width *= 4;
				g->advance.x *= 4;
			}

			int iPadding = 1;

			chardata d;
			d.w = (g->bitmap.pixel_mode == FT_PIXEL_MODE_LCD ? (g->bitmap.width / 3) : g->bitmap.width) + m_iBlurRadius * 2 + iPadding * 2;
			d.h = g->bitmap.rows + m_iBlurRadius * 2 + iPadding * 2;

			/*if(d.w > 64 || d.h > 64)
			{
				char tmp[128];
				sprintf_s(tmp, "-:%dx%d", d.w, d.h);
				MessageBoxA(NULL, tmp, "", MB_OK);
			}*/

			d.xa = g->advance.x;
			d.xo = g->bitmap_left;
			d.yo = g->bitmap_top;
			//g->metrics.
			d.x = 0;
			d.y = 0;
			d.like = 0;
			d.ins = false;
			d.data = new byte[d.w * d.h * 4];
			memset(d.data, 0, sizeof(byte)* d.w * d.h * 4);
			int cc = 0;
			FT_Bitmap * bitmap = &g->bitmap;
			FT_Bitmap tempbitmapmono;
			if(g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
			{
				//FT_Bitmap tempbitmap;
				FT_Bitmap_New(&tempbitmapmono);
				FT_Bitmap_Convert(m_pFontManager->requestFT(), bitmap, &tempbitmapmono, 1);
				bitmap = &tempbitmapmono;
				byte * buf = tempbitmapmono.buffer;
				for(UINT i = 0; i<tempbitmapmono.rows*tempbitmapmono.pitch; i++)
				{
					if(*buf > 0) *buf = 255;
					buf++;
				}
			}
			if(g->bitmap.pixel_mode == FT_PIXEL_MODE_LCD)
			{
				for(int y = bitmap->rows - 1, by = m_iBlurRadius + iPadding; y >= 0; --y, ++by)
				{
					for(UINT i = 0, bx = m_iBlurRadius + iPadding; i < bitmap->width; i += 3, ++bx)
					{
						cc = (by * d.w + bx) * 4;
						unsigned char cr = bitmap->buffer[y * bitmap->width + i];
						unsigned char cg = bitmap->buffer[y * bitmap->width + i + 1];
						unsigned char cb = bitmap->buffer[y * bitmap->width + i + 2];
						d.data[cc] = cb;
						cc++;
						d.data[cc] = cg;
						cc++;
						d.data[cc] = cr;
						cc++;
						d.data[cc] = (cr + cg + cb) / 3;
						//d.data[cc] = 255;
						cc++;
						assert(cc <= d.w * d.h * 4);
					}
				}
			}
			else
			{
				for(int y = bitmap->rows - 1, by = m_iBlurRadius + iPadding; y >= 0; --y, ++by)
				{
					for(UINT i = 0, bx = m_iBlurRadius + iPadding; i < bitmap->width; ++i, ++bx)
					{
						cc = (by * d.w + bx) * 4;
						unsigned char c = bitmap->buffer[y * bitmap->width + i];
						d.data[cc] = 255;
						cc++;
						d.data[cc] = 255;
						cc++;
						d.data[cc] = 255;
						cc++;
						d.data[cc] = c;
						//d.data[cc] = 255;
						cc++;
						assert(cc <= d.w * d.h * 4);
					}
				}
			}
			if(g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
			{
				FT_Bitmap_Done(m_pFontManager->requestFT(), &tempbitmapmono);
			}

			d.c = m_szFontChars[i]; // FIXME: Fix for two 32bit chars
			for(UINT ii = 0; ii < list.size(); ii++)
			{
				if(!list[ii].like && imageEqual(list[ii], d))
				{
					d.like = list[ii].c;
				}
			}
			list.push_back(d);
		}
		
#define CHECK_0() 
		//if(list[0].w > 64 || list[0].h > 64){char tmp[128];sprintf_s(tmp, "#:%dx%d", list[0].w, list[0].h);MessageBoxA(NULL, tmp, GEN_MSG_LOCATION, MB_OK);}

		CHECK_0();
		//
		list.quickSort([](const chardata &a, const chardata &b){
			return(a.h > b.h);
		});
		CHECK_0();
		int ss = 0;
		int maxW = -1;
		int maxH = -1;
		for(UINT i = 0; i < list.size(); i++)
		{
			//printf("%d ", list[i].h);
			if(!list[i].like)
			{
				ss += (list[i].w + 2 + (m_bEmulateBold ? 1 : 0)) * (list[i].h + 2);
				if(maxW < list[i].w)
				{
					maxW = list[i].w;
				}
				if(maxH < list[i].h)
				{
					maxH = list[i].h;
				}
			}
		}
		CHECK_0();
		int s = sqrt((float)ss);
		int pow2 = 1;
		while(pow2 <= s)
		{
			pow2 *= 2;
		}
		int width = pow2;
		int height = pow2;

		if(height / 2 * width > ss && height / 2 >= maxH)
		{
			height /= 2;
			if(!place(list, width, height))
			{
				height *= 2;
				if(width / 2 * height > ss && width / 2 >= maxW)
				{
					width /= 2;
					if(!place(list, width, height))
					{
						width *= 2;
						place(list, width, height);
					}
				}
				else
				{
					place(list, width, height);
				}
			}
		}
		else
		{
			if(width / 2 * height > ss && width / 2 >= maxW)
			{
				width /= 2;
				if(!place(list, width, height))
				{
					width *= 2;
					place(list, width, height);
				}
			}
			else
			{
				place(list, width, height);
			}
		}
		CHECK_0();
		for(UINT i = 0; i < list.size(); i++)
		{
			if(list[i].like)
			{
				for(UINT j = 0; j < list.size(); j++)
				{
					if(list[j].c == list[i].like && !list[j].like)
					{
						list[i].x = list[j].x;
						list[i].y = list[j].y;
					}
				}
			}
		}
		CHECK_0();
		printf("%dx%d\n", width, height);

		//printf("_heapchk() = %d\n", _heapchk());

		byte * image = new byte[width*height * 4];
		memset(image, 0, sizeof(byte)* width*height * 4);

		CHECK_0();
		int count = 0;
		m_chars.clear();
		CharDesc cd;
		for(UINT i = 0; i < list.size(); i++)
		{
			//	wprintf(L"%d %d %d %d  %d %d\n", list[i].w, list[i].h, list[i].x, list[i].y, list[i].w + list[i].x, list[i].h + list[i].y);
			/*if(list[i].w > 64 || list[i].h > 64)
			{
				char tmp[128];
				sprintf_s(tmp, "%d:%dx%d", i, list[i].w, list[i].h);
				MessageBoxA(NULL, tmp, "", MB_OK);
			}*/
			imageCopy(image, list[i].data, width, height, list[i].w, list[i].h, list[i].x, list[i].y, 4);
			if(m_bEmulateBold)
			{
				imageCopy(image, list[i].data, width, height, list[i].w, list[i].h, list[i].x + 1, list[i].y, 4);
			}
			count++;
			//cd.

			cd.height = list[i].h + 1;
			cd.width = list[i].w + (m_bEmulateBold ? 1 : 0);
			cd.theight = (float)cd.height / (float)height;
			cd.twidth = (float)cd.width / (float)width;
			cd.id = list[i].c;
			cd.x = list[i].x;
			cd.y = (height - list[i].y - cd.height);
			cd.tx = (float)cd.x / (float)width;
			cd.ty = ((float)cd.y + 1.0f) / (float)height;
			cd.xadvantage = list[i].xa >> 6;
			cd.xoffset = list[i].xo;
			//cd.yoffset = m_iFontSize - list[i].yo - 15.0f / 72.0f * (float)m_iFontSize;
			cd.yoffset = (float)m_iFontSize - (float)list[i].yo;

			m_chars[cd.id] = cd;
		}

		if(m_iBlurRadius)
		{
			// blur image width height
			byte * newImage = new byte[width*height * 4];
			float kernel[5][5] = {
				{0.0030f, 0.0133f, 0.0219f, 0.0133f, 0.0030f},
				{0.0133f, 0.0596f, 0.0983f, 0.0596f, 0.0133f},
				{0.0219f, 0.0983f, 0.1621f, 0.0983f, 0.0219f},
				{0.0133f, 0.0596f, 0.0983f, 0.0596f, 0.0133f},
				{0.0030f, 0.0133f, 0.0219f, 0.0133f, 0.0030f},
			};
			for(int loop = 0; loop < 8; ++loop)
			{
				for(int x = 0; x < width; ++x)
				{
					for(int y = 0; y < height; ++y)
					{
						float4_t sum;
						for(int _kx = x - 2, kernx = 0; _kx <= x + 2; ++_kx, ++kernx)
						{
							int kx = _kx;
							if(kx < 0)
							{
								kx = -kx;
							}
							else if(kx >= width)
							{
								kx = 2 * width - kx - 1;
							}
							for(int _ky = y - 2, kerny = 0; _ky <= y + 2; ++_ky, ++kerny)
							{
								int ky = _ky;
								if(ky < 0)
								{
									ky = -ky;
								}
								else if(ky >= height)
								{
									ky = 2 * height - ky - 1;
								}

								//sum.x += (float)image[(ky * width + kx) * 4 + 0] * kernel[kernx][kerny];
								//sum.y += (float)image[(ky * width + kx) * 4 + 1] * kernel[kernx][kerny];
								//sum.z += (float)image[(ky * width + kx) * 4 + 2] * kernel[kernx][kerny];
								sum.w += (float)image[(ky * width + kx) * 4 + 3] * kernel[kernx][kerny];
							}
						}

						//newImage[(y * width + x) * 4 + 0] = sum.x;
						//newImage[(y * width + x) * 4 + 1] = sum.y;
						//newImage[(y * width + x) * 4 + 2] = sum.z;
						newImage[(y * width + x) * 4 + 0] = 255;
						newImage[(y * width + x) * 4 + 1] = 255;
						newImage[(y * width + x) * 4 + 2] = 255;
						newImage[(y * width + x) * 4 + 3] = sum.w;
					}
				}
				byte *tmp = newImage;
				newImage = image;
				image = tmp;
			}

			for(int i = 0, l = width * height * 4; i < l; ++i)
			{
				//image[i] *= 2;
				/*if(i % 4 == 3)
				{
					if(image[i] < 127)
					{
						image[i] *= 2;
					}
					else
					{
						image[i] = 255;
					}
				}*/
			}

			mem_delete_a(newImage);
		}

		//printf("_heapchk() = %d\n", _heapchk());

		IGXTexture2D *pTex = GetGUI()->getDevice()->createTexture2D(width, height, 0, 0, GXFMT_A8B8G8R8, image);

		//SX_SAFE_DELETE_A(image);
		for(UINT i = 0; i < list.size(); i++)
		{
			mem_delete_a(list[i].data);
		}

		m_vpTextures.push_back(pTex);
		m_ppTextures.push_back(image);
		m_header.texCount = 1; // FIXME: allow multiple textures
		m_header.charCount = list.size();
		m_bHasBeenChanged = true;
		save(); // TODO: Find better place to call that

		// TODO: Add rebuild handler to call Layout on the document

		for(UINT i = 0; i < list.size(); i++)
		{
			applyScale(&m_chars[list[i].c]);
		}


		//printf("_heapchk() = %d\n", _heapchk());

	}

	bool CFont::place(Array<chardata> &list, int width, int height)
	{
		int x = 0;
		int y = 0;
		bool fl = true;

		for(UINT i = 0; i < list.size(); i++)
		{
			if(list[i].like)
			{
				continue;
			}
			if(x + list[i].w + (m_bEmulateBold ? 1 : 0) >= width)
			{
				x = 0;
				fl = false;
			}
			bool a = true;
			y = 0;
			bool dd = false;
			int yy = 0;
			do
			{
				if(dd)
				{
					x++;
				}
				yy = 0;
				for(UINT j = 0; j < list.size(); j++)
				{
					if(!list[j].ins)
					{
						continue;
					}
					if((x >= list[j].x - 1 && x < (list[j].x + list[j].w)) || (x + list[i].w >= list[j].x && x + list[i].w < (list[j].w + list[j].x)))
					{
						yy = list[j].y + list[j].h > yy ? list[j].y + list[j].h : yy;
					}
				}
				dd = true;
			}
			while(yy + list[i].h >= height && x + list[i].w < width);
			if(x + list[i].w > width && yy + list[i].h >= height)
			{
				for(UINT h = 0; h < list.size(); h++)
				{
					list[h].ins = false;
				}
				return(false);
			}
			y = yy;
			y++;
			x++;
			list[i].x = x;
			list[i].y = y;
			list[i].ins = true;
			x += list[i].w;
		}
		return(true);
	}

	void CFont::loadChar(UINT id)
	{
		addChar(id);
		regen();
	}

	IGXTexture2D* CFont::getAPITexture(UINT i)
	{
		if(m_vpTextures.size() <= i)
		{
			return(NULL);
		}
		return(m_vpTextures[i]);
	}

	void CFont::addChar(WCHAR c, bool full)
	{
		if(m_szFontChars.find(c) == (UINT)-1)
		{
			m_szFontChars += c;
			if(full)
			{
				const symRange * rg = getSymRange(c);
				if(rg)
				{
					for(UINT i = rg->start; i < rg->end; i++)
					{
						addChar(i, false);
					}
				}
			}
		}
	}

	CFont::~CFont()
	{
		freeFTfontFace();
		for(UINT i = 0; i < m_vpTextures.size(); i++)
		{
			mem_release(m_vpTextures[i]);
		}
		for(UINT i = 0; i < m_ppTextures.size(); i++)
		{
			mem_delete_a(m_ppTextures[i]);
		}
	}

	void CFont::freeFTfontFace()
	{
		if(m_pFTfontFace)
		{
			FT_Done_Face(m_pFTfontFace);
			m_pFTfontFace = NULL;
		}
	}

	bool CFont::imageEqual(const chardata & a, const chardata & b)
	{
		if(a.w != b.w || a.h != b.h)
		{
			return(false);
		}
		for(int i = 0; i < a.w * a.h * 4; i++)
		{
			if(a.data[i] != b.data[i])
			{
				return(false);
			}
		}
		return(true);
	}

	const CFont::symRange * CFont::getSymRange(UINT sym)
	{
		static symRange table[] = {
			{0x0000, 0x001F},
			{0x0020, 0x007F},
			{0x0080, 0x00FF},
			{0x0100, 0x017F},
			{0x0180, 0x024F},
			{0x0250, 0x02AF},
			{0x02B0, 0x02FF},
			{0x0300, 0x036F},
			{0x0370, 0x03FF},
			{0x0400, 0x04FF},
			{0x0500, 0x052F},
			{0x0530, 0x058F},
			{0x0590, 0x05FF},
			{0x0600, 0x06FF},
			{0x0700, 0x074F},
			{0x0750, 0x077F},
			{0x0780, 0x07BF},
			{0x07C0, 0x07FF},
			{0x0800, 0x083F},
			{0x0840, 0x085F},
			{0x08A0, 0x08FF},
			{0x0900, 0x097F},
			{0x0980, 0x09FF},
			{0x0A00, 0x0A7F},
			{0x0A80, 0x0AFF},
			{0x0B00, 0x0B7F},
			{0x0B80, 0x0BFF},
			{0x0C00, 0x0C7F},
			{0x0C80, 0x0CFF},
			{0x0D00, 0x0D7F},
			{0x0D80, 0x0DFF},
			{0x0E00, 0x0E7F},
			{0x0E80, 0x0EFF},
			{0x0F00, 0x0FFF},
			{0x1000, 0x109F},
			{0x10A0, 0x10FF},
			{0x1100, 0x11FF},
			{0x1200, 0x137F},
			{0x1380, 0x139F},
			{0x13A0, 0x13FF},
			{0x1400, 0x167F},
			{0x1680, 0x169F},
			{0x16A0, 0x16FF},
			{0x1700, 0x171F},
			{0x1720, 0x173F},
			{0x1740, 0x175F},
			{0x1760, 0x177F},
			{0x1780, 0x17FF},
			{0x1800, 0x18AF},
			{0x18B0, 0x18FF},
			{0x1900, 0x194F},
			{0x1950, 0x197F},
			{0x1980, 0x19DF},
			{0x19E0, 0x19FF},
			{0x1A00, 0x1A1F},
			{0x1A20, 0x1AAF},
			{0x1AB0, 0x1AFF},
			{0x1B00, 0x1B7F},
			{0x1B80, 0x1BBF},
			{0x1BC0, 0x1BFF},
			{0x1C00, 0x1C4F},
			{0x1C50, 0x1C7F},
			{0x1CC0, 0x1CCF},
			{0x1CD0, 0x1CFF},
			{0x1D00, 0x1D7F},
			{0x1D80, 0x1DBF},
			{0x1DC0, 0x1DFF},
			{0x1E00, 0x1EFF},
			{0x1F00, 0x1FFF},
			{0x2000, 0x206F},
			{0x2070, 0x209F},
			{0x20A0, 0x20CF},
			{0x20D0, 0x20FF},
			{0x2100, 0x214F},
			{0x2150, 0x218F},
			{0x2190, 0x21FF},
			{0x2200, 0x22FF},
			{0x2300, 0x23FF},
			{0x2400, 0x243F},
			{0x2440, 0x245F},
			{0x2460, 0x24FF},
			{0x2500, 0x257F},
			{0x2580, 0x259F},
			{0x25A0, 0x25FF},
			{0x2600, 0x26FF},
			{0x2700, 0x27BF},
			{0x27C0, 0x27EF},
			{0x27F0, 0x27FF},
			{0x2800, 0x28FF},
			{0x2900, 0x297F},
			{0x2980, 0x29FF},
			{0x2A00, 0x2AFF},
			{0x2B00, 0x2BFF},
			{0x2C00, 0x2C5F},
			{0x2C60, 0x2C7F},
			{0x2C80, 0x2CFF},
			{0x2D00, 0x2D2F},
			{0x2D30, 0x2D7F},
			{0x2D80, 0x2DDF},
			{0x2DE0, 0x2DFF},
			{0x2E00, 0x2E7F},
			{0x2E80, 0x2EFF},
			{0x2F00, 0x2FDF},
			{0x2FF0, 0x2FFF},
			{0x3000, 0x303F},
			{0x3040, 0x309F},
			{0x30A0, 0x30FF},
			{0x3100, 0x312F},
			{0x3130, 0x318F},
			{0x3190, 0x319F},
			{0x31A0, 0x31BF},
			{0x31C0, 0x31EF},
			{0x31F0, 0x31FF},
			{0x3200, 0x32FF},
			{0x3300, 0x33FF},
			{0x3400, 0x4DB5},
			{0x4DC0, 0x4DFF},
			{0x4E00, 0x9FCC},
			{0xA000, 0xA48F},
			{0xA490, 0xA4CF},
			{0xA4D0, 0xA4FF},
			{0xA500, 0xA63F},
			{0xA640, 0xA69F},
			{0xA6A0, 0xA6FF},
			{0xA700, 0xA71F},
			{0xA720, 0xA7FF},
			{0xA800, 0xA82F},
			{0xA830, 0xA83F},
			{0xA840, 0xA87F},
			{0xA880, 0xA8DF},
			{0xA8E0, 0xA8FF},
			{0xA900, 0xA92F},
			{0xA930, 0xA95F},
			{0xA960, 0xA97F},
			{0xA980, 0xA9DF},
			{0xA9E0, 0xA9FF},
			{0xAA00, 0xAA5F},
			{0xAA60, 0xAA7F},
			{0xAA80, 0xAADF},
			{0xAAE0, 0xAAFF},
			{0xAB00, 0xAB2F},
			{0xAB30, 0xAB6F},
			{0xABC0, 0xABFF},
			{0xAC00, 0xD7A3},
			{0xD7B0, 0xD7FF},
			{0xD800, 0xDB7F},
			{0xDB80, 0xDBFF},
			{0xDC00, 0xDFFF},
			{0xE000, 0xF8FF},
			{0xF900, 0xFAFF},
			{0xFB00, 0xFB4F},
			{0xFB50, 0xFDFF},
			{0xFE00, 0xFE0F},
			{0xFE10, 0xFE1F},
			{0xFE20, 0xFE2F},
			{0xFE30, 0xFE4F},
			{0xFE50, 0xFE6F},
			{0xFE70, 0xFEFF},
			{0xFF00, 0xFFEF},
			{0xFFF0, 0xFFFF},
			{0x10000, 0x1007F},
			{0x10080, 0x100FF},
			{0x10100, 0x1013F},
			{0x10140, 0x1018F},
			{0x10190, 0x101CF},
			{0x101D0, 0x101FF},
			{0x10280, 0x1029F},
			{0x102A0, 0x102DF},
			{0x102E0, 0x102FF},
			{0x10300, 0x1032F},
			{0x10330, 0x1034F},
			{0x10350, 0x1037F},
			{0x10380, 0x1039F},
			{0x103A0, 0x103DF},
			{0x10400, 0x1044F},
			{0x10450, 0x1047F},
			{0x10480, 0x104AF},
			{0x10500, 0x1052F},
			{0x10530, 0x1056F},
			{0x10600, 0x1077F},
			{0x10800, 0x1083F},
			{0x10840, 0x1085F},
			{0x10860, 0x1087F},
			{0x10880, 0x108AF},
			{0x10900, 0x1091F},
			{0x10920, 0x1093F},
			{0x10980, 0x1099F},
			{0x109A0, 0x109FF},
			{0x10A00, 0x10A5F},
			{0x10A60, 0x10A7F},
			{0x10A80, 0x10A9F},
			{0x10AC0, 0x10AFF},
			{0x10B00, 0x10B3F},
			{0x10B40, 0x10B5F},
			{0x10B60, 0x10B7F},
			{0x10B80, 0x10BAF},
			{0x10C00, 0x10C4F},
			{0x10E60, 0x10E7F},
			{0x11000, 0x1107F},
			{0x11080, 0x110CF},
			{0x110D0, 0x110FF},
			{0x11100, 0x1114F},
			{0x11150, 0x1117F},
			{0x11180, 0x111DF},
			{0x111E0, 0x111FF},
			{0x11200, 0x1124F},
			{0x112B0, 0x112FF},
			{0x11300, 0x1137F},
			{0x11480, 0x114DF},
			{0x11580, 0x115FF},
			{0x11600, 0x1165F},
			{0x11680, 0x116CF},
			{0x118A0, 0x118FF},
			{0x11AC0, 0x11AFF},
			{0x12000, 0x123FF},
			{0x12400, 0x1247F},
			{0x13000, 0x1342F},
			{0x16800, 0x16A3F},
			{0x16A40, 0x16A6F},
			{0x16AD0, 0x16AFF},
			{0x16B00, 0x16B8F},
			{0x16F00, 0x16F9F},
			{0x1B000, 0x1B0FF},
			{0x1BC00, 0x1BC9F},
			{0x1BCA0, 0x1BCAF},
			{0x1D000, 0x1D0FF},
			{0x1D100, 0x1D1FF},
			{0x1D200, 0x1D24F},
			{0x1D300, 0x1D35F},
			{0x1D360, 0x1D37F},
			{0x1D400, 0x1D7FF},
			{0x1E800, 0x1E8DF},
			{0x1EE00, 0x1EEFF},
			{0x1F000, 0x1F02F},
			{0x1F030, 0x1F09F},
			{0x1F0A0, 0x1F0FF},
			{0x1F100, 0x1F1FF},
			{0x1F200, 0x1F2FF},
			{0x1F300, 0x1F5FF},
			{0x1F600, 0x1F64F},
			{0x1F650, 0x1F67F},
			{0x1F680, 0x1F6FF},
			{0x1F700, 0x1F77F},
			{0x1F780, 0x1F7FF},
			{0x1F800, 0x1F8FF},

			{0, 0} // table end
		};

		UINT i = 0;
		while(table[i].end != 0)
		{
			if(table[i].start <= sym && table[i].end >= sym)
			{
				return(&table[i]);
			}
			i++;
		}
		return(NULL);
	}

	void CFont::applyScale(CharDesc *pCd)
	{
		if(m_fScale != 1.0f)
		{
			pCd->width = pCd->width / m_fScale;
			pCd->height = pCd->height / m_fScale;
			pCd->xoffset = pCd->xoffset / m_fScale;
			pCd->yoffset = pCd->yoffset / m_fScale;
			pCd->xadvantage = pCd->xadvantage / m_fScale;
		}
	}
	void CFont::getStringMetrics(const StringW & str, UINT * width, UINT * height, UINT * vertexCount, UINT * indexCount, UINT * strCount, char_rects * pcr)
	{
		float w = 0;
		float _w = 0;
		UINT h = m_iFontSize;
		UINT vc = 0;
		UINT ic = 0;
		UINT lc = 1;

		const CharDesc * cd;
		RECT cr;
		cr.top = 0;
		cr.bottom = m_iFontSize;
		if(pcr)
		{
			cr.top += pcr->iYoffs;
			cr.bottom += pcr->iYoffs;
		}
		for(UINT i = 0, l = str.length(); i < l; i++)
		{
			cd = getChar((UINT)str[i]);
			if(!iswspace(str[i]))
			{
				vc += 4;
				ic += 6;
			}
			if(str[i] == L'\n')
			{
				lc++;
				w = max(w, _w);
				_w = 0;
				cr.top += m_iFontSize;
				cr.bottom += m_iFontSize;

				if(pcr)
				{
					pcr->pResArray->push_back(cr);
				}
			}
			else
			{
				if(pcr)
				{
					cr.left = _w + pcr->iXoffs;
					cr.right = _w + pcr->iXoffs + cd->xadvantage;
					pcr->pResArray->push_back(cr);
				}

				_w += cd->xadvantage;
				
			}
		}
		w = max(w, _w);
		if(width)
		{
			*width = w;
		}
		if(height)
		{
			*height = h;
		}
		if(vertexCount)
		{
			*vertexCount = vc;
		}
		if(indexCount)
		{
			*indexCount = ic;
		}
		if(strCount)
		{
			*strCount = lc;
		}
	}

	void CFont::buildString(const StringW & str, UINT decoration, TEXT_ALIGN textAlign, IGXRenderBuffer ** ppVertexBuffer, IGXIndexBuffer ** ppIndexBuffer, UINT * vertexCount, UINT * indexCount, UINT * lineIndexCount, UINT iAreaWidth, UINT iFirstShift, UINT * pStrWidth)
	{
		UINT iIndexCount;
		UINT iVertexCount;
		UINT lineCount;
		UINT iWidth;
		getStringMetrics(str, &iWidth, NULL, &iVertexCount, &iIndexCount, &lineCount);
		if(pStrWidth)
		{
			*pStrWidth = iWidth;
		}
		if(iAreaWidth > iWidth)
		{
			iWidth = iAreaWidth;
		}

		

		Array<float2_t> alx;
		alx.reserve(lineCount);

		

		UINT lc = 0;
		if(decoration & DECORATION_UNDERLINE)
		{
			lc++;
		}
		if(decoration & DECORATION_OVERLINE)
		{
			lc++;
		}
		if(decoration & DECORATION_LINE_THROUGH)
		{
			lc++;
		}
		UINT ilineIndexCount = lc * lineCount * 6;

		iIndexCount += ilineIndexCount;
		iVertexCount += lc * lineCount * 4;

		vertex * pVB = new vertex[iVertexCount];

		UINT CV = 0,
			CI = 0;

		float cx = 0.0f;
		float cy = 0.0f;
		CPFCHAR cd;
		UINT sv = 0;

		bool bFirstLine = true;
		for(UINT i = 0, l = str.length(); i < l; i++)
		{
			cd = getChar((UINT)str[i]);

			if(!iswspace(str[i]))
			{
				/*
				
				0--1
				| /|
				|/ |
				2--3

				*/

				pVB[CV].Pos = float3_t(cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset, cy + (float)cd->yoffset, 0.0f);
				pVB[CV].Tex = float2_t(cd->tx, cd->ty + cd->theight);
				CV++;
				pVB[CV].Pos = float3_t(cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset + (float)cd->width, cy + (float)cd->yoffset, 0.0f);
				pVB[CV].Tex = float2_t(cd->tx + cd->twidth, cd->ty + cd->theight);
				CV++;
				pVB[CV].Pos = float3_t(cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset, cy + cd->height + (float)cd->yoffset, 0.0f);
				pVB[CV].Tex = float2_t(cd->tx, cd->ty);
				CV++;
				pVB[CV].Pos = float3_t(cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset + (float)cd->width, cy + cd->height + (float)cd->yoffset, 0.0f);
				pVB[CV].Tex = float2_t(cd->tx + cd->twidth, cd->ty);
				CV++;

			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset, cy + (float)cd->yoffset, 0.0f}, {cd->tx, cd->ty + cd->theight}}; CV++;
			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset + (float)cd->width, cy + (float)cd->yoffset, 0.0f}, {cd->tx + cd->twidth, cd->ty + cd->theight}}; CV++;
			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset, cy + cd->height + (float)cd->yoffset, 0.0f}, {cd->tx, cd->ty}}; CV++;
			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset + (float)cd->width, cy + cd->height + (float)cd->yoffset, 0.0f}, {cd->tx + cd->twidth, cd->ty}}; CV++;
			}

			
			if(str[i] == L'\n')
			{
				bFirstLine = false;
				alx.push_back(float2_t(cx, cy));
				cy += m_iFontSize;
				float delta = 0.0f;
				if(textAlign == TEXT_ALIGN_RIGHT)
				{
					delta = iWidth - cx;
				}
				if(textAlign == TEXT_ALIGN_CENTER)
				{
					delta = (iWidth - cx) * 0.5f;
				}
				for(UINT j = sv; j < CV; j++)
				{
					pVB[j].Pos.x += delta;
				}
				cx = 0.0f;
				sv = CV;
			}
			else
			{
				cx += (float)cd->xadvantage;
			}
		}
		if(cx != 0.0f)
		{
			alx.push_back(float2_t(cx, cy));
			cy += m_iFontSize;
			float delta = 0.0f;
			if(textAlign == TEXT_ALIGN_RIGHT)
			{
				delta = iWidth - cx;
			}
			if(textAlign == TEXT_ALIGN_CENTER)
			{
				delta = (iWidth - cx) * 0.5f;
			}
			for(UINT j = sv; j < CV; j++)
			{
				pVB[j].Pos.x += delta;
			}
			cx = 0.0f;
			sv = CV;
		}
		float lineHeight = 1.0f;
		for(UINT i = 0; i < lineCount; i++)
		{
			if(decoration & DECORATION_OVERLINE)
			{
				pVB[CV].Pos = float3_t(0.0f, alx[i].y, 0.0f);
				pVB[CV].Tex = float2_t(0.0f, 1.0f);
				CV++;
				pVB[CV].Pos = float3_t(alx[i].x, alx[i].y, 0.0f);
				pVB[CV].Tex = float2_t(1.0f, 1.0f);
				CV++;
				pVB[CV].Pos = float3_t(0.0f, alx[i].y + lineHeight, 0.0f);
				pVB[CV].Tex = float2_t(0.0f, 0.0f);
				CV++;
				pVB[CV].Pos = float3_t(alx[i].x, alx[i].y + lineHeight, 0.0f);
				pVB[CV].Tex = float2_t(1.0f, 0.0f);
				CV++;

			//	pVB[CV] = {{0.0f, alx[i].y, 0.0f}, {0.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y, 0.0f}, {1.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{0.0f, alx[i].y + lineHeight, 0.0f}, {0.0f, 0.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + lineHeight, 0.0f}, {1.0f, 0.0f}}; CV++;
			}
			if(decoration & DECORATION_UNDERLINE)
			{
				pVB[CV].Pos = float3_t(0.0f, alx[i].y + (float)m_iFontSize - lineHeight, 0.0f);
				pVB[CV].Tex = float2_t(0.0f, 1.0f);
				CV++;
				pVB[CV].Pos = float3_t(alx[i].x, alx[i].y + (float)m_iFontSize - lineHeight, 0.0f);
				pVB[CV].Tex = float2_t(1.0f, 1.0f);
				CV++;
				pVB[CV].Pos = float3_t(0.0f, alx[i].y + (float)m_iFontSize, 0.0f);
				pVB[CV].Tex = float2_t(0.0f, 0.0f);
				CV++;
				pVB[CV].Pos = float3_t(alx[i].x, alx[i].y + (float)m_iFontSize, 0.0f);
				pVB[CV].Tex = float2_t(1.0f, 0.0f);
				CV++;

			//	pVB[CV] = {{0.0f, alx[i].y + (float)m_iFontSize - lineHeight, 0.0f}, {0.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + (float)m_iFontSize - lineHeight, 0.0f}, {1.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{0.0f, alx[i].y + (float)m_iFontSize, 0.0f}, {0.0f, 0.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + (float)m_iFontSize, 0.0f}, {1.0f, 0.0f}}; CV++;
			}
			if(decoration & DECORATION_LINE_THROUGH)
			{
				pVB[CV].Pos = float3_t(0.0f, alx[i].y + (float)m_iFontSize * 0.5f, 0.0f);
				pVB[CV].Tex = float2_t(0.0f, 1.0f);
				CV++;
				pVB[CV].Pos = float3_t(alx[i].x, alx[i].y + (float)m_iFontSize * 0.5f, 0.0f);
				pVB[CV].Tex = float2_t(1.0f, 1.0f);
				CV++;
				pVB[CV].Pos = float3_t(0.0f, alx[i].y + lineHeight + (float)m_iFontSize * 0.5f, 0.0f);
				pVB[CV].Tex = float2_t(0.0f, 0.0f);
				CV++;
				pVB[CV].Pos = float3_t(alx[i].x, alx[i].y + lineHeight + (float)m_iFontSize * 0.5f, 0.0f);
				pVB[CV].Tex = float2_t(1.0f, 0.0f);
				CV++;

			//	pVB[CV] = {{0.0f, alx[i].y + (float)m_iFontSize * 0.5f, 0.0f}, {0.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + (float)m_iFontSize * 0.5f, 0.0f}, {1.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{0.0f, alx[i].y + lineHeight + (float)m_iFontSize * 0.5f, 0.0f}, {0.0f, 0.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + lineHeight + (float)m_iFontSize * 0.5f, 0.0f}, {1.0f, 0.0f}}; CV++;
			}
		}

		if(m_iBlurRadius)
		{
			float3 offs((float)-m_iBlurRadius, (float)-m_iBlurRadius, 0.0f);
			for(UINT i = 0; i < CV; ++i)
			{
				pVB[i].Pos = (float3)(pVB[i].Pos + offs);
			}
		}
		
		IGXVertexBuffer *pVertBuffer = GetGUI()->getDevice()->createVertexBuffer(sizeof(vertex)* iVertexCount, GXBUFFER_USAGE_STATIC, pVB);
		*ppVertexBuffer = GetGUI()->getDevice()->createRenderBuffer(1, &pVertBuffer, GetGUI()->getVertexDeclarations()->m_pXYZTex);
		mem_release(pVertBuffer);

		m_pFontManager->getFontIndexBuffer(iIndexCount / 6, ppIndexBuffer);

		mem_delete_a(pVB);

		if(vertexCount)
		{
			*vertexCount = iVertexCount;
		}
		if(indexCount)
		{
			*indexCount = iIndexCount - ilineIndexCount;
		}
		if(lineIndexCount)
		{
			*lineIndexCount = ilineIndexCount;
		}
	}

//##########################################################################
	
	IFontManager::IFontManager(const WCHAR *szResourceDir, CTextureManager *pTextureManager):
		m_wsResourceDir(szResourceDir),
		m_pTextureManager(pTextureManager)
	{
		if(FT_Init_FreeType(&m_pFT))
		{
			LibReport(REPORT_MSG_LEVEL_FATAL, "Failed to initialise FreeType library\n");
		}
	}

	IFontManager::~IFontManager()
	{
		if(m_pFT)
		{
			m_mFonts.clear();
			FT_Done_FreeType(m_pFT);
			m_pFT = NULL;
		}

		mem_release(m_pFontIB);
	}

	CFont* IFontManager::getFont(const WCHAR *szName, UINT size, CFont::STYLE style, int iBlurRadius, float fScale)
	{
		size = (UINT)((float)size * fScale);
		StringW n = StringW(szName) + L"_" + StringW((int)size) + L"+" + StringW((int)style) + L"-" + StringW(iBlurRadius) + L"~" + StringW(fScale);
		if(!m_mFonts.KeyExists(n))
		{
			m_mFonts[n].load(this, m_pTextureManager, szName, size, style, iBlurRadius, fScale);
		}
		return(&m_mFonts[n]);
	}

	FT_Library IFontManager::requestFT()
	{
		return(m_pFT);
	}

	void IFontManager::getFontIndexBuffer(UINT uChars, IGXIndexBuffer **ppIndexBuffer)
	{
		assert(uChars < USHRT_MAX / 4);

		if(uChars > m_uFontIBChars)
		{
			UINT m_uFontIBChars = 1;
			while(m_uFontIBChars < uChars)
			{
				m_uFontIBChars *= 2;
			}


			USHORT *pIB = new USHORT[m_uFontIBChars * 6];

			UINT CI = 0;
			for(UINT i = 0; i < m_uFontIBChars; ++i)
			{
				pIB[CI++] = i * 4 + 0;
				pIB[CI++] = i * 4 + 1;
				pIB[CI++] = i * 4 + 2;
				pIB[CI++] = i * 4 + 2;
				pIB[CI++] = i * 4 + 1;
				pIB[CI++] = i * 4 + 3;
			}

			mem_release(m_pFontIB);
			m_pFontIB = GetGUI()->getDevice()->createIndexBuffer(sizeof(USHORT)* m_uFontIBChars * 6, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pIB);

			mem_delete_a(pIB);
		}

		m_pFontIB->AddRef();
		*ppIndexBuffer = m_pFontIB;
	}
}
