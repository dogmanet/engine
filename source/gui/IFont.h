#ifndef __IFONT_H
#define __IFONT_H


class IGXTexture;

namespace gui
{
	class IFont
	{
	public:
		virtual ~IFont()
		{}

		enum STYLE
		{
			STYLE_NONE = 0x00,
			STYLE_BOLD = 0x01,
			STYLE_ITALIC = 0x02,
			STYLE_BOLD_ITALIC = 0x03
		};

		enum DECORATION
		{
			DECORATION_NONE = 0x00,
			DECORATION_UNDERLINE = 0x01,
			DECORATION_LINE_THROUGH = 0x02,
			DECORATION_OVERLINE = 0x04
		};

		enum TEXT_ALIGN
		{
			TEXT_ALIGN_LEFT,
			TEXT_ALIGN_RIGHT,
			TEXT_ALIGN_CENTER
		};

		struct char_rects
		{
			Array<RECT> * pResArray;
			int iXoffs;
			int iYoffs;
		};

		virtual IGXTexture2D* getAPITexture(UINT i) = 0;

		virtual void getStringMetrics(const StringW & str, UINT * width, UINT * height, UINT * vertexCount, UINT * indexCount, UINT * strCount, char_rects * pcr = NULL) = 0;

		virtual void buildString(const StringW & str, UINT decoration, TEXT_ALIGN textAlign, IGXRenderBuffer ** ppVertexBuffer, IGXIndexBuffer ** ppIndexBuffer, UINT * vertexCount, UINT * indexCount, UINT * lineIndexCount, UINT iAreaWidth = 0, UINT iFirstShift = 0, UINT * pStrWidth = NULL) = 0;
	};
};

#endif
