#include "ColorPicker.h"
#include <commdlg.h>

CColorPicker::CColorPicker(HWND hMainWnd, IXCore *pCore):
	m_hMainWnd(hMainWnd),
	m_pCore(pCore)
{
}

CColorPicker::~CColorPicker()
{
}

void XMETHODCALLTYPE CColorPicker::pick(const float4_t &vStartColor, IXColorPickerCallback *pCallback)
{
	callAbort();

	m_vCurrentColor = m_vStartColor = vStartColor;
	m_pCallback = pCallback;

	// init picker
	CHOOSECOLOR cc = {sizeof(cc)};
	static COLORREF s_acrCustClr[16];

	cc.hwndOwner = m_hMainWnd;
	cc.lpCustColors = s_acrCustClr;
	cc.rgbResult = RGB((UINT)(m_vStartColor.x * 255.0f), (UINT)(m_vStartColor.y * 255.0f), (UINT)(m_vStartColor.z * 255.0f));
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if(ChooseColor(&cc) == TRUE)
	{
		m_vCurrentColor.x = (float)GetRValue(cc.rgbResult) / 255.0f;
		m_vCurrentColor.y = (float)GetGValue(cc.rgbResult) / 255.0f;
		m_vCurrentColor.z = (float)GetBValue(cc.rgbResult) / 255.0f;

		callAccept();
	}
	else
	{
		callAbort();
	}
}
void XMETHODCALLTYPE CColorPicker::abort()
{
	callAbort();
}

void CColorPicker::callAccept()
{
	SAFE_CALL(m_pCallback, onAccept, m_vCurrentColor);
	m_pCallback = NULL;
}
void CColorPicker::callAbort()
{
	SAFE_CALL(m_pCallback, onCancel, m_vStartColor);
	m_pCallback = NULL;
}
