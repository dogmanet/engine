#include "GUIbase.h"
#include "IRenderFrame.h"
#include "ITEXT.h"
#include "Font.h"
#include "IVIDEO.h"
#include "DOMdocument.h"
#include "IScrollBar.h"
#include "ScrollBarSimple.h"


namespace gui
{
	void CTranslationManager::pushMatrix(const SMMATRIX &m)
	{
		m_stack.push(m_result);
		float fDet;
		m_result *= SMMatrixInverse(&fDet, m_result) * m * m_result;
		//	GetGUI()->getDevice()->SetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&m_result));
		m_pDesktopStack->setTransformWorld(m_result);
		m_pDesktopStack->updateTransformShader();
	}

	void CTranslationManager::popMatrix()
	{
		m_stack.pop(&m_result);
		//float det;
		//m_result = m_result * SMMatrixInverse(&det, m);
		//	GetGUI()->getDevice()->SetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&m_result));
		m_pDesktopStack->setTransformWorld(m_result);
		m_pDesktopStack->updateTransformShader();
	}

	SMMATRIX CTranslationManager::getCurrentMatrix()
	{
		return(m_result);
	}

//##########################################################################

	namespace dom
	{


		namespace render
		{

			IRenderFrame::IRenderFrame(CDOMnode *pNode, IRenderFrame *pRootNode):
				m_border((pRootNode ? pRootNode->getNode() : pNode)->getDocument()->getDesktopStack()->getTextureManager())
			{
				m_pNode = pNode;
				if(!pRootNode)
				{
					pRootNode = this;
				}
				m_pRootNode = pRootNode;
				m_pDoc = m_pRootNode->getNode()->getDocument();
				if(!pNode)
				{
					return;
				}
						
				m_pNode->setRenderFrame(this);
				const IDOMnodeCollection * pChilds = pNode->getChilds();
				bool bNeedBoxWrap = false;

				if(pChilds->size() == 0)
				{
					if(pNode->getNodeId() == CDOMnode::getNodeIdByName(L"textarea")
						|| pNode->getNodeId() == CDOMnode::getNodeIdByName(L"input")
						//|| pNode->getNodeId() == IDOMnode::getNodeIdByName(L"password")
						)
					{
						CDOMnode * newNode = (CDOMnode*)CDOMnode::createNode(L"text");
						newNode->setDocument(pNode->getDocument());
						pNode->appendChild(newNode, false);
					}
				}

				for(UINT i = 0; i < pChilds->size(); i++)
				{
					css::ICSSstyle * css = (*pChilds)[i]->getStyle();
					if(css->display->getInt() == css::ICSSproperty::DISPLAY_BLOCK)
					{
						bNeedBoxWrap = true;
						break;
					}
				}
				IRenderFrame * pWrapper = NULL;
				for(UINT i = 0; i < pChilds->size(); i++)
				{
					css::ICSSstyle * css = (*pChilds)[i]->getStyle();

					if(bNeedBoxWrap)
					{
						if(css->display->getInt() == css::ICSSproperty::DISPLAY_BLOCK)
						{
							pWrapper = NULL;
						}
						else if(!pWrapper && (!css->display->isSet() || css->display->getInt() != css::ICSSproperty::DISPLAY_NONE))
						{
							pWrapper = IRenderFrame::createNode(NULL, pRootNode);
							addChild(pWrapper);
						}
					}

					IRenderFrame * node = IRenderFrame::createNode((CDOMnode*)(*pChilds)[i], pRootNode);
					if(node)
					{
						if(css->position->getInt() == css::ICSSproperty::POSITION_FIXED)
						{
							pRootNode->addChild(node, true);
						}
						else if(css->position->getInt() == css::ICSSproperty::POSITION_ABSOLUTE)
						{
							IRenderFrame * pCur = this;
							while(pCur->m_pParent && pCur->getNode()->getStyle()->position->getInt() == css::ICSSproperty::POSITION_STATIC)
							{
								pCur = pCur->m_pParent;
							}
							pCur->addChild(node, true);
						}
						else
						{
							if(pWrapper)
							{
								pWrapper->addChild(node);
							}
							else
							{
								addChild(node);
							}
						}
					}

				}

				updateBorderColor();

				rc = 0x11000000 | ((rand() & 0xFF) << 16) | ((rand() & 0xFF) << 8) | (rand() & 0xFF);
				rc = 0xFFCCCCCC;
			}

			void IRenderFrame::resetLayout(bool first, bool bForce)
			{
				if(bForce)
				{
					m_bHasFixedSize = false;
				}
				if(m_pParent && m_pParent->hasFixedSize()/* && !this->hasFixedSize()*/ && isLastChild())
				{
					if(m_pParent->m_iTopPos <= m_iHeight)
					{
						m_pParent->m_iTopPos = 0;
						m_pParent->m_iScrollTopMax = 0;
					}
					else
					{
						m_pParent->m_iTopPos -= m_iHeight;
						m_pParent->m_iScrollTopMax -= m_iHeight;
					}
				}
				if(!hasFixedSize())
				{
					if(m_pParent)
					{
						m_iWidth = 0;
						m_iHeight = 0;
					}
					m_iXpos = 0;
					m_iYpos = 0;
					m_iBackgroundOffsetX = 0;
					m_iBackgroundOffsetY = 0;
				}
				m_iScrollLeftMax = 0;
				m_iScrollTopMax = 0;
				m_iTopPos = 0;
				m_iLeftPos = 0;
				m_iTopPosMax = 0;

				if(!isFreezzed())
				{
					for(UINT i = 0; i < m_pChilds.size(); i++)
					{
						m_pChilds[i]->resetLayout(false, bForce);
					}
					for(UINT i = 0; i < m_pChildsOutFlow.size(); i++)
					{
						m_pChildsOutFlow[i]->resetLayout(false, bForce);
					}
				}
			}

			void IRenderFrame::updateBorderColor()
			{
				if(m_pNode)
				{
					css::ICSSstyle * style = m_pNode->getStyle();
					if(style->border_top_color->isSet())
					{
						m_border.setColor(IRenderBorder::TOP, style->border_top_color->getColor());
					}
					if(style->border_right_color->isSet())
					{
						m_border.setColor(IRenderBorder::RIGHT, style->border_right_color->getColor());
					}
					if(style->border_bottom_color->isSet())
					{
						m_border.setColor(IRenderBorder::BOTTOM, style->border_bottom_color->getColor());
					}
					if(style->border_left_color->isSet())
					{
						m_border.setColor(IRenderBorder::LEFT, style->border_left_color->getColor());
					}
				}
			}

			IRenderFrame::~IRenderFrame()
			{
				for(UINT i = 0, l = m_pChilds.size(); i < l; i++)
				{
					mem_delete(m_pChilds[i]);
					//m_pChilds.erase(0);
				}
				for(UINT i = 0, l = m_pChildsOutFlow.size(); i < l; i++)
				{
					mem_delete(m_pChildsOutFlow[i]);
					//m_pChildsOutFlow.erase(0);
				}
				mem_delete(m_pScrollBarVert);
				mem_delete(m_pScrollBarHorz);

				mem_release(m_pSamplerState);

				mem_release(m_pBackgroundImage);
			}

			UINT IRenderFrame::getTopPosMax()
			{
				return(m_iTopPosMax);
			}
			void IRenderFrame::setTopPosMax(UINT p)
			{
				m_iTopPosMax = p;
			}

			IRenderFrame* IRenderFrame::getParent()
			{
				return(m_pParent);
			}

			IRenderFrame * IRenderFrame::getNextSibling()
			{
				return(m_pNext);
			}
			IRenderFrame * IRenderFrame::getPrevSibling()
			{
				return(m_pPrev);
			}

			CDOMnode * IRenderFrame::getNode()
			{
				return(m_pNode);
			}

			UINT IRenderFrame::getChildCount()
			{
				return(m_pChilds.size());
			}
			IRenderFrame * IRenderFrame::getChild(UINT id)
			{
				if(id < 0 || id >= m_pChilds.size())
				{
					return(NULL);
				}
				return(m_pChilds[id]);
			}

			css::ICSSstyle * IRenderFrame::getComputedStyle()
			{
				return(&m_ComputedStyle);
			}

			UINT IRenderFrame::getWidth()
			{
				return(m_iWidth);
			}
			UINT IRenderFrame::getHeight()
			{
				return(m_iHeight);
			}
			void IRenderFrame::setXpos(int x)
			{
				m_iXpos = x;
			}
			int IRenderFrame::getXpos()
			{
				return(m_iXpos);
			}
			int IRenderFrame::getYpos()
			{
				return(m_iYpos);
			}

			void IRenderFrame::addChild(IRenderFrame * pChild, bool bOutFlow)
			{
				IRenderFrame *pTarget = this;
				if(pChild->getNode())
				{
					css::ICSSstyle *pStyle = pChild->getNode()->getStyle();
					if(pStyle->position->getInt() == css::CCSSproperty::POSITION_FIXED)
					{
						bOutFlow = true;
					}
					else if(pStyle->position->getInt() == css::CCSSproperty::POSITION_ABSOLUTE)
					{
						bOutFlow = true;
						while(pTarget->m_pParent && pTarget->getNode()->getStyle()->position->getInt() == css::CCSSproperty::POSITION_STATIC)
						{
							pTarget = pTarget->m_pParent;
						}
					}
				}
				Array<IRenderFrame*> * pArr = bOutFlow ? &pTarget->m_pChildsOutFlow : &pTarget->m_pChilds;
				pChild->isOutOfFlow(bOutFlow);
				pChild->m_pPrev = pChild->m_pNext = NULL;
				if(pArr->size() > 0)
				{
					IRenderFrame * last = (*pArr)[pArr->size() - 1];
					last->m_pNext = pChild;
					pChild->m_pPrev = last;
				}
				pArr->push_back(pChild);
				pChild->m_pParent = pTarget;
			}

			void IRenderFrame::removeChild(IRenderFrame * pEl)
			{
				for(UINT i = 0, l = m_pChilds.size(); i < l; ++i)
				{
					if(m_pChilds[i] == pEl)
					{
						if(i > 0)
						{
							m_pChilds[i - 1]->m_pNext = m_pChilds[i]->m_pNext;
						}
						if(i + 1 < m_pChilds.size())
						{
							m_pChilds[i + 1]->m_pPrev = m_pChilds[i]->m_pPrev;
						}
						textClear();
						mem_delete(m_pChilds[i]);
						m_pChilds.erase(i);
						return;
					}
				}

				for(UINT i = 0, l = m_pChildsOutFlow.size(); i < l; ++i)
				{
					if(m_pChildsOutFlow[i] == pEl)
					{
						if(i > 0)
						{
							m_pChildsOutFlow[i - 1]->m_pNext = m_pChildsOutFlow[i]->m_pNext;
						}
						if(i + 1 < m_pChildsOutFlow.size())
						{
							m_pChildsOutFlow[i + 1]->m_pPrev = m_pChildsOutFlow[i]->m_pPrev;
						}
						mem_delete(m_pChildsOutFlow[i]);
						m_pChildsOutFlow.erase(i);
						return;
					}
				}
			}

			void IRenderFrame::setParent(IRenderFrame * pParent)
			{
				m_pParent = pParent;
			}

			int IRenderFrame::getScrollTop()
			{
				return(m_iScrollTop);
			}
			int IRenderFrame::getScrollLeft()
			{
				return(m_iScrollLeft);
			}

			int IRenderFrame::getScrollTopMax()
			{
				return(m_iScrollTopMax);
			}
			int IRenderFrame::getScrollLeftMax()
			{
				return(m_iScrollLeftMax);
			}

			void IRenderFrame::setScrollTop(int x, bool _check_bounds)
			{
				if(_check_bounds)
				{
					if(x < 0)
					{
						x = 0;
					}
					else if(x > m_iScrollTopMax)
					{
						x = m_iScrollTopMax;
					}
				}
				m_iScrollTop = x;
				m_pDoc->markDirty();
			}
			void IRenderFrame::setScrollLeft(int x)
			{
				m_iScrollLeft = x;
				m_pDoc->markDirty();
			}

			//m_iScrollSpeedX

			void IRenderFrame::setScrollSpeedX(int x)
			{
				m_fScrollSpeedX = x;
			}
			void IRenderFrame::setScrollSpeedY(int x)
			{
				m_fScrollSpeedY = x;
			}
			void IRenderFrame::updateScroll(float fDT)
			{
				float accell = 25.0 * fDT;
				if(!SMIsZero(m_fScrollSpeedX))
				{
					setScrollLeft(m_iScrollLeft + (int)m_fScrollSpeedX);
					int sign = m_fScrollSpeedX > 0.0f ? 1 : -1;

					m_fScrollSpeedX -= accell * sign;
					if(m_fScrollSpeedX != 0 && (m_fScrollSpeedX > 0 ? 1 : -1) != sign)
					{
						m_fScrollSpeedX = 0;
					}
				}
				if(!SMIsZero(m_fScrollSpeedY))
				{
					setScrollTop(m_iScrollTop + (int)m_fScrollSpeedY, true);
					int sign = m_fScrollSpeedY > 0 ? 1 : -1;

					m_fScrollSpeedY -= accell * sign;
					if(m_fScrollSpeedY != 0 && (m_fScrollSpeedY > 0 ? 1 : -1) != sign)
					{
						m_fScrollSpeedY = 0;
					}
				}

				if(!SMIsZero(m_fScrollSpeedY) || !SMIsZero(m_fScrollSpeedX))
				{
					m_pDoc->markDirty();
				}
			}


			IRenderFrame* IRenderFrame::createNode(CDOMnode * pNode, IRenderFrame * pRootNode)
			{
				if(!pNode)
				{
					return(new IRenderAnonymousBlock(pRootNode));
				}
				static UINT nIMG = CDOMnode::getNodeIdByName(L"img");
				static UINT nVIDEO = CDOMnode::getNodeIdByName(L"video");
				static UINT nSELECT = CDOMnode::getNodeIdByName(L"select");
				static UINT nTEXT = CDOMnode::getNodeIdByName(L"text");
				UINT nid = pNode->getNodeId();
				if(nid == nTEXT) // Text node
				{
					return(new IRenderTextNew(pNode, pRootNode));
				}
				
				if(nid == nSELECT)
				{
					return(new IRenderSelectBlock(pNode, pRootNode));
				}
				
				switch(pNode->getStyle()->display->getInt())
				{
				case css::ICSSproperty::DISPLAY_NONE:
					pNode->setRenderFrame(NULL);
					return(NULL);
				case css::ICSSproperty::DISPLAY_BLOCK:
					if(nid == nIMG || nid == nVIDEO)
					{
						return(new IRenderImageBlock(pNode, pRootNode));
					}
					else
					{
						return(new IRenderBlock(pNode, pRootNode));
					}
				case css::ICSSproperty::DISPLAY_INLINE_BLOCK:
					if(nid == nIMG || nid == nVIDEO)
					{
						return(new IRenderImageInlineBlock(pNode, pRootNode));
					}
					else
					{
						return(new IRenderInlineBlock(pNode, pRootNode));
					}
				case css::ICSSproperty::DISPLAY_INLINE:
					return(new IRenderInline(pNode, pRootNode));
				}
				return(NULL);
			}

			void IRenderFrame::debugPrint(UINT lvl)
			{
				for(UINT i = 0; i < lvl; i++)
				{
					printf(" ");
				}
				printf("%s: %d W: %d H:%d T: %d L:%d\n", m_pNode ? String(CDOMnode::getNodeNameById(m_pNode->getNodeId())).c_str() : "Anonymous", m_bHasFixedSize ? 1: 0 , m_iWidth, m_iHeight, m_iYpos, m_iXpos);
				for(UINT i = 0; i < m_pChilds.size(); i++)
				{
					m_pChilds[i]->debugPrint(lvl + 1);
				}
				for(UINT i = 0; i < m_pChildsOutFlow.size(); i++)
				{
					m_pChildsOutFlow[i]->debugPrint(lvl + 1);
				}
			}

			void IRenderFrame::setWidth(UINT w)
			{
				m_iWidth = w;
			}

			void IRenderFrame::setHeight(UINT h)
			{
				m_iHeight = h;
			}

			UINT IRenderFrame::layout(bool changed)
			{
				textClear();
				UINT h = 0;
				for(UINT i = 0; i < m_pChilds.size(); i++)
				{
					UINT p = m_pChilds[i]->layout();
					h += p;
					m_iTopPos += p;
				}
				if(getNode() && (getNode()->getStyle()->text_align->getInt() == css::ICSSproperty::TEXT_ALIGN_CENTER || getNode()->getStyle()->text_align->getInt() == css::ICSSproperty::TEXT_ALIGN_RIGHT))
				{
					int curY = 0;
					int curXpos = getContentLeft();
					BOOL ctr = getNode()->getStyle()->text_align->getInt() == css::ICSSproperty::TEXT_ALIGN_CENTER;
					for(UINT i = 0; i < m_pChilds.size(); i++)
					{
						if(m_pChilds[i]->getYpos() == curY && m_pChilds[i]->getNode() && m_pChilds[i]->getNode()->getStyle()->display->getInt() == css::ICSSproperty::DISPLAY_INLINE_BLOCK)
						{
							curXpos += m_pChilds[i]->getWidth();
						}
						else
						{
							int xDelta = ctr ? (getContentWidth() - curXpos) / 2 : getContentWidth() - curXpos;
							for(UINT ii = 0; ii < m_pChilds.size(); ii++)
							{
								if(m_pChilds[ii]->getYpos() == curY&&m_pChilds[ii]->getNode() && m_pChilds[ii]->getNode()->getStyle()->display->getInt() == css::ICSSproperty::DISPLAY_INLINE_BLOCK)
								{
									m_pChilds[ii]->setXpos(xDelta + m_pChilds[ii]->getXpos());
								}
							}

							curY = m_pChilds[i]->getYpos();
							curXpos = getContentLeft() + m_pChilds[i]->getWidth();
						}
					}
					int xDelta = ctr ? (getContentWidth() - curXpos) / 2 : getContentWidth() - curXpos;
					for(UINT ii = 0; ii < m_pChilds.size(); ii++)
					{
						if(m_pChilds[ii]->getYpos() == curY && m_pChilds[ii]->getNode() && m_pChilds[ii]->getNode()->getStyle()->display->getInt() == css::ICSSproperty::DISPLAY_INLINE_BLOCK)
						{
							m_pChilds[ii]->setXpos(xDelta + m_pChilds[ii]->getXpos());
						}
					}
				}
				/*{
					if(getNode() && getNode()->getStyle()->text_align->getInt() == css::ICSSproperty::TEXT_ALIGN_CENTER)
					{
						UINT xDelta = (getContentWidth() - m_iXpos) / 2;
						UINT curY = 0;
						for(UINT ii = 0, ll = GetChildCount(); ii < ll; ++ii)
						{
							IRenderFrame * rfcur = GetChild(ii);
							if(rfcur->getYpos() == m_iYpos)
							{
								rfcur->setXpos(rfcur->getXpos() + xDelta);
							}
						}
					}
				}*/
				for(UINT i = 0; i < m_pChildsOutFlow.size(); i++)
				{
					UINT p = m_pChildsOutFlow[i]->layout();
					//h += p;
					//m_iTopPos += p;
				}
				h += textFixUp();
				/*if(h < m_iHeight)
				{
					h = m_iHeight;
				}*/
				if(m_pParent && m_pParent->hasFixedSize() && changed && isLastChild())
				{
					m_pParent->m_iTopPos += h;
					m_pParent->m_iScrollTopMax += h;
				}
				return(h);
			}

			UINT IRenderFrame::getContentLeft()
			{
				if(!m_pNode)
				{
					return(0);
				}
				css::ICSSstyle * pStyle = m_pNode->getStyle();
				return(pStyle->margin_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ pStyle->padding_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ pStyle->border_left_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
			}

			UINT IRenderFrame::getContentTop()
			{
				if(!m_pNode)
				{
					return(getTopPos());
				}
				css::ICSSstyle * pStyle = m_pNode->getStyle();
				return(getTopPos()
					+ pStyle->margin_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ pStyle->padding_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ pStyle->border_top_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
			}

			void IRenderFrame::render(UINT lvl, float fDT)
			{
				//UpdateScroll();
				for(UINT i = 0; i < m_pChilds.size(); i++)
				{
					if(m_pChilds[i]->getNode() && (m_pChilds[i]->getNode()->isTextNode() || m_pChilds[i]->getNode()->getStyle()->display->getInt() == css::ICSSproperty::DISPLAY_INLINE))
					{
						m_pChilds[i]->render(lvl, fDT);
					}
					else
					{
						m_pChilds[i]->updateScroll(fDT);
						RECT rc = m_pChilds[i]->getVisibleRect();
						if(rc.top < rc.bottom)
						{
							m_pChilds[i]->render(lvl, fDT);
						}
					}
				}
				for(UINT i = 0; i < m_pChildsOutFlow.size(); i++)
				{
					m_pChildsOutFlow[i]->updateScroll(fDT);
					m_pChildsOutFlow[i]->render(lvl, fDT);
				}
			}

			UINT IRenderFrame::getTopPos()
			{
				return(m_iTopPos);
			}

			UINT IRenderFrame::getContentWidth()
			{
				css::ICSSstyle * style = m_pNode ? m_pNode->getStyle() : NULL;
				return(m_iWidth
					- (m_pNode ? style->margin_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->margin_right->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->padding_right->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->padding_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_left_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_right_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()) : 0)); // FIXME: Compute the actual content area width
			}

			UINT IRenderFrame::getInnerWidth()
			{
				css::ICSSstyle * style = m_pNode ? m_pNode->getStyle() : NULL;
				return(m_iWidth
					- (m_pNode ? style->margin_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->margin_right->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_left_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_right_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()) : 0)); // FIXME: Compute the actual content area width
			}

			UINT IRenderFrame::getInnerHeight()
			{
				css::ICSSstyle * style = m_pNode ? m_pNode->getStyle() : NULL;
				return(m_iHeight
					- (m_pNode ? style->margin_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->margin_bottom->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_top_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_bottom_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()) : 0)); // FIXME: Compute the actual content area width
			}

			UINT IRenderFrame::getContentHeight()
			{
				css::ICSSstyle * style = m_pNode ? m_pNode->getStyle() : NULL;
				return(m_iHeight
					- (m_pNode ? style->margin_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->margin_bottom->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->padding_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->padding_bottom->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_top_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_bottom_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()) : 0)); // FIXME: Compute the actual content area width
			}

