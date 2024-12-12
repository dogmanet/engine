
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "sxinput.h"
#include "input.h"

#include <core/sxcore.h>

#if defined(_DEBUG)
#	pragma comment(lib, "sxcore_d.lib")
#else
#	pragma comment(lib, "sxcore.lib")
#endif

SXInput::SXInput(const char* name):
	mdelta({0, 0}),
	mdeltaOld({0, 0}),
	m_pfnCallback(NULL)
{
	sprintf(Name, "%s", name);
}

long SXInput::Init(HWND hwnd)
{
	InitKeymap();

#if defined(_WINDOWS)
	RAWINPUTDEVICE Rid[1];

	Rid[0].usUsagePage = 1;
	Rid[0].usUsage = 2;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = hwnd;

	if(RegisterRawInputDevices(Rid, sizeof(Rid) / sizeof(RAWINPUTDEVICE), sizeof(Rid[0])) == FALSE)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Registering RAW Input failed");
		return(-1);
	}
#endif

	return 0;
}

SXInput::~SXInput()
{
}

void SXInput::Update()
{
	for(int i = 0; i < SXI_KEYMAP_SIZE; ++i)
	{
		m_vKeyMap[i].is_dblclick = FALSE;
		m_vKeyMap[i].changed = FALSE;
	}

	wheelCount = 0;

	mdeltaOld = mdelta;
	mdelta = {0};

	IMSG msg;
	while(!m_qMsgs.empty())
	{
		msg = m_qMsgs.front();
		m_qMsgs.pop();

		UINT key = 0;
		switch(msg.message)
		{
		case WM_XBUTTONDBLCLK:
			key = SIK_OFFS + GET_XBUTTON_WPARAM(msg.wParam) + 2;
		case WM_LBUTTONDBLCLK:
			if(!key)
				key = SIM_LBUTTON;
		case WM_RBUTTONDBLCLK:
			if(!key)
				key = SIM_RBUTTON;
		case WM_MBUTTONDBLCLK:
			if(!key)
				key = SIM_MBUTTON;
			m_vKeyMap[key].is_dblclick = TRUE;
		case WM_XBUTTONDOWN:
			if(!key)
				key = SIK_OFFS + GET_XBUTTON_WPARAM(msg.wParam) + 2;
		case WM_RBUTTONDOWN:
			if(!key)
				key = SIM_RBUTTON;
		case WM_MBUTTONDOWN:
			if(!key)
				key = SIM_MBUTTON;
		case WM_LBUTTONDOWN:
			if(!key)
				key = SIM_LBUTTON;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			SXIKD * pikd;
			pikd = (SXIKD*)&msg.lParam;

			if(!key)
				key = pikd->scanCode + (pikd->isExtended ? 128 : 0);

			if(key < SXI_KEYMAP_SIZE && !m_vKeyMap[key].state)
			{
				if(m_pfnCallback)
				{
					continue;
				}

				m_vKeyMap[key].state = TRUE;
				m_vKeyMap[key].changed = TRUE;

				if(m_vKeyMap[key].cmd != 0)
				{
					Core_0ConsoleExecCmd("%s", m_vKeyMap[key].cmd);
				}
			}
			if(
				((key == SIK_LSHIFT || key == SIK_RSHIFT) && (key = SIK_SHIFT))
				|| ((key == SIK_LALT || key == SIK_RALT) && (key = SIK_ALT))
				|| ((key == SIK_LCONTROL || key == SIK_RCONTROL) && (key = SIK_CONTROL))
				)
			{
				if(key < SXI_KEYMAP_SIZE && !m_vKeyMap[key].state)
				{
					m_vKeyMap[key].state = TRUE;
					m_vKeyMap[key].changed = TRUE;

					if(m_vKeyMap[key].cmd != 0)
					{
						Core_0ConsoleExecCmd("%s", m_vKeyMap[key].cmd);
					}
				}
			}
			break;


		case WM_XBUTTONUP:
			key = SIK_OFFS + GET_XBUTTON_WPARAM(msg.wParam) + 2;
		case WM_RBUTTONUP:
			if(!key)
				key = SIM_RBUTTON;
		case WM_MBUTTONUP:
			if(!key)
				key = SIM_MBUTTON;
		case WM_LBUTTONUP:
			if(!key)
				key = SIM_LBUTTON;
		case WM_SYSKEYUP:
		case WM_KEYUP:
			pikd = (SXIKD*)&msg.lParam;

			if(!key)
				key = pikd->scanCode + (pikd->isExtended ? 128 : 0);

			if(m_pfnCallback && key < SXI_KEYMAP_SIZE)
			{
				execOnNextKeyPress(m_vKeyMap[key].bind);
				return;
			}

			if(key < SXI_KEYMAP_SIZE && m_vKeyMap[key].state)
			{
				m_vKeyMap[key].state = FALSE;
				m_vKeyMap[key].changed = TRUE;

				if(m_vKeyMap[key].cmd != 0 && m_vKeyMap[key].cmd[0] == L'+')
				{
					Core_0ConsoleExecCmd("-%s", m_vKeyMap[key].cmd + 1);
				}
			}

			if(
				((key == SIK_LSHIFT || key == SIK_RSHIFT) && (key = SIK_SHIFT))
				|| ((key == SIK_LALT || key == SIK_RALT) && (key = SIK_ALT))
				|| ((key == SIK_LCONTROL || key == SIK_RCONTROL) && (key = SIK_CONTROL))
				)
			{
				if(key < SXI_KEYMAP_SIZE && m_vKeyMap[key].state)
				{
					m_vKeyMap[key].state = FALSE;
					m_vKeyMap[key].changed = TRUE;

					if(m_vKeyMap[key].cmd != 0 && m_vKeyMap[key].cmd[0] == L'+')
					{
						Core_0ConsoleExecCmd("-%s", m_vKeyMap[key].cmd + 1);
					}
				}
			}
			break;

		case WM_MOUSEWHEEL:
			wheelDelta += GET_WHEEL_DELTA_WPARAM(msg.wParam);
			for(; wheelDelta > WHEEL_DELTA; wheelDelta -= WHEEL_DELTA)
			{
				wheelCount++;
			}
			for(; wheelDelta < 0; wheelDelta += WHEEL_DELTA)
			{
				wheelCount--;
			}
			if(wheelCount)
			{
				key = wheelCount > 0 ? SIM_MWHEELUP : SIM_MWHEELDOWN;
				if(m_pfnCallback)
				{
					execOnNextKeyPress(m_vKeyMap[key].bind);
					return;
				}
				if(m_vKeyMap[key].cmd != 0)
				{
					Core_0ConsoleExecCmd("%s", m_vKeyMap[key].cmd);
					if(m_vKeyMap[key].cmd[0] == L'+')
					{
						Core_0ConsoleExecCmd("-%s", m_vKeyMap[key].cmd + 1);
					}
				}
			}
			break;

		case WM_KILLFOCUS:
			for(int key = 0; key < SXI_KEYMAP_SIZE; ++key)
			{
				if(m_vKeyMap[key].state)
				{
					m_vKeyMap[key].state = FALSE;
					m_vKeyMap[key].changed = TRUE;

					if(m_vKeyMap[key].cmd != 0 && m_vKeyMap[key].cmd[0] == L'+')
					{
						Core_0ConsoleExecCmd("-%s", m_vKeyMap[key].cmd + 1);
					}
				}
			}
			break;
		}
	}
}

