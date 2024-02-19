#include "AnimatedModel.h"
#include "AnimatedModelProvider.h"

CAnimatedModel::CAnimatedModel(CAnimatedModelProvider *pProvider, CAnimatedModelShared *pShared):
	m_pProvider(pProvider),
	m_pShared(pShared),
	m_pDevice(pProvider->getDevice())
{
	pShared->AddRef();
	addLayer();

	m_pBonesBlob = m_pRenderFrameBones = new ModelBoneShader[pShared->getBoneCount() * 2];
	m_pNextFrameBones = m_pRenderFrameBones + pShared->getBoneCount();
	m_pBoneControllers = new XResourceModelBone[pShared->getBoneCount()];

	if(m_pDevice)
	{
		if(m_pProvider->getCore()->isOnMainThread())
		{
			initGPUresources();
		}
		else
		{
			m_pProvider->scheduleModelGPUinit(this);
		}
	}
}
CAnimatedModel::~CAnimatedModel()
{
	m_pProvider->onModelRelease(this);
	mem_release(m_pShared);
	mem_release(m_pBoneConstantBuffer);
	mem_release(m_pWorldBuffer);
	mem_release(m_pColorBuffer);

	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		mem_delete_a(m_aLayers[i].pCurrentBones);
	}

	mem_delete_a(m_pBonesBlob);
	mem_delete_a(m_pBoneControllers);
}

void CAnimatedModel::initGPUresources()
{
	if(m_pWorldBuffer)
	{
		return;
	}
	m_pBoneConstantBuffer = m_pDevice->createConstantBuffer(sizeof(ModelBoneShader) * m_pShared->getBoneCount());
	m_pWorldBuffer = m_pDevice->createConstantBuffer(sizeof(SMMATRIX));
	m_pColorBuffer = m_pDevice->createConstantBuffer(sizeof(float4));
}

bool XMETHODCALLTYPE CAnimatedModel::isEnabled() const
{
	return(m_isEnabled);
}
void XMETHODCALLTYPE CAnimatedModel::enable(bool yesNo)
{
	m_isEnabled = yesNo;
}

UINT CAnimatedModel::addLayer()
{
	layer_s *pLayer = &m_aLayers[m_aLayers.size()];

	pLayer->pCurrentBones = new XResourceModelBone[m_pShared->getBoneCount() * 2];
	pLayer->pLastFrameBones = pLayer->pCurrentBones + m_pShared->getBoneCount();

	return(m_aLayers.size() - 1);
}

void CAnimatedModel::delLayer()
{
	layer_s *pLayer = &m_aLayers[m_aLayers.size() - 1];

	mem_delete_a(pLayer->pCurrentBones);

	m_aLayers.erase(m_aLayers.size() - 1);
}

IXAnimatedModel* XMETHODCALLTYPE CAnimatedModel::asAnimatedModel()
{
	return(this);
}
IXDynamicModel* XMETHODCALLTYPE CAnimatedModel::asDynamicModel()
{
	return(NULL);
}
IXStaticModel* XMETHODCALLTYPE CAnimatedModel::asStaticModel()
{
	return(NULL);
}

float3 XMETHODCALLTYPE CAnimatedModel::getPosition() const
{
	return(m_vPosition);
}
void XMETHODCALLTYPE CAnimatedModel::setPosition(const float3 &vPos)
{
	if((float3)m_vPosition == vPos)
	{
		return;
	}
	m_vPosition = vPos;
	m_isWorldDirty = true;
}

SMQuaternion XMETHODCALLTYPE CAnimatedModel::getOrientation() const
{
	return(m_qRotation);
}
void XMETHODCALLTYPE CAnimatedModel::setOrientation(const SMQuaternion &qRot)
{
	if(m_qRotation == qRot)
	{
		return;
	}
	m_qRotation = qRot;
	m_isLocalAABBvalid = false;
	m_isWorldDirty = true;
}

