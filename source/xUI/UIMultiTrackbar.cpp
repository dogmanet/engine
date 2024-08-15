#include "UIMultiTrackbar.h"

CUIMultiTrackbar::CUIMultiTrackbar(ULONG uID):
	BaseClass(uID, "div")
{
}

void XMETHODCALLTYPE CUIMultiTrackbar::setMarkerCount(UINT uCount)
{
	m_aMarkers.resizeFast(uCount);

	resize();
}
UINT XMETHODCALLTYPE CUIMultiTrackbar::getMarkerCount()
{
	return(m_aMarkers.size());
}
void XMETHODCALLTYPE CUIMultiTrackbar::setMarkerPos(UINT uIndex, float fPos)
{
	assert(uIndex < m_aMarkers.size());

	if(uIndex < m_aMarkers.size())
	{
		fPos = clampf(fPos, 0.0f, 1.0f);
		Marker &m = m_aMarkers[uIndex];
		m.fPos = fPos;

		if(m.pNode)
		{
			m.pNode->getStyleSelf()->left->set(fPos * 100.0f);
			//m.pNode->getStyle()->left->setDim(gui::css::ICSSproperty::DIM_PC);
			m.pNode->updateStyles();
		}
	}
}
float XMETHODCALLTYPE CUIMultiTrackbar::getMarkerPos(UINT uIndex)
{
	assert(uIndex < m_aMarkers.size());

	if(uIndex < m_aMarkers.size())
	{
		return(m_aMarkers[uIndex].fPos);
	}

	return(0.0f);
}
void XMETHODCALLTYPE CUIMultiTrackbar::removeMarker(UINT uIndex)
{
	assert(uIndex < m_aMarkers.size());

	if(uIndex < m_aMarkers.size())
	{
		Marker &m = m_aMarkers[uIndex];
		m.pNode->parentNode()->removeChild(m.pNode);
		m_aMarkers.erase(uIndex);

		if(uIndex == m_iSelectedMarker)
		{
			m_iSelectedMarker = -1;
			if(m_isDragging)
			{
				m_isDragging = false;
				m_pDoc->releaseCapture();
			}
		}

		gui::IEvent ev;
		ev.type = gui::GUI_EVENT_TYPE_CHANGE;
		ev.target = m_pNode;
		ev.relatedTarget = NULL;
		m_pNode->dispatchEvent(ev);
	}
}
void XMETHODCALLTYPE CUIMultiTrackbar::setMarkerColor(UINT uIndex, const float4_t &vColor)
{
	assert(uIndex < m_aMarkers.size());

	if(uIndex < m_aMarkers.size())
	{
		Marker &m = m_aMarkers[uIndex];
		m.hasColor = true;

		m.vColor = vColor;

		gui::dom::IDOMnode *pChildNode = m.pNode->getChilds()[0][0];
		UINT uColor =
			(((UINT)((vColor.x) * 255.f) & 0xFF) << 24) |
			(((UINT)((vColor.y) * 255.f) & 0xFF) << 16) |
			(((UINT)((vColor.z) * 255.f) & 0xFF) << 8) |
			(((UINT)((vColor.w) * 255.f) & 0xFF));

		pChildNode->getStyleSelf()->background_color->set((int)uColor);
		pChildNode->updateStyles();
	}
}

float4_t XMETHODCALLTYPE CUIMultiTrackbar::getMarkerColor(UINT uIndex)
{
	assert(uIndex < m_aMarkers.size());

	if(uIndex < m_aMarkers.size())
	{
		Marker &m = m_aMarkers[uIndex];
		return(m.vColor);
	}
	return(float4_t(0.0f, 0.0f, 0.0f, 1.0f));
}


gui::dom::IDOMnode* CUIMultiTrackbar::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	m_pDoc = pDomDocument;

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"multi-trackbar-wrapper");
	if(m_isInverted)
	{
		m_pNode->classAdd(L"-inverted");
	}

	m_pNode->setUserData(this);
	m_pNode->setAttribute(L"onmousedown", L"handler");
	m_pNode->setAttribute(L"onmouseup", L"handler");
	m_pNode->setAttribute(L"onmousemove", L"handler");
	m_pNode->setAttribute(L"onclick", L"handler");
	m_pNode->setAttribute(L"onchange", L"handler");

	resize();

	return(m_pNode);
}

void XMETHODCALLTYPE CUIMultiTrackbar::setLabel(const char *szTitle)
{
//	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUIMultiTrackbar::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;

	fora(i, m_aMarkers)
	{
		m_aMarkers[i].pNode = NULL;
	}
}