void SXInput::GetMouseDelta(int * x, int * y)
{
	if(x)
	{
		*x = mdeltaOld.x;
	}
	if(y)
	{
		*y = mdeltaOld.y;
	}
}

void SXInput::setEnable(bool bEnable)
{
	if(!bEnable)
	{
		IMSG msg;
		msg.message = WM_KILLFOCUS;
		msg.lParam = msg.wParam = 0;
		QueueMsg(msg);

		m_isFirstRIM = true;
	}

	m_bEnabled = bEnable;
}

bool SXInput::GetKeyState(InputCode Key)
{
	return(!!m_vKeyMap[Key].state);
}

InputEvents SXInput::GetKeyEvents(InputCode Key)
{
	IKM * ikm = &m_vKeyMap[Key];
	if(ikm->is_dblclick)
	{
		return(iv_k_dbl);
	}
	if(ikm->state)
	{
		return(ikm->changed ? iv_k_first : iv_k_down);
	}
	return(iv_k_up);
}

long SXInput::GetScroll()
{
	return wheelCount * WHEEL_DELTA + wheelDelta;
}

bool SXInput::GetMouseDouble(InputCode Button)
{
	return(!!m_vKeyMap[Button].is_dblclick);
}

bool SXInput::IsOtherButtonOn(InputCode Button)
{
	for(int i = SIM_START; i <= SIM_END; ++i)
	{
		if(i != Button && GetKeyState(i))
		{
			return(true);
		}
	}
	return(false);
}