float XMETHODCALLTYPE CAnimatedModel::getScale() const
{
	return(m_fScale);
}
void XMETHODCALLTYPE CAnimatedModel::setScale(float fScale)
{
	if(m_fScale == fScale)
	{
		return;
	}
	m_fScale = fScale;
	m_isLocalAABBvalid = false;
	m_isWorldDirty = true;
}

UINT XMETHODCALLTYPE CAnimatedModel::getSkin() const
{
	return(m_uSkin);
}
void XMETHODCALLTYPE CAnimatedModel::setSkin(UINT uSkin)
{
	m_uSkin = uSkin;
}

float3 XMETHODCALLTYPE CAnimatedModel::getLocalBoundMin() const
{
	_updateAABB();

	return(m_vLocalMin);
}
float3 XMETHODCALLTYPE CAnimatedModel::getLocalBoundMax() const
{
	_updateAABB();

	return(m_vLocalMax);
}
SMAABB XMETHODCALLTYPE CAnimatedModel::getLocalBound() const
{
	_updateAABB();

	return(SMAABB(m_vLocalMin, m_vLocalMax));
}

void CAnimatedModel::_updateAABB() const
{
	if(m_isLocalAABBvalid)
	{
		return;
	}

	float3 vMin = m_pShared->getLocalBoundMin() * m_fScale;
	float3 vMax = m_pShared->getLocalBoundMax() * m_fScale;

	float3 vCurrent[] = {
		m_qRotation * float3(vMin.x, vMin.y, vMin.z),
		m_qRotation * float3(vMin.x, vMin.y, vMax.z),
		m_qRotation * float3(vMin.x, vMax.y, vMin.z),
		m_qRotation * float3(vMin.x, vMax.y, vMax.z),
		m_qRotation * float3(vMax.x, vMin.y, vMin.z),
		m_qRotation * float3(vMax.x, vMin.y, vMax.z),
		m_qRotation * float3(vMax.x, vMax.y, vMin.z),
		m_qRotation * float3(vMax.x, vMax.y, vMax.z)
	};

	m_vLocalMin = (float3)SMVectorMin(
		SMVectorMin(SMVectorMin(vCurrent[0], vCurrent[1]), SMVectorMin(vCurrent[2], vCurrent[3])),
		SMVectorMin(SMVectorMin(vCurrent[4], vCurrent[5]), SMVectorMin(vCurrent[6], vCurrent[7]))
		);
	m_vLocalMax = (float3)SMVectorMax(
		SMVectorMax(SMVectorMax(vCurrent[0], vCurrent[1]), SMVectorMax(vCurrent[2], vCurrent[3])),
		SMVectorMax(SMVectorMax(vCurrent[4], vCurrent[5]), SMVectorMax(vCurrent[6], vCurrent[7]))
		);

	m_isLocalAABBvalid = true;
}

float4 XMETHODCALLTYPE CAnimatedModel::getColor() const
{
	return(m_vColor);
}
void XMETHODCALLTYPE CAnimatedModel::setColor(const float4 &vColor)
{
	m_vColor = vColor;
	m_isColorDirty = true;
}

UINT XMETHODCALLTYPE CAnimatedModel::getPhysboxCount(UINT uPartIndex) const
{
	return(m_pShared->getPhysboxCount(uPartIndex));
}
const IModelPhysbox * XMETHODCALLTYPE CAnimatedModel::getPhysBox(UINT id, UINT uPartIndex) const
{
	return(m_pShared->getPhysBox(id, uPartIndex));
}
const IXResourceModel * XMETHODCALLTYPE CAnimatedModel::getResource(UINT uIndex)
{
	return(m_pShared->getResource(uIndex));
}



