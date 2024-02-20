#ifndef _IEvent_H_
#define _IEvent_H_

#ifdef GUI_DLL
#include "GUIbase.h"
#else
#include "guimain.h"
#endif
#include "keymap.h"

namespace gui
{
	namespace dom
	{
		class IDOMnode;
	};

	enum GUI_EVENT_TYPE
	{
		GUI_EVENT_TYPE_UNKNOWN = 0,
		GUI_EVENT_TYPE_MOUSEDOWN,
		GUI_EVENT_TYPE_MOUSEUP,
		GUI_EVENT_TYPE_CLICK,
		GUI_EVENT_TYPE_DBLCLICK,
		GUI_EVENT_TYPE_MOUSEENTER,
		GUI_EVENT_TYPE_MOUSELEAVE,
		GUI_EVENT_TYPE_BLUR,
		GUI_EVENT_TYPE_FOCUS,
		GUI_EVENT_TYPE_KEYDOWN,
		GUI_EVENT_TYPE_KEYUP,
		GUI_EVENT_TYPE_KEYPRESS,
		GUI_EVENT_TYPE_MOUSEMOVE,
		GUI_EVENT_TYPE_MOUSEWHEELDOWN,
		GUI_EVENT_TYPE_MOUSEWHEELUP,
		GUI_EVENT_TYPE_CHANGE
	};

	class IEvent
	{
	public:
		void stopPropagation() // cancel bubble
		{
			propagate = false;
		}
		//protected:
		GUI_EVENT_TYPE type = GUI_EVENT_TYPE_UNKNOWN;
		bool propagate = true;
		UINT key = 0;
		bool syskey = false;
		bool preventDefault = false;

		dom::IDOMnode *target = NULL;
		dom::IDOMnode *relatedTarget = NULL;
		dom::IDOMnode *currentTarget = NULL;

		int clientX = 0;
		int clientY = 0;
		int offsetX = 0;
		int offsetY = 0;

		void *pCallbackData = NULL;
	};
};

#endif
