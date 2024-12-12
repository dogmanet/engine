#include "TransparentGraphNode.h"
#include "TransparentGraphNodeData.h"
#include <xcommon/IXCamera.h>
#include <xcommon/IXRenderable.h>
#include <xcommon/IPluginManager.h>

CTransparentGraphNode::CTransparentGraphNode(IXCore *pCore):
	m_pCore(pCore)
{
	m_pMaterialSystem = (IXMaterialSystem*)m_pCore->getPluginManager()->getInterface(IXMATERIALSYSTEM_GUID);
}
CTransparentGraphNode::~CTransparentGraphNode()
{
	mem_release(m_pTransparencyShaderClipPlanes);
	mem_release(m_pRefractionScene);
	mem_release(m_pSamplerLinearClamp);
	mem_release(m_pDepthStencilStateNoZWrite);
	mem_release(m_pDepthStencilStateNoZ);
	mem_release(m_pBlendStateAlpha);
}

void XMETHODCALLTYPE CTransparentGraphNode::startup(IXRender *pRender)
{
	m_pRender = pRender;
	m_pDevice = pRender->getDevice();

	m_pRenderPassTransparency = m_pMaterialSystem->getRenderPass("xTransparency");

	m_pTransparencyShaderClipPlanes = m_pDevice->createConstantBuffer(sizeof(float4) * MAX_TRANSPARENCY_CLIP_PANES);

	GXSamplerDesc sampDesc;
	sampDesc.addressU = sampDesc.addressV = sampDesc.addressW = GXTEXTURE_ADDRESS_CLAMP;
	sampDesc.filter = GXFILTER_MIN_MAG_MIP_POINT;
	m_pRefractionScene = m_pDevice->createSamplerState(&sampDesc);

	GXDepthStencilDesc dsDesc;
	dsDesc.cmpFuncDepth = GXCMP_GREATER_EQUAL;
	dsDesc.useDepthWrite = FALSE;
	m_pDepthStencilStateNoZWrite = m_pDevice->createDepthStencilState(&dsDesc);

	dsDesc.useDepthTest = FALSE;
	m_pDepthStencilStateNoZ = m_pDevice->createDepthStencilState(&dsDesc);

	GXBlendDesc blendDesc;
	blendDesc.renderTarget[0].useBlend = true;
	blendDesc.renderTarget[0].blendDestColor = blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_SRC_ALPHA;
	blendDesc.renderTarget[0].blendSrcColor = blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_SRC_ALPHA;
	m_pBlendStateAlpha = m_pDevice->createBlendState(&blendDesc);
	
	GXSamplerDesc samplerDesc;
	samplerDesc.filter = GXFILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.addressU = samplerDesc.addressV = samplerDesc.addressW = GXTEXTURE_ADDRESS_CLAMP;
	m_pSamplerLinearClamp = m_pDevice->createSamplerState(&samplerDesc);

	m_idShaderScreenOut = m_pRender->createShaderKit(m_pRender->loadShader(SHADER_TYPE_VERTEX, "pp_quad_render.vs"), m_pRender->loadShader(SHADER_TYPE_PIXEL, "pp_quad_render.ps"));
}

void XMETHODCALLTYPE CTransparentGraphNode::newData(IXRenderGraphNodeData **ppData)
{
	*ppData = new CTransparentGraphNodeData(m_pDevice, m_pRender);
}

void XMETHODCALLTYPE CTransparentGraphNode::newOutputTarget(UINT uOutput, UINT uWidth, UINT uHeight, IGXBaseTexture **ppTarget)
{
	*ppTarget = m_pDevice->createTexture2D(uWidth, uHeight, 1, GX_TEXFLAG_RENDERTARGET, GXFMT_A16B16G16R16F);
}

void XMETHODCALLTYPE CTransparentGraphNode::updateVisibility(IXRenderTarget *pFinalTarget, IXRenderGraphNodeData *pData)
{
	CTransparentGraphNodeData *pTarget = (CTransparentGraphNodeData*)pData;
}