UINT XMETHODCALLTYPE CAnimatedModel::getPartsCount() const
{
	return(m_pShared->getPartsCount());
}
const char * XMETHODCALLTYPE CAnimatedModel::getPartName(UINT uIndex) const
{
	return(m_pShared->getPartName(uIndex));
}
UINT XMETHODCALLTYPE CAnimatedModel::getPartIndex(const char *szName)
{
	return(m_pShared->getPartIndex(szName));
}
XMODEL_PART_FLAGS XMETHODCALLTYPE CAnimatedModel::getPartFlags(UINT uIndex) const
{
	return(m_pShared->getPartFlags(uIndex));
}
bool XMETHODCALLTYPE CAnimatedModel::isPartEnabled(UINT uIndex) const
{
	//@TODO: Implement me
	assert(!"Not implemented");
	return(false);
}
void XMETHODCALLTYPE CAnimatedModel::enablePart(UINT uIndex, bool yesNo)
{
	//@TODO: Implement me
	assert(!"Not implemented");
}

UINT XMETHODCALLTYPE CAnimatedModel::getHitboxCount(UINT uPartIndex) const
{
	return(m_pShared->getHitboxCount(uPartIndex));
}
const IModelPhysbox* XMETHODCALLTYPE CAnimatedModel::getHitbox(UINT id, UINT uPartIndex) const
{
	return(m_pShared->getHitbox(id, uPartIndex));
}
int XMETHODCALLTYPE CAnimatedModel::getHitboxBone(UINT id, UINT uPartIndex) const
{
	return(m_pShared->getHitboxBone(id, uPartIndex));
}
XHITBOXBODYPART XMETHODCALLTYPE CAnimatedModel::getHitboxBodyPart(UINT id, UINT uPartIndex) const
{
	return(m_pShared->getHitboxBodyPart(id, uPartIndex));
}

void XMETHODCALLTYPE CAnimatedModel::play(const char *szName, UINT uFadeTime, UINT uLayer, bool bReplaceActivity)
{

	//@TODO: Implement me
	if(!validateLayer(uLayer))
	{
		return;
	}

	layer_s *pLayer = &m_aLayers[uLayer];

	if(bReplaceActivity)
	{
		pLayer->iActivity = -1;
	}

	int iSequence = m_pShared->getSequenceId(szName);
	if(!ID_VALID(iSequence))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "Unable to play animation \"%s\"\n", szName);
		return;
	}

	auto pSequence = m_pShared->getSequence(iSequence);

	pLayer->uAnimationId = iSequence;
	pLayer->isPlaying = true;
	pLayer->uFrameCount = pSequence->uNumFrames;
	pLayer->iCurrentFrame = 0;
	pLayer->isNewPlaying = true;

	if(m_pCallback)
	{
		m_pCallback->onPlay(uLayer);
	}

	if(uFadeTime)
	{
		pLayer->isInFade = true;
		pLayer->uFadeTime = uFadeTime;
		pLayer->uFadeCurTime = 0;
	}
}
void XMETHODCALLTYPE CAnimatedModel::stop(UINT uLayer)
{
	if(!validateLayer(uLayer))
	{
		return;
	}

	layer_s *pLayer = &m_aLayers[uLayer];

	if(pLayer->isPlaying)
	{
		pLayer->isPlaying = false;
		if(m_pCallback)
		{
			m_pCallback->onStop(uLayer);
		}
	}
}
void XMETHODCALLTYPE CAnimatedModel::resume(UINT uLayer)
{
	if(!validateLayer(uLayer))
	{
		return;
	}

	layer_s *pLayer = &m_aLayers[uLayer];

	if(!pLayer->isPlaying)
	{
		pLayer->isPlaying = true;
		if(m_pCallback)
		{
			m_pCallback->onPlay(uLayer);
		}
	}
}
void XMETHODCALLTYPE CAnimatedModel::setProgress(float fProgress, UINT uLayer)
{
	if(!validateLayer(uLayer))
	{
		return;
	}

	layer_s *pLayer = &m_aLayers[uLayer];

	const XResourceModelSequence *pCurAnim = m_pShared->getSequence(pLayer->uAnimationId);

	float fr = 1;
	if(pCurAnim->iFramerate)
	{
		fr = 1000.0f / pCurAnim->iFramerate;
	}
	pLayer->iCurrentFrame = (int)(pLayer->uFrameCount * fProgress);
	pLayer->fTime = (fProgress - (float)pLayer->iCurrentFrame / (float)pLayer->uFrameCount) * fr * pLayer->uFrameCount;
	pLayer->isDirty = true;
}
void XMETHODCALLTYPE CAnimatedModel::startActivity(const char *szName, UINT uFadeTime, UINT uLayer)
{
	if(!validateLayer(uLayer))
	{
		return;
	}

	int iActivity = m_pShared->getActivityId(szName);
	if(!ID_VALID(iActivity))
	{
		LibReport(REPORT_MSG_LEVEL_WARNING, "CAnimatedModel::startActivity(): Activity '%s' is not found!\n", szName);
		return;
	}
	m_aLayers[uLayer].iActivity = iActivity;
	m_aLayers[uLayer].uActivityFadeTime = uFadeTime;

	playActivityNext(uLayer);
}
void XMETHODCALLTYPE CAnimatedModel::stopAll()
{
	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		layer_s *pLayer = &m_aLayers[i];
		if(pLayer->isPlaying)
		{
			pLayer->isPlaying = false;
			pLayer->isInFade = false;
			if(m_pCallback)
			{
				m_pCallback->onStop(i);
			}
		}
	}
}

