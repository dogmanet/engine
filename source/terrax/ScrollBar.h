#ifndef __SCROLL_BAR_H
#define __SCROLL_BAR_H

//#include <graphix/graphix.h>
#include <xcommon/render/IXRender.h>
//#include "terrax.h"

class IScrollEventListener
{
public:
	virtual void onPageUp() = 0;
	virtual void onPageDown() = 0;
	virtual void onLineUp() = 0;
	virtual void onLineDown() = 0;
	virtual void onScroll(int iScroll) = 0;
};

class CScrollBar final
{
public:
	CScrollBar(IXRender *pRender, IScrollEventListener *pEventListener);
	~CScrollBar();

	void render();

	void setPos(int iX, int iY);
	void setSize(UINT uSize);

	void setScrollMax(int iMax);
	void setScrollPos(int iPos);
	void setPageSize(UINT uSize);

	int getScrollPos()
	{
		return(m_iScrollPos);
	}

	UINT getWidth() const
	{
		return(18);
	}

	void onMouseDown(int y);
	void onMouseUp();
	void onMouseMove(int y);
	void onMouseHover(bool isHover);

	void update(float fDT);
private:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDev = NULL;

	IGXIndexBuffer *m_pFrameIB = NULL;
	IGXRenderBuffer *m_pFrameRB = NULL;
	UINT m_uFrameVC = 0;
	UINT m_uFramePC = 0;

	IGXConstantBuffer *m_pInformCB = NULL;
	
	bool m_isDirty = true;

	struct FrameState
	{
		float4_t vHighlight; // x: top, y: bottom, z: handler, w: line

		float2_t vOffset;
		float fSize;
		float fHandlerPos;

		float fHandlerSize;
		float3_t _dummy;
	};

	FrameState m_frameState;

	ID m_idFrameShader = -1;

	UINT m_uSize = 0;
	UINT m_uPageSize = 0;
	int m_iScrollMax = 0;
	int m_iScrollPos = 0;

	enum HOVER_EL
	{
		HEL_NONE = 0,
		HEL_UP,
		HEL_DOWN,
		HEL_HANDLER
	};

	HOVER_EL m_hoverEl = HEL_NONE;
	HOVER_EL m_activeEl = HEL_NONE;

	UINT m_uHandlerSize = 0;
	UINT m_uHandlerPos = 0;

	IScrollEventListener *m_pEventListener;

	int m_iDragStartPos = 0;
	int m_iDragStartY = 0;

	float m_fTimer = 0.0f;
private:
	void initHelpers();

	void updateScroll();
};

#endif
