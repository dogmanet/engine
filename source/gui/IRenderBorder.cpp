#include "IRenderBorder.h"

namespace gui
{
	namespace dom
	{
		namespace render
		{
			IRenderBorder::IRenderBorder(CTextureManager *pTextureManager):
				m_pTextureManager(pTextureManager)
			{
				for(UINT i = 0; i < 4; i++)
				{
					m_iWidth[i] = 0;
					m_iRadius[i] = 0;
					m_method[i] = CORNER_METHOD_RADIUS;
					m_iIndexStart[i] = 0;
					m_iIndexCount[i] = 0;
					//m_iColor[i] = 0xFF000000;
					m_pColor[i] = float4_t(0.0f, 0.0f, 0.0f, 0.0f);
					m_iVertexCount[i] = 0;
					m_iVertexStart[i] = 0;
					m_iSideCount[i] = 0;
					m_pColorsConstant[i] = GetGUI()->getDevice()->createConstantBuffer(sizeof(float4));
					m_pColorsConstant[i]->update(&m_pColor[i]);
				}

				m_pColorBlack = GetGUI()->getDevice()->createConstantBuffer(sizeof(float4));
				m_pColorBlack->update(&float4(0.0f, 0.0f, 0.0f, 0.0f));
				m_pColorWhite = GetGUI()->getDevice()->createConstantBuffer(sizeof(float4));
				m_pColorWhite->update(&float4(1.0f, 1.0f, 1.0f, 1.0f));
			}

			IRenderBorder::~IRenderBorder()
			{
				mem_release(m_pIndexBuffer);
				mem_release(m_pRenderBuffer);
				mem_release(m_pVertexBuffer);
				mem_release(m_pColorBlack);
				mem_release(m_pColorWhite);

				for(UINT i = 0; i < 4; i++)
				{
					mem_release(m_pColorsConstant[i]);
				}
			}

			void IRenderBorder::setWidth(SIDE s, UINT w)
			{
				m_iWidth[s] = w;
			}

			void IRenderBorder::setRadius(CORNER c, UINT r)
			{
				m_iRadius[c] = r;
			}

			void IRenderBorder::setCornerMethod(CORNER c, CORNER_METHOD m)
			{
				m_method[c] = m;
			}


			void IRenderBorder::setContentSize(UINT w, UINT h)
			{
				m_iInnerWidth = w;
				m_iInnerHeight = h;
			}


			void IRenderBorder::setColor(SIDE s, float4_t clr)
			{
				m_pColorsConstant[s]->update(&clr);
				m_pColor[s] = clr;
			}


			UINT IRenderBorder::getWidth(SIDE s)
			{
				return(m_iWidth[s]);
			}

			UINT IRenderBorder::getRadius(CORNER c)
			{
				return(m_iRadius[c]);
			}

			IRenderBorder::CORNER_METHOD IRenderBorder::getCornerMethod(CORNER c)
			{
				return(m_method[c]);
			}

			float4_t IRenderBorder::getColor(SIDE s)
			{
				return(m_pColor[s]);
			}


			void IRenderBorder::getContentSize(UINT * w, UINT * h)
			{
				if(w)
				{
					*w = m_iInnerWidth;
				}

				if(h)
				{
					*h = m_iInnerHeight;
				}
			}

			void IRenderBorder::render()
			{
				if(!m_pVertexBuffer)
				{
					return;
				}
				static IXTexture *texWhite = m_pTextureManager->getWhite();

				IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

			//	GetGUI()->getDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

				m_pTextureManager->bindTexture(texWhite);
			//	CTextureManager::bindShader(shText);
				auto shader = GetGUI()->getShaders()->m_baseColored;
				GetGUI()->getRender()->bindShader(pCtx, shader.m_idShaderKit);
				bool need = false;
				for(UINT i = 0; i < 4; i++)
				{
					if(m_iWidth[i] != 0)
					{
						need = true;
					}
				}
				if(need)
				{
				//	pCtx->SetFVF(D3DFVF_XYZ);
					pCtx->setRenderBuffer(m_pRenderBuffer);
					pCtx->setIndexBuffer(m_pIndexBuffer);
				}

				//	UINT iVC = 0;
				//	UINT iIC = 0;
				for(UINT i = 0; i < 4; i++)
				{
					if(m_iWidth[i] != 0)
					{
						pCtx->setPSConstant(m_pColorsConstant[i]);
						pCtx->drawIndexed(m_iVertexCount[i], m_iIndexCount[i] / 3, m_iIndexStart[i], m_iVertexStart[i]);
					}
					//		iVC += m_iVertexCount[i];
					//		iIC += m_iIndexCount[i];
				}
				//	if(m_iWidth[0] > 1)
				{
					//		GetGUI()->getDevice()->SetPixelShaderConstantF(0, (float*)&float4_t(0.0f, 1.0f, 0.0f, 0.4f), 1);
					//		GetGUI()->getDevice()->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, iVC, iIC, m_iFillIndexCount / 3);
				}
				pCtx->setPSConstant(m_pColorBlack);
			}