void CAnimatedModel::playActivityNext(UINT uLayer)
{
	if(!validateLayer(uLayer))
	{
		return;
	}

	layer_s *pLayer = &m_aLayers[uLayer];
	if(pLayer->iActivity < 0)
	{
		return;
	}
	float fMaxChance = 0.0f;
	int iCount = 0;
	for(UINT i = 0, l = m_pShared->getSequenceCount(); i < l; ++i)
	{
		if(m_pShared->getSequence(i)->iActivity == pLayer->iActivity)
		{
			fMaxChance += (float)m_pShared->getSequence(i)->uActivityChance;
			++iCount;
		}
	}
	float fch = randf(0.0f, fMaxChance);
	fMaxChance = 0.0f;
	for(int i = 0, l = m_pShared->getSequenceCount(); i < l; ++i)
	{
		if(m_pShared->getSequence(i)->iActivity == pLayer->iActivity)
		{
			fMaxChance += (float)m_pShared->getSequence(i)->uActivityChance;
			if(fMaxChance >= fch)
			{
				play(m_pShared->getSequence(i)->szName, pLayer->uActivityFadeTime, uLayer, false);
				break;
			}
		}
	}
}

float3 XMETHODCALLTYPE CAnimatedModel::getBoneTransformPos(UINT id, XMODEL_BONE_TRANSFORM boneTranform)
{
	if(id >= m_pShared->getBoneCount())
	{
		assert(!"Invalid ID supplied");
		LibReport(REPORT_MSG_LEVEL_WARNING, "CAnimatedModel::getBoneTransformPos() Invalid bone id requested");
		return(0);
	}
	switch(boneTranform)
	{
	case XMBT_WORLD:
		return(getOrientation() * ((m_pRenderFrameBones[id].position - m_pRenderFrameBones[id].orient * (float3)m_pShared->getInvertedBindPose()[id].position)) * m_fScale + getPosition());
	case XMBT_RENDER:
		return(getOrientation() * m_pRenderFrameBones[id].position * m_fScale + getPosition());
	}

	return(0);
}
SMQuaternion XMETHODCALLTYPE CAnimatedModel::getBoneTransformRot(UINT id, XMODEL_BONE_TRANSFORM boneTranform)
{
	if(id >= m_pShared->getBoneCount())
	{
		assert(!"Invalid ID supplied");
		LibReport(REPORT_MSG_LEVEL_WARNING, "CAnimatedModel::getBoneTransformRot() Invalid bone id requested");
		return(SMQuaternion());
	}

	switch(boneTranform)
	{
	case XMBT_WORLD:
	case XMBT_RENDER:
		return(m_pRenderFrameBones[id].orient * getOrientation());
	}
	
	return(SMQuaternion());
}

