#include "Font.h"
#include <core/IFile.h>
#include <common/StreamReader.h>
#include "FontManager.h"
//#include <math.h>

#include FT_BITMAP_H

static void ImageCopy(byte *dst, byte *src, int dstwidth, int dstheight, int srcwidth, int srcheight, int posX, int posY, int bpp)
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

	char szFileName[MAX_PATH];
	sprintf(szFileName, "/cache/fonts/%s.sxf", m_szIdent);

	IFile *pFile = m_pFileSystem->openFile(szFileName, FILE_MODE_WRITE);

	if(!pFile)
	{
		printf(COLOR_LRED "[Error]: Unable to open \"%s\" file for writing\n" COLOR_RESET, szFileName);
		return;
	}
	UINT c = 0;
	for(UINT i = 0, l = m_chars.size(); i < l; i++)
	{
		if(m_chars[i].id != 0)
		{
			c++;
		}
	}
	m_header.charCount = c;
	m_header.size = m_uFontSize;
	pFile->writeBin(&m_header, sizeof(SXFheader));
	for(UINT i = 0, l = m_chars.size(); i < l; i++)
	{
		if(m_chars[i].id != 0)
		{
			pFile->writeBin(&m_chars[i], sizeof(CharDesc));
		}
	}
	uint32_t w, h;
	byte *data;
	for(UINT i = 0, l = m_apTextures.size(); i < l; ++i)
	{
		w = m_apTextures[i]->getWidth();
		h = m_apTextures[i]->getHeight();
		data = m_ppTextures[i];

		pFile->writeBin(&w, sizeof(w));
		pFile->writeBin(&h, sizeof(h));
		pFile->writeBin(data, sizeof(byte) * w * h * 4);
	}
	mem_release(pFile);
}

const CFont::CharDesc* CFont::getChar(UINT id)
{
	const CFont::CharDesc *ch = &m_chars[id];
	if(ch->id == 0)
	{
		loadChar(id);
		ch = &m_chars[id];
	}
	return(ch);
}

void CFont::loadFTfontFace()
{
	if(m_pFTfontFace)
	{
		return;
	}

	IFile *pFile = m_pFileSystem->openFile(m_sName.c_str());
	if(!pFile)
	{
		LogError("Unable to open \"%s\" font! Defaulting\n", m_sName.c_str());

		pFile = m_pFileSystem->openFile("/gui/fonts/default.ttf");
		if(!pFile)
		{
			LogFatal("Unable to open \"%s\" font! Failing!\n", m_sName.c_str());
		}
		return;
	}

	size_t uSize = pFile->getSize();
	m_pFontBlob = new byte[uSize];
	pFile->readBin(m_pFontBlob, uSize);
	mem_release(pFile);

	if(FT_New_Memory_Face(m_pFT, m_pFontBlob, (FT_Long)uSize, 0, &m_pFTfontFace))
	{
		LogError("Unable to load \"%s\" font! Failing!\n", m_sName.c_str());
	}

	FT_Set_Pixel_Sizes(m_pFTfontFace, 0, m_uFontSize);
}

void CFont::load(CFontManager *pFontManager, IFileSystem *pFileSystem, FT_Library pFT, IGXDevice *pDev, const char *szIdent, const char *szFont, UINT uSize, XFONT_STYLE style, int iBlurRadius)
{
	m_pFontManager = pFontManager;
	m_pFontManager->AddRef();
	m_pFileSystem = pFileSystem;
	m_style = style;
	m_pFT = pFT;
	m_pDev = pDev;
	freeFTfontFace();
	m_uFontSize = uSize;
	m_szIdent = szIdent;
	m_bEmulateBold = false;
	m_bEmulateItalic = false;
	m_iBlurRadius = iBlurRadius;
	m_sName = szFont;

	char szFileName[MAX_PATH];
	sprintf(szFileName, "/cache/fonts/%s.sxf", m_szIdent);

	IFile *pFile = m_pFileSystem->openFile(szFileName);

	if(!pFile)
	{
		LogWarning("[Warning]: Unable to open \"%s\" file to reading, generating font\n", szFileName);
		generateBase();
		m_header.size = uSize;
		return;
	}

	pFile->readBin(&m_header, sizeof(m_header));
	CharDesc cd;
	for(int i = 0; i < m_header.charCount; i++)
	{
		pFile->readBin(&cd, sizeof(cd));
		m_chars[cd.id] = cd;
	}
	UINT w, h;
	byte * data;
	for(int i = 0; i < m_header.texCount; i++)
	{
		pFile->readBin(&w, sizeof(w));
		pFile->readBin(&h, sizeof(h));
		data = new byte[w * h * 4];
		pFile->readBin(data, sizeof(byte) * w * h * 4);
		m_ppTextures.push_back(data);

		IGXTexture2D *pTex = m_pDev->createTexture2D(w, h, 0, GX_TEXFLAG_DEFAULT, GXFMT_A8B8G8R8, data);
		m_apTextures.push_back(pTex);
	}
	//LoadFTfontFace();
	mem_release(pFile);
}