			void IRenderBorder::renderInnerFill()
			{
				IGXContext *pCtx = GetGUI()->getDevice()->getThreadContext();

				pCtx->setRenderBuffer(m_pRenderBuffer);
				pCtx->setIndexBuffer(m_pIndexBuffer);

				auto shader = GetGUI()->getShaders()->m_baseColored;
				GetGUI()->getRender()->bindShader(pCtx, shader.m_idShaderKit);


				UINT iVC = 0;
				UINT iIC = 0;
				for(UINT i = 0; i < 4; i++)
				{
					iVC += m_iVertexCount[i];
					iIC += m_iIndexCount[i];
				}

				pCtx->setPSConstant(m_pColorWhite);
				pCtx->drawIndexed(iVC, m_iFillIndexCount / 3, iIC, 0);
			}

			void IRenderBorder::build()
			{
				mem_release(m_pIndexBuffer);
				mem_release(m_pVertexBuffer);
				mem_release(m_pRenderBuffer);

				float w = m_iInnerWidth;
				float h = m_iInnerHeight;

				float t = m_iWidth[TOP];
				float l = m_iWidth[LEFT];
				float rt = m_iWidth[RIGHT];
				float bt = m_iWidth[BOTTOM];

				float minr = min((h + t + bt) / 2, (w + l + rt) / 2);
				float r = min((float)m_iRadius[TOP_LEFT], minr);
				float r1 = min((float)m_iRadius[BOTTOM_LEFT], minr);
				float r2 = min((float)m_iRadius[TOP_RIGHT], minr);
				float r3 = min((float)m_iRadius[BOTTOM_RIGHT], minr);

				buff_t tl, bl, tr, br;

				void(*BuildFunc)(IRB_PARAM);

				USHORT ** pIDXs[4];
				USHORT * pIDXC[4];
				for(UINT i = 0; i < 4; i++)
				{
					pIDXC[i] = new USHORT[2];
					pIDXs[i] = new USHORT*[2];
				}

				BuildFunc = (m_method[TOP_LEFT] == CORNER_METHOD_CUT ? buildCut : buildRadius);
				BuildFunc(t, l, r, &tl.vb, &tl.ib, tl.iVC, tl.iIC, tl.br, &m_iSideCount[TOP_LEFT], &pIDXs[TOP_LEFT], &(pIDXC[TOP_LEFT]));
				BuildFunc = (m_method[BOTTOM_LEFT] == CORNER_METHOD_CUT ? buildCut : buildRadius);
				BuildFunc(bt, l, r1, &bl.vb, &bl.ib, bl.iVC, bl.iIC, bl.br, &m_iSideCount[BOTTOM_LEFT], &pIDXs[BOTTOM_LEFT], &pIDXC[BOTTOM_LEFT]);
				BuildFunc = (m_method[TOP_RIGHT] == CORNER_METHOD_CUT ? buildCut : buildRadius);
				BuildFunc(t, rt, r2, &tr.vb, &tr.ib, tr.iVC, tr.iIC, tr.br, &m_iSideCount[TOP_RIGHT], &pIDXs[TOP_RIGHT], &pIDXC[TOP_RIGHT]);
				BuildFunc = (m_method[BOTTOM_RIGHT] == CORNER_METHOD_CUT ? buildCut : buildRadius);
				BuildFunc(bt, rt, r3, &br.vb, &br.ib, br.iVC, br.iIC, br.br, &m_iSideCount[BOTTOM_RIGHT], &pIDXs[BOTTOM_RIGHT], &pIDXC[BOTTOM_RIGHT]);

				for(UINT j = 0; j < 2; j++)
				{
					for(UINT i = 0; i < bl.iVC[j]; i++)
					{
						bl.vb[j][i].y = -bl.vb[j][i].y + bt + t + h;
					}
				}
				for(UINT i = 0; i < 4; i++)
				{
					bl.br[i].y = -bl.br[i].y + bt + t + h;
				}
				for(UINT j = 0; j < 2; j++)
				{
					for(UINT i = 0; i < tr.iVC[j]; i++)
					{
						tr.vb[j][i].x = -tr.vb[j][i].x + l + rt + w;
					}
				}
				for(UINT i = 0; i < 4; i++)
				{
					tr.br[i].x = -tr.br[i].x + l + rt + w;
				}
				for(UINT j = 0; j < 2; j++)
				{
					for(UINT i = 0; i < br.iVC[j]; i++)
					{
						br.vb[j][i].y = -br.vb[j][i].y + bt + t + h;
						br.vb[j][i].x = -br.vb[j][i].x + l + rt + w;
					}
				}
				for(UINT i = 0; i < 4; i++)
				{
					br.br[i].y = -br.br[i].y + bt + t + h;
					br.br[i].x = -br.br[i].x + l + rt + w;
				}

				float3_t cbr[4];

				buff_t tlr, blr, ltb, rtb;


				cbr[0] = tl.br[0];
				cbr[1] = tl.br[2];
				cbr[2] = tr.br[2];
				cbr[3] = tr.br[0];
				concatBuffers(&tlr, &tl, 0, &tr, 0, cbr);

				cbr[0] = bl.br[2];
				cbr[1] = bl.br[0];
				cbr[2] = br.br[0];
				cbr[3] = br.br[2];
				concatBuffers(&blr, &br, 0, &bl, 0, cbr);

				cbr[0] = tl.br[1];
				cbr[1] = bl.br[1];
				cbr[2] = bl.br[3];
				cbr[3] = tl.br[3];
				concatBuffers(&ltb, &bl, 1, &tl, 1, cbr);

				cbr[0] = tr.br[3];
				cbr[1] = br.br[3];
				cbr[2] = br.br[1];
				cbr[3] = tr.br[1];
				concatBuffers(&rtb, &tr, 1, &br, 1, cbr);

				releaseBuffer(&tr);
				releaseBuffer(&tl);
				releaseBuffer(&br);
				releaseBuffer(&bl);

				USHORT * pInx;
				USHORT px;

				createFill(&tlr, &rtb, &blr, &ltb, &pInx, &px, pIDXs, pIDXC);
				createAPIbuffers(&tlr, &rtb, &blr, &ltb, pInx, px);

				mem_delete_a(pInx);

				releaseBuffer(&tlr);
				releaseBuffer(&rtb);
				releaseBuffer(&blr);
				releaseBuffer(&ltb);

				for(UINT i = 0; i < 4; i++)
				{
					for(UINT j = 0; j < 2; j++)
					{
						mem_delete_a(pIDXs[i][j]);
					}
					mem_delete_a(pIDXC[i]);
					mem_delete_a(pIDXs[i]);
				}
			}

