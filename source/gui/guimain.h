#ifndef _GUIMAIN_H_
#define _GUIMAIN_H_

#include <graphix/graphix.h>
//#include <Windows.h>
#include <common/array.h>
#include <common/MemAlloc.h>
#include <common/stack.h>
#include <common/assotiativearray.h>
#include <common/string.h>
#include <common/Math.h>
#include <gdefines.h>
#include <xcommon/IFileSystem.h>
#include <xcommon/render/IXRender.h>
#include <mtrl/IXMaterialSystem.h>

#include "IFont.h"

#ifndef GUI_DLL
#	include "IDesktop.h"
#endif

namespace gui
{
	class IEvent;
	class IDesktop;
	typedef void(*GUI_CALLBACK)(IEvent *ev);
	typedef void(*GUI_CALLBACK_WC)(const WCHAR *cb_name, IEvent *ev);

	class IDesktopStack: public IXUnknown
	{
	public:

		//############################
		//#      Базовые методы      #
		//############################

		/** Обработка ввода
		Вызывается из цикла обработки сообщений окна
		*/
		virtual BOOL putMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;

		/**	Отрисовка
		Должно быть вызвано только в контексте потока рендера
		*/
		virtual void render() = 0;
		
		//############################
		//#        Методы GUI        #
		//############################

		//Создание Desktop'а
		virtual IDesktop* createDesktopA(const char *name, const char *file) = 0;
		virtual IDesktop* createDesktopW(const WCHAR *name, const WCHAR *file) = 0;

		//Установка активного Desktop'а
		virtual void setActiveDesktop(IDesktop *d, BOOL clearStack = TRUE) = 0;
		virtual void setActiveDesktopA(const char *name) = 0;
		virtual void setActiveDesktopW(const WCHAR *name) = 0;

		//Получить текущий активный Desktop
		virtual IDesktop* getActiveDesktop() = 0;

		//Получить Desktop по имени
		virtual IDesktop* getDesktopA(const char *name) = 0;
		virtual IDesktop* getDesktopW(const WCHAR *name) = 0;

		//Регистрация обработчика
		virtual void registerCallback(const char *cbName, GUI_CALLBACK cb, void *pUserData = NULL) = 0;

		//Регистрация обработчика по умолчанию
		virtual void registerCallbackDefault(GUI_CALLBACK_WC cb, BOOL greedy = FALSE, void *pUserData = NULL) = 0;

		virtual GUI_CALLBACK getCallbackByName(const char *cbName) = 0;
		virtual GUI_CALLBACK getCallbackByName(const StringW &cbName) = 0;


		virtual void messageBox(const WCHAR *title, const WCHAR *text, ...) = 0;
		virtual void pushDesktop(IDesktop *dp) = 0;
		virtual IDesktop* popDesktop() = 0;

		virtual IFont* getFont(const WCHAR *szName, UINT size, IFont::STYLE style, int iBlurRadius) = 0;
	};

	class IGUI: public IXUnknown
	{
	public:
		virtual IDesktopStack* newDesktopStack(const char *szResPath, UINT uWidth, UINT uHeight) = 0;
	};

	typedef IGUI*(*PFNINITINSTANCE)(IXRender*, IXMaterialSystem*, IFileSystem*);
};

#ifdef SX_STATIC_BUILD
gui::IGUI * GUI_InitInstance(IGXContext *pDev, IXMaterialSystem *pMaterialSystem, IFileSystem *pFileSystem);
#endif

#endif
