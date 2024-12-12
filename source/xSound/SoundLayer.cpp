
#include "SoundLayer.h"

#include "SoundEmitter.h"
#include "SoundPlayer.h"
#include "SoundSystem.h"

//##########################################################################

CSoundLayer::~CSoundLayer()
{
	mem_release(m_pPrimaryBuffer);
	mem_release(m_pParent);
}

//**************************************************************************

void XMETHODCALLTYPE CSoundLayer::FinalRelease()
{
	if(m_pParent)
	{
		SndQueueMsg oMsg;
		oMsg.type = SND_QUEUE_MSG_TYPE_LAYER_DELETE;
		oMsg.pLayer = this;
		oMsg.pOwner = m_pParent;
		m_pParent->addMessage(oMsg);
	}
	else
	{
		delete this;
	}
}

//**************************************************************************

bool CSoundLayer::init(CSoundSystem *pSoundSystem, CSoundLayer *pParent, const AudioRawDesc *pDesc, const char *szName)
{
	if(!pDesc || !pSoundSystem)
		return false;

#if 0
	if(m_mapLayers.KeyExists(szName))
	{
		return false;
	}
#endif

	m_sName = szName;
	m_pParent = pParent;
	m_pSoundSystem = pSoundSystem;

	AudioRawDesc oDesc = *pDesc;
	oDesc.uSize = 0;

	if(pParent == NULL)
		m_pPrimaryBuffer = pSoundSystem->createMasterBuffer(&oDesc);
	else
		m_pPrimaryBuffer = dynamic_cast<IAudioBufferEx*>(pParent->createAudioBuffer(AB_TYPE_PRIMARY, &oDesc));

	if(m_pParent)
		m_pParent->AddRef();

	return true;
}

//**************************************************************************

void CSoundLayer::addLayer(CSoundLayer *pLayer)
{
	if(m_pSoundSystem->findLayer(pLayer->getName()))
		LibReport(REPORT_MSG_LEVEL_FATAL, "Layer name '%s' exists", pLayer->getName());

	m_mapLayers[pLayer->getName()] = pLayer;
	m_aLayers.push_back(pLayer);
}

void CSoundLayer::delLayer(CSoundLayer *pLayer)
{
	for(MapLayer::Iterator i = m_mapLayers.begin(); i; ++i)
	{
		if(*i.second == pLayer)
		{
			*i.second = NULL;
			break;
		}
	}

	int idx = m_aLayers.indexOf(pLayer);
	if(idx >= 0)
	{
		m_aLayers.erase(idx);
	}

	mem_delete(pLayer);
}

//**************************************************************************

void CSoundLayer::addSndPlayer(CSoundPlayer *pSndPlayer)
{
	m_mapSndPlayers[pSndPlayer->getName()].push_back(pSndPlayer);
	m_aSndPlayers.push_back(pSndPlayer);
}

void CSoundLayer::delSndPlayer(const CSoundPlayer *pSndPlayer)
{
	const char *szName = pSndPlayer->getName();
	if(!m_mapSndPlayers.KeyExists(szName))
		return;

	ArrayPlayer &oAP = m_mapSndPlayers[szName];
	int idx = oAP.indexOf(pSndPlayer);
	if(idx >= 0)
	{
		oAP.erase(idx);
	} 
	
	idx = m_aSndPlayers.indexOf(pSndPlayer);
	if(idx >= 0)
		{
		m_aSndPlayers.erase(idx);
		}

	mem_delete(pSndPlayer);
}

//**************************************************************************

void CSoundLayer::addSndEmitter(CSoundEmitter *pSndEmitter)
{
	m_mapSndEmitters[pSndEmitter->getName()].push_back(pSndEmitter);
	m_aSndEmitters.push_back(pSndEmitter);
}

