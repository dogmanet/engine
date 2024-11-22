#ifndef __WINDOW_H
#define __WINDOW_H

#include "IXWindowSystem.h"

class CWindow final: public IXUnknownImplementation<IXWindow>
{
public:
	CWindow(HINSTANCE hInst, UINT uId, const XWINDOW_DESC *pWindowDesc, IXWindowCallback *pCallback = NULL, IXWindow *pParent = NULL);
	~CWindow();

	XWINDOW_OS_HANDLE XMETHODCALLTYPE getOSHandle() override;

	void XMETHODCALLTYPE hide() override;
	void XMETHODCALLTYPE close() override;
	void XMETHODCALLTYPE show() override;
	bool XMETHODCALLTYPE isVisible() override;
	void XMETHODCALLTYPE setTitle(const char *szTitle) override;
	void XMETHODCALLTYPE update(const XWINDOW_DESC *pWindowDesc) override;

	INT_PTR XMETHODCALLTYPE runDefaultCallback(UINT msg, WPARAM wParam, LPARAM lParam) override;

	const XWINDOW_DESC* XMETHODCALLTYPE getDesc() override;
	
	IXWindow* XMETHODCALLTYPE getParent() override;

	bool XMETHODCALLTYPE getPlacement(XWindowPlacement *pPlacement) override;
	void XMETHODCALLTYPE setPlacement(const XWindowPlacement &placement, bool bSkipVisibility = false) override;

	INT_PTR runCallback(UINT msg, WPARAM wParam, LPARAM lParam);

	float XMETHODCALLTYPE getScale() override;
private:
	HWND m_hWnd = NULL;
	HINSTANCE m_hInst;
	XWINDOW_DESC m_windowDesc;
	UINT m_uId;
	IXWindowCallback *m_pCallback;
	IXWindow *m_pParent;

	float m_fScale = 1.0f;

	bool m_isVisible = false;

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif
