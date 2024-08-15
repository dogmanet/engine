#include "ScrollBarSimple.h"
#include "IRenderFrame.h"
#include "DOMdocument.h"

namespace gui
{
	namespace dom
	{
		namespace render
		{
			CScrollBarSimple::CScrollBarSimple(IRenderFrame * _parent, SCROLLBAR_DIR _dir):
				IScrollBar(_parent, _dir)
			{
				m_pVertices = GetGUI()->getDevice()->createVertexBuffer(sizeof(point) * 4, GXBUFFER_USAGE_STREAM);
				m_pRenderBuffer = GetGUI()->getDevice()->createRenderBuffer(1, &m_pVertices, GetGUI()->getVertexDeclarations()->m_pXYZTex);
			}

			CScrollBarSimple::~CScrollBarSimple()
			{
				mem_release(m_pVertices);
				mem_release(m_pRenderBuffer);
			}
			
			int CScrollBarSimple::getWidth()
			{
				return(16);
			}

			void CScrollBarSimple::render()
			{
				
				static IXTexture *texWhite = m_pParent->getNode()->getDocument()->getDesktopStack()->getTextureManager()->getWhite();
			//	static CSHADER shText = CTextureManager::loadShader(L"text");
			//	GetGUI()->getDevice()->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);

				IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();
				auto shader = GetGUI()->getShaders()->m_baseTexturedColored;
				GetGUI()->getRender()->bindShader(pCtx, shader.m_idShaderKit);

			//	CTextureManager::bindShader(shText);
				m_pParent->getNode()->getDocument()->getDesktopStack()->getTextureManager()->bindTexture(texWhite);
			//	float4_t asd(1, 1, 1, 0.5);

				static IGXConstantBuffer *s_pColorConstant = NULL;
				if(!s_pColorConstant)
				{
					s_pColorConstant = GetGUI()->getDevice()->createConstantBuffer(sizeof(float4));
					s_pColorConstant->update(&float4_t(1, 1, 1, 0.5));
				}


				pCtx->setPSConstant(s_pColorConstant);
				//SGCore_ShaderSetVRF(SHADER_TYPE_PIXEL, shader.m_idPS, "g_vColor", (float*)&float4_t(1, 1, 1, 0.5), 1);
			//	DX_CALL(GetGUI()->getDevice()->SetPixelShaderConstantF(0, (float*)&asd, 1));

				updateData();
				if(m_iScrollMax == 0)
				{
					return;
				}
				int P = 4; // Padding 4px
				int W = getWidth();
				int S = m_iLength * (m_iLength - P) / (m_iScrollMax + m_iLength);
				int T = m_iScrollCur * (m_iLength - 2 * P - S) / m_iScrollMax;

				point *pData;
				if(m_pVertices->lock((void**)&pData, GXBL_WRITE))
				{
					pData[0] = {(float)P, (float)(P + T), 0.0f, 0.0f, 0.0f};
					pData[1] = {(float)(W - P), (float)(P + T), 0.0f, 0.0f, 1.0f};
					pData[2] = {(float)(W - P), (float)(P + T + S), 0.0f, 1.0f, 1.0f};
					pData[3] = {(float)P, (float)(P + T + S), 0.0f, 1.0f, 0.0f};

					if(m_eDir == SCROLLBAR_DIR_HORIZONTAL)
					{
						float _x;
						for(int i = 0; i < 4; i++)
						{
							_x = pData[i].x;
							pData[i].x = pData[i].y;
							pData[i].y = _x;
						}
					}

					m_pVertices->unlock();
				}

				
				pCtx->setRenderBuffer(m_pRenderBuffer);
				pCtx->setIndexBuffer(GetGUI()->getQuadIndexBuffer());
				pCtx->drawIndexed(4, 2, 0, 0);
			//	DX_CALL(GetGUI()->getDevice()->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point)));
			}

			void CScrollBarSimple::dispatchEvent(IEvent & ev)
			{
				if(m_eDir == SCROLLBAR_DIR_VERTICAL && CKeyMap::keyState(KEY_LBUTTON))
				{
					if(ev.type == GUI_EVENT_TYPE_MOUSEMOVE)
					{
						if(!m_bDragging)
						{
							if(ev.offsetX > (int)m_pParent->getContentWidth() - getWidth())
							{
								m_bDragging = true;
								m_iDragPos = ev.clientY;
								m_iScrollStart = m_iScrollCur;
							}
						}
						else
						{
							int P = 4; // Padding 4px
							//int S = m_iLength * (m_iLength - P) / m_iScrollMax;

							int S = m_iLength * (m_iLength - P) / (m_iScrollMax + m_iLength);

							int delta = ev.clientY - m_iDragPos;
							m_pParent->setScrollTop(m_iScrollStart + delta * m_iScrollMax / (m_iLength - 2 * P - S));
							//m_pParent->setScrollTop(m_iScrollStart + delta * (m_iLength - 2 * P) / m_iScrollMax);
						}
					}
				}
				else
				{
					m_bDragging = false;
				}
				//do nothing now
			}

		};
	};
};