			void IRenderBorder::releaseBuffer(buff_t * pIn)
			{
				for(UINT i = 0; i < 2; i++)
				{
					if(pIn->ib)
					{
						mem_delete_a(pIn->ib[i]);
					}
					if(pIn->vb)
					{
						mem_delete_a(pIn->vb[i]);
					}
				}
				mem_delete_a(pIn->ib);
				mem_delete_a(pIn->vb);
			}

			void IRenderBorder::concatBuffers(buff_t * pOut, const buff_t * b1, USHORT idx1, const buff_t * b2, USHORT idx2, float3_t * br)
			{
				//[0, 1, 2, 0, 2, 3]

				pOut->iVC[0] = b1->iVC[idx1] + b2->iVC[idx2] + 4;
				pOut->iIC[0] = b1->iIC[idx1] + b2->iIC[idx2] + 6;

				pOut->vb = new float3_t*[2];
				pOut->ib = new USHORT*[2];
				pOut->vb[1] = NULL;
				pOut->ib[1] = NULL;

				pOut->vb[0] = new float3_t[pOut->iVC[0]];
				pOut->ib[0] = new USHORT[pOut->iIC[0]];

				UINT VC = 0;
				UINT IC = 0;
				for(UINT i = 0; i < b1->iVC[idx1]; i++)
				{
					pOut->vb[0][i] = b1->vb[idx1][i];
				}
				for(UINT i = 0; i < b1->iIC[idx1]; i++)
				{
					pOut->ib[0][i] = b1->ib[idx1][i];
				}
				VC += b1->iVC[idx1];
				IC += b1->iIC[idx1];
				for(UINT i = 0; i < b2->iVC[idx2]; i++)
				{
					pOut->vb[0][VC + i] = b2->vb[idx2][i];
				}
				for(UINT i = 0; i < b2->iIC[idx2]; i++)
				{
					pOut->ib[0][IC + i] = b2->ib[idx2][i] + VC;
				}
				VC += b2->iVC[idx2];
				IC += b2->iIC[idx2];
				for(UINT i = 0; i < 4; i++)
				{
					pOut->vb[0][VC + i] = br[i];
				}

				UINT i = 0;
				pOut->ib[0][IC + i] = VC + 0; i++;
				pOut->ib[0][IC + i] = VC + 2; i++;
				pOut->ib[0][IC + i] = VC + 1; i++;
				pOut->ib[0][IC + i] = VC + 0; i++;
				pOut->ib[0][IC + i] = VC + 3; i++;
				pOut->ib[0][IC + i] = VC + 2; i++;
			}

