#ifndef _IRenderFrame_H_
#define _IRenderFrame_H_

#include "GUIbase.h"
#include "DOM.h"
#include "ICSS.h"
#include "Font.h"

#include "IRenderBorder.h"

namespace gui
{
	namespace dom
	{
		//class IFont;
		namespace render
		{
			class IScrollBar;

			class IRenderFrame
			{
				friend class CDOMnode;
			public:
				IRenderFrame(CDOMnode *pNode, IRenderFrame *pRootNode);
				virtual ~IRenderFrame();

				virtual UINT layout(bool changed=true);
				virtual void render(UINT lvl, float fDT);

				virtual void onCreated();

				struct CRenderElement
				{
					CRenderElement():m_pNextREl(NULL)
					{
					}
					StringW m_szStr;
				//	IGXVertexBuffer * m_pVertexBuffer;
					IGXRenderBuffer *m_pRenderBuffer;
					IGXIndexBuffer * m_pIndexBuffer;
					UINT m_iVertexCount;
					UINT m_iIndexBaseCount;
					UINT m_iIndexAddCount;
					UINT m_iTopOffset;
					UINT m_iLeftOffset;
					UINT m_iHeight;
					UINT m_iWIdth;

					int m_iLineIdx;

					UINT m_iFirstSym;
					UINT m_iLastSym;
					Array<RECT> *m_prc;

					CRenderElement *m_pNextREl;
				};

				/*enum CUT_TYPE
				{
				CUT_NONE,
				CUT_BORDER,
				CUT_CONTENT
				};*/

				IRenderFrame* getParent();
				IRenderFrame* getNextSibling();
				IRenderFrame* getPrevSibling();

				IRenderFrame* m_pRootNode;

				UINT getChildCount();
				IRenderFrame* getChild(UINT id);
				CDOMdocument* m_pDoc = NULL;

				css::ICSSstyle* getComputedStyle();

				bool m_bIsOutOfFlow = false;

				UINT getWidth();
				UINT getContentWidth();
				UINT getContentHeight();
				UINT getHeight();
				int getXpos();
				void setXpos(int x);
				int getYpos();

				UINT getInnerWidth();
				UINT getInnerHeight();

				void addChild(IRenderFrame *pChild, bool bOutFlow = false);
				void setParent(IRenderFrame *pParent);

				static IRenderFrame* createNode(CDOMnode *pNode, IRenderFrame *pRootNode);

				void debugPrint(UINT lvl = 0);

				void setWidth(UINT width);
				void setHeight(UINT width);

				UINT getTopPos();
				UINT getLeftPos();
				void setLeftPos(UINT p);
				void setTopPos(UINT p);
				UINT getTopPosMax();
				void setTopPosMax(UINT p);

				UINT getTopOffset();
				UINT getLeftOffset();

				UINT getClientTop();
				UINT getClientLeft();

				CDOMnode* getNode();

				UINT getContentTop();
				UINT getContentLeft();

				UINT getInnerTop();
				UINT getInnerLeft();

				void textStart();
				void textClear();
				void textBreak();
				void textAppend(CRenderElement *rel, int iLineIdx);
				int textGetLineIdx();
				UINT textFixUp();

				void updateStyles();

				void updateBorderColor();

				void resetLayout(bool first = true, bool bForce = false);

				RECT getClientRect();
				RECT getVisibleRect(bool *puseForceCrop = NULL, RECT *prcForceCrop = NULL);

				int getScrollLeft();
				int getScrollLeftMax();
				void setScrollLeft(int x);

				int getScrollTop();
				int getScrollTopMax();
				void setScrollTop(int x, bool _check_bounds = false);

				void setScrollSpeedX(int x);
				void setScrollSpeedY(int x);
				void updateScroll(float fDT);

				void findElementByXY(int x, int y, IDOMnode **ppNode, bool sendEnterLeave = false);

				bool inRect(const RECT &rc, int x, int y);

				bool hasFixedSize()
				{
					return(m_bHasFixedSize || isFreezzed());
				}