bool SXInput::GetExeEvents(InMess *Event)
{
	return GetKeyEvents(Event->Section) == Event->Code;
}

bool SXInput::GetExeEvents(InputCode sect, InputEvents code)
{
	return GetKeyEvents(sect) == code;
}


#define SXI_BIND_KM(k, v) m_vKeyMap[k] = {v, NULL, FALSE, FALSE};

void SXInput::InitKeymap()
{
	for(int i = 0; i < SXI_KEYMAP_SIZE; ++i)
	{
		m_vKeyMap[i] = {"#", NULL, FALSE, FALSE};
	}

	SXI_BIND_KM(SIM_LBUTTON, "mouse1");
	SXI_BIND_KM(SIM_MBUTTON, "mouse2");
	SXI_BIND_KM(SIM_RBUTTON, "mouse3");
	SXI_BIND_KM(SIM_XBUTTON1, "x1");
	SXI_BIND_KM(SIM_XBUTTON2, "x2");
	SXI_BIND_KM(SIM_MWHEELUP, "mwheelup");
	SXI_BIND_KM(SIM_MWHEELDOWN, "mwheeldown");

	SXI_BIND_KM(SIK_ESCAPE, "escape");
	SXI_BIND_KM(SIK_BACKSPACE, "backspace");
	SXI_BIND_KM(SIK_TAB, "tab");
	SXI_BIND_KM(SIK_ENTER, "enter");
	SXI_BIND_KM(SIK_NUMPADENTER, "num_enter");
	SXI_BIND_KM(SIK_SPACE, "space");

	SXI_BIND_KM(SIK_SHIFT, "shift");
	SXI_BIND_KM(SIK_CONTROL, "ctrl");
	SXI_BIND_KM(SIK_ALT, "alt");

	SXI_BIND_KM(SIK_LSHIFT, "lshift");
	SXI_BIND_KM(SIK_LCONTROL, "lctrl");
	SXI_BIND_KM(SIK_LALT, "lalt");
	SXI_BIND_KM(SIK_LWIN, "lwin");
	SXI_BIND_KM(SIK_RSHIFT, "rshift");
	SXI_BIND_KM(SIK_RCONTROL, "rctrl");
	SXI_BIND_KM(SIK_RALT, "ralt");
	SXI_BIND_KM(SIK_RWIN, "rwin");

	SXI_BIND_KM(SIK_APPS, "menu");
	

	SXI_BIND_KM(SIK_PAUSE, "pause");
	SXI_BIND_KM(SIK_CAPSLOCK, "capslock");
	SXI_BIND_KM(SIK_NUMLOCK, "numlock");
	SXI_BIND_KM(SIK_SCROLLLOCK, "scrolllock");

	SXI_BIND_KM(SIK_PGUP, "pageup");
	SXI_BIND_KM(SIK_PGDOWN, "pagedown");
	SXI_BIND_KM(SIK_HOME, "home");
	SXI_BIND_KM(SIK_END, "end");
	SXI_BIND_KM(SIK_INSERT, "insert");
	SXI_BIND_KM(SIK_DELETE, "delete");

	SXI_BIND_KM(SIK_LEFT, "left");
	SXI_BIND_KM(SIK_UP, "up");
	SXI_BIND_KM(SIK_RIGHT, "right");
	SXI_BIND_KM(SIK_DOWN, "down");

	SXI_BIND_KM(SIK_0, "0");
	SXI_BIND_KM(SIK_1, "1");
	SXI_BIND_KM(SIK_2, "2");
	SXI_BIND_KM(SIK_3, "3");
	SXI_BIND_KM(SIK_4, "4");
	SXI_BIND_KM(SIK_5, "5");
	SXI_BIND_KM(SIK_6, "6");
	SXI_BIND_KM(SIK_7, "7");
	SXI_BIND_KM(SIK_8, "8");
	SXI_BIND_KM(SIK_9, "9");

	SXI_BIND_KM(SIK_A, "a");
	SXI_BIND_KM(SIK_B, "b");
	SXI_BIND_KM(SIK_C, "c");
	SXI_BIND_KM(SIK_D, "d");
	SXI_BIND_KM(SIK_E, "e");
	SXI_BIND_KM(SIK_F, "f");
	SXI_BIND_KM(SIK_G, "g");
	SXI_BIND_KM(SIK_H, "h");
	SXI_BIND_KM(SIK_I, "i");
	SXI_BIND_KM(SIK_J, "j");
	SXI_BIND_KM(SIK_K, "k");
	SXI_BIND_KM(SIK_L, "l");
	SXI_BIND_KM(SIK_M, "m");
	SXI_BIND_KM(SIK_N, "n");
	SXI_BIND_KM(SIK_O, "o");
	SXI_BIND_KM(SIK_P, "p");
	SXI_BIND_KM(SIK_Q, "q");
	SXI_BIND_KM(SIK_R, "r");
	SXI_BIND_KM(SIK_S, "s");
	SXI_BIND_KM(SIK_T, "t");
	SXI_BIND_KM(SIK_U, "u");
	SXI_BIND_KM(SIK_V, "v");
	SXI_BIND_KM(SIK_W, "w");
	SXI_BIND_KM(SIK_X, "x");
	SXI_BIND_KM(SIK_Y, "y");
	SXI_BIND_KM(SIK_Z, "z");

	SXI_BIND_KM(SIK_GRAVE, "~");
	SXI_BIND_KM(SIK_MINUS, "-");
	SXI_BIND_KM(SIK_EQUALS, "=");
	SXI_BIND_KM(SIK_BACKSLASH, "\\");
	SXI_BIND_KM(SIK_LBRACKET, "[");
	SXI_BIND_KM(SIK_RBRACKET, "]");
	SXI_BIND_KM(SIK_SEMICOLON, ";");
	SXI_BIND_KM(SIK_APOSTROPHE, "'");
	SXI_BIND_KM(SIK_COMMA, ",");
	SXI_BIND_KM(SIK_PERIOD, ".");
	SXI_BIND_KM(SIK_SLASH, "/");

	SXI_BIND_KM(SIK_NUMPAD0, "num_0");
	SXI_BIND_KM(SIK_NUMPAD1, "num_1");
	SXI_BIND_KM(SIK_NUMPAD2, "num_2");
	SXI_BIND_KM(SIK_NUMPAD3, "num_3");
	SXI_BIND_KM(SIK_NUMPAD4, "num_4");
	SXI_BIND_KM(SIK_NUMPAD5, "num_5");
	SXI_BIND_KM(SIK_NUMPAD6, "num_6");
	SXI_BIND_KM(SIK_NUMPAD7, "num_7");
	SXI_BIND_KM(SIK_NUMPAD8, "num_8");
	SXI_BIND_KM(SIK_NUMPAD9, "num_9");

	SXI_BIND_KM(SIK_MULTIPLY, "num_*");
	SXI_BIND_KM(SIK_DIVIDE, "num_/");
	SXI_BIND_KM(SIK_ADD, "num_+");
	SXI_BIND_KM(SIK_SUBTRACT, "num_-");
	SXI_BIND_KM(SIK_DECIMAL, "num_.");

	SXI_BIND_KM(SIK_F1, "f1");
	SXI_BIND_KM(SIK_F2, "f2");
	SXI_BIND_KM(SIK_F3, "f3");
	SXI_BIND_KM(SIK_F4, "f4");
	SXI_BIND_KM(SIK_F5, "f5");
	SXI_BIND_KM(SIK_F6, "f6");
	SXI_BIND_KM(SIK_F7, "f7");
	SXI_BIND_KM(SIK_F8, "f8");
	SXI_BIND_KM(SIK_F9, "f9");
	SXI_BIND_KM(SIK_F10, "f10");
	SXI_BIND_KM(SIK_F11, "f11");
	SXI_BIND_KM(SIK_F12, "f12");
}