			void IRenderBorder::createAPIbuffers(buff_t * t, buff_t * r, buff_t * b, buff_t * l, USHORT * pIndices, USHORT IndexCount)
			{
				USHORT iIndexCount = t->iIC[0] + r->iIC[0] + b->iIC[0] + l->iIC[0] + IndexCount;
				USHORT iVertexCount = t->iVC[0] + r->iVC[0] + b->iVC[0] + l->iVC[0];

				m_iFillIndexCount = IndexCount;

				UINT iCV = 0;
				UINT iCI = 0;
				float3_t *pData;
				pData = new float3_t[iVertexCount];

				memcpy(pData + iCV, t->vb[0], sizeof(float3_t) * t->iVC[0]);
				iCV += t->iVC[0];
				memcpy(pData + iCV, r->vb[0], sizeof(float3_t) * r->iVC[0]);
				iCV += r->iVC[0];
				memcpy(pData + iCV, b->vb[0], sizeof(float3_t) * b->iVC[0]);
				iCV += b->iVC[0];
				memcpy(pData + iCV, l->vb[0], sizeof(float3_t) * l->iVC[0]);
				iCV += l->iVC[0];

				m_pVertexBuffer = GetGUI()->getDevice()->createVertexBuffer(sizeof(float3_t)* iVertexCount, GXBUFFER_USAGE_STATIC, pData);
				mem_delete_a(pData);

				m_pRenderBuffer = GetGUI()->getDevice()->createRenderBuffer(1, &m_pVertexBuffer, GetGUI()->getVertexDeclarations()->m_pXYZ);

				USHORT *pData2 = new USHORT[iIndexCount];
				
				memcpy(pData2 + iCI, t->ib[0], sizeof(USHORT) * t->iIC[0]);
				iCI += t->iIC[0];
				memcpy(pData2 + iCI, r->ib[0], sizeof(USHORT) * r->iIC[0]);
				iCI += r->iIC[0];
				memcpy(pData2 + iCI, b->ib[0], sizeof(USHORT) * b->iIC[0]);
				iCI += b->iIC[0];
				memcpy(pData2 + iCI, l->ib[0], sizeof(USHORT) * l->iIC[0]);
				iCI += l->iIC[0];
				memcpy(pData2 + iCI, pIndices, sizeof(USHORT) * IndexCount);

				m_pIndexBuffer = GetGUI()->getDevice()->createIndexBuffer(sizeof(USHORT)* iIndexCount, GXBUFFER_USAGE_DYNAMIC, GXIT_UINT16, pData2);
				mem_delete_a(pData2);

			}

