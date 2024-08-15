#ifndef __COLOR_PICKER_H
#define __COLOR_PICKER_H


#include <xUI/IXUI.h>
#include <xcommon/IXCore.h>

#include <xcommon/editor/IXColorPicker.h>

class CColorPicker final: public IXUnknownImplementation<IXColorPicker>
{
public:
	CColorPicker(HWND hMainWnd, IXCore *pCore);
	~CColorPicker();

	void XMETHODCALLTYPE pick(const float4_t &vStartColor, IXColorPickerCallback *pCallback) override;
	void XMETHODCALLTYPE abort() override;

private:
	HWND m_hMainWnd = NULL;
	IXCore *m_pCore = NULL;

	float4_t m_vCurrentColor;
	float4_t m_vStartColor;

	IXColorPickerCallback *m_pCallback = NULL;

private:
	void callAccept();
	void callAbort();
};

#endif