			UINT IRenderFrame::getInnerTop()
			{
				css::ICSSstyle * style = m_pNode ? m_pNode->getStyle() : NULL;
				return(m_pNode ? /*style->margin_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ */style->border_top_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						: 0);
			}
			UINT IRenderFrame::getInnerLeft()
			{
				css::ICSSstyle * style = m_pNode ? m_pNode->getStyle() : NULL;
				return(m_pNode ? /*style->margin_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ */style->border_left_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()) : 0);
			}

			UINT IRenderFrame::getLeftPos()
			{
				return(m_iLeftPos);
			}

			void IRenderFrame::setLeftPos(UINT p)
			{
				m_iLeftPos = p;
			}

			void IRenderFrame::setTopPos(UINT p)
			{
				m_iTopPos = p;
			}

			UINT IRenderFrame::getTopOffset()
			{
				UINT r = m_iYpos;
				if(m_pParent)
				{
					r += m_pParent->getTopOffset();
				}
				return(r);
			}
			UINT IRenderFrame::getLeftOffset()
			{
				UINT r = m_iXpos;
				if(m_pParent)
				{
					r += m_pParent->getLeftOffset();
				}
				return(r);
			}

			UINT IRenderFrame::getClientTop()
			{
				UINT r = m_iYpos - m_iScrollTop;
				if(m_pParent)
				{
					r += m_pParent->getClientTop();
				}
				return(r);
			}
			UINT IRenderFrame::getClientLeft()
			{
				UINT r = m_iXpos - m_iScrollLeft;
				if(m_pParent)
				{
					r += m_pParent->getClientLeft();
				}
				return(r);
			}

			void IRenderFrame::textStart()
			{
			}
			void IRenderFrame::textBreak()
			{
				m_iTextIdx++;
			}
			void IRenderFrame::textAppend(CRenderElement * rel, int iLineIdx)
			{
				m_mTextRELs[iLineIdx].push_back(rel);
			}
			int IRenderFrame::textGetLineIdx()
			{
				return(m_iTextIdx);
			}
			void IRenderFrame::textClear()
			{
				for(UINT i = 0, l = m_mTextRELs.size(); i < l; ++i)
				{
					for(UINT j = 0, jl = m_mTextRELs[i].size(); j < jl; ++j)
					{
						CRenderElement *rel = m_mTextRELs[i][j];
						mem_release(rel->m_pIndexBuffer);
						mem_release(rel->m_pRenderBuffer);
						if(rel->m_pNextREl)
						{
							mem_release(rel->m_pNextREl->m_pIndexBuffer);
							mem_release(rel->m_pNextREl->m_pRenderBuffer);
							mem_delete(rel->m_pNextREl);
						}
					}
				}
				m_mTextRELs.clear();
				m_iTextIdx = 0;
			}
			UINT IRenderFrame::textFixUp()
			{
				UINT h = 0;
				UINT iMaxOffs = 0;
				UINT iMaxSize = 0;
				
				if(m_pNode)
				{
					UINT iMaxWidth = 0;

					for(UINT i = 0; i < m_mTextRELs.size(); i++)
					{
						UINT _iMaxWidth = 0;
						for(UINT j = 0; j < m_mTextRELs[i].size(); j++)
						{
							_iMaxWidth += m_mTextRELs[i][j]->m_iWIdth;
						}
						iMaxWidth = max(_iMaxWidth, iMaxWidth);
					}
					//msvcprtd.lib;LIBCMTD.lib;
					css::ICSSstyle * style = m_pNode->getStyle();
					UINT width = m_pParent ? m_pParent->getWidth() : getWidth();
					UINT iNewWidth = iMaxWidth
					//	+ style->margin_left->getPX(width)
					//	+ style->margin_right->getPX(width)
						+ style->padding_left->getPX(width, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->padding_right->getPX(width, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->border_left_width->getPX(width, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->border_right_width->getPX(width, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					if(!isWidthSet)
					{
						m_iWidth = iNewWidth;
					}
					else
					{
						if(m_iWidth < iNewWidth)
						{
							m_bNeedCut = true;
							m_iScrollLeftMax = iNewWidth - m_iWidth;
							m_iScrollLeft = m_iScrollLeftMax;
						}
					}
				}
				UINT iAreaWidth = getContentWidth();
				css::ICSSstyle * pStyle = m_pNode ? m_pNode->getStyle() : m_pParent->getNode()->getStyle();
				css::ICSSproperty::TYPE_TEXT_ALIGN_e _textAlign = (css::ICSSproperty::TYPE_TEXT_ALIGN_e)pStyle->text_align->getInt();
				bool bAlignCenter = false;
				bool bAlignRight = false;
				if(_textAlign == css::ICSSproperty::TEXT_ALIGN_CENTER)
				{
					bAlignCenter = true;
				}
				if(_textAlign == css::ICSSproperty::TEXT_ALIGN_RIGHT)
				{
					bAlignRight = true;
				}

				for(UINT i = 0; i < m_mTextRELs.size(); i++)
				{
					iMaxOffs = 0;
					iMaxSize = 0;
					if(m_mTextRELs[i].size() > 1)
					{
						
						for(UINT j = 0; j < m_mTextRELs[i].size(); j++)
						{
							iMaxSize = max(iMaxSize, m_mTextRELs[i][j]->m_iHeight);
						}
						for(UINT j = 0; j < m_mTextRELs[i].size(); j++)
						{
							h = max(h, iMaxSize - m_mTextRELs[i][j]->m_iHeight);
							m_mTextRELs[i][j]->m_iTopOffset += iMaxSize - m_mTextRELs[i][j]->m_iHeight;
						}
					}
					else if(m_mTextRELs[i].size() == 1)
					{
						iMaxSize = max(iMaxSize, m_mTextRELs[i][0]->m_iHeight);
					}
					h += iMaxOffs;

					if(bAlignCenter || bAlignRight)
					{
						UINT _w = 0;
						for(UINT j = 0; j < m_mTextRELs[i].size(); j++)
						{
							_w += m_mTextRELs[i][j]->m_iWIdth;
						}
						int _offs = (int)iAreaWidth - (int)_w;;
						if(bAlignCenter)
						{
							_offs /= 2;
						}
						if(_offs > 0)
						{
							for(UINT j = 0; j < m_mTextRELs[i].size(); j++)
							{
								m_mTextRELs[i][j]->m_iLeftOffset += _offs;
								for(UINT o = m_mTextRELs[i][j]->m_iFirstSym; o <= m_mTextRELs[i][j]->m_iLastSym; o++)
								{
									assert((*m_mTextRELs[i][j]->m_prc)[o].left <= 65555);
									(*m_mTextRELs[i][j]->m_prc)[o].left += _offs;
									(*m_mTextRELs[i][j]->m_prc)[o].right += _offs;
								}
							}
						}
					}
				}
			//	if(m_mTextRELs.size() == 1)
				{
					h += iMaxSize;
				}
				return(h);
			}


			void IRenderFrame::updateBorder()
			{
				if(m_pNode)
				{
					css::ICSSstyle * style = m_pNode->getStyle();

					m_border.setRadius(IRenderBorder::TOP_LEFT, 0);
					m_border.setRadius(IRenderBorder::TOP_RIGHT, 0);
					m_border.setRadius(IRenderBorder::BOTTOM_LEFT, 0);
					m_border.setRadius(IRenderBorder::BOTTOM_RIGHT, 0);

					m_border.setCornerMethod(IRenderBorder::TOP_LEFT, IRenderBorder::CORNER_METHOD_RADIUS);
					m_border.setCornerMethod(IRenderBorder::TOP_RIGHT, IRenderBorder::CORNER_METHOD_RADIUS);
					m_border.setCornerMethod(IRenderBorder::BOTTOM_LEFT, IRenderBorder::CORNER_METHOD_RADIUS);
					m_border.setCornerMethod(IRenderBorder::BOTTOM_RIGHT, IRenderBorder::CORNER_METHOD_RADIUS);

					m_border.setWidth(IRenderBorder::TOP, 0);
					m_border.setWidth(IRenderBorder::RIGHT, 0);
					m_border.setWidth(IRenderBorder::BOTTOM, 0);
					m_border.setWidth(IRenderBorder::LEFT, 0);

					if(style->border_top_left_radius->isSet())
					{
						m_border.setRadius(IRenderBorder::TOP_LEFT, style->border_top_left_radius->getPX(0, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					if(style->border_top_right_radius->isSet())
					{
						m_border.setRadius(IRenderBorder::TOP_RIGHT, style->border_top_right_radius->getPX(0, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					if(style->border_bottom_left_radius->isSet())
					{
						m_border.setRadius(IRenderBorder::BOTTOM_LEFT, style->border_bottom_left_radius->getPX(0, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					if(style->border_bottom_right_radius->isSet())
					{
						m_border.setRadius(IRenderBorder::BOTTOM_RIGHT, style->border_bottom_right_radius->getPX(0, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}



					if(style->border_top_left_method->isSet())
					{
						m_border.setCornerMethod(IRenderBorder::TOP_LEFT, style->border_top_left_method->getInt() == css::ICSSproperty::BORDER_METHOD_RADIUS ? IRenderBorder::CORNER_METHOD_RADIUS : IRenderBorder::CORNER_METHOD_CUT);
					}
					if(style->border_top_right_method->isSet())
					{
						m_border.setCornerMethod(IRenderBorder::TOP_RIGHT, style->border_top_right_method->getInt() == css::ICSSproperty::BORDER_METHOD_RADIUS ? IRenderBorder::CORNER_METHOD_RADIUS : IRenderBorder::CORNER_METHOD_CUT);
					}
					if(style->border_bottom_right_method->isSet())
					{
						m_border.setCornerMethod(IRenderBorder::BOTTOM_RIGHT, style->border_bottom_right_method->getInt() == css::ICSSproperty::BORDER_METHOD_RADIUS ? IRenderBorder::CORNER_METHOD_RADIUS : IRenderBorder::CORNER_METHOD_CUT);
					}
					if(style->border_bottom_left_method->isSet())
					{
						m_border.setCornerMethod(IRenderBorder::BOTTOM_LEFT, style->border_bottom_left_method->getInt() == css::ICSSproperty::BORDER_METHOD_RADIUS ? IRenderBorder::CORNER_METHOD_RADIUS : IRenderBorder::CORNER_METHOD_CUT);
					}

					if(style->border_top_width->isSet())
					{
						m_border.setWidth(IRenderBorder::TOP, style->border_top_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					if(style->border_right_width->isSet())
					{
						m_border.setWidth(IRenderBorder::RIGHT, style->border_right_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					if(style->border_bottom_width->isSet())
					{
						m_border.setWidth(IRenderBorder::BOTTOM, style->border_bottom_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					if(style->border_left_width->isSet())
					{
						m_border.setWidth(IRenderBorder::LEFT, style->border_left_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()));
					}
					{
						UINT t = m_border.getWidth(IRenderBorder::TOP);
						UINT l = m_border.getWidth(IRenderBorder::LEFT);
						UINT b = m_border.getWidth(IRenderBorder::BOTTOM);
						UINT r = m_border.getWidth(IRenderBorder::RIGHT);
						UINT tl = m_border.getRadius(IRenderBorder::TOP_LEFT);
						UINT tr = m_border.getRadius(IRenderBorder::TOP_RIGHT);
						UINT bl = m_border.getRadius(IRenderBorder::BOTTOM_LEFT);
						UINT br = m_border.getRadius(IRenderBorder::BOTTOM_RIGHT);

						if((tl > t || tl > l)
							|| (tr > t || tr > r)
							|| (bl > b || bl > l)
							|| (br > b || br > r)
							)
						{
							m_bNeedCut = true;
						}

						if(m_pNode->getStyle()->overflow_y->isSet() && m_pNode->getStyle()->overflow_y->getInt() == css::ICSSproperty::OVERFLOW_VISIBLE)
						{
							m_bNeedCut = false;
						}
					}
					m_border.setContentSize(m_iWidth
						- style->margin_left->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						- style->margin_right->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						- style->border_left_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						- style->border_right_width->getPX(m_iWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()),
						m_iHeight
						- style->margin_top->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						- style->margin_bottom->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						- style->border_top_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						- style->border_bottom_width->getPX(m_iHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						);
					m_border.build();
				}
			}

			void IRenderFrame::updateBackground()
			{
				if(!m_pNode)
				{
					return;
				}

				css::ICSSstyle * pStyle = m_pNode->getStyle();

				m_bHasBackgroundImage = false;
				m_bHasBackground = false;

				if((pStyle->background_color->getInt() & 0x000000FF) != 0)
				{
					m_pBackgroundColor = pStyle->background_color->getColor();
					m_iBackgroundColor = ((pStyle->background_color->getInt() & 0xFFFFFF00) >> 8) | ((pStyle->background_color->getInt() & 0xFF) << 24);
					m_bHasBackground = true;
				}
				if(pStyle->background_image->isSet())
				{
					m_bHasBackgroundImage = true;
					if(!m_bHasBackground)
					{
						m_iBackgroundColor = 0x00FFFFFF;
						m_pBackgroundColor = float4_t(1.0f, 1.0f, 1.0f, 0.0f);
					}
				}
				
				if(!m_bHasBackgroundImage && !m_bHasBackground)
				{
					return;
				}

				float w = getInnerWidth();
				float h = getInnerHeight();

				float fSpinPercent = 1.0f;

				if(m_pNode->getStyle()->_gui_spin_percent->isSet())
				{
					fSpinPercent = (float)m_pNode->getStyle()->_gui_spin_percent->getPX(1000.0f, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight()) * 0.001;
					if(fSpinPercent < 0.0f)
					{
						fSpinPercent = 0.0f;
					}
				}

				if(fSpinPercent >= 1.0f)
				{
					m_pVBackground[0].Pos = float3_t(0.0f, 0.0f, 0.0f);
					m_pVBackground[1].Pos = float3_t(w, 0.0f, 0.0f);
					m_pVBackground[2].Pos = float3_t(0.0f, h, 0.0f);

					m_pVBackground[3].Pos = float3_t(0.0f, h, 0.0f);
					m_pVBackground[4].Pos = float3_t(w, 0.0f, 0.0f);
					m_pVBackground[5].Pos = float3_t(w, h, 0.0f);

					m_iTCBackground = 2;
				}
				else
				{
					m_pVBackground[0].Pos = float3_t(w * 0.5f, h * 0.5f, 0.0f);
					m_pVBackground[3].Pos = float3_t(w * 0.5f, h * 0.5f, 0.0f);
					m_pVBackground[6].Pos = float3_t(w * 0.5f, h * 0.5f, 0.0f);
					m_pVBackground[9].Pos = float3_t(w * 0.5f, h * 0.5f, 0.0f);
					m_pVBackground[12].Pos = float3_t(w * 0.5f, h * 0.5f, 0.0f);

					m_pVBackground[1].Pos = float3_t(w * 0.5f, 0.0f, 0.0f);

					if(fSpinPercent < 0.125f)
					{
						m_pVBackground[2].Pos = float3_t((w + h * tanf(SM_2PI * fSpinPercent)) * 0.5f, 0.0f, 0.0f);
						m_iTCBackground = 1;
					}
					else
					{
						m_pVBackground[2].Pos = float3_t(w, 0.0f, 0.0f);
						m_pVBackground[4].Pos = float3_t(w, 0.0f, 0.0f);

						if(fSpinPercent < 0.375f)
						{
							m_pVBackground[5].Pos = float3_t(w, (h - w * tanf(SM_PIDIV4 - SM_2PI * (fSpinPercent - 0.125f))) * 0.5f, 0.0f);
							m_iTCBackground = 2;
						}
						else
						{
							m_pVBackground[5].Pos = float3_t(w, h, 0.0f);
							m_pVBackground[7].Pos = float3_t(w, h, 0.0f);

							if(fSpinPercent < 0.625f)
							{
								m_pVBackground[8].Pos = float3_t((w + h * tanf(SM_PIDIV4 - SM_2PI * (fSpinPercent - 0.375f))) * 0.5f, h, 0.0f);
								m_iTCBackground = 3;
							}
							else
							{
								m_pVBackground[8].Pos = float3_t(0.0f, h, 0.0f);
								m_pVBackground[10].Pos = float3_t(0.0f, h, 0.0f);

								if(fSpinPercent < 0.875f)
								{
									m_pVBackground[11].Pos = float3_t(0.0f, (h + w * tanf(SM_PIDIV4 - SM_2PI * (fSpinPercent - 0.625f))) * 0.5f, 0.0f);
									m_iTCBackground = 4;
								}
								else
								{
									m_pVBackground[11].Pos = float3_t(0.0f, 0.0f, 0.0f);
									m_pVBackground[13].Pos = float3_t(0.0f, 0.0f, 0.0f);

									m_pVBackground[14].Pos = float3_t((w - h * tanf(SM_PIDIV4 - SM_2PI * (fSpinPercent - 0.875f))) * 0.5f, 0.0f, 0.0f);
									m_iTCBackground = 5;
								}
							}
						}
					}
				}

				if(m_bHasBackgroundImage)
				{
					IXTexture *pTex;
					m_pDoc->getDesktopStack()->getTextureManager()->getTexture(pStyle->background_image->getString(), &pTex);
					if(pTex != m_pBackgroundImage)
					{
						mem_release(m_pBackgroundImage);
						m_pBackgroundImage = pTex;
					}
					else
					{
						mem_release(pTex);
					}
					UINT tw = m_pBackgroundImage->getWidth();
					UINT th = m_pBackgroundImage->getHeight();

					

					IRenderFrame * pCur = this;
					m_bBackgroundFixed = false;
					switch(pStyle->background_attachment->getInt())
					{
					case css::ICSSproperty::BACKGROUND_ATTACHMENT_FIXED:
						m_bBackgroundScrolling = false;
						while(pCur->m_pParent)
						{
							pCur = pCur->m_pParent;
						}
						w = pCur->m_iWidth;
						h = pCur->m_iHeight;
						m_bBackgroundFixed = true;
						break;

					case css::ICSSproperty::BACKGROUND_ATTACHMENT_LOCAL:
						m_bBackgroundScrolling = true;
						break;

					case css::ICSSproperty::BACKGROUND_ATTACHMENT_SCROLL:
						m_bBackgroundScrolling = false;
						break;
					}

					float fAspect = (float)tw / (float)th;
					bool s[2] = {false};
					if(pStyle->background_size_x->isSet())
					{
						tw = pStyle->background_size_x->getPX(w, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
						s[0] = true;
					}
					if(pStyle->background_size_y->isSet())
					{
						th = pStyle->background_size_y->getPX(h, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
						s[1] = true;
					}

					if(s[0] && !s[1])
					{
						th = (float)tw / fAspect;
					}
					if(!s[0] && s[1])
					{
						tw = (float)th * fAspect;
					}

					float tu = w / (float)tw;
					float tv = h / (float)th;

					m_fBackgroundImageSize = float2_t(tw, th);

					if(pStyle->background_position_x->isSet())
					{
						m_iBackgroundOffsetX = -pStyle->background_position_x->getPX(w - tw, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					}
					else
					{
						m_iBackgroundOffsetX = 0;
					}

					if(pStyle->background_position_y->isSet())
					{
						m_iBackgroundOffsetY = -pStyle->background_position_y->getPX(h - th, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					}
					else
					{
						m_iBackgroundOffsetY = 0;
					}

					if(pStyle->background_repeat->isSet())
					{
						int val = pStyle->background_repeat->getInt();
						m_bBackgroundRepeatX = 0 != (val & css::ICSSproperty::BACKGROUND_REPEAT_REPEAT_X);
						m_bBackgroundRepeatY = 0 != (val & css::ICSSproperty::BACKGROUND_REPEAT_REPEAT_Y);
					}

					tu /= w;
					tv /= h;
					for(int i = 0; i < m_iTCBackground * 3; ++i)
					{
						m_pVBackground[i].Tex = (float2)(float2(tu, tv) * float2(m_pVBackground[i].Pos));
					}
				}
				

				//background-image
			}

			void IRenderFrame::renderBackground(UINT lvl)
			{
				if(!m_bHasBackgroundImage && !m_bHasBackground)
				{
					return;
				}
				static IXTexture *texWhite = m_pDoc->getDesktopStack()->getTextureManager()->getWhite();
			//	static CSHADER shText = CTextureManager::loadShader(L"text");

				IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

				IGXSamplerState *pOldSampler = pCtx->getSamplerState(0);

				auto shader = GetGUI()->getShaders()->m_baseTexturedColored;

				if(m_bHasBackgroundImage)
				{
					//	CTextureManager::bindTexture(m_pBackgroundImage);

					//SMMATRIX m = SMMatrixTranslation(0.5f, -m_iScrollTop, 0.0f);
					SMMATRIX m;


					int pt = 0;
					int pl = 0;
					if(m_bBackgroundFixed)
					{
						SMMATRIX mt = m_pDoc->getTranslationManager()->getCurrentMatrix();
						pl = mt._41;
						pt = mt._42;
					}

					float2 vTexTransform((float)((m_bBackgroundScrolling ? m_iScrollLeft : 0) + m_iBackgroundOffsetX + pl) / m_fBackgroundImageSize.x, // X-shift
						(float)((m_bBackgroundScrolling ? m_iScrollTop : 0) + m_iBackgroundOffsetY + pt) / m_fBackgroundImageSize.y);

				//	vTexTransform.x = 0;
				//	vTexTransform.y = 0;
				//	vTexTransform.z = 0;
				//	vTexTransform.w = 0;


					GXTEXTURE_ADDRESS_MODE addressU = m_bBackgroundRepeatX ? GXTEXTURE_ADDRESS_WRAP : GXTEXTURE_ADDRESS_BORDER;
					GXTEXTURE_ADDRESS_MODE addressV = m_bBackgroundRepeatY ? GXTEXTURE_ADDRESS_WRAP : GXTEXTURE_ADDRESS_BORDER;

					bool diff = !m_pSamplerState;
					if(addressU != m_samplerDesc.addressU)
					{
						m_samplerDesc.addressU = addressU;
						diff = true;
					}
					if(addressV != m_samplerDesc.addressV)
					{
						m_samplerDesc.addressV = addressV;
						diff = true;
					}
					if(memcmp(&m_pBackgroundColor, &(m_samplerDesc.f4BorderColor), sizeof(m_pBackgroundColor)))
					{
						m_samplerDesc.f4BorderColor = m_pBackgroundColor;
						diff = true;
					}

					if(diff)
					{
						m_samplerDesc.filter = GXFILTER_ANISOTROPIC;
						mem_release(m_pSamplerState);
						m_pSamplerState = GetGUI()->getDevice()->createSamplerState(&m_samplerDesc);
					}

					pCtx->setSamplerState(m_pSamplerState, 0);

					shader = GetGUI()->getShaders()->m_baseTexturedTextransformColored;

					static IGXConstantBuffer *s_pTransformConstant = GetGUI()->getDevice()->createConstantBuffer(sizeof(float2));
					s_pTransformConstant->update(&vTexTransform);
					pCtx->setVSConstant(s_pTransformConstant, 1);
				}
				else
				{
					
				}

				SGCore_ShaderBind(shader.m_idShaderKit);

				pCtx->setStencilRef(lvl);
			
				static IGXConstantBuffer *s_pColorConstant = GetGUI()->getDevice()->createConstantBuffer(sizeof(float2));
				s_pColorConstant->update(&m_pBackgroundColor);
				pCtx->setPSConstant(s_pColorConstant);

			//	SGCore_SetSamplerFilter(0, D3DTEXF_ANISOTROPIC);
				//DX_CALL(GetGUI()->getDevice()->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC));
				
				IGXRenderBuffer *pRB = m_pDoc->getDesktopStack()->getQuadRenderBufferXYZTex16((float*)m_pVBackground);

				pCtx->setRenderBuffer(pRB);

				m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(getInnerLeft(), getInnerTop(), 0.0f));
				if(m_bHasBackground && m_iTCBackground > 0)
				{
				//	CTextureManager::bindShader(shText);
					m_pDoc->getDesktopStack()->getTextureManager()->bindTexture(texWhite);
				//	DX_CALL(GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_iTCBackground, &m_pVBackground, sizeof(pointtex)));
					pCtx->drawPrimitive(0, m_iTCBackground);
				}
				if(m_bHasBackgroundImage && m_iTCBackground > 0)
				{
				//	CTextureManager::unbindShader();
					s_pColorConstant->update(&float4_t(1.0f, 1.0f, 1.0f, 1.0f));
					//SGCore_ShaderSetVRF(SHADER_TYPE_PIXEL, shader.m_idPS, "g_vColor", (float*)&float4_t(1.0f, 1.0f, 1.0f, 1.0f), 1);
					m_pDoc->getDesktopStack()->getTextureManager()->bindTexture(m_pBackgroundImage);
				//	DX_CALL(GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, m_iTCBackground, &m_pVBackground, sizeof(pointtex)));
					pCtx->drawPrimitive(0, m_iTCBackground);
				}
				m_pDoc->getTranslationManager()->popMatrix();
				

				
				
				

				

				if(m_bHasBackgroundImage)
				{
					pCtx->setSamplerState(pOldSampler, 0);
				//	GetGUI()->getDevice()->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0xFFFFFFFF);
				//	GetGUI()->getDevice()->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
				}
				mem_release(pOldSampler);
			}

			void IRenderFrame::updateStyles()
			{
				if(m_pNode)
				{
					UINT flags = ((css::CCSSstyle*)m_pNode->getStyle())->getChangesFlags();
					if(flags)
					{
						if(flags & css::ICSSproperty::FLAG_UPDATE_MASK)
						{
							m_pDoc->markDirty();
						}
						
						if(flags & css::ICSSproperty::FLAG_UPDATE_STRUCTURE)
						{
							//TODO: Make structure changes
							//m_pNode->getDocument()->
							return;
						}
						if(flags & css::ICSSproperty::FLAG_UPDATE_LAYOUT)
						{
						//	if(m_pNode->getNodeId() != IDOMnode::getNodeIdByName(L"video"))
							{
								//getNode()->getDocument()->
								m_bHasFixedSize = false; 
								m_pDoc->addReflowItem(this);
							}
							//	Layout();
							//	return;
						}
						else
						{
							if(flags & (css::ICSSproperty::FLAG_UPDATE_BACKGROUNDLAYOUT | css::ICSSproperty::FLAG_UPDATE_BACKGROUNDVIEW))
							{
								updateBackground();
							}
							if(flags & css::ICSSproperty::FLAG_UPDATE_BORDERLAYOUT)
							{
								updateBorder();
							}
						}
						if(flags & css::ICSSproperty::FLAG_UPDATE_BORDERVIEW)
						{
							updateBorderColor();
						}
					}
				}
				for(UINT i = 0; i < m_pChilds.size(); i++)
				{
					m_pChilds[i]->updateStyles();
				}
				for(UINT i = 0; i < m_pChildsOutFlow.size(); i++)
				{
					m_pChildsOutFlow[i]->updateStyles();
				}
			}

			RECT IRenderFrame::getClientRect()
			{
				RECT rc;

				rc.top = (m_pParent ? (m_pParent->getClientTop()/* + m_pParent->getInnerTop()*/) : 0) + m_iYpos;
				rc.left = (m_pParent ? (m_pParent->getClientLeft()/* + m_pParent->getInnerLeft()*/) : 0) + m_iXpos;
				rc.right = rc.left + m_iWidth;
				rc.bottom = rc.top + m_iHeight;

				return(rc);
			}

			RECT IRenderFrame::getVisibleRect()
			{
				RECT rc;
				if(m_pNode && m_pNode->getStyle()->visibility->getInt() == css::ICSSproperty::VISIBILITY_HIDDEN)
				{
					memset(&rc, 0, sizeof(rc));
					return(rc);
				}

				if(!m_pParent)
				{
					return(getClientRect());
				}

				RECT prc = m_pParent->getVisibleRect();


				rc = getClientRect();
				if(m_pParent->getNode() && m_pParent->getNode()->getStyle()->overflow_y->getInt() != css::ICSSproperty::OVERFLOW_VISIBLE)
				{
					rc.top = max(rc.top, prc.top);
					rc.bottom = min(rc.bottom, prc.bottom);
				}
				if(m_pParent->getNode() && m_pParent->getNode()->getStyle()->overflow_x->getInt() != css::ICSSproperty::OVERFLOW_VISIBLE)
				{
					rc.left = max(rc.left, prc.left);
					rc.right = min(rc.right, prc.right);
				}

				return(rc);
			}

			void IRenderFrame::findElementByXY(int x, int y, IDOMnode ** ppNode, bool sendEnterLeave)
			{
				RECT rc = getVisibleRect();
				bool bHovered = sendEnterLeave && m_pNode && m_pNode->pseudoclassExists(css::ICSSrule::PSEUDOCLASS_HOVER);
				bool InMe = false;
				if(inRect(rc, x, y) && m_pNode)
				{
					(*ppNode) = m_pNode;
					if(sendEnterLeave && !bHovered)
					{
						IEvent ev;
						ev.type = GUI_EVENT_TYPE_MOUSEENTER;
						ev.clientX = x;
						ev.clientY = y;
						ev.offsetX = x - rc.left;
						ev.offsetY = y - rc.top;
						ev.target = m_pNode;
						m_pNode->dispatchEvent(ev);
					}
					InMe = true;
				}
				else
				{
					if(sendEnterLeave && bHovered)
					{
						IEvent ev;
						ev.type = GUI_EVENT_TYPE_MOUSELEAVE;
						ev.clientX = x;
						ev.clientY = y;
						ev.offsetX = x - rc.left;
						ev.offsetY = y - rc.top;
						ev.target = m_pNode;
						m_pNode->dispatchEvent(ev);
					}
				}
			//	if(InMe)
				{
					for(UINT i = 0; i < m_pChilds.size(); i++)
					{
						m_pChilds[i]->findElementByXY(x, y, ppNode, sendEnterLeave);
					}
				}
				for(UINT i = 0; i < m_pChildsOutFlow.size(); i++)
				{
					m_pChildsOutFlow[i]->findElementByXY(x, y, ppNode, sendEnterLeave);
				}
			}

			bool IRenderFrame::inRect(const RECT & rc, int x, int y)
			{
				return(rc.top <= y && rc.left <= x && rc.bottom > y && rc.right > x);
			}

			void IRenderFrame::onCreated()
			{
				// !!! sizeof m_pChilds or m_pChildsOutFlow can be changed during iteration
				for(UINT i = 0; i < m_pChilds.size(); ++i)
				{
					m_pChilds[i]->onCreated();
				}
				for(UINT i = 0; i < m_pChildsOutFlow.size(); ++i)
				{
					m_pChildsOutFlow[i]->onCreated();
				}
			}

			//##########################################################################

			IRenderBlock::IRenderBlock(CDOMnode * pNode, IRenderFrame * pRootNode):BaseClass(pNode, pRootNode)
			{

			}

			UINT IRenderBlock::layout(bool changed)
			{
				bool bChanged = !hasFixedSize();
				m_bHasFixedSize = false;
				css::ICSSstyle * style = m_pNode->getStyle();
				UINT _parentWidth = 0;
				UINT _parentHeight = 0;
				if(bChanged)
				{
					if(m_pParent)
					{
						_parentWidth = m_pParent->getContentWidth();
						_parentHeight = m_pParent->getContentHeight();
						if(!style->width->isSet())
						{
							m_iWidth = _parentWidth;
						}
						else
						{
							m_iWidth = style->width->getPX(_parentWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
						}
						m_iYpos = m_pParent->getContentTop();
					}
					else
					{
						_parentWidth = m_iWidth;
						_parentHeight = m_iHeight;
					}
				}
				bool hSet = m_iHeight != 0;
				if(style->height->isSet() && !hSet)
				{
					m_iHeight = style->height->getPX(_parentHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					hSet = true;
				}
				bool bReturnHeight = true;
				if(bChanged)
				{
					if(m_pParent)
					{
						m_iXpos = m_pParent->getContentLeft();
					}
					
					if(style->position->isSet())
					{
						switch(style->position->getInt())
						{
						case css::ICSSproperty::POSITION_RELATIVE:
							if(style->left->isSet())
							{
								m_iXpos += style->left->getPX(_parentWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
							}
							else if(style->right->isSet())
							{
								m_iXpos -= style->right->getPX(_parentWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
							}
							if(style->top->isSet())
							{
								m_iYpos += style->top->getPX(_parentHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
							}
							else if(style->bottom->isSet())
							{
								m_iYpos -= style->bottom->getPX(_parentHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
							}
							break;

						case css::ICSSproperty::POSITION_FIXED:
						case css::ICSSproperty::POSITION_ABSOLUTE:
							if(style->left->isSet())
							{
								m_iXpos = style->left->getPX(_parentWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
								if(/*!style->p_width->isSet() && */style->right->isSet())
								{
									m_iWidth = _parentWidth - m_iXpos - style->right->getPX(_parentWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
								}
							}
							else if(style->right->isSet())
							{
								m_iXpos = _parentWidth - m_iWidth - style->right->getPX(_parentWidth, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
							}

							if(style->top->isSet())
							{
								m_iYpos = style->top->getPX(_parentHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
								if(!style->height->isSet() && style->bottom->isSet())
								{
									m_iHeight = _parentHeight - m_iYpos - style->bottom->getPX(_parentHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
									hSet = true;
								}
							}
							else if(style->bottom->isSet())
							{
								m_iYpos = _parentHeight - m_iHeight - style->bottom->getPX(_parentHeight, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
							}
							bReturnHeight = false;
							break;
						}
					}
				}
				if(!isFreezzed())
				{
					UINT height = BaseClass::layout(bChanged);
					UINT hNew = height
						+ style->margin_top->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->margin_bottom->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->padding_top->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->padding_bottom->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->border_top_width->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
						+ style->border_bottom_width->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					if(!hSet/* && !m_iHeight*/)
					{
						m_iHeight = hNew;
						hSet = true;
					}
					else
					{
						m_bHasFixedSize = true;
						if(hNew > m_iHeight)
						{
							m_iScrollTopMax = hNew - m_iHeight;
							m_bNeedCut = true;
							if(m_pNode->getStyle()->overflow_y->isSet())
							{
								switch(m_pNode->getStyle()->overflow_y->getInt())
								{
								case css::ICSSproperty::OVERFLOW_HIDDEN:
									m_iScrollTopMax = 0;
									break;
								case css::ICSSproperty::OVERFLOW_VISIBLE:
									m_bNeedCut = false;
									m_iScrollTopMax = 0;
									break;
								}
							}
						}
					}
					updateBorder();
					updateBackground();
				}
				return(bReturnHeight ? m_iHeight : 0);
			}

			void IRenderBlock::render(UINT lvl, float fDT)
			{
				IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

				pCtx->setStencilRef(lvl);
				if(m_iScrollTop != 0 || m_iScrollTopMax != 0)
				{
					//m_iScrollTop = Config::TestScrollPos;
					if(m_iScrollTop > m_iScrollTopMax)
					{
						m_iScrollTop = /*Config::TestScrollPos = */m_iScrollTopMax;
					}
					if(m_iScrollTop < 0)
					{
						m_iScrollTop/* = Config::TestScrollPos*/ = 0;
					}
				}
				if(m_iScrollLeftMax != 0)
				{
					//m_iScrollTop = Config::TestScrollPos;
					if(m_iScrollLeft > m_iScrollLeftMax)
					{
						m_iScrollLeft = /*Config::TestScrollPos = */m_iScrollLeftMax;
					}
					if(m_iScrollLeft < 0)
					{
						m_iScrollLeft/* = Config::TestScrollPos*/ = 0;
					}
				}
				if(m_iScrollTopMax != 0 && !m_pScrollBarVert)
				{
					m_pScrollBarVert = new CScrollBarSimple(this, SCROLLBAR_DIR_VERTICAL);
				}
				if(m_iScrollLeftMax != 0 && !m_pScrollBarHorz)
				{
					m_pScrollBarHorz = new CScrollBarSimple(this, SCROLLBAR_DIR_HORIZONTAL);
				}
				m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, m_pNode->getStyle()->_gui_z->getPX(getParent() ? getParent()->getContentWidth() : getWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())));
				if(m_pNode->getStyle()->transform->isSet())
				{
					SMMATRIX mCurrent = /*CTranslationManager::getCurrentMatrix() * */SMMatrixTranslation((float)getWidth() * 0.5f, (float)getHeight() * 0.5f, 0.0f);
					float fDet;
					m_pDoc->getTranslationManager()->pushMatrix(SMMatrixInverse(&fDet, mCurrent) * m_pNode->getStyle()->transform->getMatrix() * mCurrent);
				}

				if(m_pNode->getStyle()->visibility->getInt() != css::ICSSproperty::VISIBILITY_HIDDEN)
				{

					struct point
					{
						float x;
						float y;
						float z;
						DWORD rc;
					};
					struct point2
					{
						float x;
						float y;
						float z;
					};
					point a[6] = {
						{0, 0, 0, rc},
						{(float)m_iWidth, 0, 0, rc},
						{0, (float)m_iHeight, 0, rc},
						{0, (float)m_iHeight, 0, rc},
						{(float)m_iWidth, 0, 0, rc},
						{(float)m_iWidth, (float)(m_iHeight), 0, rc}
					};
					point2 apdx8[] = {
						{0, 0, 0},
						{(float)m_iWidth, 0, 0},
						{(float)m_iWidth, (float)(m_iHeight), 0},
						{0, (float)(m_iHeight), 0}
					};
					

				//	GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point));

					if(m_bNeedCut)
					{
						pCtx->setDepthStencilState(GetGUI()->getDepthStencilStates()->m_pStencilIncr);
						pCtx->setBlendState(GetGUI()->getBlendStates()->m_pNoColorWrite);
					//	pCtx->SetRenderState(D3DRS_COLORWRITEENABLE, FALSE);
					//	pCtx->SetRenderState(D3DRS_STENCILENABLE, TRUE);
					//	pCtx->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
					//	pCtx->setStencilRef(lvl);
						m_border.renderInnerFill();

						pCtx->setBlendState(GetGUI()->getBlendStates()->m_pDefault);
					//	pCtx->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
					//	pCtx->SetRenderState(D3DRS_STENCILREF, lvl + 1);
					//	pCtx->SetRenderState(D3DRS_STENCILENABLE, FALSE);
						pCtx->setDepthStencilState(GetGUI()->getDepthStencilStates()->m_pStencilKeep);
					//	pCtx->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
					}

					m_border.render();
							
					if(m_bNeedCut)
					{
					//	GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILENABLE, TRUE);
					//	GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
					}
					renderBackground(lvl + (m_bNeedCut ? 1 : 0));

					if(m_pScrollBarVert || m_pScrollBarHorz)
					{
						pCtx->setStencilRef(lvl + (m_bNeedCut ? 1 : 0));
					}
					if(m_pScrollBarVert)
					{
						int sbOffset = m_iWidth - m_pScrollBarVert->getWidth();
						m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(sbOffset, 0.0f, 0.0f));
						m_pScrollBarVert->setLength(m_iHeight);
						m_pScrollBarVert->render();
						m_pDoc->getTranslationManager()->popMatrix();
					}
					if(m_pScrollBarHorz)
					{
						int sbOffset = m_iHeight - m_pScrollBarHorz->getWidth();
						m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(0.0f, sbOffset, 0.0f));
						m_pScrollBarHorz->setLength(m_iWidth);
						m_pScrollBarHorz->render();
						m_pDoc->getTranslationManager()->popMatrix();
					}


					m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(-m_iScrollLeft, -m_iScrollTop, 0.0f));
					BaseClass::render(lvl + (m_bNeedCut ? 1 : 0), fDT);
					m_pDoc->getTranslationManager()->popMatrix();
					if(m_bNeedCut)
					{
						auto shader = GetGUI()->getShaders()->m_baseColored;
						SGCore_ShaderBind(shader.m_idShaderKit);

						pCtx->setDepthStencilState(GetGUI()->getDepthStencilStates()->m_pStencilDecr);
					//	pCtx->SetRenderState(D3DRS_COLORWRITEENABLE, FALSE);
						pCtx->setBlendState(GetGUI()->getBlendStates()->m_pNoColorWrite);
					//	pCtx->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);

						IGXRenderBuffer *pRB = m_pDoc->getDesktopStack()->getQuadRenderBufferXYZ((float3_t*)apdx8);
					//	GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ);
					//	DX_CALL(GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &apdx8, sizeof(point2)));
						pCtx->setRenderBuffer(pRB);
						pCtx->setIndexBuffer(GetGUI()->getQuadIndexBuffer());
						pCtx->drawIndexed(4, 2, 0, 0);

						pCtx->setDepthStencilState(GetGUI()->getDepthStencilStates()->m_pStencilKeep);
					//	pCtx->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
					//	pCtx->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
						pCtx->setBlendState(GetGUI()->getBlendStates()->m_pDefault);
						if(lvl == 0)
						{
							pCtx->setDepthStencilState(GetGUI()->getDepthStencilStates()->m_pDefault);
					//		pCtx->SetRenderState(D3DRS_STENCILENABLE, FALSE);
						}
					}
				}

				if(m_pNode->getStyle()->transform->isSet())
				{
					m_pDoc->getTranslationManager()->popMatrix();
				}
				m_pDoc->getTranslationManager()->popMatrix();
			}

			//##########################################################################

			IRenderAnonymousBlock::IRenderAnonymousBlock(IRenderFrame * pRootNode):BaseClass(NULL, pRootNode)
			{
			}

			UINT IRenderAnonymousBlock::layout(bool changed)
			{
				m_iXpos = m_pParent->getContentLeft();
				m_iYpos = m_pParent->getContentTop();

				m_iWidth = m_pParent->getContentWidth();
				m_iHeight = BaseClass::layout();
				return(m_iHeight);
			}

			void IRenderAnonymousBlock::render(UINT lvl, float fDT)
			{
				GetGUI()->getDevice()->getThreadContext()->setStencilRef(lvl);
				m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, 0.0f));
				BaseClass::render(lvl, fDT);
				m_pDoc->getTranslationManager()->popMatrix();
			}

			//##########################################################################

			IRenderInlineBlock::IRenderInlineBlock(CDOMnode * pNode, IRenderFrame * pRootNode):BaseClass(pNode, pRootNode)
			{
			}

			UINT IRenderInlineBlock::layout(bool changed)
			{
				bool bChanged = !hasFixedSize();
				m_bHasFixedSize = true;
				css::ICSSstyle * style = m_pNode->getStyle();
				if(bChanged)
				{
					m_iXpos = m_pParent->getLeftPos();
					m_iYpos = m_pParent->getTopPos();
					if(!m_iXpos)
					{
						m_pParent->setLeftPos(m_iXpos = m_pParent->getContentLeft());
					}
					if(!m_iYpos)
					{
						m_pParent->setTopPos(m_iYpos = m_pParent->getContentTop());
					}
					isWidthSet = false;
					
					if(!style->width->isSet())
					{
						m_iWidth = m_pParent->getInnerWidth() - m_iXpos;
						m_bHasFixedSize = false;
					}
					else
					{
						m_iWidth = style->width->getPX(m_pParent->getContentWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
						isWidthSet = true;
					}
				}
				bool hSet = false;
				if(bChanged)
				{
					UINT _PH = m_pParent->getContentHeight();
					if(style->height->isSet())
					{
						m_iHeight = style->height->getPX(_PH, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
						hSet = true;
					}
					else if(style->_gui_text_appearance->isSet() && style->_gui_text_appearance->getInt() == css::ICSSproperty::_GUI_TEXT_APPEARANCE_ONE_LINE)
					{
						m_iHeight = style->font_size->getPX(_PH, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
							+ style->padding_top->getPX(_PH, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
							+ style->padding_bottom->getPX(_PH, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
							+ style->border_top_width->getPX(_PH, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
							+ style->border_bottom_width->getPX(_PH, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
						hSet = true;
					}
				}
				else
				{
					hSet = true;
				}
				UINT height = BaseClass::BaseClass::layout();
				
				UINT hNew = height
					+ style->margin_top->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->margin_bottom->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->padding_top->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->padding_bottom->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_top_width->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight())
					+ style->border_bottom_width->getPX(height, m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
				if(!hSet)
				{
					m_bHasFixedSize = false;
					m_iHeight = hNew;
					hSet = true;
				}
				else
				{
					if(hNew > m_iHeight)
					{
						m_iScrollTopMax = hNew - m_iHeight;
						m_bNeedCut = true;
					}
				}
				if(bChanged)
				{
					if(m_iWidth + m_iXpos > m_pParent->getContentWidth())
					{
						/*if(m_pParent->getNode() && m_pParent->getNode()->getStyle()->text_align->getInt() == css::ICSSproperty::TEXT_ALIGN_CENTER)
						{
							UINT xDelta = (m_pParent->getContentWidth() - m_iXpos) / 2;
							for(UINT ii = 0, ll = m_pParent->GetChildCount(); ii < ll; ++ii)
							{
								IRenderFrame * rfcur = m_pParent->GetChild(ii);
								if(rfcur->getYpos() == m_iYpos)
								{
									rfcur->setXpos(rfcur->getXpos() + xDelta);
								}
							}
						}*/
						m_iXpos = m_pParent->getContentLeft();
						m_pParent->setTopPos(m_pParent->getTopPosMax());
						m_iYpos = m_pParent->getTopPos();
						//request line break;
					}
					else
					{
						m_pParent->setTopPosMax(max(m_pParent->getTopPosMax(), m_iYpos + m_iHeight));
						//UINT t = max(m_iYpos + m_iHeight
					}
				}
				m_pParent->setLeftPos(m_iXpos + m_iWidth);
				m_pParent->setTopPos(m_iYpos - (m_iXpos == m_pParent->getContentLeft() ? m_iHeight : 0));
				if(bChanged)
				{
					if(m_pParent->getHeight() == 0)
					{
						m_pParent->setHeight(m_pParent->getTopPosMax());
					}
					updateBorder();
					updateBackground();
				}
				return(m_iXpos == m_pParent->getContentLeft() ? m_iHeight : 0);
			}

#if 0
			void IRenderInlineBlock::render(UINT lvl)
			{
				/*GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILREF, lvl);
				CTranslationManager::pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, 0.0f));
				struct point
				{
					float x;
					float y;
					float z;
					DWORD rc;
				};
				//UINT y = GetTopOffset();
				//UINT x = GetLeftOffset();
				point a[6] = {
					{0, 0, 0, rc},
					{m_iWidth, 0, 0, rc},
					{0, (float)(m_iHeight), 0, rc},
					{0, (float)(m_iHeight), 0, rc},
					{m_iWidth, 0, 0, rc},
					{m_iWidth, (float)(m_iHeight), 0, rc}
				};

				GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point));

				BaseClass::Render(lvl + 1);
				CTranslationManager::popMatrix();*/

				GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILREF, lvl);
				if(m_iScrollTopMax != 0)
				{
					//m_iScrollTop = Config::TestScrollPos;
					if(m_iScrollTop > m_iScrollTopMax)
					{
						m_iScrollTop = /*Config::TestScrollPos = */m_iScrollTopMax;
					}
					if(m_iScrollTop < 0)
					{
						m_iScrollTop/* = Config::TestScrollPos*/ = 0;
					}
				}
				if(m_iScrollLeftMax != 0)
				{
					//m_iScrollTop = Config::TestScrollPos;
					if(m_iScrollLeft > m_iScrollLeftMax)
					{
						m_iScrollLeft = /*Config::TestScrollPos = */m_iScrollLeftMax;
					}
					if(m_iScrollLeft < 0)
					{
						m_iScrollLeft/* = Config::TestScrollPos*/ = 0;
					}
				}
				CTranslationManager::pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, m_pNode->getStyle()->_gui_z->getPX(getParent() ? getParent()->getContentWidth() : getWidth())));

				if(m_pNode->getStyle()->visibility->getInt() != css::ICSSproperty::VISIBILITY_HIDDEN)
				{

					struct point
					{
						float x;
						float y;
						float z;
						DWORD rc;
					};

					point a[6] = {
						{0, 0, 0, rc},
						{m_iWidth, 0, 0, rc},
						{0, (float)(m_iHeight), 0, rc},
						{0, (float)(m_iHeight), 0, rc},
						{m_iWidth, 0, 0, rc},
						{m_iWidth, (float)(m_iHeight), 0, rc}
					};



					//	GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point));

					if(m_bNeedCut)
					{
						GetGUI()->getDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, FALSE);
						GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILENABLE, TRUE);
						GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
						GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILREF, lvl);
						m_border.renderInnerFill();

						GetGUI()->getDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
						//	GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILREF, lvl + 1);
						//	GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILENABLE, FALSE);
						GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
					}

					m_border.render();
					if(m_bNeedCut)
					{
						//	GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILENABLE, TRUE);
						//	GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
					}
					renderBackground(lvl + (m_bNeedCut ? 1 : 0));
					CTranslationManager::pushMatrix(SMMatrixTranslation(-m_iScrollLeft, -m_iScrollTop, 0.0f));
					BaseClass::render(lvl + (m_bNeedCut ? 1 : 0));
					CTranslationManager::popMatrix();
					if(m_bNeedCut)
					{
						GetGUI()->getDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, FALSE);
						GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
						DX_CALL(GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point)));
						GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
						GetGUI()->getDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, 0x0F);
						if(lvl == 0)
						{
							GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILENABLE, FALSE);
						}
					}
				}

				CTranslationManager::popMatrix();
			}
#endif
			//##########################################################################

			IRenderInline::IRenderInline(CDOMnode * pNode, IRenderFrame * pRootNode):BaseClass(pNode, pRootNode)
			{
			}

			UINT IRenderInline::layout(bool changed)
			{
				m_iXpos = m_pParent->getContentLeft();
				m_iYpos = m_pParent->getContentTop();

				m_iHeight = BaseClass::layout();
				updateBorder();
				updateBackground();
				return(m_iHeight);
			}

			void IRenderInline::render(UINT lvl, float fDT)
			{
				GetGUI()->getDevice()->getThreadContext()->setStencilRef(lvl);
				m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, 0.0f));
				m_border.render();
				renderBackground(lvl);
				BaseClass::render(lvl, fDT);
				m_pDoc->getTranslationManager()->popMatrix();
			}

			//##########################################################################
#if 0

			IRenderText::IRenderText(CDOMnode * pNode, IRenderFrame * pRootNode):BaseClass(pNode, pRootNode)
			{
				setText(((IDOMnodeText*)pNode)->getText());
			}

			UINT IRenderText::layout(bool changed)
			{
				IRenderFrame * pBlock = m_pParent;
				while(pBlock->getNode() && pBlock->getNode()->getStyle()->display->getInt() == css::ICSSproperty::DISPLAY_INLINE)
				{
					pBlock = pBlock->getParent();
				}
				css::ICSSstyle * pStyle = m_pParent->getNode() ? m_pParent->getNode()->getStyle() : m_pParent->getParent()->getNode()->getStyle();
				UINT iTextSize = pStyle->font_size->getPX(pBlock->getContentHeight());
				const WCHAR * szFontName = pStyle->font_name->getString();

				css::ICSSproperty::TYPE_FONT_STYLE_e fontStyle = (css::ICSSproperty::TYPE_FONT_STYLE_e)pStyle->font_style->getInt();
				css::ICSSproperty::TYPE_FONT_WEIGHT_e fontWeight = (css::ICSSproperty::TYPE_FONT_WEIGHT_e)pStyle->font_weight->getInt();
				css::ICSSproperty::TYPE_TEXT_ALIGN_e _textAlign = (css::ICSSproperty::TYPE_TEXT_ALIGN_e)pStyle->text_align->getInt();
				css::ICSSproperty::TYPE_TEXT_DECORATION_LINE_e _textDecoration = (css::ICSSproperty::TYPE_TEXT_DECORATION_LINE_e)pStyle->text_decoration_line->getInt();


				m_iXpos = m_pParent->getContentLeft();
				m_iYpos = m_pParent->getContentTop();

				CFont::TEXT_ALIGN textAlign;
				switch(_textAlign)
				{
				case css::ICSSproperty::TEXT_ALIGN_CENTER:
					textAlign = CFont::TEXT_ALIGN_CENTER;
					break;
				case css::ICSSproperty::TEXT_ALIGN_RIGHT:
					textAlign = CFont::TEXT_ALIGN_RIGHT;
					break;
				default:
					textAlign = CFont::TEXT_ALIGN_LEFT;
				}

				UINT textDecoration = CFont::DECORATION_NONE;
				switch(_textDecoration)
				{
				case css::ICSSproperty::TEXT_DECORATION_LINE_LINE_THROUGH:
					textDecoration = CFont::DECORATION_LINE_THROUGH;
					break;
				case css::ICSSproperty::TEXT_DECORATION_LINE_OVERLINE:
					textDecoration = CFont::DECORATION_OVERLINE;
					break;
				case css::ICSSproperty::TEXT_DECORATION_LINE_UNDERLINE:
					textDecoration = CFont::DECORATION_UNDERLINE;
					break;
				}

				UINT m_iAreaWidth = pBlock->getContentWidth();

				CFont::STYLE style = (CFont::STYLE)((fontStyle == css::ICSSproperty::FONT_STYLE_ITALIC ? CFont::STYLE_ITALIC : 0) | (fontWeight == css::ICSSproperty::FONT_WEIGHT_BOLD ? CFont::STYLE_BOLD : 0));

				pFont = IFontManager::getFont(szFontName, iTextSize, style);

				struct word
				{
					UINT start_pos;
					UINT length;
				};

				word * pWords = new word[m_iWordCount];
				UINT _len = 0;
				UINT _pos = 0;
				UINT _idx = 0;
				for(UINT i = 0, l = m_szClearText.length(); i < l; i++)
				{
					if(iswspace(m_szClearText[i]))
					{
						pWords[_idx].start_pos = _pos;
						pWords[_idx].length = _len;
						//pWords[_idx] = {_pos, _len};
						_idx++;
						_len = 1;
						_pos = i;
					}
					else
					{
						_len++;
					}
				}
				pWords[_idx].start_pos = _pos;
				pWords[_idx].length = _len;
				//pWords[_idx] = {_pos, _len};

				StringW tmp;
				UINT _w
					,_h;
				UINT _cw = pBlock->getLeftPos();
				UINT fo = _cw;
				UINT breaks = 0;
				for(UINT i = 0; i < m_iWordCount; i++)
				{
					tmp = m_szClearText.substr(pWords[i].start_pos, pWords[i].length);
					pFont->getStringMetrics(tmp, &_w, &_h, NULL, NULL, NULL);
					if(_cw + _w >= m_iAreaWidth && i > 0)
					{
						m_szClearText[pWords[i].start_pos] = L'\n';
						_cw = 0;
						breaks++;
					}
					_cw += _w;
				}
				
				pBlock->setLeftPos(_cw);
				pFont->buildString(m_szClearText, textDecoration, textAlign, &m_pVertexBuffer, &m_pIndexBuffer, &m_iVertexCount, &m_iIndexBaseCount, &m_iIndexAddCount, m_iAreaWidth, fo);
				//m_pParent->setTopPos(m_iYpos - iTextSize);
				mem_delete_a(pWords);
				return(iTextSize * (breaks));
			}

			void IRenderText::render(UINT lvl)
			{
				GetGUI()->getDevice()->SetRenderState(D3DRS_STENCILREF, lvl);
				static CPITexture texWhite = NULL;
				if(!texWhite)
				{
					texWhite = CTextureManager::getTexture(TEX_WHITE);
				}
				GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
				CTranslationManager::pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, 0.0f));
				//SMMATRIX m = SMMatrixTranslation(m_pParent->GetLeftOffset(), m_pParent->GetTopOffset(), 0.0f);
				//GetGUI()->getDevice()->SetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&m));
				CTextureManager::bindTexture(pFont->getTexture(0));
				DX_CALL(GetGUI()->getDevice()->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(CFont::vertex)));
				DX_CALL(GetGUI()->getDevice()->SetIndices(m_pIndexBuffer));
				DX_CALL(GetGUI()->getDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_iVertexCount, 0, m_iIndexBaseCount / 3));

				CTextureManager::bindTexture(texWhite);

				if(m_iIndexAddCount)
				{
					GetGUI()->getDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, m_iVertexCount, m_iIndexBaseCount, m_iIndexAddCount / 3);
				}
				CTranslationManager::popMatrix();
				GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
			}

			void IRenderText::setText(const StringW & text)
			{
				m_szClearText = L"";
				m_iWordCount = 1;
				bool issp = true;
				for(UINT i = 0; i < text.length(); i++)
				{
					if(iswspace(text[i]))
					{
						if(!issp)
						{
							m_szClearText += L' ';
							m_iWordCount++;
						}
						issp = true;
					}
					else
					{
						issp = false;
						m_szClearText += text[i];
					}
				}
			}

#endif
			//##########################################################################

			void IRenderTextNew::setText(const StringW & text)
			{
				if(m_pParent && m_pParent->getNode() && m_pParent->getNode()->getStyle()->_gui_text_format->getInt() == css::ICSSproperty::_GUI_TEXT_FORMAT_PREFORMATTED)
				{
					m_szClearText = L"";
					m_iWordCount = 1;
					bool issp = true;
					for(UINT i = 0; i < text.length(); i++)
					{
						if(iswspace(text[i]))
						{
							if(!issp)
							{
								m_iWordCount++;
							}
							issp = true;
						}
						else
						{
							issp = false;
						}
					}
					m_szClearText = text;
				}
				else
				{
					m_szClearText = L"";
					m_iWordCount = 1;
					bool issp = true;
					for(UINT i = 0; i < text.length(); i++)
					{
						if(iswspace(text[i]))
						{
							if(!issp)
							{
								m_szClearText += L' ';
								m_iWordCount++;
							}
							issp = true;
						}
						else
						{
							issp = false;
							m_szClearText += text[i];
						}
					}
				}
			}
			void IRenderTextNew::render(UINT lvl, float fDT)
			{
				IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

				pCtx->setStencilRef(lvl);
				static IXTexture *texWhite = m_pDoc->getDesktopStack()->getTextureManager()->getWhite();
			/*	if(!shText)
				{
					shText = CTextureManager::loadShader(L"text");
				}*/
			//	GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
				m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(m_iXpos, m_iYpos, 0.0f));
				//SMMATRIX m = SMMatrixTranslation(m_pParent->GetLeftOffset(), m_pParent->GetTopOffset(), 0.0f);
				//GetGUI()->getDevice()->SetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&m));
				
				auto shader = GetGUI()->getShaders()->m_baseTexturedColored;
				SGCore_ShaderBind(shader.m_idShaderKit);
			//	CTextureManager::bindShader(shText);
				m_pDoc->getDesktopStack()->getTextureManager()->bindTexture(texWhite);
				renderSelection();

				float4_t vColor = m_pStyle->color->getColor();
				float4_t vShadowColor = m_pStyle->text_shadow_color->isSet() ? m_pStyle->text_shadow_color->getColor() : vColor;

				static IGXConstantBuffer *s_pColorConstant = GetGUI()->getDevice()->createConstantBuffer(sizeof(float4));
				s_pColorConstant->update(&vColor);
				pCtx->setPSConstant(s_pColorConstant);

				for(UINT i = 0; i < m_pRenderElems.size(); i++)
				{
					CRenderElement * el = &m_pRenderElems[i];
					m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(0.0f, el->m_iTopOffset, 0.0f));
					m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(el->m_iLeftOffset, 0.0f, 0.0f));

					if(pShadowFont && el->m_pNextREl)
					{
						m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(el->m_pNextREl->m_iLeftOffset, el->m_pNextREl->m_iTopOffset, 0.0f));
						pCtx->setPSTexture(pShadowFont->getAPITexture(0));
						s_pColorConstant->update(&vShadowColor);

						pCtx->setRenderBuffer(el->m_pNextREl->m_pRenderBuffer);
						pCtx->setIndexBuffer(el->m_pNextREl->m_pIndexBuffer);
						pCtx->drawIndexed(el->m_pNextREl->m_iVertexCount, el->m_pNextREl->m_iIndexBaseCount / 3, 0, 0);

						s_pColorConstant->update(&vColor);
						m_pDoc->getTranslationManager()->popMatrix();
					}

					pCtx->setPSTexture(pFont->getAPITexture(0));
					pCtx->setRenderBuffer(el->m_pRenderBuffer);
					pCtx->setIndexBuffer(el->m_pIndexBuffer);
					if(el->m_iIndexBaseCount)
					{
						pCtx->drawIndexed(el->m_iVertexCount, el->m_iIndexBaseCount / 3, 0, 0);
					}
					if(el->m_iIndexAddCount)
					{
						m_pDoc->getDesktopStack()->getTextureManager()->bindTexture(texWhite);
						pCtx->drawIndexed(el->m_iVertexCount, el->m_iIndexAddCount / 3, el->m_iIndexBaseCount, 0);
					}

					m_pDoc->getTranslationManager()->popMatrix();

				}
				for(UINT i = 0; i < m_pRenderElems.size(); i++)
				{
					m_pDoc->getTranslationManager()->popMatrix();
				}
				
				m_pDoc->getDesktopStack()->getTextureManager()->bindTexture(texWhite);
				drawCaret();
				m_pDoc->getTranslationManager()->popMatrix();
			}

			void IRenderTextNew::selectionStart()
			{
				m_bInSelection = true;
			}
			void IRenderTextNew::selectionEnd()
			{
				m_bInSelection = false;
			}

			UINT IRenderTextNew::layout(bool changed)
			{
				setText(((IDOMnodeText*)m_pNode)->getText());
				((IDOMnodeText*)m_pNode)->setText(m_szClearText);
				IRenderFrame * pBlock = m_pParent;
				while(pBlock->getNode() && pBlock->getNode()->getStyle()->display->getInt() == css::ICSSproperty::DISPLAY_INLINE)
				{
					pBlock = pBlock->getParent();
				}
				css::ICSSstyle * pStyle = m_pStyle = m_pParent->getNode() ? m_pParent->getNode()->getStyle() : m_pParent->getParent()->getNode()->getStyle();
				UINT iTextSize = pStyle->font_size->getPX(pBlock->getContentHeight(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
				const WCHAR * szFontName = pStyle->font_name->getString();

				bool isOneLine = (pStyle->_gui_text_appearance->isSet() && pStyle->_gui_text_appearance->getInt() == css::ICSSproperty::_GUI_TEXT_APPEARANCE_ONE_LINE);

				css::ICSSproperty::TYPE_FONT_STYLE_e fontStyle = (css::ICSSproperty::TYPE_FONT_STYLE_e)pStyle->font_style->getInt();
				css::ICSSproperty::TYPE_FONT_WEIGHT_e fontWeight = (css::ICSSproperty::TYPE_FONT_WEIGHT_e)pStyle->font_weight->getInt();
				css::ICSSproperty::TYPE_TEXT_ALIGN_e _textAlign = (css::ICSSproperty::TYPE_TEXT_ALIGN_e)pStyle->text_align->getInt();
				css::ICSSproperty::TYPE_TEXT_DECORATION_LINE_e _textDecoration = (css::ICSSproperty::TYPE_TEXT_DECORATION_LINE_e)pStyle->text_decoration_line->getInt();

				m_iXpos = m_pParent->getContentLeft();
				m_iYpos = m_pParent->getContentTop();

				//m_pColor = pStyle->p_color.getColor();

				CFont::TEXT_ALIGN textAlign;
				switch(_textAlign)
				{
				case css::ICSSproperty::TEXT_ALIGN_CENTER:
					textAlign = CFont::TEXT_ALIGN_CENTER;
					break;
				case css::ICSSproperty::TEXT_ALIGN_RIGHT:
					textAlign = CFont::TEXT_ALIGN_RIGHT;
					break;
				default:
					textAlign = CFont::TEXT_ALIGN_LEFT;
				}

				UINT textDecoration = CFont::DECORATION_NONE;
				switch(_textDecoration)
				{
				case css::ICSSproperty::TEXT_DECORATION_LINE_LINE_THROUGH:
					textDecoration = CFont::DECORATION_LINE_THROUGH;
					break;
				case css::ICSSproperty::TEXT_DECORATION_LINE_OVERLINE:
					textDecoration = CFont::DECORATION_OVERLINE;
					break;
				case css::ICSSproperty::TEXT_DECORATION_LINE_UNDERLINE:
					textDecoration = CFont::DECORATION_UNDERLINE;
					break;
				}

				UINT m_iAreaWidth = pBlock->getContentWidth();

				CFont::STYLE style = (CFont::STYLE)((fontStyle == css::ICSSproperty::FONT_STYLE_ITALIC ? CFont::STYLE_ITALIC : 0) | (fontWeight == css::ICSSproperty::FONT_WEIGHT_BOLD ? CFont::STYLE_BOLD : 0));

				pFont = m_pDoc->getDesktopStack()->getFontManager()->getFont(szFontName, iTextSize, style);

				int iShadowX = 0;
				int iShadowY = 0;
				int iShadowRadius = 0;
				pShadowFont = NULL;
				if(pStyle->text_shadow_offset_x->isSet())
				{
					iShadowX = pStyle->text_shadow_offset_x->getPX(pBlock->getContentWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					if(pStyle->text_shadow_offset_y->isSet())
					{
						iShadowY = pStyle->text_shadow_offset_y->getPX(pBlock->getContentWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					}
					if(pStyle->text_shadow_blur_radius->isSet())
					{
						iShadowRadius = pStyle->text_shadow_blur_radius->getPX(pBlock->getContentWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					}

					pShadowFont = m_pDoc->getDesktopStack()->getFontManager()->getFont(szFontName, iTextSize, style, iShadowRadius);
				}

				struct word
				{
					UINT start_pos;
					UINT length;
				};

				word * pWords = new word[m_iWordCount];
				UINT _len = 0;
				UINT _pos = 0;
				UINT _idx = 0;
				// FIXME: move that into setText method
				for(UINT i = 0, l = m_szClearText.length(); i < l; i++)
				{
					if(iswspace(m_szClearText[i]))
					{
						pWords[_idx].start_pos = _pos;
						pWords[_idx].length = _len;
						//pWords[_idx] = {_pos, _len};
						_idx++;
						_len = 1;
						_pos = i;
					}
					else
					{
						_len++;
					}
				}
				pWords[_idx].start_pos = _pos;
				pWords[_idx].length = _len;
				//pWords[_idx] = {_pos, _len};

				StringW tmp;
				UINT _w, _h;
				UINT _cw = pBlock->getLeftPos();
				UINT fo = _cw;
				UINT breaks = 0;
				CRenderElement rel;
				UINT sh = 0;
				pBlock->textStart();
				m_pRenderElems.clearFast();
				//m_pRenderElemsShadow.clearFast();
				CFont::char_rects cr;
				m_vCharRects.clearFast();
				cr.pResArray = &m_vCharRects;
				UINT icsh = 0;
				bool isFirstWord = false;
				UINT crc;
				UINT fsym = 0;
				for(UINT i = 0; i < m_iWordCount; i++)
				{
					tmp = m_szClearText.substr(pWords[i].start_pos, pWords[i].length);
					cr.iXoffs = _cw - icsh;
			//		icsh = 0;
					isFirstWord = false;
					cr.iYoffs = sh;
					crc = m_vCharRects.size();
					pFont->getStringMetrics(tmp, &_w, &_h, NULL, NULL, NULL, &cr);
					
					if(_cw + _w >= m_iAreaWidth && (i > 0 || _cw != 0) && !isOneLine)
					{
						//m_szClearText[pWords[i].start_pos] = L'\n'
						UINT isw = 0;
						if(rel.m_szStr.length())
						{
							pFont->buildString(rel.m_szStr, textDecoration, CFont::TEXT_ALIGN_LEFT, &rel.m_pRenderBuffer, &rel.m_pIndexBuffer, &rel.m_iVertexCount, &rel.m_iIndexBaseCount, &rel.m_iIndexAddCount, m_iAreaWidth, 0, &isw);

							rel.m_iLeftOffset = breaks == 0 ? fo : 0;
							rel.m_iTopOffset = breaks > 0 ? iTextSize : 0;
							rel.m_iHeight = iTextSize;
							rel.m_iWIdth = isw;
							rel.m_prc = &m_vCharRects;
							rel.m_iFirstSym = fsym;
							rel.m_iLastSym = crc;
							rel.m_iLineIdx = pBlock->textGetLineIdx();


							if(pShadowFont)
							{
								rel.m_pNextREl = NULL;
								rel.m_pNextREl = new CRenderElement(rel);
								if(iShadowRadius > 0)
								{
									pShadowFont->buildString(rel.m_szStr, CFont::DECORATION_NONE, CFont::TEXT_ALIGN_LEFT, &rel.m_pNextREl->m_pRenderBuffer, &rel.m_pNextREl->m_pIndexBuffer, &rel.m_pNextREl->m_iVertexCount, &rel.m_pNextREl->m_iIndexBaseCount, &rel.m_pNextREl->m_iIndexAddCount, m_iAreaWidth, 0, &isw);
								}
								else
								{
									rel.m_pIndexBuffer->AddRef();
									rel.m_pRenderBuffer->AddRef();
								}
								rel.m_pNextREl->m_iLeftOffset = iShadowX;
								rel.m_pNextREl->m_iTopOffset = iShadowY;


								//m_pRenderElemsShadow.push_back(rel);
							}
							m_pRenderElems.push_back(rel);

						}
						sh += _h;

						bool applycsh = icsh == 0;

						icsh = m_vCharRects[crc].right - m_vCharRects[crc].left;
						
						for(UINT o = crc; o < m_vCharRects.size(); o++)
						{
							m_vCharRects[o].top += _h;
							m_vCharRects[o].bottom += _h;
							m_vCharRects[o].right -= _cw + (applycsh ? icsh : 0);
							m_vCharRects[o].left -= _cw + (applycsh ? icsh : 0);
						}
						m_vCharRects[crc].bottom -= m_vCharRects[crc].top;
						m_vCharRects[crc].top = m_vCharRects[crc - 1].top;
						m_vCharRects[crc].bottom += m_vCharRects[crc].top;

						m_vCharRects[crc].right -= m_vCharRects[crc].left;
						m_vCharRects[crc].left = m_vCharRects[crc - 1].right;
						m_vCharRects[crc].right += m_vCharRects[crc].left;

						pBlock->textBreak();
						_cw = 0;
						rel.m_szStr = tmp.substr(i > 0 ? 1 : 0);
						isFirstWord = true;
						fsym = crc + 1;
						breaks++;
					}
					else
					{
						rel.m_szStr += tmp;
					}
					_cw += _w;
				}
				UINT isw = 0;
				if(rel.m_szStr.length())
				{
					pFont->buildString(rel.m_szStr, textDecoration, CFont::TEXT_ALIGN_LEFT, &rel.m_pRenderBuffer, &rel.m_pIndexBuffer, &rel.m_iVertexCount, &rel.m_iIndexBaseCount, &rel.m_iIndexAddCount, m_iAreaWidth, 0, &isw);
					rel.m_iLeftOffset = breaks == 0 ? fo : 0;
					rel.m_iTopOffset = breaks > 0 ? iTextSize : 0;
					rel.m_iHeight = iTextSize;
					rel.m_iWIdth = isw;
					rel.m_prc = &m_vCharRects;
					rel.m_iFirstSym = fsym;
					rel.m_iLastSym = m_vCharRects.size() - 1;
					rel.m_iLineIdx = pBlock->textGetLineIdx();

					if(pShadowFont)
					{
						rel.m_pNextREl = NULL;
						rel.m_pNextREl = new CRenderElement(rel);
						if(iShadowRadius > 0)
						{
							pShadowFont->buildString(rel.m_szStr, CFont::DECORATION_NONE, CFont::TEXT_ALIGN_LEFT, &rel.m_pNextREl->m_pRenderBuffer, &rel.m_pNextREl->m_pIndexBuffer, &rel.m_pNextREl->m_iVertexCount, &rel.m_pNextREl->m_iIndexBaseCount, &rel.m_pNextREl->m_iIndexAddCount, m_iAreaWidth, 0, &isw);
						}
						else
						{
							rel.m_pIndexBuffer->AddRef();
							rel.m_pRenderBuffer->AddRef();
						}
						rel.m_pNextREl->m_iLeftOffset = iShadowX;
						rel.m_pNextREl->m_iTopOffset = iShadowY;
					}
					m_pRenderElems.push_back(rel);
							
				}

				for(UINT i = 0; i < m_pRenderElems.size(); i++)
				{
					pBlock->textAppend(&m_pRenderElems[i], m_pRenderElems[i].m_iLineIdx);
				}

				pBlock->setLeftPos(_cw);

	//@@		/	m_pParent->setHeight(sh);
			//	pFont->buildString(m_szClearText, textDecoration, textAlign, &m_pVertexBuffer, &m_pIndexBuffer, &m_iVertexCount, &m_iIndexBaseCount, &m_iIndexAddCount, m_iAreaWidth, fo);
				//m_pParent->setTopPos(m_iYpos - iTextSize);
				mem_delete_a(pWords);
				return(iTextSize * (breaks));
			}

			UINT IRenderTextNew::getCaretPos()
			{
				return(m_iCaretPos);
			}

			void IRenderTextNew::setSelectionStart(int cp)
			{
				int maxPos = m_vCharRects.size();
				if(cp > maxPos)
				{
					cp = maxPos;
				}
				if(cp < 0)
				{
					cp = 0;
				}
				m_iSelectionStart = cp;
			}
			void IRenderTextNew::setSelectionEnd(int cp)
			{
				int maxPos = m_vCharRects.size();
				if(cp > maxPos)
				{
					cp = maxPos;
				}
				if(cp < 0)
				{
					cp = 0;
				}
				m_iSelectionEnd = cp;
			}

			void IRenderTextNew::setCaretPos(int cp, bool force)
			{
				int maxPos = m_vCharRects.size();
				if(cp > maxPos && !force)
				{
					cp = maxPos;
				}
				if(cp < 0)
				{
					cp = 0;
				}
				if(m_bInSelection)
				{
					m_iSelectionEnd = cp;
				}
				else
				{
					m_iSelectionStart = m_iSelectionEnd = cp;
				}
				m_iCaretPos = cp;
			}

			UINT IRenderTextNew::getCaretMaxPos()
			{
				return(m_vCharRects.size());
			}

			void IRenderTextNew::moveCaretPos(int shift)
			{
				setCaretPos((int)m_iCaretPos + shift);
			}

			void IRenderTextNew::drawCaret()
			{
				if(!m_pNode || !m_pNode->parentNode())
				{
					return;
				}
				if(m_pNode->parentNode()->getStyle()->_gui_text_cursor->getInt() == css::ICSSproperty::_GUI_TEXT_CURSOR_SHOW)
				{
					IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

					int iTextSize = m_pNode->parentNode()->getStyle()->font_size->getPX(m_pParent->getInnerHeight(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					m_iTextSize = iTextSize;
					float4_t color = m_pNode->parentNode()->getStyle()->color->getColor();
					struct point
					{
						float x;
						float y;
						float z;
					};

					point a[] = {
						{0, 0, 0},
						{1.0f, 0, 0},
						{1.0f, (float)(iTextSize), 0},
						{0, (float)(iTextSize), 0}
					};

					/*m_iCaretPos = 40;*/
					bool bLastPos = m_iCaretPos >= m_vCharRects.size();
					UINT iUseIndex = m_iCaretPos;
					if(bLastPos)
					{
						iUseIndex = m_vCharRects.size() - 1;
					}

					int _x = 0;
					int _y = 0;
					if(m_vCharRects.size() == 0)
					{
						iUseIndex = 0;
					}
					else
					{
						if(bLastPos)
						{
							_x = m_vCharRects[iUseIndex].right;
						}
						else
						{
							_x = m_vCharRects[iUseIndex].left;
						}
						_y = m_vCharRects[iUseIndex].top;
					}

					auto shader = GetGUI()->getShaders()->m_baseColored;
					SGCore_ShaderBind(shader.m_idShaderKit);

					//float op = sinf((float)GetTickCount() * 0.007f);
					float op = sinf((float)GetTickCount() * 0.003f);
					op *= op;
					color.w = op;
					SGCore_ShaderSetVRF(SHADER_TYPE_PIXEL, shader.m_idPS, "g_vColor", (float*)&color, 1);
				//	DX_CALL(GetGUI()->getDevice()->SetPixelShaderConstantF(0, (float*)&color, 1));
					m_pDoc->getTranslationManager()->pushMatrix(SMMatrixTranslation(_x - 1.0f, _y, 0.0f));
				//	DX_CALL(GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ));
					IGXRenderBuffer *pRB = m_pDoc->getDesktopStack()->getQuadRenderBufferXYZ((float3_t*)a);
					pCtx->setRenderBuffer(pRB);
					pCtx->setIndexBuffer(GetGUI()->getQuadIndexBuffer());
					pCtx->drawIndexed(4, 2, 0, 0);
				//	DX_CALL(GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point)));
					m_pDoc->getTranslationManager()->popMatrix();

					int _w = m_pParent->getInnerWidth();
					int _scrollX = m_pParent->getScrollLeft();
					if(_x < _scrollX)
					{
						_scrollX = _x - 5;
					}
					if(_x > _w + _scrollX)
					{
						_scrollX = _x - _w + 10;
					}
					m_pParent->setScrollLeft(_scrollX);

					int _h = m_pParent->getInnerHeight();
					int _scrollY = m_pParent->getScrollTop();
					if(_y < _scrollY)
					{
						_scrollY = _y - 2;
					}
					if(_y + iTextSize > _h + _scrollY)
					{
						_scrollY = _y + iTextSize - _h + 7;
					}
					m_pParent->setScrollTop(_scrollY);

					m_iCaretX = _x;
					m_iCaretY = _y;
					

					/*for(UINT i = 0; i < m_vCharRects.size(); i++)
					{
						int _x = m_vCharRects[i].left;
						int _y = m_vCharRects[i].top;
						CTranslationManager::pushMatrix(SMMatrixTranslation(_x - 1.0f, _y, 0.0f));
						GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point));
						CTranslationManager::popMatrix();
					}
					_x = m_vCharRects[m_vCharRects.size()-1].right;
					_y = m_vCharRects[m_vCharRects.size() - 1].top;
					CTranslationManager::pushMatrix(SMMatrixTranslation(_x - 1.0f, _y, 0.0f));
					GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point));
					CTranslationManager::popMatrix();*/
				}
			}

			void IRenderTextNew::placeCaretByXY(int x, int y)
			{
				if(y < 0)
				{
					setCaretPos(0);
					return;
				}
				for(UINT i = 0; i < m_vCharRects.size(); i++)
				{
					if(m_vCharRects[i].bottom > y && m_vCharRects[i].top <= y)
					{
						if(x < m_vCharRects[i].left)
						{
							setCaretPos(i);
							return;
						}
						if(x > m_vCharRects[i].right && i < m_vCharRects.size() - 1 && m_vCharRects[i + 1].top != m_vCharRects[i].top)
						{
							setCaretPos(i);
							return;
						}
						if(x >= m_vCharRects[i].left && x <= m_vCharRects[i].right)
						{
							int _w = m_vCharRects[i].right - m_vCharRects[i].left;
							int dw = x - m_vCharRects[i].left;
							if(dw < _w / 2)
							{
								setCaretPos(i);
							}
							else
							{
								setCaretPos(i + 1);
							}
							return;
						}
					}
				}
				setCaretPos(m_vCharRects.size());
			}

			UINT IRenderTextNew::getSelectionStart()
			{
				return(min(m_iSelectionEnd, m_iSelectionStart));
			}
			UINT IRenderTextNew::getSelectionEnd()
			{
				return(max(m_iSelectionEnd, m_iSelectionStart));
			}

			StringW IRenderTextNew::getSelectionText()
			{
				return(m_szClearText.substr(getSelectionStart(), getSelectionEnd() - getSelectionStart()));
			}

			void IRenderTextNew::moveCaretLine(int shift)
			{
				placeCaretByXY(m_iCaretX, m_iCaretY + m_iTextSize * shift);
			}

			void IRenderTextNew::renderSelection()
			{
			//	m_iSelectionStart = 0;
			//	m_iSelectionEnd = 2;
				if(m_iSelectionEnd == m_iSelectionStart)
				{
					return;
				}
				if(!m_pNode || !m_pNode->parentNode())
				{
					return;
				}
				if(m_pNode->parentNode()->getStyle()->_gui_text_cursor->getInt() == css::ICSSproperty::_GUI_TEXT_CURSOR_SHOW)
				{
					IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

					UINT iTextSize = m_pNode->parentNode()->getStyle()->font_size->getPX(m_pParent->getInnerHeight(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
					m_iTextSize = iTextSize;
					float4_t color = float4_t(0.0f, 0.0f, 1.0f, 0.5f);
					struct point
					{
						float x;
						float y;
						float z;
					};


					point a[] = {
						{0, 0, 0},
						{1.0f, 0, 0},
						{1.0f, (float)(iTextSize), 0},
						{0, (float)(iTextSize), 0}
					};

					UINT selStart = min(m_iSelectionEnd, m_iSelectionStart);
					UINT selEnd = max(m_iSelectionEnd, m_iSelectionStart);

					if(selEnd > m_vCharRects.size())
					{
						selEnd = m_vCharRects.size();
					}

					auto shader = GetGUI()->getShaders()->m_baseColored;
					SGCore_ShaderBind(shader.m_idShaderKit);

					SGCore_ShaderSetVRF(SHADER_TYPE_PIXEL, shader.m_idPS, "g_vColor", (float*)&color, 1);
				//	DX_CALL(GetGUI()->getDevice()->SetPixelShaderConstantF(0, (float*)&color, 1));
				//	DX_CALL(GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ));
					
					pCtx->setIndexBuffer(GetGUI()->getQuadIndexBuffer());

					for(UINT i = selStart; i < selEnd; i++)
					{
						a[0].x = m_vCharRects[i].left;
						a[1].x = m_vCharRects[i].right;
						a[2].x = a[1].x;
						a[3].x = a[0].x;

						a[0].y = m_vCharRects[i].top;
						a[1].y = a[0].y;
						a[3].y = m_vCharRects[i].bottom;
						a[2].y = a[3].y;

						IGXRenderBuffer *pRB = m_pDoc->getDesktopStack()->getQuadRenderBufferXYZ((float3_t*)a);
						pCtx->setRenderBuffer(pRB);

						pCtx->drawIndexed(4, 2, 0, 0);
					}
				}
			}

			//##########################################################################

			IRenderImageBlock::IRenderImageBlock(CDOMnode * pNode, IRenderFrame * pRootFrame):BaseClass(pNode, pRootFrame)
			{
			}

			UINT IRenderImageBlock::layout(bool changed)
			{
				static UINT nIMG = CDOMnode::getNodeIdByName(L"img");
				static UINT nVIDEO = CDOMnode::getNodeIdByName(L"video");

				css::ICSSstyle * style = m_pNode->getStyle();

				if(m_pNode->getNodeId() == nVIDEO)
				{
#ifndef _GUI_NO_VIDEO
					StringW src = ((IVIDEO*)m_pNode)->initRenderer();
					style->background_image->set(src);
#endif
				}
				else
				{
					style->background_image->set(m_pNode->getAttribute(L"src"));
				}

				
				
				style->background_attachment->set(css::ICSSproperty::BACKGROUND_ATTACHMENT_LOCAL);
				style->background_position_x->set(0);
				style->background_position_y->set(0);
				IXTexture *tex;
				m_pDoc->getDesktopStack()->getTextureManager()->getTexture(style->background_image->getString(), &tex);

				int _w = tex->getWidth();
				int _h = tex->getHeight();

				mem_release(tex);

				StringW a = m_pNode->getAttribute(L"width");
				if(a.length())
				{
					_w = a.toInt();
				}
				a = m_pNode->getAttribute(L"height");
				if(a.length())
				{
					_h = a.toInt();
				}

				if(style->width->isSet())
				{
					_w = style->width->getPX(m_pParent->getWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
				}


				if(style->height->isSet())
				{
					_h = style->height->getPX(m_pParent->getHeight(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
				}

				bool _ch = !((css::CCSSproperty*)style->height)->isChanged();
				bool _cw = !((css::CCSSproperty*)style->width)->isChanged();

				style->width->set((float)(m_iWidth = _w));
				style->height->set((float)(m_iHeight = _h));
				style->width->setDim(css::ICSSproperty::DIM_PX);
				style->height->setDim(css::ICSSproperty::DIM_PX);

	//			if(_ch)
				{
					((css::CCSSproperty*)style->height)->unChange();
				}
	//			if(_cw)
				{
					((css::CCSSproperty*)style->width)->unChange();
				}

				style->background_size_x->set(100.0f);
				style->background_size_x->setDim(css::ICSSproperty::DIM_PC);
				style->background_size_y->set(100.0f);
				style->background_size_y->setDim(css::ICSSproperty::DIM_PC);
				return(BaseClass::layout());
			}

			void IRenderImageBlock::render(UINT lvl, float fDT)
			{
				BaseClass::render(lvl, fDT);
			}

			//##########################################################################

			IRenderImageInlineBlock::IRenderImageInlineBlock(CDOMnode * pNode, IRenderFrame * pRootFrame) : BaseClass(pNode, pRootFrame)
			{
				
			}

			UINT IRenderImageInlineBlock::layout(bool changed)
			{
				static UINT nIMG = CDOMnode::getNodeIdByName(L"img");
				static UINT nVIDEO = CDOMnode::getNodeIdByName(L"video");

				css::ICSSstyle * style = m_pNode->getStyle();

				if(m_pNode->getNodeId() == nVIDEO)
				{
#ifndef _GUI_NO_VIDEO
					StringW src = ((IVIDEO*)m_pNode)->initRenderer();
					style->background_image->set(src);
#endif
				}
				else
				{
					style->background_image->set(m_pNode->getAttribute(L"src"));
				}

				style->background_attachment->set(css::ICSSproperty::BACKGROUND_ATTACHMENT_LOCAL);
				style->background_position_x->set(0);
				style->background_position_y->set(0);
				IXTexture *tex;
				m_pDoc->getDesktopStack()->getTextureManager()->getTexture(style->background_image->getString(), &tex);

				int _w = tex->getWidth();
				int _h = tex->getHeight();

				mem_release(tex);

				StringW a = m_pNode->getAttribute(L"width");
				if(a.length())
				{
					_w = a.toInt();
				}
				a = m_pNode->getAttribute(L"height");
				if(a.length())
				{
					_h = a.toInt();
				}

				if(style->width->isSet())
				{
					_w = style->width->getPX(m_pParent->getWidth(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
				}


				if(style->height->isSet())
				{
					_h = style->height->getPX(m_pParent->getHeight(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());
				}

				bool _ch = !((css::CCSSproperty*)style->height)->isChanged();
				bool _cw = !((css::CCSSproperty*)style->width)->isChanged();

				style->width->set((float)(m_iWidth = _w));
				style->height->set((float)(m_iHeight = _h));
				style->width->setDim(css::ICSSproperty::DIM_PX);
				style->height->setDim(css::ICSSproperty::DIM_PX);

		//		if(_ch)
				{
					((css::CCSSproperty*)style->height)->unChange();
				}
		//		if(_cw)
				{
					((css::CCSSproperty*)style->width)->unChange();
				}


				style->background_size_x->set(100.0f);
				style->background_size_x->setDim(css::ICSSproperty::DIM_PC);
				style->background_size_y->set(100.0f);
				style->background_size_y->setDim(css::ICSSproperty::DIM_PC);
				return(BaseClass::layout());
			}

			void IRenderImageInlineBlock::render(UINT lvl, float fDT)
			{
				BaseClass::render(lvl, fDT);
			}

			//##########################################################################

			UINT IRenderSelectBlock::layout(bool changed)
			{
#if 0
				m_pDoc->addReflowItem(m_pOptionsFrame);
#endif 

				UINT iHeight = BaseClass::layout(changed);
				return(iHeight);
			}

			void IRenderSelectBlock::onCreated()
			{
				IRenderSelectOptionsBlock * pFrame = new IRenderSelectOptionsBlock(this, m_pRootNode);

				for(UINT i = 0, l = m_pChilds.size(); i < l; ++i)
				{
					mem_delete(m_pChilds[i]);
					//pFrame->addChild(m_pChilds[i]);
				}
				m_pChilds.clear();


				CDOMnode * pTextNode = (CDOMnode*)CDOMnode::createNode(L"text");
				pTextNode->setDocument(m_pNode->getDocument());

				const IDOMnodeCollection &nodeChildren = *(m_pNode->getChilds());
				StringW sCurrentValue = m_pNode->getAttribute(L"value");
				bool bFound = false;
				for(int i = 0, l = nodeChildren.size(); i < l; ++i)
				{
					if(sCurrentValue == nodeChildren[i]->getAttribute(L"value"))
					{
						pTextNode->setText(nodeChildren[i]->getText());
						bFound = true;
						break;
					}
				}
				if(!bFound && nodeChildren.size())
				{
					pTextNode->setText(nodeChildren[0]->getText());
				}

				m_pNode->appendChild(pTextNode, false, nodeChildren[0]);

				IRenderTextNew * pTextFrame = new IRenderTextNew(pTextNode, m_pRootNode);

				addChild(pTextFrame);

				IRenderFrame * pCur = this;
				while(pCur->getParent() && pCur->getNode()->getStyle()->position->getInt() == css::ICSSproperty::POSITION_STATIC)
				{
					pCur = pCur->getParent();
				}
				pCur->addChild(pFrame, true);

				m_pOptionsFrame = pFrame;

				BaseClass::onCreated();
			}

			//##########################################################################

			IRenderSelectOptionsBlock::IRenderSelectOptionsBlock(IRenderFrame * pSelectFrame, IRenderFrame * pRootFrame):
				BaseClass(pSelectFrame->getNode(), pRootFrame),
				m_pSelectFrame(pSelectFrame)
			{
			}

			UINT IRenderSelectOptionsBlock::layout(bool changed)
			{
				m_iXpos = m_pSelectFrame->getClientLeft() - m_pParent->getClientLeft();
				m_iYpos = m_pSelectFrame->getClientTop() - m_pParent->getClientTop() + m_pSelectFrame->getHeight()
					- m_pSelectFrame->getNode()->getStyle()->margin_bottom->getPX(m_pSelectFrame->getParent()->getInnerHeight(), m_pDoc->getDesktopStack()->getScreenWidth(), m_pDoc->getDesktopStack()->getScreenHeight());

				m_iWidth = m_pSelectFrame->getWidth();
				m_iHeight = BaseClass::BaseClass::layout(changed);

				if(!m_pSelectFrame->getNode()->pseudoclassExists(css::ICSSrule::PSEUDOCLASS_CHECKED))
				{
					m_iHeight = 0;
				}
				else
				{
					if(m_iHeight > 300)
					{
						m_iScrollTopMax = m_iHeight - 250;
						m_iHeight = 250;
						m_bNeedCut = true;
					}
				}

				m_border.setContentSize(m_iWidth, m_iHeight);
				m_border.build();
				//m_iHeight = m_pSelectFrame->getNode()->getStyle()->max_height->getPX(1);

				return(m_iHeight);
			}

			void IRenderSelectOptionsBlock::render(UINT lvl, float fDT)
			{
				/*if(!m_pSelectFrame->getNode()->pseudoclassExists(css::ICSSrule::PSEUDOCLASS_CHECKED))
				{
					return;
				}*/
				BaseClass::render(lvl, fDT);
			}
		};
	};
};