			void IRenderBorder::buildRadius(IRB_PARAM)
			{
				float c1 = (2 * r * (l + t) - 2 * r * sqrt((l + t) * (l + t) - (l * l + t * t))) / (2 * (l * l + t * t));
				if(l + t == 0)
				{
					c1 = 0;
				}
				float x1 = l * c1;
				float y1 = t * c1;

				float f1 = SM_PI + atan((y1 - r) / (x1 - r));

				float c2 = (-sqrt((-2 * l*l * r*t - 2 * l * r*r*r + 8 * l * r*r * t - 2 * l *r*t * t - 2 * r*r*r * t) * (-2 * l*l * r*t - 2 * l * r*r*r + 8 * l * r*r * t - 2 * l * r * t*t - 2 * r*r*r * t) - 4 * (2 * l*l * r * t - l*l * t*t - 4 * l * r*r * t + 2 * l * r * t*t + r*r*r*r) * (l*l * r*r - 2 * l*l * r * t + 2 * l*l * t*t - 2 * l * r * t*t + r*r * t*t)) + 2 * l*l * r * t + 2 * l * r*r*r - 8 * l * r*r * t + 2 * l * r * t*t + 2 * r*r*r * t) / (2 * (l*l * r*r - 2 * l*l * r * t + 2 * l*l * t*t - 2 * l * r * t*t + r*r*t * t));
				if(l + t == 0)
				{
					c2 = 0;
				}
				float x2 = l * c2;
				float y2 = t * c2;

				if(l + t == 0)
				{
					x1 = x2 = r * cos(f1) + r;
					y1 = y2 = r * sin(f1) + r;
				}

				float a = r - l;
				float b = r - t;

				float f2 = SM_PI + atan((y2 - r) / (x2 - r));
				f2 = 2 * SM_PI - acos((x2 - r) / a);
				if(f2 > 2 * SM_PI)
				{
					f2 -= 2 * SM_PI;
				}
				if(f1 < SM_PI)
				{
					f1 += SM_PI;
				}

				UINT steps = 2 + r / 20;

				float3_t * borders = pBrd;

				UINT vc = 0;
				UINT ic = 0;
				if(r > 0.0f)
				{
					vc += steps + 1;
				}
				else
				{
					vc++;
				}
				if(r > t && r > l)
				{
					vc += steps + 1;
					ic += steps * 6;
				}
				else
				{
					if(r > 0.0f)
					{
						vc++;
						ic += steps * 3;
					}
				}

				iVertexCount[0] = vc;
				iVertexCount[1] = vc;
				iIndexCount[0] = ic;
				iIndexCount[1] = ic;

				(*pppVertices) = new float3_t*[2];
				(*pppVertices)[0] = new float3_t[vc];
				(*pppVertices)[1] = new float3_t[vc];

				(*pppIndexes) = new USHORT*[2];
				(*pppIndexes)[0] = new USHORT[ic];
				(*pppIndexes)[1] = new USHORT[ic];

				(*pFillIdxCount)[0] = 0;
				(*pFillIdxCount)[1] = 0;
				(*ppFillIDx)[0] = NULL;
				(*ppFillIDx)[1] = NULL;


				float3_t ** pVB = (*pppVertices);
				USHORT ** pIB = (*pppIndexes);

				UINT iCV = 0;
				UINT iCV1 = 0;

				if(r > 0.0f)
				{
					float _stp = (3.0f * SM_PI / 2.0f - f1) / (float)steps;
					pVB[0][iCV] = float3_t(x1, y1, 0.0f);
					iCV++;
					for(UINT i = 1; i <= steps; i++)
					{
						float f = i * _stp + f1;
						int x = r * cos(f);
						int y = r * sin(f);
						pVB[0][iCV] = float3_t(x + r, y + r, 0.0f);
						iCV++;
						if(i == steps)
						{
							borders[0] = float3_t(x + r, y + r, 0.0f);
						}
					}
					/*if(l > r){
					pVB.push({x: l, y: 0});
					add[0] = 1;
					}*/

					//ctx.moveTo(x1, y1);
					_stp = (f1 - SM_PI) / steps;
					pVB[1][iCV1] = float3_t(x1, y1, 0.0f);
					iCV1++;
					for(int i = steps - 1; i >= 0; i--)
					{
						float f = i * _stp + SM_PI;
						float x = r * cos(f);
						float y = r * sin(f);
						pVB[1][iCV1] = float3_t(x + r, y + r, 0.0f);
						iCV1++;
						if(i == 0)
						{
							borders[1] = float3_t(x + r, y + r, 0.0f);
						}
					}

				}
				else
				{
					borders[0] = float3_t(0.0f, 0.0f, 0.0f);
					borders[1] = float3_t(0.0f, 0.0f, 0.0f);
					*c = 0;

					pVB[0][iCV++] = float3_t(l, t, 0.0f);
					pVB[1][iCV1++] = float3_t(l, t, 0.0f);

					(*pFillIdxCount)[0] = 1;
					(*pFillIdxCount)[1] = 0;
					(*ppFillIDx)[0] = new USHORT[(*pFillIdxCount)[0]];
					(*ppFillIDx)[1] = NULL;
					(*ppFillIDx)[0][0] = 0;
				}

				if(r > t && r > l)
				{
					//m_iSideCount[c] = steps;

					(*pFillIdxCount)[0] = steps + 1;
					(*pFillIdxCount)[1] = steps + 1;
					(*ppFillIDx)[0] = new USHORT[(*pFillIdxCount)[0]];
					(*ppFillIDx)[1] = new USHORT[(*pFillIdxCount)[1]];

					*c = steps + 1;
					pVB[0][iCV] = float3_t(x2, y2, 0.0f);
					iCV++;
					float _stp = (3 * SM_PI / 2 - f2) / steps;
					for(UINT i = 1; i <= steps; i++)
					{
						float f = i * _stp + f2;
						float x = a * cos(f) + r;
						float y = b * sin(f) + r;
						pVB[0][iCV] = float3_t(x, y, 0.0f);
						iCV++;
						if(i == steps)
						{
							borders[2] = float3_t(x, y, 0.0f);
						}
					}


					pVB[1][iCV1] = float3_t(x2, y2, 0.0f);
					iCV1++;
					_stp = (f2 - SM_PI) / steps;
					for(int i = steps - 1; i >= 0; i--)
					{
						float f = i * _stp + SM_PI;
						float x = a * cos(f) + r;
						float y = b * sin(f) + r;
						pVB[1][iCV1] = float3_t(x, y, 0.0f);
						iCV1++;
						if(i == 0)
						{
							borders[3] = float3_t(x, y, 0.0f);
						}
					}

					UINT iCI = 0;
					UINT iCI1 = 0;
					UINT CV = steps + 1;

					UINT iCBX = 0;
					UINT i;
					for(i = 0; i < steps; i++)
					{
						pIB[0][iCI] = i; iCI++;
						pIB[0][iCI] = i + 1; iCI++;
						pIB[0][iCI] = (i + CV); iCI++;

						pIB[0][iCI] = (i + CV); iCI++;
						pIB[0][iCI] = (i + CV + 1); iCI++;
						pIB[0][iCI] = (i + 1); iCI++;

						(*ppFillIDx)[0][iCBX] = i + CV;
						(*ppFillIDx)[1][iCBX++] = i + CV;

						pIB[1][iCI1] = (i); iCI1++;
						pIB[1][iCI1] = (i + 1); iCI1++;
						pIB[1][iCI1] = (i + CV); iCI1++;

						pIB[1][iCI1] = (i + CV); iCI1++;
						pIB[1][iCI1] = (i + CV + 1); iCI1++;
						pIB[1][iCI1] = (i + 1); iCI1++;
					}
					(*ppFillIDx)[0][iCBX] = i + CV;
					(*ppFillIDx)[1][iCBX++] = i + CV;
				}
				else
				{
					borders[2] = float3_t(l, t, 0.0f);
					borders[3] = float3_t(l, t, 0.0f);
					if(r > 0)
					{
						(*pFillIdxCount)[0] = 1;
						(*pFillIdxCount)[1] = 0;
						(*ppFillIDx)[0] = new USHORT[(*pFillIdxCount)[0]];
						(*ppFillIDx)[1] = NULL;
						(*ppFillIDx)[0][0] = steps + 1;

						*c = 2;
						pVB[0][iCV] = float3_t(l, t, 0.0f);
						iCV++;
						pVB[1][iCV1] = float3_t(l, t, 0.0f);
						iCV1++;
						UINT CV = 0;
						UINT iCI = 0;
						UINT iCI1 = 0;
						for(UINT i = 0; i < steps; i++)
						{
							pIB[0][iCI] = (CV + i); iCI++;
							pIB[0][iCI] = (CV + i + 1); iCI++;
							pIB[0][iCI] = (steps + 1); iCI++;

							pIB[1][iCI1] = (CV + i); iCI1++;
							pIB[1][iCI1] = (CV + i + 1); iCI1++;
							pIB[1][iCI1] = (steps + 1); iCI1++;
						}
					}
				}

				*c = vc;
			}