int XMETHODCALLTYPE CAnimatedModel::getBoneParent(UINT id)
{
	if(id >= m_pShared->getBoneCount())
	{
		assert(!"Invalid ID supplied");
		LibReport(REPORT_MSG_LEVEL_WARNING, "CAnimatedModel::getBoneParent() Invalid bone id requested");
		return(-1);
	}

	return(m_pShared->getBoneParent(id));
}

SMMATRIX XMETHODCALLTYPE CAnimatedModel::getBoneTransform(UINT id)
{
	//@TODO: Implement me
	assert(!"Not implemented");
	return(SMMatrixIdentity());
}

UINT XMETHODCALLTYPE CAnimatedModel::getBoneId(const char *szName)
{
	return(m_pShared->getBoneId(szName));
}
UINT XMETHODCALLTYPE CAnimatedModel::getBoneCount() const
{
	return(m_pShared->getBoneCount());
}
const char * XMETHODCALLTYPE CAnimatedModel::getBoneName(UINT id) const
{
	return(m_pShared->getBoneName(id));
}

bool XMETHODCALLTYPE CAnimatedModel::isPlayingAnimations()
{
	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		if(m_aLayers[i].isPlaying)
		{
			return(true);
		}
	}
	return(false);
}
bool XMETHODCALLTYPE CAnimatedModel::isPlayingAnimation(const char *szName)
{
	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		if(m_aLayers[i].isPlaying && !strcasecmp(m_pShared->getSequence(m_aLayers[i].uAnimationId)->szName, szName))
		{
			return(true);
		}
	}
	return(false);
}

void XMETHODCALLTYPE CAnimatedModel::setController(UINT id, float fValue)
{
	//@TODO: Implement me
	assert(!"Not implemented");
}

UINT XMETHODCALLTYPE CAnimatedModel::getControllersCount() const
{
	return(m_pShared->getControllersCount());
}
const char* XMETHODCALLTYPE CAnimatedModel::getControllerName(UINT id)
{
	return(m_pShared->getControllerName(id));
}
UINT XMETHODCALLTYPE CAnimatedModel::getControllerId(const char *szName)
{
	return(m_pShared->getControllerId(szName));
}

void XMETHODCALLTYPE CAnimatedModel::setCallback(IAnimationCallback *pCallback)
{
	m_pCallback = pCallback;
}

