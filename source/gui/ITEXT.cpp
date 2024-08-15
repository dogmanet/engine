#include "GUIbase.h"
#include "ITEXT.h"

#include "IRenderFrame.h"


namespace gui
{
	namespace dom
	{
		DEFINE_DOM_NODE(IDOMnodeText);
		LINK_DOM_NODE_TO_TAG(IDOMnodeText, L"text");

		void IDOMnodeText::debugPrintMe(UINT lvl)
		{
			wprintf(L"%s\n", m_wsText.c_str());
		}


		void IDOMnodeText::insertChar(UINT ch)
		{
			render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
			m_wsText.insert(t->getCaretPos(), (WCHAR)ch);
			t->setCaretPos(t->getCaretPos() + 1, true);
		}

		void IDOMnodeText::deleteChar(bool back)
		{
			render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
			m_wsText.remove(t->getCaretPos() + (back ? -1 : 0), 1);
			t->setCaretPos(t->getCaretPos() + (back ? -1 : 0));
		}

		void IDOMnodeText::fromClipboard()
		{
			OpenClipboard(NULL);
			HANDLE h = GetClipboardData(CF_UNICODETEXT);
			WCHAR* s = (WCHAR*)GlobalLock(h);
			if(s)
			{
				render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
				m_wsText.insert(t->getCaretPos(), s);
				t->setCaretPos(t->getCaretPos() + (int)wcslen(s), true);
			}
			GlobalUnlock(h);
			CloseClipboard();
		}

		void IDOMnodeText::toClipboard(bool cut)
		{
			render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
			if(t->getSelectionEnd() == t->getSelectionStart())
			{
				return;
			}
			if(OpenClipboard(0))
			{
				EmptyClipboard();
				StringW str = t->getSelectionText();

				HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (str.length() + 1) * sizeof(WCHAR));
				if(hglbCopy == NULL)
				{
					CloseClipboard();
					return;
				}
				WCHAR * lptstrCopy = (WCHAR*)GlobalLock(hglbCopy);
				memcpy(lptstrCopy, str.c_str(), str.length() * sizeof(WCHAR));
				lptstrCopy[str.length()] = 0;
				GlobalUnlock(hglbCopy);
				SetClipboardData(CF_UNICODETEXT, hglbCopy);
				CloseClipboard();

				GlobalFree(hglbCopy);
				//	GlobalFree(lptstrCopy);

				if(cut)
				{
					deleteSelection();
				}
			}
		}
		void IDOMnodeText::deleteSelection()
		{
			render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
			if(t->getSelectionEnd() == t->getSelectionStart())
			{
				return;
			}
			m_wsText.remove(t->getSelectionStart(), t->getSelectionEnd() - t->getSelectionStart());
			t->setCaretPos(t->getSelectionStart(), true);
		}

		bool IDOMnodeText::hasSelection()
		{
			render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
			return(t->getSelectionEnd() != t->getSelectionStart());
		}


		void IDOMnodeText::selectAll()
		{
			render::IRenderTextNew * t = (render::IRenderTextNew*)m_pRenderFrame;
			t->setSelectionStart(0);
			t->setSelectionEnd(t->getCaretMaxPos());
		}
	};
};
