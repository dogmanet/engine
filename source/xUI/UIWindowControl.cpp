#include "UIWindowControl.h"


CUIWindowControl::CUIWindowControl(CUIWindow *pWindow, ULONG uID):
	BaseClass(uID, "window")
{
	m_pUIWindow = pWindow;

	m_pContainerNode = m_pNode = pWindow->getDesktop()->getDocument()->getElementsByTag(L"body")[0][0];
}