void CAnimatedModel::update(float fDT)
{
	int deltat = (int)(fDT * 1000.0f);

	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		layer_s *pLayer = &m_aLayers[i];
		if(pLayer->isInFade)
		{
			if(!m_wasInitiated)
			{
				m_wasInitiated = true;
			}
			if(pLayer->uFadeCurTime == 0)
			{
				memcpy(pLayer->pLastFrameBones, pLayer->pCurrentBones, sizeof(XResourceModelBone) * m_pShared->getBoneCount());
			}

			pLayer->uFadeCurTime += deltat;

			auto pCurAnim = m_pShared->getSequence(pLayer->uAnimationId);

			bool rev = pCurAnim->iFramerate < 0;
			int frame = 0;
			if(rev)
			{
				frame = pLayer->uFrameCount - 2;
			}

			float delta = (float)pLayer->uFadeCurTime / (float)pLayer->uFadeTime;
			for(UINT j = 0, jl = m_pShared->getBoneCount(); j < jl; ++j)
			{
				pLayer->pCurrentBones[j].orient = SMquaternionSlerp(pLayer->pLastFrameBones[j].orient, pCurAnim->m_ppSequenceData[frame][j].orient * m_pBoneControllers[j].orient, delta)/*.Normalize()*/;
				pLayer->pCurrentBones[j].position = (float3_t)(pLayer->pLastFrameBones[j].position + (pCurAnim->m_ppSequenceData[frame][j].position + m_pBoneControllers[j].position - pLayer->pLastFrameBones[j].position) * delta);
			}
			fillBoneMatrix();

			if(pLayer->uFadeCurTime >= pLayer->uFadeTime)
			{
				pLayer->isInFade = false;
				deltat = pLayer->uFadeCurTime - pLayer->uFadeTime;
			}
			else
			{
				continue;
			}
		}

		if(pLayer->isPlaying/* || pLayer->isDirty*/)
		{
			if(!m_wasInitiated)
			{
				m_wasInitiated = true;
			}
			if(!pLayer->uFrameCount)
			{
				pLayer->isPlaying = false;
				if(m_pCallback)
				{
					m_pCallback->onProgress(i, 0.0f);
					m_pCallback->onStop(i);
				}
				continue;
			}

			auto pCurAnim = m_pShared->getSequence(pLayer->uAnimationId);
			

			float fr = 1;
			bool rev = pCurAnim->iFramerate < 0;
			if(pCurAnim->iFramerate)
			{
				fr = 1000.0f / pCurAnim->iFramerate;
			}
			if(rev)
			{
				fr = -fr;
			}
			if(pLayer->isNewPlaying)
			{
				pLayer->fTime = rev ? fr : 0.0f;
				pLayer->isNewPlaying = false;
				if(rev)
				{
					pLayer->iCurrentFrame = -2;
				}
			}
			if(pCurAnim->iFramerate)
			{
				if(pLayer->bDoAdvance)
				{
					pLayer->fTime += rev ? -deltat : deltat;
				}
				bool cont = false;
				while((pLayer->fTime >= fr && !rev) || (pLayer->fTime <= 0 && rev))
				{
					pLayer->iCurrentFrame += rev ? -1 : 1;
					
					if((!rev && pLayer->iCurrentFrame >= (int)pLayer->uFrameCount - 1) || (rev && pLayer->iCurrentFrame <= 0))
					{
						if(pCurAnim->isLooped)
						{
							if(m_pCallback)
							{
								m_pCallback->onLoop(i);
							}
							playActivityNext(i);
						}
						else
						{
							pLayer->isPlaying = false;
							if(m_pCallback)
							{
								m_pCallback->onStop(i);
							}
							pLayer->fTime = 0.0f;
							cont = true;
							break;
						}
					}

					pLayer->fTime += rev ? fr : -fr;
				}
				/*if(cont)
				{
					continue;
				}*/
				if(pLayer->iCurrentFrame < 0)
				{
					pLayer->iCurrentFrame += pLayer->uFrameCount;
				}
				pLayer->iCurrentFrame %= pLayer->uFrameCount;

			}

			int nextFrame = pLayer->iCurrentFrame + (rev ? -1 : 1);
			if(nextFrame < 0)
			{
				nextFrame += pLayer->uFrameCount;
			}
			nextFrame %= pLayer->uFrameCount;

			float delta = pLayer->fTime / fr;
			if(rev)
			{
				delta = 1.0f - delta;
			}

			if(m_pCallback)
			{
				m_pCallback->onProgress(i, ((float)pLayer->iCurrentFrame + (rev ? -delta : delta)) / (float)pLayer->uFrameCount);
			}
			
			for(UINT j = 0, jl = m_pShared->getBoneCount(); j < jl; ++j)
			{
				pLayer->pCurrentBones[j].orient = (SMquaternionSlerp(pCurAnim->m_ppSequenceData[pLayer->iCurrentFrame][j].orient, pCurAnim->m_ppSequenceData[nextFrame][j].orient, delta) * m_pBoneControllers[j].orient);
				pLayer->pCurrentBones[j].position = (float3_t)(pCurAnim->m_ppSequenceData[pLayer->iCurrentFrame][j].position + (pCurAnim->m_ppSequenceData[nextFrame][j].position - pCurAnim->m_ppSequenceData[pLayer->iCurrentFrame][j].position) * delta + m_pBoneControllers[j].position);
			}
		}
		//fillBoneMatrix();
	}

	if(!m_wasInitiated)
	{
		m_wasInitiated = true;

		layer_s *pLayer = &m_aLayers[0];

		for(UINT j = 0, jl = m_pShared->getBoneCount(); j < jl; ++j)
		{
			pLayer->pCurrentBones[j] = *m_pShared->getBone(j);
		}
	}

	fillBoneMatrix();
}