void XMETHODCALLTYPE CTransparentGraphNode::process(float fDeltaTime, IXRenderTarget *pFinalTarget, IGXBaseTexture **ppInputs, UINT uInputCount, IXRenderGraphNodeData *pData, IGXSurface **ppOutputs, UINT uOutputCount)
{
	XPROFILE_FUNCTION();

	assert(uInputCount == 3);
	assert(uOutputCount == 1);

	CTransparentGraphNodeData *pTarget = (CTransparentGraphNodeData*)pData;

	IGXContext *pCtx = m_pDevice->getThreadContext();
		
	IGXBaseTexture *pGBufferColor = ppInputs[0];
	IGXBaseTexture *pGBufferDepth = ppInputs[1];
	IGXBaseTexture *pLightTexture = ppInputs[2];

	IXCamera *pCamera;
	pFinalTarget->getCamera(&pCamera);

	IXRenderableVisibility *pVis;
	pCamera->getVisibility(&pVis);

	m_pRender->setSceneFilter(pCtx);

	m_pMaterialSystem->bindRenderPass(m_pRenderPassTransparency);

	//	pCtx->setRasterizerState(NULL);

	pCtx->setDepthStencilState(m_pDepthStencilStateNoZWrite);
	pCtx->setBlendState(m_pBlendStateAlpha);
	pCtx->setSamplerState(m_pRefractionScene, 1);
	pCtx->setSamplerState(m_pSamplerLinearClamp, 2);
	//m_pDevice->setTexture(m_pSceneTexture, 11);
	pCtx->setPSTexture(pGBufferColor);
	//	pCtx->setPSTexture(m_pGBufferNormals, 1);
	pCtx->setPSTexture(pGBufferDepth, 2);
	//	pCtx->setPSTexture(m_pGBufferParams, 3);

		//! @todo implement me!
		// for(UINT i = 0; i < 3; ++i)
		// {
		// 	pCtx->setPSTexture(m_aLPVs[i].pGIAccumRed, 5 + i);
		// 	pCtx->setPSTexture(m_aLPVs[i].pGIAccumGreen, 8 + i);
		// 	pCtx->setPSTexture(m_aLPVs[i].pGIAccumBlue, 11 + i);
		// }

	pTarget->m_iRefractiveSource = -1;
	//IGXSurface *pSceneTarget = pLightTexture->asRenderTarget();
	//pCtx->setColorTarget(pSceneTarget);
	//mem_release(pSceneTarget);
	pCtx->setColorTarget(ppOutputs[0]);

	pTarget->m_pRefractiveTextureWrite = pLightTexture;

	//renderStage(XRS_TRANSPARENT, m_pMainCameraVisibility);

	Array<XTransparentNode> &aNodes = m_aTmpNodes;
	Array<XTransparentPSP> &aPSPs = m_aTmpPSPs;

	UINT uTransparentCount = m_pRender->getTransparentObjectsCount(pVis);

	if(uTransparentCount)
	{
		XRenderTransparentObject *pObjects = (XRenderTransparentObject*)alloca(sizeof(XRenderTransparentObject) * uTransparentCount);
		m_pRender->getTransparentObjects(pVis, pObjects);

		for(UINT i = 0; i < uTransparentCount; ++i)
		{
			XTransparentNode &tpNode = aNodes[aNodes.size()];
			tpNode.obj = pObjects[i];

			if(tpNode.obj.obj.hasPSP)
			{
				XTransparentPSP psp;
				psp.psp = tpNode.obj.obj.psp;
				psp.uNode = aNodes.size() - 1;
				aPSPs.push_back(psp);
			}
		}

		if(aNodes.size())
		{
			UINT uTotalPlanes = 0;
			// Определение секущих плоскостей
			for(UINT i = 0, l = aPSPs.size(); i < l; ++i)
			{
				XTransparentPSP *pPSP = &aPSPs[i];
				XTransparentNode *pPSPNode = &aNodes[pPSP->uNode];

				for(UINT j = 0, jl = aNodes.size(); j < jl; ++j)
				{
					if(j == pPSP->uNode)
					{
						continue;
					}

					XTransparentNode *pNode = &aNodes[j];

					if(SMAABBIntersectAABB(pNode->obj.obj.vMin, pNode->obj.obj.vMax, pPSPNode->obj.obj.vMin, pPSPNode->obj.obj.vMax)
						&& SMPlaneIntersectAABB(pPSP->psp, pNode->obj.obj.vMin, pNode->obj.obj.vMax))
					{
						pPSP->useMe = true;
						++uTotalPlanes;
						break;
					}
				}
			}

			// Определение одинаковых плоскостей
			for(UINT i = 0, l = aPSPs.size(); i < l; ++i)
			{
				XTransparentPSP *pPSP = &aPSPs[i];
				if(pPSP->useMe)
				{
					for(UINT j = i + 1; j < l; ++j)
					{
						XTransparentPSP *pPSP2 = &aPSPs[j];
						if(pPSP2->useMe && SMPlaneEqualEpsilon(pPSP2->psp, pPSP->psp, 0.01f))
						{
							pPSP2->iBasePSP = (int)i;
							--uTotalPlanes;
						}
					}
				}
			}

			XTransparentBSPObject *pRootObject = NULL, *pTailObject = NULL;
			for(UINT i = 0, l = aNodes.size(); i < l; ++i)
			{
				XTransparentBSPObject *pObj = m_poolTransparencyBSPobjects.Alloc();
				pObj->uNode = i;
				if(!pRootObject)
				{
					pRootObject = pObj;
					pTailObject = pObj;
				}
				else
				{
					pTailObject->pNext = pObj;
					pTailObject = pObj;
				}
			}
			//m_poolTransparencyBSPobjects

			float3 vCamPos = pCamera->getPosition();

			// Построение дерева
			XTransparentBSPNode *pRootNode = m_poolTransparencyBSPnodes.Alloc();
			buildTransparencyBSP(aPSPs, aPSPs.size(), 0, pRootNode, pRootObject, aNodes, vCamPos);

			//float4 vPlanes[MAX_TRANSPARENCY_CLIP_PANES];
			//vPlanes[0] = SMPlaneNormalize(SMPlaneTransform(SMPLANE(0.0f, 1.0f, 0.0f, -1.0f), gdata::mCamView * gdata::mCamProj));
			//vPlanes[0] /= fabsf(vPlanes[0].w);
			//m_pTransparencyShaderClipPlanes->update(vPlanes);

			// Отрисовка дерева
			pCtx->setPSConstant(m_pTransparencyShaderClipPlanes, 6);

			SMMATRIX mTransInvVP = SMMatrixTranspose(SMMatrixInverse(NULL, pCamera->getViewMatrix() * pCamera->getProjMatrix()));
			UINT puPlanesStack[MAX_TRANSPARENCY_CLIP_PANES];
			renderTransparencyBSP(pTarget, pFinalTarget, pVis, pRootNode, aPSPs, aNodes, vCamPos, puPlanesStack, 0, mTransInvVP);

			m_poolTransparencyBSPobjects.clearFast();
			m_poolTransparencyBSPnodes.clearFast();
			m_poolTransparencyBSPsplitPlanes.clearFast();
			m_aTmpNodes.clearFast();
			m_aTmpPSPs.clearFast();
		}
	}
	pCtx->setBlendState(NULL);
	if(pTarget->m_iRefractiveSource != -1)
	{
		pCtx->setColorTarget(ppOutputs[0]);
		showTexture(pTarget->m_pRefractiveTextureWrite, pFinalTarget);
	}

	pCtx->addTimestamp("transparency");

	mem_release(pCamera);
	mem_release(pVis);
}