void CUIMultiTrackbar::resize()
{
	if(!m_pNode)
	{
		return;
	}

	const gui::dom::IDOMnodeCollection *pChildren = m_pNode->getChilds();

	while(pChildren->size() > m_aMarkers.size())
	{
		m_pNode->removeChild(pChildren[0][pChildren->size() - 1]);
	}
	
	gui::css::ICSSstyle *pStyle;

	for(UINT i = pChildren->size(), l = m_aMarkers.size(); i < l; ++i)
	{
		gui::dom::IDOMnode *pNode = m_pDoc->createNode(L"div");
		pNode->classAdd(L"trackbar-marker");
		
		pStyle = pNode->getStyleSelf();
		pStyle->left->set(m_aMarkers[i].fPos * 100.0f);
		pStyle->left->setDim(gui::css::ICSSproperty::DIM_PC);

		
		gui::dom::IDOMnode *pNode2 = m_pDoc->createNode(L"div");
		
		if(m_aMarkers[i].hasColor)
		{
			UINT uColor =
				(((UINT)((m_aMarkers[i].vColor.x) * 255.f) & 0xff) << 24) |
				(((UINT)((m_aMarkers[i].vColor.y) * 255.f) & 0xff) << 16) |
				(((UINT)((m_aMarkers[i].vColor.z) * 255.f) & 0xff) << 8) |
				(((UINT)((m_aMarkers[i].vColor.w) * 255.f) & 0xff));

			pNode2->getStyleSelf()->background_color->set((int)uColor);
		}

		pNode->appendChild(pNode2);

		pNode2->setAttribute(L"onkeypress", L"handler");
		pNode2->setUserData(this);

		m_pNode->appendChild(pNode);
		m_aMarkers[i].pNode = pNode;

		if(m_iSelectedMarker == i)
		{
			TODO("Use constant for pseudoclass");
			pNode->addPseudoclass(2 /* PSEUDOCLASS_CHECKED */);
		}
	}
}

void CUIMultiTrackbar::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_MOUSEDOWN)
	{
		int iRow = m_aMarkers.indexOf(ev->target, [](const Marker &row, gui::dom::IDOMnode *pNode){
			return(row.pNode == pNode || row.pNode == pNode->parentNode());
		});

		if(iRow >= 0)
		{
			setSelection(iRow);
			m_isDragging = true;

			m_pDoc->setCapture(m_pNode);
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_MOUSEUP)
	{
		m_pDoc->releaseCapture();
		m_isDragging = false;
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_MOUSEMOVE)
	{
		if(m_isDragging)
		{
			setMarkerPos(m_iSelectedMarker, getPosByOffset(ev->offsetX));

			gui::IEvent ev;
			ev.type = gui::GUI_EVENT_TYPE_CHANGE;
			ev.target = m_pNode;
			ev.relatedTarget = m_iSelectedMarker >= 0 ? m_aMarkers[m_iSelectedMarker].pNode : NULL;
			m_pNode->dispatchEvent(ev);
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_KEYPRESS)
	{
		if(ev->key == KEY_DELETE && m_iSelectedMarker >= 0)
		{
			removeMarker(m_iSelectedMarker);
			ev->stopPropagation();
			ev->preventDefault = true;
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		if(ev->target == m_pNode)
		{
			UINT uNewMarker = getMarkerCount();
			float fPos = getPosByOffset(ev->offsetX);
			setMarkerCount(uNewMarker + 1);
			setMarkerPos(uNewMarker, fPos);
			setSelection(uNewMarker);
			m_pDoc->requestFocus(m_aMarkers[uNewMarker].pNode);
		}
	}
}

void XMETHODCALLTYPE CUIMultiTrackbar::setSelection(int iMarker)
{
	if((UINT)iMarker >= m_aMarkers.size())
	{
		iMarker = -1;
	}

	if(iMarker == m_iSelectedMarker)
	{
		return;
	}

	if(m_iSelectedMarker >= 0 && m_aMarkers[m_iSelectedMarker].pNode)
	{
		TODO("Use constant for pseudoclass");
		m_aMarkers[m_iSelectedMarker].pNode->removePseudoclass(2 /* PSEUDOCLASS_CHECKED */);
	}

	m_iSelectedMarker = iMarker;
	if(m_iSelectedMarker >= 0 && m_aMarkers[m_iSelectedMarker].pNode)
	{
		TODO("Use constant for pseudoclass");
		m_aMarkers[m_iSelectedMarker].pNode->addPseudoclass(2 /* PSEUDOCLASS_CHECKED */);
	}

	gui::IEvent ev;
	ev.type = gui::GUI_EVENT_TYPE_CHANGE;
	ev.target = m_pNode;
	ev.relatedTarget = m_iSelectedMarker >= 0 ? m_aMarkers[m_iSelectedMarker].pNode : NULL;
	m_pNode->dispatchEvent(ev);
}
int XMETHODCALLTYPE CUIMultiTrackbar::getSelection()
{
	return(m_iSelectedMarker);
}

void XMETHODCALLTYPE CUIMultiTrackbar::setEnabled(bool bEnable)
{
	if(bEnable)
	{
		m_pNode->removePseudoclass(0x00004);
	}
	else
	{
		m_pNode->addPseudoclass(0x00004);
	}
}

float CUIMultiTrackbar::getPosByOffset(int iOffset)
{
	if(iOffset < 0)
	{
		iOffset = 0;
	}
	int iWidth = (int)m_pNode->getInnerWidth();
	if(iOffset > iWidth)
	{
		iOffset = iWidth;
	}

	return((float)iOffset / (float)iWidth);
}

void XMETHODCALLTYPE CUIMultiTrackbar::setInvertedMode(bool yesNo)
{
	m_isInverted = yesNo;

	if(m_isInverted)
	{
		m_pNode->setAttribute(L"class", L"multi-trackbar-wrapper -inverted");
	}
	else
	{
		m_pNode->setAttribute(L"class", L"multi-trackbar-wrapper");
	}

	m_pNode->updateStyles();
}
