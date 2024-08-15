#ifndef __UIMULTI_TRACKBAR_H
#define __UIMULTI_TRACKBAR_H

#include "IUIWindow.h"
#include "UIControl.h"
#include "IUIMultiTrackbar.h"

class CUIMultiTrackbar: public CUIControl<IUIMultiTrackbar>
{
public:
	CUIMultiTrackbar(ULONG uID);

	void XMETHODCALLTYPE setMarkerCount(UINT uCount) override;
	UINT XMETHODCALLTYPE getMarkerCount() override;
	void XMETHODCALLTYPE setMarkerPos(UINT uIndex, float fPos) override;
	float XMETHODCALLTYPE getMarkerPos(UINT uIndex) override;
	void XMETHODCALLTYPE removeMarker(UINT uIndex) override;
	void XMETHODCALLTYPE setMarkerColor(UINT uIndex, const float4_t &vColor) override;
	float4_t XMETHODCALLTYPE getMarkerColor(UINT uIndex) override;

	void XMETHODCALLTYPE setSelection(int iMarker) override;
	int XMETHODCALLTYPE getSelection() override;

	void XMETHODCALLTYPE setLabel(const char *szTitle) override;

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;
	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;

	void XMETHODCALLTYPE setEnabled(bool bEnable) override;

	void XMETHODCALLTYPE setInvertedMode(bool yesNo) override;

private:
	struct Marker
	{
		float fPos = 0.0f;
		float4_t vColor;
		bool hasColor = false;
		gui::dom::IDOMnode *pNode = NULL;
	};

private:
	gui::dom::IDOMnode *m_pLabel = NULL;

	Array<Marker> m_aMarkers;

	int m_iSelectedMarker = -1;
	
	gui::dom::IDOMdocument *m_pDoc = NULL;

	bool m_isDragging = false;

	bool m_isInverted = false;

private:
	void cleanupNodes() override;

	void resize();

	float getPosByOffset(int iOffset);
};

#endif