void CSoundLayer::delSndEmitter(const CSoundEmitter *pSndEmitter)
{
	const char *szName = pSndEmitter->getName();
	if(!m_mapSndEmitters.KeyExists(szName))
		return;

	ArrayEmitter &oAE = m_mapSndEmitters[szName];
	int idx = oAE.indexOf(pSndEmitter);
	if(idx >= 0)
	{
		oAE.erase(idx);
	}

	idx = m_aSndEmitters.indexOf(pSndEmitter);
	if(idx >= 0)
		{
		m_aSndEmitters.erase(idx);
		}

	mem_delete(pSndEmitter);
}

//**************************************************************************

void XMETHODCALLTYPE CSoundLayer::getDesc(AudioRawDesc *pDesc) const
{
	if(!m_pPrimaryBuffer)
		return;

	m_pPrimaryBuffer->getDesc(pDesc);
}

//**************************************************************************

const char* XMETHODCALLTYPE CSoundLayer::getName() const
{
	return m_sName.c_str();
}

//**************************************************************************

IXSoundLayer* XMETHODCALLTYPE CSoundLayer::findLayer(const char *szName)
{
	if(!szName)
		return NULL;

	if(strcasecmp(getName(), szName) == 0)
		return this;

	IXSoundLayer *pFound = NULL;
	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		if((pFound = m_aLayers[i]->findLayer(szName)))
			break;
	}

	return pFound;
}

//**************************************************************************

size_t CSoundLayer::getStreamChunkSize(AudioRawDesc *pDesc) const
{
	return m_pPrimaryBuffer->getStreamChunkSize(pDesc);
}

IAudioBuffer* CSoundLayer::createAudioBuffer(AB_TYPE type, const AudioRawDesc *pDesc)
{
	return m_pPrimaryBuffer->createAudioBuffer(type, pDesc);
}

//##########################################################################

IXSoundLayer* XMETHODCALLTYPE CSoundLayer::newSoundLayer(const char *szName, const AudioRawDesc *pDesc)
{
	AudioRawDesc desc;
	if(!pDesc)
	{
		pDesc = &desc;
		m_pPrimaryBuffer->getDesc(&desc);
	}
	else if(!matchPrimaryLayer(pDesc))
	{
		return(NULL);
	}

	CSoundLayer *pLayer = new CSoundLayer();
	pLayer->init(m_pSoundSystem, this, pDesc, szName);

	addLayer(pLayer);

//	SndQueueMsg oMsg;
//	oMsg.type = SND_QUEUE_MSG_TYPE_LAYER_NEW;
//	oMsg.pLayer = pLayer;
//	oMsg.pOwner = this;
//	m_pSoundSystem->addMessage(oMsg);

	return pLayer;
}

//**************************************************************************

IXSoundEmitter* XMETHODCALLTYPE CSoundLayer::newSoundEmitter(const char *szName, SOUND_SPACE space)
{
	if(!szName)
		return NULL;

	CSoundEmitter *pEmitter = NULL;

	if(m_mapSndEmitters.KeyExists(szName) && m_mapSndEmitters[szName].size() > 0)
	{
		CSoundEmitter *pEmitterOrigin = m_mapSndEmitters[szName][0];
		if(pEmitterOrigin->canInstance())
			pEmitter = (CSoundEmitter*)pEmitterOrigin->newInstance();
	}

	if(!pEmitter)
	{
		IXAudioCodecTarget *pCodecTarget = m_pSoundSystem->getCodecTarget(szName);
		if(!pCodecTarget)
			return NULL;

		pEmitter = new CSoundEmitter();
		pEmitter->create(szName, this, pCodecTarget, space);
	}

	SndQueueMsg oMsg;
	oMsg.type = SND_QUEUE_MSG_TYPE_SND_NEW;
	oMsg.pEmitter = pEmitter;
	oMsg.pOwner = this;
	m_pSoundSystem->addMessage(oMsg);

	return pEmitter;
}

//**************************************************************************