bool CAnimatedModel::validateLayer(UINT uLayer) const
{
	if(uLayer < m_aLayers.size())
	{
		return(true);
	}

	LibReport(REPORT_MSG_LEVEL_WARNING, "CAnimatedModel::validateLayer(): Invalid layer %u\n", uLayer);

	return(false);
}

void CAnimatedModel::fillBoneMatrix()
{
	auto pInvBindPose = m_pShared->getInvertedBindPose();

	for(UINT i = 0, l = m_pShared->getBoneCount(); i < l; ++i)
	{
		m_pNextFrameBones[i].orient = SMQuaternion();
		m_pNextFrameBones[i].position = float4();
	}

	//рассчитываем суммарную анимацию
	for(UINT j = 0, jl = m_aLayers.size(); j < jl; ++j)
	{
		layer_s *pLayer = &m_aLayers[j];

		for(UINT i = 0, l = m_pShared->getBoneCount(); i < l; ++i)
		{
			//if(j == 0/* || !m_pIsBoneWorld[0][i]*/)
			{
				auto bineBind = m_pShared->getBone(i);
				m_pNextFrameBones[i].position = (float4)(m_pNextFrameBones[i].position + (pLayer->pCurrentBones[i].position - bineBind->position)) * pLayer->fWeight;
				m_pNextFrameBones[i].orient = SMquaternionSlerp(SMQuaternion(), m_pNextFrameBones[i].orient * (pLayer->pCurrentBones[i].orient * bineBind->orient.Conjugate()), pLayer->fWeight);
			}
		}
	}
	for(UINT i = 0, l = m_pShared->getBoneCount(); i < l; ++i)
	{
		auto bineBind = m_pShared->getBone(i);

		m_pNextFrameBones[i].position = (float4)(m_pNextFrameBones[i].position + (bineBind->position));
		m_pNextFrameBones[i].orient = m_pNextFrameBones[i].orient * bineBind->orient;
	}


	//Переводим рассчитанный скелет в Мировую СК
	for(UINT i = 0, l = m_pShared->getBoneCount(); i < l; ++i)
	{
		int iParent = m_pShared->getBoneParent(i);
		if(/*!m_pIsBoneWorld[0][i] && */iParent != -1)
		{
			assert(iParent >= 0 && (UINT)iParent < l);
			m_pNextFrameBones[i].orient = (m_pNextFrameBones[i].orient * m_pNextFrameBones[iParent].orient);
			m_pNextFrameBones[i].position = (float4)(float4(m_pNextFrameBones[iParent].orient * (float3)m_pNextFrameBones[i].position, 1.0f) + m_pNextFrameBones[iParent].position);
		}
	}
			
	//Домножаем на инвертированные трансформации позы скиннинга
	for(UINT i = 0; i < m_pShared->getBoneCount(); i++)
	{
		SMQuaternion q = pInvBindPose[i].orient * m_pNextFrameBones[i].orient;
		m_pNextFrameBones[i].orient = q;

		m_pNextFrameBones[i].position = (float4)(m_pNextFrameBones[i].position + float4(q * (float3)pInvBindPose[i].position, 1.0));
	}

	m_isBoneMatrixReFilled = true;
}