void CFont::generateBase()
{
	const char *s = " qwertyuiop[]asdfghjkl;'zxcvbnm,./`1234567890-=\\~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?йцукенгшщзхъфывапролджэячсмитьбюЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮёЁ	";
	
	StreamReader sr(s);
	
	while(!sr.isEnd())
	{
		addChar(sr.readChar());
	}

	regen();
}

void CFont::regen()
{
	//printf("_heapchk() = %d\n", _heapchk());
	for(UINT i = 0, l = m_apTextures.size(); i < l; ++i)
	{
		mem_release(m_apTextures[i]);
	}
	for(UINT i = 0, l = m_ppTextures.size(); i < l; ++i)
	{
		mem_delete_a(m_ppTextures[i]);
	}
	m_apTextures.clearFast();
	m_ppTextures.clearFast();
	m_chars.clearFast();
	//TODO: Chear caches
	loadFTfontFace();
	if(!m_pFTfontFace)
	{
		return;
	}
	FT_GlyphSlot g = m_pFTfontFace->glyph;
	m_chars.clearFast();
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
	for(UINT i = 0, l = m_aFontChars.size(); i < l; ++i)
	{
		if(m_aFontChars[i] != '\t')
		{
			if(FT_Load_Char(m_pFTfontFace, m_aFontChars[i], FT_LOAD_RENDER | (m_uFontSize > 13 ? FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT/* | FT_LOAD_TARGET_LCD*/ : FT_LOAD_TARGET_MONO)))
			//if(FT_Load_Char(m_pFTfontFace, m_aFontChars[i], FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_FORCE_AUTOHINT))
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

		if(d.w > 64 || d.h > 64)
		{
			char tmp[128];
			sprintf_s(tmp, "-:%dx%d", d.w, d.h);
			MessageBoxA(NULL, tmp, "", MB_OK);
		}

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
			FT_Bitmap_Convert(m_pFT, bitmap, &tempbitmapmono, 1);
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
			FT_Bitmap_Done(m_pFT, &tempbitmapmono);
		}

		d.c = m_aFontChars[i]; // FIXME: Fix for two 32bit chars
		for(UINT ii = 0; ii < list.size(); ii++)
		{
			if(!list[ii].like && imageEqual(list[ii], d))
			{
				d.like = list[ii].c;
			}
		}
		list.push_back(d);
	}
		
#define CHECK_0() if(list[0].w > 64 || list[0].h > 64){char tmp[128];sprintf_s(tmp, "#:%dx%d", list[0].w, list[0].h);MessageBoxA(NULL, tmp, GEN_MSG_LOCATION, MB_OK);}

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
	int s = (int)sqrt((float)ss);
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
		if(list[i].w > 64 || list[i].h > 64)
		{
			char tmp[128];
			sprintf_s(tmp, "%d:%dx%d", i, list[i].w, list[i].h);
			MessageBoxA(NULL, tmp, "", MB_OK);
		}
		ImageCopy(image, list[i].data, width, height, list[i].w, list[i].h, list[i].x, list[i].y, 4);
		if(m_bEmulateBold)
		{
			ImageCopy(image, list[i].data, width, height, list[i].w, list[i].h, list[i].x + 1, list[i].y, 4);
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
		cd.yoffset = (int)m_uFontSize - list[i].yo;
		cd.texid = 0;
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
					newImage[(y * width + x) * 4 + 3] = (byte)sum.w;
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

	IGXTexture2D *pTex = m_pDev->createTexture2D(width, height, 0, GX_TEXFLAG_DEFAULT, GXFMT_A8B8G8R8, image);

	for(UINT i = 0; i < list.size(); i++)
	{
		mem_delete_a(list[i].data);
	}

	m_apTextures.push_back(pTex);
	m_ppTextures.push_back(image);
	m_header.texCount = 1; // FIXME: allow multiple textures
	m_header.charCount = list.size();
	m_bHasBeenChanged = true;
	save(); // TODO: Find better place to call that

	// TODO: Add rebuild handler to call Layout on the document


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

void XMETHODCALLTYPE CFont::getTexture(UINT uIdx, IGXTexture2D **ppTexture)
{
	assert(uIdx < m_apTextures.size());

	if(uIdx < m_apTextures.size())
	{
		m_apTextures[uIdx]->AddRef();
		*ppTexture = m_apTextures[uIdx];
	}
	
}

void CFont::addChar(UINT c, bool full)
{
	if(m_aFontChars.indexOf(c) < 0)
	{
		m_aFontChars.push_back(c);
		if(full)
		{
			const symRange *rg = getSymRange(c);
			if(rg)
			{
				for(UINT i = rg->start; i < rg->end; ++i)
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
	mem_release(m_pFontManager);
}

void CFont::freeFTfontFace()
{
	if(m_pFTfontFace)
	{
		FT_Done_Face(m_pFTfontFace);
		m_pFTfontFace = NULL;
	}
	mem_delete_a(m_pFontBlob);
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

const CFont::symRange* CFont::getSymRange(UINT sym)
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

void CFont::getStringMetrics(const char *szString, UINT uFirstLineShift, UINT * width, UINT * height, UINT * vertexCount, UINT * indexCount, UINT * strCount, XCharRect *pCharRects)
{
	UINT w = 0;
	UINT _w = 0;
	UINT h = m_uFontSize;
	UINT vc = 0;
	UINT ic = 0;
	UINT lc = 1;

	const CharDesc * cd;
	XCharRect cr;
	cr.iTop = 0;
	cr.iBottom = m_uFontSize;
#if 0
	if(pcr)
	{
		cr.top += pcr->iYoffs;
		cr.bottom += pcr->iYoffs;
	}
#endif

	UINT uCharRect = 0;

	StreamReader stream(szString);
	UINT ch;
	while(!stream.isEnd())
	{
		ch = stream.readChar();
		cr.uChar = ch;
		cd = getChar(ch);
		if(!iswspace(ch))
		{
			vc += 4;
			ic += 6;
		}
		if(ch == '\n')
		{
			lc++;
			w = max(w, _w);
			_w = 0;
			cr.iTop += m_uFontSize;
			cr.iBottom += m_uFontSize;

			if(pCharRects)
			{
				pCharRects[uCharRect++] = cr;
			}
		}
		else
		{
			if(pCharRects)
			{
				cr.iLeft = _w + uFirstLineShift;
				cr.iRight = _w + uFirstLineShift + cd->xadvantage;
				pCharRects[uCharRect++] = cr;
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

// void CFont::buildString(const char *str, UINT decoration, TEXT_ALIGN textAlign, IGXRenderBuffer ** ppVertexBuffer, IGXIndexBuffer ** ppIndexBuffer, UINT * vertexCount, UINT * indexCount, UINT * lineIndexCount, UINT iAreaWidth, UINT iFirstShift, UINT * pStrWidth)
void XMETHODCALLTYPE CFont::buildString(const char *szString, const XFontBuildParams &params, XFontStringMetrics *pMetrics)
{
	UINT iIndexCount;
	UINT iVertexCount;
	UINT lineCount;
	UINT iWidth;
	UINT iHeight;
	getStringMetrics(szString, params.uFirstLineShift, &iWidth, &iHeight, &iVertexCount, &iIndexCount, &lineCount, params.pCharRects);
	
	if(params.uAreaWidth > iWidth)
	{
		iWidth = params.uAreaWidth;
	}
	pMetrics->uWidth = iWidth;
	pMetrics->uHeight = iHeight;
	pMetrics->uLineCount = lineCount;

	UINT lc = 0;
	if(params.decoration & XFONT_DECORATION_UNDERLINE)
	{
		lc++;
	}
	if(params.decoration & XFONT_DECORATION_OVERLINE)
	{
		lc++;
	}
	if(params.decoration & XFONT_DECORATION_LINE_THROUGH)
	{
		lc++;
	}
	UINT ilineIndexCount = lc * lineCount * 6;

	iIndexCount += ilineIndexCount;
	iVertexCount += lc * lineCount * 4;


	pMetrics->uVertexCount = iVertexCount;
	pMetrics->uQuadCount = iIndexCount / 6;

	XFontVertex *pVB = params.pVertices;

	if(!pVB)
	{
		return;
	}

	Array<float2_t> alx;
	alx.reserve(lineCount);
	UINT CV = 0,
		CI = 0;

	float cx = 0.0f;
	float cy = 0.0f;
	const CharDesc *cd;
	UINT sv = 0;

	bool bFirstLine = true;

	StreamReader stream(szString);

	// for(UINT i = 0, l = str.length(); i < l; i++)
	UINT ch;
	while(!stream.isEnd())
	{
		ch = stream.readChar();
		cd = getChar(ch);

		if(!iswspace(ch))
		{
			/*

			0--1
			| /|
			|/ |
			2--3

			*/

			byte u8TexId = 0;// cd->texid;

			pVB[CV].vPos = float2_t(cx + (bFirstLine ? (float)params.uFirstLineShift : 0) + (float)cd->xoffset, cy + (float)cd->yoffset);
			pVB[CV].vTex = float2_t(cd->tx, cd->ty + cd->theight);
			pVB[CV].uTexIdx = u8TexId;
			CV++;
			pVB[CV].vPos = float2_t(cx + (bFirstLine ? (float)params.uFirstLineShift : 0) + (float)cd->xoffset + (float)cd->width, cy + (float)cd->yoffset);
			pVB[CV].vTex = float2_t(cd->tx + cd->twidth, cd->ty + cd->theight);
			pVB[CV].uTexIdx = u8TexId;
			CV++;
			pVB[CV].vPos = float2_t(cx + (bFirstLine ? (float)params.uFirstLineShift : 0) + (float)cd->xoffset, cy + cd->height + (float)cd->yoffset);
			pVB[CV].vTex = float2_t(cd->tx, cd->ty);
			pVB[CV].uTexIdx = u8TexId;
			CV++;
			pVB[CV].vPos = float2_t(cx + (bFirstLine ? (float)params.uFirstLineShift : 0) + (float)cd->xoffset + (float)cd->width, cy + cd->height + (float)cd->yoffset);
			pVB[CV].vTex = float2_t(cd->tx + cd->twidth, cd->ty);
			pVB[CV].uTexIdx = u8TexId;
			CV++;

			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset, cy + (float)cd->yoffset, 0.0f}, {cd->tx, cd->ty + cd->theight}}; CV++;
			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset + (float)cd->width, cy + (float)cd->yoffset, 0.0f}, {cd->tx + cd->twidth, cd->ty + cd->theight}}; CV++;
			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset, cy + cd->height + (float)cd->yoffset, 0.0f}, {cd->tx, cd->ty}}; CV++;
			//	pVB[CV] = {{cx + (bFirstLine ? (float)iFirstShift : 0) + (float)cd->xoffset + (float)cd->width, cy + cd->height + (float)cd->yoffset, 0.0f}, {cd->tx + cd->twidth, cd->ty}}; CV++;
		}


		if(ch == '\n')
		{
			bFirstLine = false;
			alx.push_back(float2_t(cx, cy));
			cy += m_uFontSize;
			float delta = 0.0f;
			if(params.textAlign == XFONT_TEXT_ALIGN_RIGHT)
			{
				delta = iWidth - cx;
			}
			if(params.textAlign == XFONT_TEXT_ALIGN_CENTER)
			{
				delta = (iWidth - cx) * 0.5f;
			}
			for(UINT j = sv; j < CV; j++)
			{
				pVB[j].vPos.x += delta;
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
		cy += m_uFontSize;
		float delta = 0.0f;
		if(params.textAlign == XFONT_TEXT_ALIGN_RIGHT)
		{
			delta = iWidth - cx;
		}
		if(params.textAlign == XFONT_TEXT_ALIGN_CENTER)
		{
			delta = (iWidth - cx) * 0.5f;
		}
		for(UINT j = sv; j < CV; j++)
		{
			pVB[j].vPos.x += delta;
		}
		cx = 0.0f;
		sv = CV;
	}
	float lineHeight = 1.0f;
	for(UINT i = 0; i < lineCount; i++)
	{
		if(params.decoration & XFONT_DECORATION_OVERLINE)
		{
			pVB[CV].vPos = float2_t(0.0f, alx[i].y);
			pVB[CV].vTex = float2_t(0.0f, 1.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(alx[i].x, alx[i].y);
			pVB[CV].vTex = float2_t(1.0f, 1.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(0.0f, alx[i].y + lineHeight);
			pVB[CV].vTex = float2_t(0.0f, 0.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(alx[i].x, alx[i].y + lineHeight);
			pVB[CV].vTex = float2_t(1.0f, 0.0f);
			pVB[CV].uTexIdx = 0;
			CV++;

			//	pVB[CV] = {{0.0f, alx[i].y, 0.0f}, {0.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y, 0.0f}, {1.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{0.0f, alx[i].y + lineHeight, 0.0f}, {0.0f, 0.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + lineHeight, 0.0f}, {1.0f, 0.0f}}; CV++;
		}
		if(params.decoration & XFONT_DECORATION_UNDERLINE)
		{
			pVB[CV].vPos = float2_t(0.0f, alx[i].y + (float)m_uFontSize - lineHeight);
			pVB[CV].vTex = float2_t(0.0f, 1.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(alx[i].x, alx[i].y + (float)m_uFontSize - lineHeight);
			pVB[CV].vTex = float2_t(1.0f, 1.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(0.0f, alx[i].y + (float)m_uFontSize);
			pVB[CV].vTex = float2_t(0.0f, 0.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(alx[i].x, alx[i].y + (float)m_uFontSize);
			pVB[CV].vTex = float2_t(1.0f, 0.0f);
			pVB[CV].uTexIdx = 0;
			CV++;

			//	pVB[CV] = {{0.0f, alx[i].y + (float)m_iFontSize - lineHeight, 0.0f}, {0.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + (float)m_iFontSize - lineHeight, 0.0f}, {1.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{0.0f, alx[i].y + (float)m_iFontSize, 0.0f}, {0.0f, 0.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + (float)m_iFontSize, 0.0f}, {1.0f, 0.0f}}; CV++;
		}
		if(params.decoration & XFONT_DECORATION_LINE_THROUGH)
		{
			pVB[CV].vPos = float2_t(0.0f, alx[i].y + (float)m_uFontSize * 0.5f);
			pVB[CV].vTex = float2_t(0.0f, 1.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(alx[i].x, alx[i].y + (float)m_uFontSize * 0.5f);
			pVB[CV].vTex = float2_t(1.0f, 1.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(0.0f, alx[i].y + lineHeight + (float)m_uFontSize * 0.5f);
			pVB[CV].vTex = float2_t(0.0f, 0.0f);
			pVB[CV].uTexIdx = 0;
			CV++;
			pVB[CV].vPos = float2_t(alx[i].x, alx[i].y + lineHeight + (float)m_uFontSize * 0.5f);
			pVB[CV].vTex = float2_t(1.0f, 0.0f);
			pVB[CV].uTexIdx = 0;
			CV++;

			//	pVB[CV] = {{0.0f, alx[i].y + (float)m_iFontSize * 0.5f, 0.0f}, {0.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + (float)m_iFontSize * 0.5f, 0.0f}, {1.0f, 1.0f}}; CV++;
			//	pVB[CV] = {{0.0f, alx[i].y + lineHeight + (float)m_iFontSize * 0.5f, 0.0f}, {0.0f, 0.0f}}; CV++;
			//	pVB[CV] = {{alx[i].x, alx[i].y + lineHeight + (float)m_iFontSize * 0.5f, 0.0f}, {1.0f, 0.0f}}; CV++;
		}
	}

	if(m_iBlurRadius)
	{
		float2 offs((float)-m_iBlurRadius, (float)-m_iBlurRadius);
		for(UINT i = 0; i < CV; ++i)
		{
			pVB[i].vPos = (float2)(pVB[i].vPos + offs);
		}
	}
}

void XMETHODCALLTYPE CFont::FinalRelease()
{
	m_pFontManager->onFontReleased(this);
}