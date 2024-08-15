
#include "SoundSystem.h"
#include "AudioConverter.h"

#include "SoundLayer.h"
#include "SoundEmitter.h"
#include "SoundPlayer.h"

#include <common/path_utils.h>

#ifdef _MSC_VER
#include <direct.h>
#endif

//##########################################################################

CSoundSystem::CSoundSystem(IXCore *pXCore)
{
	m_pXCore = pXCore;

	auto pPluginManager = m_pXCore->getPluginManager();

	IXAudioCodec *pAudioCodec;
	UINT ic = 0;
	while((pAudioCodec = (IXAudioCodec*)pPluginManager->getInterface(IXAUDIOCODEC_GUID, ic++)))
	{
		if(pAudioCodec->getVersion() == IXAUDIOCODEC_VERSION)
		{
			LibReport(REPORT_MSG_LEVEL_NOTICE, "Registered audio codec: %s\n", pAudioCodec->getFormat());

			addCodec(pAudioCodec->getFormat(), pAudioCodec);
		}
	}

	m_pAudio = MitAl_InitAudio();
}

CSoundSystem::~CSoundSystem()
{
	mem_release(m_pMasterLayer);
	update(float3(), float3(), float3());
}

//**************************************************************************

void CSoundSystem::addMessage(SndQueueMsg &oMsg)
{
	m_queue.push(oMsg);
}

