#include "UIColor.h"

//#include <windowsx.h>
#include <commdlg.h>
//#include <commctrl.h>

CUIColor::CUIColor(ULONG uID):
	BaseClass(uID, "input")
{
	m_vValue.w = 1.0f;
}

gui::dom::IDOMnode* CUIColor::createNode(gui::dom::IDOMdocument *pDomDocument)
{
	//m_pNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	//m_pNode->setAttribute(L"controld_id", StringW(m_id));
	//return(m_pNode);

	m_pNode = pDomDocument->createNode(L"div");
	m_pNode->classAdd(L"color-wrapper");
	m_pLabel = pDomDocument->createNode(L"label");

	gui::dom::IDOMnode *pDiv = pDomDocument->createNode(L"div");
	m_pPreview = pDomDocument->createNode(L"div");
	m_pPreview->classAdd(L"color-preview");
	m_pPreview->setAttribute(L"onclick", L"handler");
	m_pPreview->setUserData(this);

	m_pInputNode = pDomDocument->createNode(CMB2WC(m_sName.c_str()));
	m_pInputNode->setUserData(this);
	m_pInputNode->setAttribute(L"onchange", L"handler");
	m_pInputNode->setAttribute(L"onkeyup", L"handler");
	m_pInputNode->setAttribute(L"onkeypress", L"handler");

	StringW wsId = StringW(L"input_") + StringW(m_id);

	m_pInputNode->setAttribute(L"id", StringW(m_id));
	m_pLabel->setAttribute(L"for", StringW(m_id));
	
	setValue(m_vValue);

	m_pNode->appendChild(m_pLabel);
	pDiv->appendChild(m_pPreview);
	pDiv->appendChild(m_pInputNode);
	m_pNode->appendChild(pDiv);
	return(m_pNode);
}

void CUIColor::setLabel(const char *szTitle)
{
	m_pLabel->setText(StringW(CMB2WC(szTitle)), true);
}

void CUIColor::cleanupNodes()
{
	BaseClass::cleanupNodes();
	m_pLabel = NULL;
	m_pPreview = NULL;
}

void XMETHODCALLTYPE CUIColor::setValue(const float4_t &vColor)
{
	m_vValue = vColor;

	wchar_t wszValue[256];
	swprintf(wszValue, L"%g %g %g %g", vColor.x, vColor.y, vColor.z, vColor.w);
	m_pInputNode->setText(wszValue, TRUE);
	
	UINT uColor = 
		(((UINT)((vColor.x) * 255.f) & 0xFF) << 24) | 
		(((UINT)((vColor.y) * 255.f) & 0xFF) << 16) | 
		(((UINT)((vColor.z) * 255.f) & 0xFF) << 8) |
		0xFF;

	m_pPreview->getStyleSelf()->background_color->set((int)uColor);
	m_pPreview->updateStyles();
}
float4_t XMETHODCALLTYPE CUIColor::getValue()
{
	return(m_vValue);
}

void CUIColor::dispatchEvent(gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		if(ev->target == m_pPreview)
		{
			// create color picker dialogue
			CHOOSECOLOR cc = {sizeof(cc)};
			static COLORREF s_acrCustClr[16];

			cc.hwndOwner = (HWND)m_pUIWindow->getXWindow()->getOSHandle();
			cc.lpCustColors = s_acrCustClr;
			cc.rgbResult = RGB((UINT)(m_vValue.x * 255.0f), (UINT)(m_vValue.y * 255.0f), (UINT)(m_vValue.z * 255.0f));
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;

			if(ChooseColor(&cc) == TRUE)
			{
				m_vValue.x = (float)GetRValue(cc.rgbResult) / 255.0f;
				m_vValue.y = (float)GetGValue(cc.rgbResult) / 255.0f;
				m_vValue.z = (float)GetBValue(cc.rgbResult) / 255.0f;

				setValue(m_vValue);

				gui::IEvent ev;
				ev.type = gui::GUI_EVENT_TYPE_CHANGE;
				ev.target = m_pInputNode;
				m_pInputNode->dispatchEvent(ev);
			}
		}
	}
	else if(ev->type == gui::GUI_EVENT_TYPE_KEYPRESS || ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		if(ev->target == m_pInputNode)
		{
			// update color
			float4_t vColor;
			int nParts = swscanf(m_pInputNode->getText().c_str(), L"%g %g %g %g", &vColor.x, &vColor.y, &vColor.z, &vColor.w);
			if(nParts < 4)
			{
				vColor.w = 1.0f;
			}
			if(nParts == 1)
			{
				vColor.y = vColor.z = vColor.x;
			}
			else if(nParts == 2)
			{
				vColor.z = 0.0f;
			}

			m_vValue = vColor;

			UINT uColor =
				(((UINT)((vColor.x) * 255.f) & 0xFF) << 24) |
				(((UINT)((vColor.y) * 255.f) & 0xFF) << 16) |
				(((UINT)((vColor.z) * 255.f) & 0xFF) << 8) |
				0xFF;

			m_pPreview->getStyleSelf()->background_color->set((int)uColor);
			m_pPreview->updateStyles();

			gui::IEvent ev;
			ev.type = gui::GUI_EVENT_TYPE_CHANGE;
			ev.target = m_pInputNode;
			m_pInputNode->dispatchEvent(ev);
		}
	}
}

void XMETHODCALLTYPE CUIColor::setEnabled(bool bEnable)
{
	BaseClass::setEnabled(bEnable);

	if(bEnable)
	{
		m_pPreview->removePseudoclass(0x00004);
	}
	else
	{
		m_pPreview->addPseudoclass(0x00004);
	}
}