#undef DSI_BIND_KM

void SXInput::Bind(const char * key, const char * cmd)
{
	for(int i = 0; i < SXI_KEYMAP_SIZE; ++i)
	{
		if(!fstrcmp(m_vKeyMap[i].bind, key))
		{
			int len = (int)strlen(cmd);
			if(!m_vKeyMap[i].cmd)
			{
				if(len)
				{
					m_vKeyMap[i].cmd = new char[len + 1];
					strcpy(m_vKeyMap[i].cmd, cmd);
				}
			}
			else
			{
				if(len)
				{
					if(len <= strlen(m_vKeyMap[i].cmd))
					{
						strcpy(m_vKeyMap[i].cmd, cmd);
					}
					else
					{
						mem_delete_a(m_vKeyMap[i].cmd);
						m_vKeyMap[i].cmd = new char[len + 1];
						strcpy(m_vKeyMap[i].cmd, cmd);
					}
				}
				else
				{
					mem_delete_a(m_vKeyMap[i].cmd);
				}
			}
			return;
		}
	}
}

void SXInput::QueueMsg(const IMSG & msg)
{
	if(!m_bEnabled)
	{
		return;
	}
	switch(msg.message)
	{
	case WM_INPUT:
		RAWINPUT raw;
		UINT dwSize;
		dwSize = sizeof(raw);
		if(~0 != GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)))
		{
			if(raw.header.dwType == RIM_TYPEMOUSE)
			{
				const RAWMOUSE &rawMouse = raw.data.mouse;
				if(rawMouse.usFlags & MOUSE_MOVE_ABSOLUTE)
				{
					bool isVirtualDesktop = rawMouse.usFlags & MOUSE_VIRTUAL_DESKTOP;

					int width = GetSystemMetrics(isVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
					int height = GetSystemMetrics(isVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

					int absoluteX = int((rawMouse.lLastX / 65535.0f) * width);
					int absoluteY = int((rawMouse.lLastY / 65535.0f) * height);

					static int s_iLastX = 0, s_iLastY = 0;
					if(m_isFirstRIM)
					{
						m_isFirstRIM = false;
					}
					else
					{
						RECT rc;
						GetClientRect(GetForegroundWindow(), &rc);
						
						rc.right = (rc.right - rc.left) / 2;
						rc.bottom = (rc.bottom - rc.top) / 2;

						mdelta.x += absoluteX - s_iLastX;
						mdelta.y += absoluteY - s_iLastY;

						if(mdelta.x > rc.right || mdelta.x < -rc.right || mdelta.y > rc.bottom || mdelta.y < -rc.bottom)
						{
							mdelta.x = 0;
							mdelta.y = 0;
						}
					}
					s_iLastX = absoluteX;
					s_iLastY = absoluteY;
				}
				else if(rawMouse.lLastX != 0 || rawMouse.lLastY != 0)
				{
					mdelta.x += rawMouse.lLastX;
					mdelta.y += rawMouse.lLastY;
				}
			}
		}
		return;
	}

	m_qMsgs.push(msg);
}

void SXInput::getBindEntry(int n, const char **pszKey, const char **pszCmd)
{
	assert(n >= 0 && n < SXI_KEYMAP_SIZE);
	if(pszKey)
	{
		*pszKey = m_vKeyMap[n].bind;
	}
	if(pszCmd)
	{
		*pszCmd = m_vKeyMap[n].cmd;
	}
}

void SXInput::onNextKeyPress(void(*pfnCallback)(const char *szKey))
{
	m_pfnCallback = pfnCallback;
	m_bDisableOnNextKeyPress = !m_bEnabled;
	if(!m_bEnabled)
	{
		setEnable(true);
	}
}

void SXInput::execOnNextKeyPress(const char *szKey)
{
	m_pfnCallback(szKey);
	m_pfnCallback = NULL;
	if(m_bDisableOnNextKeyPress)
	{
		setEnable(false);
	}
}