			void IRenderBorder::createFill(buff_t * t, buff_t * r, buff_t * b, buff_t * l, USHORT ** ppIndices, USHORT * pIndexCount, USHORT ** ppFillIDx[4], USHORT * pFillIdxCount[4])
			{
				m_iVertexCount[TOP] = t->iVC[0];
				m_iVertexStart[TOP] = 0;
				m_iIndexCount[TOP] = t->iIC[0];
				m_iIndexStart[TOP] = 0;

				m_iVertexCount[RIGHT] = r->iVC[0];
				m_iVertexStart[RIGHT] = m_iVertexStart[TOP] + m_iVertexCount[TOP];
				m_iIndexCount[RIGHT] = r->iIC[0];
				m_iIndexStart[RIGHT] = m_iIndexCount[TOP] + m_iIndexStart[TOP];

				m_iVertexCount[BOTTOM] = b->iVC[0];
				m_iVertexStart[BOTTOM] = m_iVertexStart[RIGHT] + m_iVertexCount[RIGHT];
				m_iIndexCount[BOTTOM] = b->iIC[0];
				m_iIndexStart[BOTTOM] = m_iIndexCount[RIGHT] + m_iIndexStart[RIGHT];

				m_iVertexCount[LEFT] = l->iVC[0];
				m_iVertexStart[LEFT] = m_iVertexStart[BOTTOM] + m_iVertexCount[BOTTOM];
				m_iIndexCount[LEFT] = l->iIC[0];
				m_iIndexStart[LEFT] = m_iIndexCount[BOTTOM] + m_iIndexStart[BOTTOM];

				UINT ic = 0;

				for(UINT i = 0; i < 4; i++)
				{
					for(UINT j = 0; j < 2; j++)
					{
						ic += pFillIdxCount[i][j];
					}
				}
				if(ic)
				{
					(*ppIndices) = new USHORT[ic];

					UINT iCI = 0;

					for(UINT i = 0; i < pFillIdxCount[TOP_LEFT][0]; i++)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[TOP] + ppFillIDx[TOP_LEFT][0][i];
					}

					for(int i = pFillIdxCount[TOP_RIGHT][0] - 1; i >= 0; i--)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[TOP] + ppFillIDx[TOP_RIGHT][0][i] + m_iSideCount[TOP_LEFT];
					}

					for(UINT i = 0; i < pFillIdxCount[TOP_RIGHT][1]; i++)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[RIGHT] + ppFillIDx[TOP_RIGHT][1][i];
					}

					for(int i = pFillIdxCount[BOTTOM_RIGHT][1] - 1; i >= 0; i--)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[RIGHT] + ppFillIDx[BOTTOM_RIGHT][1][i] + m_iSideCount[TOP_RIGHT];
					}