const XRenderGraphNodeInputDesc* XMETHODCALLTYPE CTransparentGraphNode::getInputsDesc(UINT *puCount)
{
	static XRenderGraphNodeInputDesc aDesc[] = {
		{"color", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8},
		{"depth", GXTEXTURE_TYPE_2D, GXFMT_A8B8G8R8},
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A16B16G16R16F, true, true}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

const XRenderGraphNodeOutputDesc* XMETHODCALLTYPE CTransparentGraphNode::getOutputsDesc(UINT *puCount)
{
	static XRenderGraphNodeOutputDesc aDesc[] = {
		{"litScene", GXTEXTURE_TYPE_2D, GXFMT_A16B16G16R16F}
	};

	*puCount = ARRAYSIZE(aDesc);
	return(aDesc);
}

void CTransparentGraphNode::renderTransparencyBSP(CTransparentGraphNodeData *pTarget, IXRenderTarget *pFinalTarget, IXRenderableVisibility *pVis, XTransparentBSPNode *pNode, XTransparentPSP *pPSPs, XTransparentNode *pObjectNodes, const float3 &vCamPos, UINT *puPlanesStack, UINT uPlaneCount, const SMMATRIX &mTransInvVP)
{
	XTransparentBSPNode *pFirst, *pSecond;

	IGXContext *pCtx = m_pDevice->getThreadContext();

	bool isInFront = (pNode->iPSP >= 0) && SMVector3Dot(pPSPs[pNode->iPSP].psp, vCamPos) > -pPSPs[pNode->iPSP].psp.w;
	if(isInFront)
	{
		pFirst = pNode->pBack;
		pSecond = pNode->pFront;
	}
	else
	{
		pFirst = pNode->pFront;
		pSecond = pNode->pBack;
	}
	if(pNode->iPSP >= 0 && uPlaneCount < MAX_TRANSPARENCY_CLIP_PANES)
	{
		pPSPs[pNode->iPSP].isRenderFront = !isInFront;
		puPlanesStack[uPlaneCount++] = (UINT)pNode->iPSP;
		// assert(uPlaneCount <= MAX_TRANSPARENCY_CLIP_PANES);
	}
	if(pFirst)
	{
		renderTransparencyBSP(pTarget, pFinalTarget, pVis, pFirst, pPSPs, pObjectNodes, vCamPos, puPlanesStack, uPlaneCount, mTransInvVP);
	}

	static float4 vPlanes[MAX_TRANSPARENCY_CLIP_PANES];
	XTransparentBSPObject *pObj = pNode->pObjects;
	bool isRefractiveSwapped = false;
	while(pObj)
	{
		XTransparentNode *pNode = &pObjectNodes[pObj->uNode];

		XTransparentNodeSP *pCurSP = pNode->pSplitPlanes;
		for(UINT i = 0; i < MAX_TRANSPARENCY_CLIP_PANES; ++i)
		{
			if(pCurSP)
			{
				bool isFound = false;
				while(!isFound && pCurSP)
				{
					for(UINT j = 0; j < uPlaneCount; ++j)
					{
						if(puPlanesStack[j] == (UINT)pCurSP->iPSP)
						{
							isFound = true;
							break;
						}
					}
					if(!isFound)
					{
						pCurSP = pCurSP->pNext;
					}
				}

				if(isFound)
				{
					SMPLANE plane = pPSPs[pCurSP->iPSP].psp;
					if(!pPSPs[pCurSP->iPSP].isRenderFront)
					{
						plane *= -1.0f;
					}
					vPlanes[i] = SMPlaneNormalize(SMPlaneTransformTI(plane, mTransInvVP));
					vPlanes[i] /= fabsf(vPlanes[i].w);

					pCurSP = pCurSP->pNext;
					continue;
				}
			}

			vPlanes[i] = SMPLANE(0, 0, 0, 1.0f);
		}
		m_pTransparencyShaderClipPlanes->update(vPlanes);

		if(!isRefractiveSwapped && pNode->obj.obj.pMaterial && pNode->obj.obj.pMaterial->isRefractive())
		{
			pTarget->m_pRefractiveTextureRead = pTarget->m_pRefractiveTextureWrite;
			pTarget->m_iRefractiveSource = (pTarget->m_iRefractiveSource + 1) & 1;
			pTarget->m_pRefractiveTextureWrite = pTarget->m_pRefractiveTextures[pTarget->m_iRefractiveSource];
			// swap render targets
			pCtx->setPSTexture(pTarget->m_pRefractiveTextureRead, 4);

			IGXSurface *pSceneTarget = pTarget->m_pRefractiveTextureWrite->asRenderTarget();
			pCtx->setColorTarget(pSceneTarget);
			mem_release(pSceneTarget);


			TODO("copy only changed parts (use stencil?)");
			pCtx->setBlendState(NULL);
			showTexture(pTarget->m_pRefractiveTextureRead, pFinalTarget);
			pCtx->setBlendState(m_pBlendStateAlpha);

			isRefractiveSwapped = true;
		}

		m_pRender->renderTransparentObject(pVis, &pNode->obj, 0);
		pObj = pObj->pNext;
	}

	if(pNode->iPSP >= 0)
	{
		pPSPs[pNode->iPSP].isRenderFront = isInFront;
	}
	if(pSecond)
	{
		renderTransparencyBSP(pTarget, pFinalTarget, pVis, pSecond, pPSPs, pObjectNodes, vCamPos, puPlanesStack, uPlaneCount, mTransInvVP);
	}
}

void CTransparentGraphNode::buildTransparencyBSP(XTransparentPSP *pPSPs, UINT uPSPcount, UINT uStartPSP, XTransparentBSPNode *pBSPNode, XTransparentBSPObject *pObjects, XTransparentNode *pObjectNodes, const float3 &vCamPos)
{
	XTransparentPSP *pPSP;
	UINT uPSPIndex = uStartPSP;

	XTransparentBSPObject *pRootObject = NULL, *pTailObject = NULL;

	do
	{
		pPSP = NULL;
		// Выберем плоскость для текущего рассечения
		for(; uPSPIndex < uPSPcount; ++uPSPIndex)
		{
			if(pPSPs[uPSPIndex].useMe && pPSPs[uPSPIndex].iBasePSP < 0)
			{
				pPSP = &pPSPs[uPSPIndex];
				pBSPNode->iPSP = (int)uPSPIndex;
				break;
			}
		}
		if(!pPSP)
		{
			XTransparentBSPObject *pCurObj = pObjects, *pNextObj;
			XTransparentNode *pNode;
			while(pCurObj)
			{
				pNextObj = pCurObj->pNext;
				pCurObj->pNext = NULL;

				pNode = &pObjectNodes[pCurObj->uNode];
				pCurObj->fCamDist = SMDistancePointAABB(vCamPos, pNode->obj.obj.vMin, pNode->obj.obj.vMax);



				XTransparentBSPObject *pTmpCur = pBSPNode->pObjects, *pTmpPrev = NULL;
				if(pTmpCur)
				{
					bool isFound = false;
					while(pTmpCur)
					{
						//pSplitPlane->pNext = pNode->pSplitPlanes;

						if(pTmpCur->fCamDist < pCurObj->fCamDist)
						{
							pCurObj->pNext = pTmpCur;

							if(pTmpPrev)
							{
								pTmpPrev->pNext = pCurObj;
							}
							else
							{
								pBSPNode->pObjects = pCurObj;
							}
							isFound = true;
							break;
						}

						pTmpPrev = pTmpCur;
						pTmpCur = pTmpCur->pNext;
					}

					if(!isFound)
					{
						if(pTmpPrev)
						{
							pTmpPrev->pNext = pCurObj;
						}
						else
						{
							pBSPNode->pObjects = pCurObj;
						}
					}
				}
				else
				{
					pBSPNode->pObjects = pCurObj;
				}



				pCurObj = pNextObj;
			}

			//pBSPNode->pObjects = pObjects;
			return;
		}

		// Найти объекты узла (совпадающие с плоскостью)
		// Для каждого оставшегося объекта:
		//     Если объект пересекает плоскость: то
		//         Если объект пересекается с одним из объектов, порождающих PSP: то добавляем плоскость рассечения в него, помещаем объект в оба новых списка
		//         Иначе: Помещаем объект в узел
		//     Иначе: Помещаем объект в правый или левый список, в зависимости от расположения относительно плоскости
		// Повторяем рекурсивно для правого и левого поддерева

		// Найти объекты узла (совпадающие с плоскостью)
		for(UINT i = 0; i < uPSPcount; ++i)
		{
			if(pPSPs[i].iBasePSP == pBSPNode->iPSP || i == pBSPNode->iPSP)
			{
				XTransparentBSPObject *pCur = pObjects, *pPrev = NULL;
				while(pCur)
				{
					if(pCur->uNode == pPSPs[i].uNode)
					{
						if(pPrev)
						{
							pPrev->pNext = pCur->pNext;
						}
						else
						{
							pObjects = pCur->pNext;
						}
						pCur->pNext = NULL;
						if(!pRootObject)
						{
							pRootObject = pTailObject = pCur;
						}
						else
						{
							pTailObject->pNext = pCur;
							pTailObject = pCur;
						}

						break;
					}

					pPrev = pCur;
					pCur = pCur->pNext;
				}

			}
		}
		++uPSPIndex;
	}
	while(!pRootObject);

	XTransparentBSPObject *pFrontRootObject = NULL, *pFrontTailObject = NULL;
	XTransparentBSPObject *pBackRootObject = NULL, *pBackTailObject = NULL;

	// Для каждого оставшегося объекта:
	XTransparentBSPObject *pCur = pObjects, *pNext;
	while(pCur)
	{
		pNext = pCur->pNext;

		XTransparentNode *pNode = &pObjectNodes[pCur->uNode];
		if(SMPlaneIntersectAABB(pPSP->psp, pNode->obj.obj.vMin, pNode->obj.obj.vMax))
		{
			bool isIntersects = false;
			for(UINT i = 0; i < uPSPcount; ++i)
			{
				XTransparentNode *pPSPNode = &pObjectNodes[pPSPs[i].uNode];
				if(pPSPs[i].useMe && SMAABBIntersectAABB(pNode->obj.obj.vMin, pNode->obj.obj.vMax, pPSPNode->obj.obj.vMin, pPSPNode->obj.obj.vMax))
				{
					isIntersects = true;
					break;
				}
			}

			pCur->pNext = NULL;

			if(isIntersects)
			{
				// добавляем плоскость рассечения pPSP в pCur

				XTransparentNodeSP *pSplitPlane = m_poolTransparencyBSPsplitPlanes.Alloc();
				pSplitPlane->fCamDist = SMDistancePointAABB(vCamPos, pNode->obj.obj.vMin, pNode->obj.obj.vMax);
				pSplitPlane->iPSP = pBSPNode->iPSP;

				XTransparentNodeSP *pTmpCur = pNode->pSplitPlanes, *pTmpPrev = NULL;
				if(pTmpCur)
				{
					bool isFound = false;
					while(pTmpCur)
					{
						//pSplitPlane->pNext = pNode->pSplitPlanes;

						if(pTmpCur->fCamDist < pSplitPlane->fCamDist)
						{
							pSplitPlane->pNext = pTmpCur;

							if(pTmpPrev)
							{
								//!!!
								pTmpPrev->pNext = pSplitPlane;
							}
							else
							{
								pNode->pSplitPlanes = pSplitPlane;
							}
							isFound = true;
							break;
						}

						pTmpPrev = pTmpCur;
						pTmpCur = pTmpCur->pNext;
					}

					if(!isFound)
					{
						if(pTmpPrev)
						{
							pTmpPrev->pNext = pSplitPlane;
						}
						else
						{
							pNode->pSplitPlanes = pSplitPlane;
						}
					}
				}
				else
				{
					pNode->pSplitPlanes = pSplitPlane;
				}

				++pNode->uSplitPlanes;


				if(!pFrontRootObject)
				{
					pFrontRootObject = pFrontTailObject = pCur;
				}
				else
				{
					pFrontTailObject->pNext = pCur;
					pFrontTailObject = pCur;
				}
				XTransparentBSPObject *pClone = m_poolTransparencyBSPobjects.Alloc();
				pClone->uNode = pCur->uNode;
				if(!pBackRootObject)
				{
					pBackRootObject = pBackTailObject = pClone;
				}
				else
				{
					pBackTailObject->pNext = pClone;
					pBackTailObject = pClone;
				}
			}
			else
			{
				if(!pRootObject)
				{
					pRootObject = pTailObject = pCur;
				}
				else
				{
					pTailObject->pNext = pCur;
					pTailObject = pCur;
				}
			}

		}
		else
		{
			pCur->pNext = NULL;

			//front
			if(SMVector3Dot(pNode->obj.obj.vMin, pPSP->psp) > -pPSP->psp.w)
			{
				if(!pFrontRootObject)
				{
					pFrontRootObject = pFrontTailObject = pCur;
				}
				else
				{
					pFrontTailObject->pNext = pCur;
					pFrontTailObject = pCur;
				}
			}
			else // back
			{
				if(!pBackRootObject)
				{
					pBackRootObject = pBackTailObject = pCur;
				}
				else
				{
					pBackTailObject->pNext = pCur;
					pBackTailObject = pCur;
				}
			}

		}

		pCur = pNext;
	}

	if(pFrontRootObject)
	{
		XTransparentBSPNode *pFrontNode = m_poolTransparencyBSPnodes.Alloc();
		buildTransparencyBSP(pPSPs, uPSPcount, pBSPNode->iPSP + 1, pFrontNode, pFrontRootObject, pObjectNodes, vCamPos);
		if(pFrontNode->pObjects)
		{
			pBSPNode->pFront = pFrontNode;
		}
		else
		{
			m_poolTransparencyBSPnodes.Delete(pFrontNode);
		}
	}
	if(pBackRootObject)
	{
		XTransparentBSPNode *pBackNode = m_poolTransparencyBSPnodes.Alloc();
		buildTransparencyBSP(pPSPs, uPSPcount, pBSPNode->iPSP + 1, pBackNode, pBackRootObject, pObjectNodes, vCamPos);
		pBSPNode->pBack = pBackNode;
		if(pBackNode->pObjects)
		{
			pBSPNode->pBack = pBackNode;
		}
		else
		{
			m_poolTransparencyBSPnodes.Delete(pBackNode);
		}
	}
	pBSPNode->pObjects = pRootObject;
}

void CTransparentGraphNode::showTexture(IGXBaseTexture *pTexture, IXRenderTarget *pFinalTarget)
{
	IGXContext *pCtx = m_pDevice->getThreadContext();

	IGXDepthStencilState *pOldState = pCtx->getDepthStencilState();
	pCtx->setDepthStencilState(m_pDepthStencilStateNoZ);

	pCtx->setRasterizerState(NULL);

	pCtx->setPSTexture(pTexture);
	m_pRender->bindShader(pCtx, m_idShaderScreenOut);

	m_pRender->drawScreenQuad(pCtx, pFinalTarget);

	pCtx->setDepthStencilState(pOldState);
	mem_release(pOldState);
}