void CSoundSystem::update(const float3 &vListenerPos, const float3 &vListenerDir, const float3 &vListenerUp)
{
	if(m_pMasterLayer)
		m_pMasterLayer->update(vListenerPos, vListenerDir, vListenerUp);
	
	SndQueueMsg oMsg;
	while(m_queue.pop(&oMsg))
	{
		switch(oMsg.type)
		{
		case SND_QUEUE_MSG_TYPE_SND_NEW:
			{
				if(oMsg.pEmitter)
					oMsg.pOwner->addSndEmitter(oMsg.pEmitter);
				else if(oMsg.pPlayer)
					oMsg.pOwner->addSndPlayer(oMsg.pPlayer);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_DELETE:
			{
				if(oMsg.pEmitter)
					oMsg.pOwner->delSndEmitter(oMsg.pEmitter);
				else if(oMsg.pPlayer)
					oMsg.pOwner->delSndPlayer(oMsg.pPlayer);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_PLAY:
			{
				if(oMsg.arg.state == SOUND_STATE_PLAY)
				{
					if(oMsg.pEmitter)
						oMsg.pEmitter->_play();
					else if(oMsg.pPlayer)
						oMsg.pPlayer->_play();
				}
				else if(oMsg.arg.state == SOUND_STATE_PAUSE)
				{
					if(oMsg.pEmitter)
						oMsg.pEmitter->_pause();
					else if(oMsg.pPlayer)
						oMsg.pPlayer->_pause();
				}
				else if(oMsg.arg.state == SOUND_STATE_STOP)
				{
					if(oMsg.pPlayer)
						oMsg.pPlayer->_stop();
				}

				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_RESUME:
			{
				if(oMsg.pEmitter)
					oMsg.pEmitter->_resume();
				else if(oMsg.pPlayer)
					oMsg.pPlayer->_resume();
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_LOOP:
			{
				if(oMsg.pPlayer)
					oMsg.pPlayer->_setLoop(oMsg.arg.loop);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_SPACE:
			{
				if(oMsg.pEmitter)
					oMsg.pEmitter->_setSpace(oMsg.arg.space);
				else if(oMsg.pPlayer)
					oMsg.pPlayer->_setSpace(oMsg.arg.space);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_PAN:
			{
				if(oMsg.pEmitter)
					oMsg.pEmitter->_setPan(oMsg.arg.f);
				else if(oMsg.pPlayer)
					oMsg.pPlayer->_setPan(oMsg.arg.f);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_VOLUME:
			{
				if(oMsg.pEmitter)
					oMsg.pEmitter->_setVolume(oMsg.arg.f);
				else if(oMsg.pPlayer)
					oMsg.pPlayer->_setVolume(oMsg.arg.f);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_POS:
			{
				float3 vPos(oMsg.arg.vector[0], oMsg.arg.vector[1], oMsg.arg.vector[2]);
				if(oMsg.pEmitter)
					oMsg.pEmitter->_setWorldPos(vPos);
				else if(oMsg.pPlayer)
					oMsg.pPlayer->_setWorldPos(vPos);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_DIST:
			{
				if(oMsg.pEmitter)
					oMsg.pEmitter->_setDistance(oMsg.arg.f);
				else if(oMsg.pPlayer)
					oMsg.pPlayer->_setDistance(oMsg.arg.f);
				break;
			}
		case SND_QUEUE_MSG_TYPE_SND_TIME:
			{
				if(oMsg.pPlayer)
					oMsg.pPlayer->_setTime(oMsg.arg.f);
				break;
			}
		case SND_QUEUE_MSG_TYPE_LAYER_NEW:
			{
				if(oMsg.pOwner && oMsg.pLayer)
					oMsg.pOwner->addLayer(oMsg.pLayer);
				break;
			}
		case SND_QUEUE_MSG_TYPE_LAYER_DELETE:
			{
				if(oMsg.pOwner && oMsg.pLayer)
					oMsg.pOwner->delLayer(oMsg.pLayer);
				break;
			}

		case SND_QUEUE_MSG_TYPE_LAYER_PLAY:
			{
				if(oMsg.pLayer)
					oMsg.pLayer->_play(oMsg.arg.b, false);
				break;
			}

		default:
			assert(!"Unknown message type!");
		}
	}
}

//**************************************************************************

IXSoundLayer* XMETHODCALLTYPE CSoundSystem::createMasterLayer(const AudioRawDesc *pDesc, const char *szName)
{
	if(m_pMasterLayer)
		return m_pMasterLayer;

	if(!pDesc)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Not found audio buffer description");
		return NULL;
	}

	if(!supportedDesc(pDesc, AB_TYPE_PRIMARY))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unsupported audio buffer description");
		return NULL;
	}

	CSoundLayer *pLayer = new CSoundLayer();
	pLayer->init(this, NULL, pDesc, szName);

	m_pMasterLayer = pLayer;

	return pLayer;
}

//**************************************************************************

IXCore* CSoundSystem::getCore() const
{
	return m_pXCore;
}

//**************************************************************************

IXAudioCodecTarget* CSoundSystem::getCodecTarget(const char *szName)
{
	if(!m_pMasterLayer)
		return NULL;

	if(!szName)
		return NULL;

	const char *szPath = szName;

	if(!m_pXCore->getFileSystem()->fileExists(szPath))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "File '%s' not found\n", szPath);
		return NULL;
	}

	IXAudioCodec *pCodec = NULL;
	IXAudioCodecTarget *pTarget = NULL;
	IXAudioCodecTarget *pTarget2 = NULL;

	for(MapCodec::Iterator i = m_mapCodecs.begin(); i; ++i)
	{
		if(m_mapCodecs[i.first]->open(szPath, "", &pTarget, false))
		{
			pCodec = m_mapCodecs[i.first];
			break;
		}
	}

	if(!pCodec || !pTarget)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Unsupported audio format '%s'\n", szPath);
		return NULL;
	}

	AudioRawDesc oDescMaster, oDescSnd, oDescCache;
	m_pMasterLayer->getDesc(&oDescMaster);
	pTarget->getDesc(&oDescSnd);

	//если звук соответствует требованиям мастер буфера, тогда не надо конвертировать, возвращает целевой кодек
	if(oDescSnd.uSampleRate == oDescMaster.uSampleRate)
		return pTarget;


	//если кодек может сохранять, тогда пересохраняем файл
	if(pCodec->canSave())
	{
		//выдергиваем pcm данные из таргета и удаляем его
		IXBuffer *pBufferIn = m_pXCore->newBuffer();
		IXBuffer *pBufferOut = m_pXCore->newBuffer();
		pBufferIn->alloc(oDescSnd.uSize);
		BYTE *pData = pBufferIn->get();
		pTarget->decode(0, pBufferIn->size(), (void**)&pData);
		pTarget->Release();

		//открываем файл на запись
		if(!pCodec->open(szPath, "", &pTarget, true))
			return NULL;

		//создаем нужное описание и конвертируем pcm данные
		AudioRawDesc oDescOut = oDescSnd;
		oDescOut.uSampleRate = oDescMaster.uSampleRate;
		oDescOut.calc();
		if(!AudioConverter(pData, &oDescSnd, pBufferOut, &oDescOut))
			return NULL;

		//записываем новые данные и сносим таргет
		pTarget->encode(pBufferOut, &oDescOut);
		pTarget->Release();

		//открываем файл на чтение
		if(!pCodec->open(szPath, "", &pTarget, false))
			return NULL;

		return pTarget;
	}

	//ищем первый попавшийся кодек который может записывать
	IXAudioCodec *pFullCodec = getCodecSave();
	if(!pFullCodec)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "Not found codec for coding, file '%s' does not match of parameters master buffer\n", szPath);
		return NULL;
	}

	//создаем путь до файла в кэше
	String sCachePath = String(SOUND_CACHE) + "/" + szPath;
	sCachePath = PathSetExt(sCachePath.c_str(), pFullCodec->getExt());

	//нужно ли конвертировать файл
	bool canConvert = false;

	//если есть разница по времени (изменения/модификации), тогда надо конвертировать
	if(m_pXCore->getFileSystem()->getFileModifyTime(szPath) > m_pXCore->getFileSystem()->getFileModifyTime(sCachePath.c_str()))
	{
		canConvert = true;
	}
	else
	{
		//если не удается открыть файл кэша, то надо конвертировать
		if(!pFullCodec->open(sCachePath.c_str(), "", &pTarget2, false))
		{
			canConvert = true;
		}
		else
		{
			//если файл кэша не соответствует, тогда надо конвертировать
			pTarget2->getDesc(&oDescCache);
			if(oDescCache.uSampleRate != oDescMaster.uSampleRate)
				canConvert = true;
		}
	}

	//если конвертирование не требуется, возвращаем целевой кодек
	if(!canConvert)
		return pTarget2;

	mem_release(pTarget2);

	//выдергиваем pcm данные из таргета и удаляем его
	IXBuffer *pBufferIn = m_pXCore->newBuffer();
	IXBuffer *pBufferOut = m_pXCore->newBuffer();
	pBufferIn->alloc(oDescSnd.uSize);
	BYTE *pData = pBufferIn->get();
	pTarget->decode(0, pBufferIn->size(), (void**)&pData);
	pTarget->Release();

	//создаем нужное описание и конвертируем pcm данные
	AudioRawDesc oDescOut = oDescSnd;
	oDescOut.uSampleRate = oDescMaster.uSampleRate;
	oDescOut.calc();
	if(!AudioConverter(pData, &oDescSnd, pBufferOut, &oDescOut))
		return NULL;

	//открываем файл на запись
	if(!pFullCodec->open(sCachePath.c_str(), "", &pTarget2, true))
		return NULL;

	pTarget2->encode(pBufferOut, &oDescOut);
	mem_release(pTarget2);

	if(!pFullCodec->open(sCachePath.c_str(), "", &pTarget2, false))
		return NULL;

	return pTarget2;
}

//**************************************************************************

void CSoundSystem::addCodec(const char *szFmt, IXAudioCodec *pCodec)
{
	if(!szFmt || !pCodec)
		return;

	m_mapCodecs[szFmt] = pCodec;
}

//**************************************************************************

IAudioBufferEx* CSoundSystem::createMasterBuffer(AudioRawDesc *pDesc)
{
	return m_pAudio->createMasterBuffer(pDesc);
}

//**************************************************************************

bool CSoundSystem::supportedDesc(const AudioRawDesc *pDesc, AB_TYPE type)
{
	return m_pAudio->supportedDesc(pDesc, type);
}

//**************************************************************************

IXSoundLayer* XMETHODCALLTYPE CSoundSystem::findLayer(const char *szName)
{
	if(!szName || !m_pMasterLayer)
		return NULL;

	if(strcasecmp(m_pMasterLayer->getName(), szName) == 0)
		return m_pMasterLayer;

	return m_pMasterLayer->findLayer(szName);
}

//**************************************************************************

IXAudioCodec* CSoundSystem::getCodecSave()
{
	for(MapCodec::Iterator i = m_mapCodecs.begin(); i; ++i)
	{
		if(m_mapCodecs[i.first]->canSave())
			return m_mapCodecs[i.first];
	}

	return NULL;
}