					for(UINT i = 0; i < pFillIdxCount[BOTTOM_RIGHT][0]; i++)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[BOTTOM] + ppFillIDx[BOTTOM_RIGHT][0][i];
					}

					for(int i = pFillIdxCount[BOTTOM_LEFT][0] - 1; i >= 0; i--)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[BOTTOM] + ppFillIDx[BOTTOM_LEFT][0][i] + m_iSideCount[BOTTOM_RIGHT];
					}

					for(UINT i = 0; i < pFillIdxCount[BOTTOM_LEFT][1]; i++)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[LEFT] + ppFillIDx[BOTTOM_LEFT][1][i];
					}

					for(int i = pFillIdxCount[TOP_LEFT][1] - 1; i >= 0; i--)
					{
						(*ppIndices)[iCI++] = m_iVertexStart[LEFT] + ppFillIDx[TOP_LEFT][1][i] + m_iSideCount[BOTTOM_LEFT];
					}

					/*UINT ic = 0;
					for(UINT i = 0; i < 4; i++)
					{
					if(m_iRadius[i] != 0)
					{
					if(m_iSideCount[i] == -2)
					{
					ic += 2;
					}
					else if(m_iSideCount[i] == -1)
					{
					ic++;
					}
					else if(m_iSideCount[i] == 0) // use border
					{
					ic++;
					}
					else if(m_iSideCount[i] == 1)
					{
					ic++;
					}
					else
					{
					ic += (m_iSideCount[i] + 1) * 2;
					}
					}
					else
					{
					ic++;
					}
					}

					(*ppIndices) = new UINT[ic];


					UINT iCI = 0;

					if(m_iRadius[TOP_LEFT] != 0)
					{
					if(m_iSideCount[TOP_LEFT] == -2)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[LEFT] + 3;//from left
					(*ppIndices)[iCI++] = m_iVertexStart[TOP] + 3;//from top
					}
					else if(m_iSideCount[TOP_LEFT] == -1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[TOP] + 2;
					}
					else if(m_iSideCount[TOP_LEFT] == 0) // use border
					{
					(*ppIndices)[iCI++] = m_iVertexStart[TOP] + t->ib[0][t->iIC[0] - 6];
					}
					else if(m_iSideCount[TOP_LEFT] == 1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[TOP] + m_iSideCount[TOP_LEFT] + 1;
					}
					else
					{
					for(int i = m_iSideCount[TOP_LEFT]; i >= 0; i--)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[LEFT] + m_iSideCount[TOP_LEFT] + 1 + i;
					}
					for(UINT i = 0; i < m_iSideCount[TOP_LEFT] + 1; i++)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[TOP] + m_iSideCount[TOP_LEFT] + 1 + i;
					}
					}
					}
					else
					{
					(*ppIndices)[iCI++] = m_iVertexStart[TOP] + t->ib[0][t->iIC[0] - 6];
					}

					UINT c = TOP_RIGHT;
					UINT s1 = TOP;
					UINT s2 = RIGHT;
					buff_t * bf = r;

					if(m_iRadius[c] != 0)
					{
					if(m_iSideCount[c] == -2)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s1] + 3;//from left
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + 3;//from top
					}
					else if(m_iSideCount[c] == -1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + 2;
					}
					else if(m_iSideCount[c] == 0) // use border
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + bf->ib[0][t->iIC[0] - 6];
					}
					else if(m_iSideCount[c] == 1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + m_iSideCount[c] + 1;
					}
					else
					{
					for(int i = m_iSideCount[c]; i >= 0; i--)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s1] + m_iSideCount[c] + 1 + i;
					}
					for(UINT i = 0; i < m_iSideCount[c] + 1; i++)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + m_iSideCount[c] + 1 + i;
					}
					}
					}
					else
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + bf->ib[0][t->iIC[0] - 4];
					}

					c = BOTTOM_RIGHT;
					s1 = RIGHT;
					s2 = BOTTOM;
					bf = b;

					if(m_iRadius[c] != 0)
					{
					if(m_iSideCount[c] == -2)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s1] + 3;//from left
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + 3;//from top
					}
					else if(m_iSideCount[c] == -1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + 2;
					}
					else if(m_iSideCount[c] == 0) // use border
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + bf->ib[0][t->iIC[0] - 6];
					}
					else if(m_iSideCount[c] == 1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + m_iSideCount[c] + 1;
					}
					else
					{
					for(int i = m_iSideCount[c]; i >= 0; i--)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s1] + m_iSideCount[c] + 1 + i;
					}
					for(UINT i = 0; i < m_iSideCount[c] + 1; i++)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + m_iSideCount[c] + 1 + i;
					}
					}
					}
					else
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + bf->ib[0][t->iIC[0] - 6];
					}

					c = BOTTOM_LEFT;
					s1 = BOTTOM;
					s2 = LEFT;
					bf = l;

					if(m_iRadius[c] != 0)
					{
					if(m_iSideCount[c] == -2)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s1] + 3;//from left
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + 3;//from top
					}
					else if(m_iSideCount[c] == -1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + 2;
					}
					else if(m_iSideCount[c] == 0) // use border
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + bf->ib[0][t->iIC[0] - 6];
					}
					else if(m_iSideCount[c] == 1)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + m_iSideCount[c] + 1;
					}
					else
					{
					for(int i = m_iSideCount[c]; i >= 0; i--)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s1] + m_iSideCount[c] + 1 + i;
					}
					for(UINT i = 0; i < m_iSideCount[c] + 1; i++)
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + m_iSideCount[c] + 1 + i;
					}
					}
					}
					else
					{
					(*ppIndices)[iCI++] = m_iVertexStart[s2] + bf->ib[0][t->iIC[0] - 6];
					}*/



					USHORT * pIB = new USHORT[((int)ic - 2) * 3];
					iCI = 0;
					for(UINT i = 0; i < ic - 2; i++)
					{
						pIB[iCI++] = (*ppIndices)[0];
						pIB[iCI++] = (*ppIndices)[i + 1];
						pIB[iCI++] = (*ppIndices)[i + 2];
					}
					*pIndexCount = (ic - 2) * 3;
					mem_delete_a((*ppIndices));
					//		memset(pIB, 0, sizeof(UINT) * (ic - 2) * 3);
					(*ppIndices) = pIB;
				}
				else
				{
					*pIndexCount = 0;
					(*ppIndices) = NULL;
				}
			}

			void IRenderBorder::buildCut(IRB_PARAM)
			{
				UINT iVC[2] = {4, 4};
				UINT iIC[2] = {6, 6};

				(*pppVertices) = new float3_t*[2];
				(*pppVertices)[0] = new float3_t[iVC[0]];
				(*pppVertices)[1] = new float3_t[iVC[1]];

				(*pppIndexes) = new USHORT*[2];
				(*pppIndexes)[0] = new USHORT[iIC[0]];
				(*pppIndexes)[1] = new USHORT[iIC[1]];


				float3_t ** pVB = (*pppVertices);
				USHORT ** pIB = (*pppIndexes);

				float3_t * borders = pBrd;


				float x1 = l * r / (t + l);
				float y1 = r - x1;

				float y2 = -(l * t - r*r) / ((r - t) * l / t + r - l);
				float x2 = y2 * l / t;


				borders[0] = float3_t(r, 0.0f, 0.0f);
				borders[1] = float3_t(0.0f, r, 0.0f);

				pVB[0][0] = float3_t(x1, y1, 0.0f);
				pVB[0][1] = float3_t(r, 0.0f, 0.0f);

				pVB[1][0] = float3_t(x1, y1, 0.0f);
				pVB[1][1] = float3_t(0.0f, r, 0.0f);

				for(UINT i = 0; i < 2; i++)
				{
					pIB[i][0] = 0;
					pIB[i][1] = 1;
					pIB[i][2] = 2;
					pIB[i][3] = 2;
					pIB[i][4] = 1;
					pIB[i][5] = 3;
				}


				if(r > t && r > l)
				{
					pVB[0][2] = float3_t(x2, y2, 0.0f);
					pVB[0][3] = float3_t(r, t, 0.0f);

					pVB[1][2] = float3_t(x2, y2, 0.0f);
					pVB[1][3] = float3_t(l, r, 0.0f);

					borders[2] = float3_t(r, t, 0.0f);
					borders[3] = float3_t(l, r, 0.0f);
					*c = 4;
					(*pFillIdxCount)[0] = 1;
					(*pFillIdxCount)[1] = 1;
					(*ppFillIDx)[0] = new USHORT[(*pFillIdxCount)[0]];
					(*ppFillIDx)[1] = new USHORT[(*pFillIdxCount)[1]];
					(*ppFillIDx)[0][0] = 3;
					(*ppFillIDx)[1][0] = 3;
				}
				else
				{
					pVB[0][2] = float3_t(l, t, 0.0f);
					pVB[0][3] = float3_t(l, t, 0.0f);

					pVB[1][2] = float3_t(l, t, 0.0f);
					pVB[1][3] = float3_t(l, t, 0.0f);

					borders[2] = float3_t(l, t, 0.0f);
					borders[3] = float3_t(l, t, 0.0f);
					*c = 4;
					(*pFillIdxCount)[0] = 1;
					(*pFillIdxCount)[1] = 0;
					(*ppFillIDx)[0] = new USHORT[(*pFillIdxCount)[0]];
					(*ppFillIDx)[1] = NULL;
					(*ppFillIDx)[0][0] = 2;
				}

				iVertexCount[0] = iVC[0];
				iVertexCount[1] = iVC[1];
				iIndexCount[0] = iIC[0];
				iIndexCount[1] = iIC[1];
			}
		};
	};
};