				bool isOutOfFlow()
				{
					return(m_bIsOutOfFlow);
				}

				void isOutOfFlow(bool is)
				{
					m_bIsOutOfFlow = is;
				}

				bool isChildOf(IRenderFrame *pNode)
				{
					IRenderFrame *pCur = getParent();
					while(pCur)
					{
						if(pCur == pNode)
						{
							return(true);
						}
						pCur = pCur->getParent();
					}
					return(false);
				}

				void setDocument(CDOMdocument *doc)
				{
					m_pDoc = doc;
				}

				bool isLastChild()
				{
					if(!m_pParent)
					{
						return(true);
					}
					return(m_pParent->getChild(m_pParent->getChildCount() - 1) == this);
				}

				void freeze(bool dof = true)
				{
					m_bFreezed = dof;
				}

				bool isFreezzed()
				{
					return(m_bFreezed);
				}

				void removeChild(IRenderFrame *prf);

			protected:

				struct pointtex
				{
					float3_t Pos;
					float2_t Tex;
					//DWORD color;
				};

				bool m_bFreezed = false;

				pointtex m_pVBackground[16];
				int m_iTCBackground;

				void updateBorder();
				void updateBackground();

				void renderBackground(UINT lvl);

				bool m_bHasBackground = false;
				bool m_bHasBackgroundImage;

				float4_t m_pBackgroundColor;
				UINT m_iBackgroundColor;

				int m_iBackgroundOffsetX = 0;
				int m_iBackgroundOffsetY = 0;

				IXTexture *m_pBackgroundImage = NULL;

				CDOMnode *m_pNode;
				css::CCSSstyle m_ComputedStyle;

				IRenderFrame *m_pParent = NULL;
				IRenderFrame *m_pNext = NULL;
				IRenderFrame *m_pPrev = NULL;
				Array<IRenderFrame*> m_pChilds;
				Array<IRenderFrame*> m_pChildsOutFlow;

				int m_iXpos = 0;
				int m_iYpos = 0;
				UINT m_iWidth = 0;
				UINT m_iHeight = 0;

				UINT m_iTopPos = 0;
				UINT m_iTopPosMax = 0;
				UINT m_iLeftPos = 0;

				bool isWidthSet = true;

				int m_iScrollTop = 0;
				int m_iScrollLeft = 0;
				int m_iScrollTopMax = 0;
				int m_iScrollLeftMax = 0;
				float m_fScrollSpeedX = 0.0f;
				float m_fScrollSpeedY = 0.0f;

				bool m_bBackgroundRepeatX = true;
				bool m_bBackgroundRepeatY = true;

				bool m_bBackgroundScrolling;
				bool m_bBackgroundFixed;

				bool m_bHasFixedSize = false;
				bool m_bIsRectValid;

				float2_t m_fBackgroundImageSize;

				bool m_bNeedCut = false;

				IRenderBorder m_border;

				IScrollBar *m_pScrollBarVert = NULL;
				IScrollBar *m_pScrollBarHorz = NULL;

				DWORD rc;

				UINT m_iTextIdx = 0;
				Array<Array<CRenderElement*>> m_mTextRELs;
				
				GXSamplerDesc m_samplerDesc;
				IGXSamplerState *m_pSamplerState = NULL;
			};

			class IRenderBlock: public IRenderFrame
			{
				DECLARE_CLASS(IRenderBlock, IRenderFrame);
			public:
				IRenderBlock(CDOMnode *pNode, IRenderFrame *pRootNode);
				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;
			};

			class IRenderAnonymousBlock: public IRenderFrame
			{
				DECLARE_CLASS(IRenderAnonymousBlock, IRenderFrame);
			public:
				IRenderAnonymousBlock(IRenderFrame * pRootNode);
				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;
			};


			class IRenderInlineBlock: public IRenderBlock
			{
				DECLARE_CLASS(IRenderInlineBlock, IRenderBlock);
			public:
				IRenderInlineBlock(CDOMnode * pNode, IRenderFrame * pRootNode);
				UINT layout(bool changed = true) override;
				//void render(UINT lvl);
			};