void XMETHODCALLTYPE CAnimatedModel::render(UINT uLod, XMODEL_FEATURE bmFeatures)
{
	if(!m_pDevice || !m_isEnabled || !m_pWorldBuffer)
	{
		return;
	}

	if(m_isBoneMatrixReFilled)
	{
		m_pBoneConstantBuffer->update(m_pRenderFrameBones);
		m_isBoneMatrixReFilled = false;
	}

	if(m_isWorldDirty)
	{
		m_pWorldBuffer->update(&SMMatrixTranspose(SMMatrixScaling(m_fScale) * m_qRotation.GetMatrix() * SMMatrixTranslation(m_vPosition)));
		m_isWorldDirty = false;
	}

	if(m_isColorDirty)
	{
		m_pColorBuffer->update(&m_vColor);
		m_isColorDirty = false;
	}

	m_pProvider->bindVertexFormat();

	IGXContext *pCtx = m_pDevice->getThreadContext();

	pCtx->setVSConstant(m_pWorldBuffer, 1 /* SCR_OBJECT */);
	m_pDevice->getThreadContext()->setPSConstant(m_pColorBuffer, 8);
	pCtx->setVSConstant(m_pBoneConstantBuffer, 10);

	m_pShared->render(m_uSkin, uLod);
}

void CAnimatedModel::sync()
{
	ModelBoneShader * tmp = m_pRenderFrameBones;
	m_pRenderFrameBones = m_pNextFrameBones;
	m_pNextFrameBones = tmp;
}

bool XMETHODCALLTYPE CAnimatedModel::rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut, float3 *pvNormal, bool isRayInWorldSpace, bool bReturnNearestPoint)
{
	float3 vRayStart = vStart;
	float3 vRayEnd = vEnd;
	if(isRayInWorldSpace)
	{
		float fInvScale = 1.0f / getScale();
		float3 vDir = getOrientation().Conjugate() * (vEnd - vStart) * fInvScale;
		vRayStart = (getOrientation().Conjugate() * (vRayStart - getPosition())) * fInvScale;
		vRayEnd = vRayStart + vDir;
	}

	// FIXME implement in more accurate way

	if(SMRayIntersectAABB(SMAABB(m_vLocalMin, m_vLocalMax), vRayStart, vRayEnd - vRayStart))
	{
		if(pvOut)
		{
			*pvOut = isRayInWorldSpace ? getPosition() : float3(0.0f);
		}
		if(pvNormal)
		{
			*pvNormal = vRayStart - vRayEnd;
		}
		return(true);
	}

	return(false);
}

void XMETHODCALLTYPE CAnimatedModel::setLayer(UINT uLayer)
{
	m_uLayer = uLayer;
}
UINT XMETHODCALLTYPE CAnimatedModel::getLayer()
{
	return(m_uLayer);
}

UINT XMETHODCALLTYPE CAnimatedModel::getLayersCount() const
{
	return(m_aLayers.size());
}
void XMETHODCALLTYPE CAnimatedModel::setLayersCount(UINT uCount)
{
	for(UINT i = m_aLayers.size(); i < uCount; ++i)
	{
		addLayer();
	}
	
	for(UINT i = uCount, l = m_aLayers.size(); i < l; ++i)
	{
		delLayer();
	}
}
float XMETHODCALLTYPE CAnimatedModel::getLayerBlendWeight(UINT uLayer) const
{
	if(!validateLayer(uLayer))
	{
		return(0.0f);
	}

	const layer_s *pLayer = &m_aLayers[uLayer];

	return(pLayer->fWeight);
}
void XMETHODCALLTYPE CAnimatedModel::setLayerBlendWeight(UINT uLayer, float fWeight)
{
	if(!validateLayer(uLayer))
	{
		return;
	}

	layer_s *pLayer = &m_aLayers[uLayer];

	pLayer->fWeight = fWeight;
}