IXSoundPlayer* XMETHODCALLTYPE CSoundLayer::newSoundPlayer(const char *szName, SOUND_SPACE space)
{
	if(!szName)
		return NULL;

	CSoundPlayer *pPlayer = NULL;

	if(m_mapSndPlayers.KeyExists(szName) && m_mapSndPlayers[szName].size() > 0)
	{
		CSoundPlayer *pPlayerOrigin = m_mapSndPlayers[szName][0];
		if(pPlayerOrigin->canInstance())
			pPlayer = (CSoundPlayer*)pPlayerOrigin->newInstance();
	}

	if(!pPlayer)
	{
		IXAudioCodecTarget *pCodecTarget = m_pSoundSystem->getCodecTarget(szName);
		if(!pCodecTarget)
			return NULL;

		pPlayer = new CSoundPlayer();
		pPlayer->create(szName, this, pCodecTarget, space);
	}

	SndQueueMsg oMsg;
	oMsg.type = SND_QUEUE_MSG_TYPE_SND_NEW;
	oMsg.pPlayer = pPlayer;
	oMsg.pOwner = this;
	m_pSoundSystem->addMessage(oMsg);

	return pPlayer;
}

//##########################################################################

void XMETHODCALLTYPE CSoundLayer::play(bool canPlay)
{
	SndQueueMsg oMsg;
	oMsg.type = SND_QUEUE_MSG_TYPE_LAYER_PLAY;
	oMsg.pLayer = this;
	oMsg.arg.b = canPlay;
	addMessage(oMsg);
}

//**************************************************************************

bool XMETHODCALLTYPE CSoundLayer::isPlaying() const
{
	return(m_isPlayingTotal);
}

//##########################################################################

void CSoundLayer::update(const float3 &vListenerPos, const float3 &vListenerDir, const float3 &vListenerUp)
{
	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		m_aLayers[i]->update(vListenerPos, vListenerDir, vListenerUp);
	}

	for(UINT i = 0, l = m_aSndPlayers.size(); i < l; ++i)
	{
		m_aSndPlayers[i]->update(vListenerPos, vListenerDir, vListenerUp);
	}

	for(UINT i = 0, l = m_aSndEmitters.size(); i < l; ++i)
	{
		m_aSndEmitters[i]->update(vListenerPos, vListenerDir, vListenerUp);
	}
}

//**************************************************************************

void CSoundLayer::addMessage(SndQueueMsg &oMsg)
{
	m_pSoundSystem->addMessage(oMsg);
}

//##########################################################################

bool CSoundLayer::matchPrimaryLayer(const AudioRawDesc *pDesc)
{
	if(!pDesc || !m_pPrimaryBuffer)
		return false;

	AudioRawDesc oPrimaryDesc;
	m_pPrimaryBuffer->getDesc(&oPrimaryDesc);

	return(oPrimaryDesc.uSampleRate == pDesc->uSampleRate);
}

//##########################################################################
//##########################################################################

void CSoundLayer::_play(bool canPlay, bool isFromParent)
{
	//if(m_isPlaying == canPlay || (m_pParent && !m_pParent->isPlaying() && canPlay))
	//	return;
	//
	//m_isPlaying = canPlay;
	if(!isFromParent)
	{
		m_isPlaying = canPlay;
	}
	bool isPlaying = (!m_pParent || m_pParent->isPlaying()) && m_isPlaying;

	if(m_isPlayingTotal == isPlaying)
	{
		return;
	}
	m_isPlayingTotal = isPlaying;

	for(UINT i = 0, l = m_aLayers.size(); i < l; ++i)
	{
		m_aLayers[i]->_play(isPlaying, true);
	}

	for(UINT i = 0, l = m_aSndPlayers.size(); i < l; ++i)
		{
		m_aSndPlayers[i]->_onLayerPlay(isPlaying);
	}

	for(UINT i = 0, l = m_aSndEmitters.size(); i < l; ++i)
		{
		m_aSndEmitters[i]->_onLayerPlay(isPlaying);
	}

	m_pPrimaryBuffer->play(isPlaying);
}