			class IRenderInline: public IRenderFrame
			{
				DECLARE_CLASS(IRenderInline, IRenderFrame);
			public:
				IRenderInline(CDOMnode * pNode, IRenderFrame * pRootNode);
				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;
			};

#if 0

			class IRenderText: public IRenderFrame
			{
				DECLARE_CLASS(IRenderText, IRenderFrame);
			public:
				IRenderText(CDOMnode * pNode, IRenderFrame * pRootNode);
				UINT layout(bool changed = true);
				void render(UINT lvl);

			protected:
				IDirect3DVertexBuffer9 * m_pVertexBuffer;
				IDirect3DIndexBuffer9 * m_pIndexBuffer;
				UINT m_iVertexCount;
				UINT m_iIndexBaseCount;
				UINT m_iIndexAddCount;

				StringW m_szClearText;
				UINT m_iWordCount;

				CFont * pFont;

				void setText(const StringW & text);
			};

#endif

			class IRenderTextNew: public IRenderFrame
			{
				DECLARE_CLASS(IRenderTextNew, IRenderFrame);
			public:
				IRenderTextNew(CDOMnode *pNode, IRenderFrame *pRootNode):
					BaseClass(pNode, pRootNode)
				{
				}

				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;

				UINT getCaretPos();
				void setCaretPos(int cp, bool force = false);
				void moveCaretPos(int shift);
				void moveCaretLine(int shift);
				UINT getCaretMaxPos();


				void placeCaretByXY(int x, int y);
				void selectionStart();
				void selectionEnd();
				StringW getSelectionText();

				UINT getSelectionStart();
				UINT getSelectionEnd();

				void setSelectionStart(int cp);
				void setSelectionEnd(int cp);

			protected:

				Array<CRenderElement> m_pRenderElems;
				//Array<CRenderElement> m_pRenderElemsShadow;
				StringW m_szClearText;
				UINT m_iWordCount = 0;

				Array<RECT> m_vCharRects;

				float4_t m_pColor;

				css::ICSSstyle *m_pStyle = NULL;

				CFont *pFont = NULL;
				CFont *pShadowFont = NULL;

				void setText(const StringW & text);

				UINT m_iCaretPos = 0;
				UINT m_iSelectionStart = 0;
				UINT m_iSelectionEnd = 0;
				void drawCaret();
				void renderSelection();
				bool m_bInSelection = false;

				int m_iCaretX = 0;
				int m_iCaretY = 0;
				int m_iTextSize = 0;
			};

			class IRenderImageBlock: public IRenderBlock
			{
				DECLARE_CLASS(IRenderImageBlock, IRenderBlock);
			public:
				IRenderImageBlock(CDOMnode * pNode, IRenderFrame * pRootNode);
				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;
			};

			class IRenderImageInlineBlock: public IRenderInlineBlock
			{
				DECLARE_CLASS(IRenderImageInlineBlock, IRenderInlineBlock);
			public:
				IRenderImageInlineBlock(CDOMnode *pNode, IRenderFrame *pRootNode);
				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;
			};

			class IRenderSelectBlock: public IRenderBlock
			{
				DECLARE_CLASS(IRenderSelectBlock, IRenderBlock);
			public:
				IRenderSelectBlock(CDOMnode *pNode, IRenderFrame *pRootNode):
					BaseClass(pNode, pRootNode)
				{
				}

				UINT layout(bool changed = true) override;

				void onCreated() override;

			protected:
				IRenderFrame *m_pOptionsFrame = NULL;
			};

			class IRenderSelectOptionsBlock: public IRenderBlock
			{
				DECLARE_CLASS(IRenderSelectOptionsBlock, IRenderBlock);
			public:
				IRenderSelectOptionsBlock(IRenderFrame * pSelectFrame, IRenderFrame * pRootNode);

				UINT layout(bool changed = true) override;
				void render(UINT lvl, float fDT) override;

			protected:
				IRenderFrame *m_pSelectFrame;
			};
		};
	};
};

#endif
