
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "Weather.h"

CWeatherRndSnd::CWeatherRndSnd()
{
	m_ulNextPlay = 0;
	m_iVolumeB = 50;
	m_iVolumeE = 90;

	m_ulPeriodB = 1000;
	m_ulPeriodE = 10000;

	m_idCurrPlay = -1;
	m_isPlaying = false;
}

CWeatherRndSnd::~CWeatherRndSnd()
{
	clear();
}

void CWeatherRndSnd::clear()
{
	resetOld();

	for (UINT i = 0; i < m_aArrSnds.size(); ++i)
	{
		if (m_aArrSnds[i].m_id >= 0)
		{
			SSCore_SndStop(m_aArrSnds[i].m_id);
			SSCore_SndDelete(m_aArrSnds[i].m_id);
		}
	}
	m_aArrSnds.clear();
}

void CWeatherRndSnd::resetOld()
{
	for (UINT i = 0; i < m_aCurrSndIDs.size(); ++i)
	{
		if (m_aCurrSndIDs[i] >= 0)
			SSCore_SndStop(m_aCurrSndIDs[i]);
	}
	m_aCurrSndIDs.clearFast();
}

void CWeatherRndSnd::setParamPlayVolume(int iBVol, int iEVol)
{
	m_iVolumeB = iBVol;
	m_iVolumeE = iEVol;
}

void CWeatherRndSnd::setParamPlayPeriod(DWORD ulBPer, DWORD ulEPer)
{
	m_ulPeriodB = ulBPer;
	m_ulPeriodE = ulEPer;
}

void CWeatherRndSnd::addSound(const char *szPath)
{
	bool isunic = true;
	ID tmpid = -1;
	for (UINT i = 0; i<m_aArrSnds.size(); i++)
	{
		if (stricmp(m_aArrSnds[i].m_sPath.c_str(), szPath) == 0)
		{
			isunic = false;
			tmpid = m_aArrSnds[i].m_id;
			break;
		}
	}

	if (isunic)
	{
		ID tmpid = SSCore_SndCreate2d(szPath, SX_SOUND_CHANNEL_GAME);
		m_aArrSnds.push_back(CSnd(szPath, tmpid));
		m_aCurrSndIDs.push_back(tmpid);
	}
	else
	{
		m_aCurrSndIDs.push_back(tmpid);
	}
}

void CWeatherRndSnd::update()
{
	if (!m_isPlaying)
		return;

	static const float * env_weather_snd_volume = GET_PCVAR_FLOAT("env_weather_snd_volume");

	if (m_aCurrSndIDs.size() > 0 && (m_ulNextPlay == 0 || m_ulNextPlay < TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER))))
	{
		int tmpkeysnd = rand() % (m_aCurrSndIDs.size());
		if((int)m_aCurrSndIDs.size() > tmpkeysnd && m_aCurrSndIDs[tmpkeysnd] && m_aCurrSndIDs[tmpkeysnd] >= 0)
		{
			m_idCurrPlay = m_aCurrSndIDs[tmpkeysnd];
			SSCore_SndSetPosPlay(m_idCurrPlay, 0);
			int tmprndvol = (rand() % (m_iVolumeE - m_iVolumeB)) + m_iVolumeB;
			SSCore_SndSetVolume(m_idCurrPlay, ((float)(env_weather_snd_volume ? (float)tmprndvol * (*env_weather_snd_volume) : tmprndvol))*0.01f);
			SSCore_SndPlay(m_idCurrPlay);

			DWORD tmprnd = (rand() % (m_ulPeriodE - m_ulPeriodB)) + m_ulPeriodB;
			m_ulNextPlay = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER)) + tmprnd + ((SSCore_SndGetLengthSec(m_idCurrPlay) + 1) * 1000);
		}
	}
}

void CWeatherRndSnd::play()
{
	if (m_isPlaying)
		return;

	m_isPlaying = true;

	if (m_idCurrPlay >= 0 && SSCore_SndGetState(m_idCurrPlay) == SOUND_OBJSTATE_PAUSE)
		SSCore_SndPlay(m_idCurrPlay);
}

void CWeatherRndSnd::pause()
{
	if (!m_isPlaying)
		return;

	m_isPlaying = false;

	if (m_idCurrPlay >= 0 && SSCore_SndGetState(m_idCurrPlay) == SOUND_OBJSTATE_PLAY)
		SSCore_SndPlay(SOUND_OBJSTATE_PAUSE);
}

bool CWeatherRndSnd::getPlaying()
{
	return m_isPlaying;
}

//#############################################################################

CWeather::CWeather(IXLightSystem *pLightSystem):
	m_pLightSystem(pLightSystem)
{
	m_isPlaying = false;
	m_iSectionOld = -1;
	m_iSectionCurr = -1;
	m_hasUpdate = false;
	m_ulTimeRainVolSndLast = 0;
	m_ulTimeMlsecOld = m_ulTimeMlsecCurr = 0;

	float3 vGeomMin, vGeomMax;
	XEventLevelSize levelSize;
	Core_GetIXCore()->getEventChannel<XEventLevelSize>(EVENT_LEVEL_GET_SIZE_GUID)->broadcastEvent(&levelSize);
	vGeomMin = levelSize.vMin;
	vGeomMax = levelSize.vMax;
	m_fLevelMaxY = vGeomMax.y + 10.f;

	m_idEffRain = SPE_EffectGetByName("rain");

	m_iTrackPosCount = 0;
	m_aTrackPos = 0;
	if (m_idEffRain >= 0)
	{
		m_iTrackPosCount = SPE_EmitterGetCount(m_idEffRain, 0);
		m_aTrackPos = new float3[m_iTrackPosCount];
	}
	else
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s - not found effect 'rain'", GEN_MSG_LOCATION);
	}

	m_idEffThunderbolt = SPE_EffectGetByName("thunderbolt");
	if (m_idEffThunderbolt < 0)
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s - not found effect 'thunderbolt'", GEN_MSG_LOCATION);
	}
	m_ulTimeBoltNext = m_ulTimeBoltLast = 0;

	if(m_pLightSystem)
	{
		m_pLightThunderbolt = m_pLightSystem->newPoint();
		m_pLightThunderbolt->setColor(float4(1.0f, 1.0f, 1.0f, 200.0f));
		m_pLightThunderbolt->setEnabled(false);
	}
	m_idSndRain = SSCore_SndCreate2d("nature/rain.ogg", SX_SOUND_CHANNEL_GAME, true);
	m_idSndThunder = SSCore_SndCreate2d("nature/thunder.ogg", SX_SOUND_CHANNEL_GAME);

	m_fRainVolume = 0;
}

CWeather::~CWeather()
{
	m_aTimeSections.clear();
	mem_release(m_pLightThunderbolt);
	SSCore_SndDelete(m_idSndRain);
	SSCore_SndDelete(m_idSndThunder);
}

void CWeather::load(const char *szPath)
{
	float3 vGeomMin, vGeomMax;
	XEventLevelSize levelSize;
	Core_GetIXCore()->getEventChannel<XEventLevelSize>(EVENT_LEVEL_GET_SIZE_GUID)->broadcastEvent(&levelSize);
	vGeomMin = levelSize.vMin;
	vGeomMax = levelSize.vMax;

	m_fLevelMaxY = vGeomMax.y + 10.f;

	if (szPath == 0 || m_aTimeSections.size() > 0)
	{
		m_aTimeSections.clear();
		m_iSectionOld = m_iSectionCurr = -1;
		m_ulTimeMlsecOld = m_ulTimeMlsecCurr = 0;
		m_hasUpdate = false;
		m_isPlaying = false;
		m_RndSnd.pause();
		m_RndSnd.resetOld();

		SGCore_SkyBoxLoadTex(0);
		SGCore_SkyBoxSetRot(0);
		SGCore_SkyCloudsLoadTex(0);
		SGCore_SkyCloudsSetSpeed(0);
		SGCore_SkyCloudsSetRot(0);

		
		SPE_EffectSetEnable(m_idEffRain, false);
		SSCore_SndStop(m_idSndRain);

		SPE_EffectSetEnable(m_idEffThunderbolt, false);
		if(m_pLightThunderbolt)
		{
			m_pLightThunderbolt->setEnabled(false);
		}
		SSCore_SndStop(m_idSndThunder);

		m_aTimeSections.clear();

		if (szPath == 0)
			return;
	}

	ISXConfig* config = Core_OpConfig(szPath);

	if (!config->sectionExists("sections"))
	{
		LibReport(REPORT_MSG_LEVEL_ERROR, "%s - not found section 'sections' \nfile '%s'", GEN_MSG_LOCATION, szPath);
		mem_release_del(config);
		return;
	}

	int key_count = config->getKeyCount("sections");

	for (int i = 0; i < key_count; ++i)
	{
		int time = 0;
		const char* str = config->getKeyName("sections", i);

		if (strlen(str) != 8)
		{
			LibReport(REPORT_MSG_LEVEL_ERROR, "%s - unresolved name of key '%s' \nfile '%s' \nsection '%s'", GEN_MSG_LOCATION, str, szPath, "sections");
			mem_release_del(config);
			return;
		}

		char tmpall[32];
		sprintf(tmpall, str);

		char* tmpstr = strtok(tmpall, ":");
		WEATHER_CONFIG_SECTION_KEY(tmpstr, str, szPath, "sections");
		time = atoi(tmpstr) * 10000;

		tmpstr = strtok(NULL, ":");
		WEATHER_CONFIG_SECTION_KEY(tmpstr, str, szPath, "sections");
		time += atoi(tmpstr) * 100;

		tmpstr = strtok(NULL, ":");
		WEATHER_CONFIG_SECTION_KEY(tmpstr, str, szPath, "sections");
		time += atoi(tmpstr);

		m_aTimeSections.push_back(CTimeSection(time, config->getKey("sections", str)));

		int qwerty = 0;
	}

	Array<CTimeSection> tmpts = m_aTimeSections;
	m_aTimeSections.clear();
	for (int i = 0, il = tmpts.size(); i < il; ++i)
	{
		int num = 0;
		for (int k = 0, kl = tmpts.size(); k < kl; ++k)
		{
			if (i != k && tmpts[i].m_iTime > tmpts[k].m_iTime)
				++num;
		}

		m_aTimeSections[num] = tmpts[i];
	}

	tmpts.clear();

	for (int i = 0, il = m_aTimeSections.size(); i < il; ++i)
	{
		//LibReport(0, "%d\n", m_aTimeSections[i].time);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "sky_texture"))
			sprintf(m_aTimeSections[i].m_DataSection.m_szSkyTex, "%s", config->getKey(m_aTimeSections[i].m_szSection, "sky_texture"));
		else
			WEATHER_CONFIG_LACKS_KEY("sky_texture", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "clouds_texture"))
			sprintf(m_aTimeSections[i].m_DataSection.m_szCloudsTex, "%s", config->getKey(m_aTimeSections[i].m_szSection, "clouds_texture"));
		else
			WEATHER_CONFIG_LACKS_KEY("clouds_texture", szPath, m_aTimeSections[i].m_szSection);


		if (config->keyExists(m_aTimeSections[i].m_szSection, "sun_texture"))
			sprintf(m_aTimeSections[i].m_DataSection.m_szSunTex, "%s", config->getKey(m_aTimeSections[i].m_szSection, "sun_texture"));
		else
			WEATHER_CONFIG_LACKS_KEY("sun_texture", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "sun_pos"))
		{
			const char* text_sp = config->getKey(m_aTimeSections[i].m_szSection, "sun_pos");
			char text_sp2[64];
			strcpy(text_sp2, text_sp);

			char* tmpstr = strtok(text_sp2, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sun_pos", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSunPos.x = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sun_pos", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSunPos.y = (float)atof(tmpstr);
		}
		else
			WEATHER_CONFIG_LACKS_KEY("sun_pos", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "sky_rotation"))
			m_aTimeSections[i].m_DataSection.m_fSkyRotation = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "sky_rotation")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("sky_rotation", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "sky_color"))
		{
			const char* text_sp = config->getKey(m_aTimeSections[i].m_szSection, "sky_color");
			char text_sp2[64];
			strcpy(text_sp2, text_sp);

			char* tmpstr = strtok(text_sp2, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sky_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSkyColor.x = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sky_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSkyColor.y = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sky_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSkyColor.z = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sky_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSkyColor.w = (float)atof(tmpstr);
		}
		else
			WEATHER_CONFIG_LACKS_KEY("sky_color", szPath, m_aTimeSections[i].m_szSection);


		if (config->keyExists(m_aTimeSections[i].m_szSection, "clouds_color"))
		{
			const char* text_sp = config->getKey(m_aTimeSections[i].m_szSection, "clouds_color");
			char text_sp2[64];
			strcpy(text_sp2, text_sp);

			char* tmpstr = strtok(text_sp2, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "clouds_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vCloudsColor.x = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "clouds_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vCloudsColor.y = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "clouds_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vCloudsColor.z = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "clouds_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vCloudsColor.w = (float)atof(tmpstr);
		}
		else
			WEATHER_CONFIG_LACKS_KEY("clouds_color", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "clouds_rotate"))
			m_aTimeSections[i].m_DataSection.m_fCloudsRotate = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "clouds_rotate")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("clouds_rotate", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "clouds_transparency"))
			m_aTimeSections[i].m_DataSection.m_fCloudsTransparency = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "clouds_transparency")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("clouds_transparency", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "clouds_speed"))
			m_aTimeSections[i].m_DataSection.m_fCloudsSpeed = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "clouds_speed")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("clouds_speed", szPath, m_aTimeSections[i].m_szSection);


		if (config->keyExists(m_aTimeSections[i].m_szSection, "sun_color"))
		{
			const char* text_sp = config->getKey(m_aTimeSections[i].m_szSection, "sun_color");
			char text_sp2[64];
			strcpy(text_sp2, text_sp);

			char* tmpstr = strtok(text_sp2, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sun_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSunColor.x = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sun_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSunColor.y = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sun_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vSunColor.z = (float)atof(tmpstr);
		}
		else
			WEATHER_CONFIG_LACKS_KEY("sun_color", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "far"))
			m_aTimeSections[i].m_DataSection.m_fFar = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "far")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("far", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "rain_density"))
			m_aTimeSections[i].m_DataSection.m_fRainDensity = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "rain_density")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("rain_density", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "rain_color"))
		{
			const char* text_sp = config->getKey(m_aTimeSections[i].m_szSection, "rain_color");
			char text_sp2[64];
			strcpy(text_sp2, text_sp);

			char* tmpstr = strtok(text_sp2, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "rain_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vRainColor.x = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "rain_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vRainColor.y = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "rain_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vRainColor.z = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "rain_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vRainColor.w = (float)atof(tmpstr);
		}
		else
			WEATHER_CONFIG_LACKS_KEY("rain_color", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "fog_density"))
			m_aTimeSections[i].m_DataSection.m_fFogDensity = (float)String(config->getKey(m_aTimeSections[i].m_szSection, "fog_density")).toDouble();
		else
			WEATHER_CONFIG_LACKS_KEY("fog_density", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "fog_color"))
		{
			const char* text_sp = config->getKey(m_aTimeSections[i].m_szSection, "fog_color");
			char text_sp2[64];
			strcpy(text_sp2, text_sp);

			char* tmpstr = strtok(text_sp2, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "fog_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vFogColor.x = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "fog_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vFogColor.y = (float)atof(tmpstr);

			tmpstr = strtok(NULL, ",");
			WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "fog_color", szPath, m_aTimeSections[i].m_szSection);
			m_aTimeSections[i].m_DataSection.m_vFogColor.z = (float)atof(tmpstr);
		}
		else
			WEATHER_CONFIG_LACKS_KEY("fog_color", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "thunderbolt"))
			m_aTimeSections[i].m_DataSection.m_hasThunderbolt = String(config->getKey(m_aTimeSections[i].m_szSection, "thunderbolt")).toBool();
		else
			WEATHER_CONFIG_LACKS_KEY("thunderbolt", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "thunder_period"))
			m_aTimeSections[i].m_DataSection.m_ulThunderPeriod = String(config->getKey(m_aTimeSections[i].m_szSection, "thunder_period")).toUnsLongInt();
		else
			WEATHER_CONFIG_LACKS_KEY("thunder_period", szPath, m_aTimeSections[i].m_szSection);

		if (config->keyExists(m_aTimeSections[i].m_szSection, "env_ambient"))
		{
			const char* text_env = config->getKey(m_aTimeSections[i].m_szSection, "env_ambient");

			if (!config->sectionExists(text_env))
			{
				LibReport(REPORT_MSG_LEVEL_ERROR, "%s - lacks env_ambient section '%s' \nszPath '%s' \nsection '%s'", GEN_MSG_LOCATION, text_env, szPath, m_aTimeSections[i].m_szSection);
				return;
			}

			if (config->keyExists(text_env, "period"))
			{
				const char* text_sp = config->getKey(text_env, "period");
				char text_sp2[64];
				strcpy(text_sp2, text_sp);

				char* tmpstr = strtok(text_sp2, ",");
				WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "period", szPath, text_env);
				m_aTimeSections[i].m_DataSection.Snds.period_b = atol(tmpstr);

				tmpstr = strtok(NULL, ",");
				WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "period", szPath, text_env);
				m_aTimeSections[i].m_DataSection.Snds.period_e = atol(tmpstr);
			}
			else
				WEATHER_CONFIG_LACKS_KEY("period", szPath, text_env);

			if (config->keyExists(text_env, "volume"))
			{
				const char* text_sp = config->getKey(text_env, "volume");
				char text_sp2[64];
				strcpy(text_sp2, text_sp);

				char* tmpstr = strtok(text_sp2, ",");
				WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "volume", szPath, text_env);
				m_aTimeSections[i].m_DataSection.Snds.volume_b = atol(tmpstr);

				tmpstr = strtok(NULL, ",");
				WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "volume", szPath, text_env);
				m_aTimeSections[i].m_DataSection.Snds.volume_e = atol(tmpstr);
			}
			else
				WEATHER_CONFIG_LACKS_KEY("volume", szPath, text_env);

			if (config->keyExists(text_env, "sounds"))
			{
				const char* text_sp = config->getKey(text_env, "sounds");
				char text_sp2[4096];
				strcpy(text_sp2, text_sp);

				char* tmpstr = strtok(text_sp2, ",");
				WEATHER_CONFIG_LACKS_COMPONENT(tmpstr, "sounds", szPath, text_env);
				m_aTimeSections[i].m_DataSection.Snds.arr_path.push_back(tmpstr);

				while (tmpstr != NULL)
				{
					tmpstr = strtok(NULL, " ,");

					if (tmpstr)
						m_aTimeSections[i].m_DataSection.Snds.arr_path.push_back(tmpstr);
				}

			}
			else
				WEATHER_CONFIG_LACKS_KEY("sounds", szPath, text_env);
		}
	}

	mem_release_del(config);
}

void CWeather::update()
{
	if (m_isPlaying)
		m_RndSnd.update();

	IXLightSun *pSun = NULL;
	if(m_pLightSystem)
	{
		pSun = m_pLightSystem->getSun();
	}

	static const float * env_weather_snd_volume = GET_PCVAR_FLOAT("env_weather_snd_volume");

	//если есть дождь то обновл¤ем его позицию и громкость
	if (m_iSectionCurr >= 0 && (int)m_aTimeSections.size() > m_iSectionCurr && m_aTimeSections[m_iSectionCurr].m_DataSection.m_fRainDensity > 0.f)
	{
		static float3 campos;
		Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_POSITION, &campos);
		SPE_EffectSetPos(m_idEffRain, &float3(campos.x, campos.y - WEATHER_RAIN_MIN_Y_OBSERVER, campos.z));
		updateRainSound();
	}

	static float * env_default_rain_density = (float*)GET_PCVAR_FLOAT("env_default_rain_density");
	static float env_default_rain_density_old = 1.f;

	if (env_default_rain_density && (*env_default_rain_density) != env_default_rain_density_old)
	{
		env_default_rain_density_old = *env_default_rain_density;
		if (m_aTimeSections[m_iSectionCurr].m_DataSection.m_fRainDensity > 0.f)
			SPE_EmitterSet(m_idEffRain, 0, m_iReCreateCount, env_default_rain_density_old * m_aTimeSections[m_iSectionCurr].m_DataSection.m_fRainDensity * float(WEATHER_RAIN_RECREATE_COUNT));
	}

	//получаем текущую игровую дату
	tm g_tm;
	time_t g_time = Core_TimeUnixCurrGet(Core_RIntGet(G_RI_INT_TIMER_GAME));
	localtime_s(&g_tm, &g_time);

	//цифровое представление времени суток
	int ltime = g_tm.tm_hour * 10000 + g_tm.tm_min * 100 + g_tm.tm_sec;
	
	//представление времени суток в млсекундах
	DWORD ltime_mlsec = (g_tm.tm_sec * 1000) + (g_tm.tm_min * 60 * 1000) + (g_tm.tm_hour * 60 * 60 * 1000);
	int curr_section = -1;

	//находим секцию дл¤ текущего времени суток
	for (int i = 0, il = m_aTimeSections.size(); i < il; ++i)
	{
		if (ltime >= m_aTimeSections[i].m_iTime)
			curr_section = i;
	}

	//если только что обновл¤емс¤ либо сменилась секци¤
	if (curr_section >= 0 && curr_section != m_iSectionCurr)
	{
		m_iSectionOld = m_iSectionCurr;
		m_ulTimeMlsecOld = m_ulTimeMlsecCurr;
		if (m_iSectionOld < 0)
		{
			m_iSectionOld = curr_section;
			m_ulTimeMlsecOld = ltime_mlsec;
		}
		
		m_iSectionCurr = curr_section;
		m_ulTimeMlsecCurr = ltime_mlsec;

		SGCore_SkyBoxChangeTex(m_aTimeSections[m_iSectionCurr].m_DataSection.m_szSkyTex);
		SGCore_SkyBoxSetRot(m_aTimeSections[m_iSectionCurr].m_DataSection.m_fSkyRotation);
		SGCore_SkyCloudsChangeTex(m_aTimeSections[m_iSectionCurr].m_DataSection.m_szCloudsTex);
		SGCore_SkyCloudsSetSpeed(m_aTimeSections[m_iSectionCurr].m_DataSection.m_fCloudsSpeed);
		SGCore_SkyCloudsSetRot(m_aTimeSections[m_iSectionCurr].m_DataSection.m_fCloudsRotate);
		
		char *pStrSunTex = m_aTimeSections[m_iSectionCurr].m_DataSection.m_szSunTex;
		if (pStrSunTex[0] != '0' && pStrSunTex[0] != '1')
			SPP_ChangeTexSun(m_aTimeSections[m_iSectionCurr].m_DataSection.m_szSunTex);
		
		if(pSun)
		{
			//установка/сброс состояния включения
			pSun->setEnabled(pStrSunTex[0] != '0');
#if 0
			//установка/сброс состояния "все в тени от глобального источника"
			SLight_SetCastGlobalShadow(pStrSunTex[0] == '1');
#endif
		}

		m_hasUpdate = true;

		//если плотность дожд¤ больше нул¤ тогда включаем дождь
		if (m_aTimeSections[m_iSectionCurr].m_DataSection.m_fRainDensity > 0.f)
		{
			SPE_EmitterSet(m_idEffRain, 0, m_vColor, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vRainColor);
			SPE_EmitterSet(m_idEffRain, 0, m_iReCreateCount, env_default_rain_density_old * m_aTimeSections[m_iSectionCurr].m_DataSection.m_fRainDensity * float(WEATHER_RAIN_RECREATE_COUNT));
			SPE_EffectSetEnable(m_idEffRain, true);

			SSCore_SndSetPosPlay(m_idSndRain, 0);
			SSCore_SndSetVolume(m_idSndRain, 0);

			if (m_isPlaying)
				SSCore_SndPlay(m_idSndRain);
			else
				SSCore_SndPause(m_idSndRain);
		}
		//иначе выключаем
		else
		{
			SPE_EffectSetEnable(m_idEffRain, false);
			SSCore_SndStop(m_idSndRain);
		}

		//очищаем старые звуки
		m_RndSnd.resetOld();

		//если есть пути до новых случайных звуков
		if (m_aTimeSections[m_iSectionCurr].m_DataSection.Snds.arr_path.size() > 0)
		{
			//загружаем эти звуки
			CDataSection::CSndRnd Snds = m_aTimeSections[m_iSectionCurr].m_DataSection.Snds;

			for (int i = 0, il = Snds.arr_path.size(); i < il; ++i)
			{
				m_RndSnd.addSound(Snds.arr_path[i].c_str());
				m_RndSnd.setParamPlayPeriod(Snds.period_b, Snds.period_e);
				m_RndSnd.setParamPlayVolume(Snds.volume_b, Snds.volume_e);
			}
		}
	}

	
	
	if (m_hasUpdate)
	{
		//расчет коэфициента интерпол¤ции
		float lerp_factor = 1.f;
			
		if (m_ulTimeMlsecCurr - m_ulTimeMlsecOld > 0)
			lerp_factor = float(ltime_mlsec - m_ulTimeMlsecCurr) / float(m_ulTimeMlsecCurr - m_ulTimeMlsecOld);

		//если коэфициент интерпол¤ции больше либо равен 1 то значит интерпол¤ци¤ больше не нужна на текущей секции
		if (lerp_factor >= 1.f)
			m_hasUpdate = false;

		//цвет скайбокса
		float4_t tmp_sky_color;
		tmp_sky_color.x = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSkyColor.x, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSkyColor.x, lerp_factor);
		tmp_sky_color.y = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSkyColor.y, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSkyColor.y, lerp_factor);
		tmp_sky_color.z = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSkyColor.z, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSkyColor.z, lerp_factor);
		tmp_sky_color.w = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSkyColor.w, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSkyColor.w, lerp_factor);
		SGCore_SkyBoxSetColor(&tmp_sky_color);

		//цвет облаков
		float4_t tmp_clouds_color;
		tmp_clouds_color.x = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vCloudsColor.x, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vCloudsColor.x, lerp_factor);
		tmp_clouds_color.y = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vCloudsColor.y, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vCloudsColor.y, lerp_factor);
		tmp_clouds_color.z = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vCloudsColor.z, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vCloudsColor.z, lerp_factor);
		tmp_clouds_color.w = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vCloudsColor.w, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vCloudsColor.w, lerp_factor);
		SGCore_SkyCloudsSetColor(&tmp_sky_color);

		//прозрачность облаков
		float tmp_clouds_transparency = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_fCloudsTransparency, m_aTimeSections[m_iSectionCurr].m_DataSection.m_fCloudsTransparency, lerp_factor);
		SGCore_SkyCloudsSetAlpha(tmp_clouds_transparency);

		if(pSun)
		{
			//цвет солнца

			float3 tmp_scolor;
			tmp_scolor.x = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSunColor.x, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSunColor.x, lerp_factor);
			tmp_scolor.y = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSunColor.y, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSunColor.y, lerp_factor);
			tmp_scolor.z = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSunColor.z, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSunColor.z, lerp_factor);
			//SLight_SetColor(gid, &tmp_scolor);
			pSun->setColor(float4(tmp_scolor, 100.0f));

			//позици¤ солнца
			float3 tmp_spos;
			tmp_spos.x = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSunPos.x, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSunPos.x, lerp_factor);
			tmp_spos.y = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vSunPos.y, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vSunPos.y, lerp_factor);
			tmp_spos.z = 0;
#if 0
			SLight_SetPos(gid, &tmp_spos, false);
#endif
		}

		//дальность видимости
		float tmpfar = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_fFar, m_aTimeSections[m_iSectionCurr].m_DataSection.m_fFar, lerp_factor);
		Core_0SetCVarFloat("r_far", tmpfar);

		//плотность тумана
		static float * pp_fog_density = (float*)GET_PCVAR_FLOAT("pp_fog_density");
		if (pp_fog_density)
			*pp_fog_density = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_fFogDensity, m_aTimeSections[m_iSectionCurr].m_DataSection.m_fFogDensity, lerp_factor);

		//цвет тумана
		static const UINT_PTR *pp_fog_color = GET_PCVAR_POINTER("pp_fog_color");
		if (pp_fog_color && *pp_fog_color)
		{
			float3_t* tmp_fog_color2 = (float3_t*)(*pp_fog_color);
			tmp_fog_color2->x = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vFogColor.x, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vFogColor.x, lerp_factor);
			tmp_fog_color2->y = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vFogColor.y, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vFogColor.y, lerp_factor);
			tmp_fog_color2->z = lerpf(m_aTimeSections[m_iSectionOld].m_DataSection.m_vFogColor.z, m_aTimeSections[m_iSectionCurr].m_DataSection.m_vFogColor.z, lerp_factor);
		}
		else
			LibReport(REPORT_MSG_LEVEL_WARNING, "cvar pp_fog_color is not init");
	}

	//если в текущей секции есть частота молнии
	if (m_iSectionCurr >= 0 && m_aTimeSections[m_iSectionCurr].m_DataSection.m_ulThunderPeriod > 0)
	{
		//если следующее врем¤ грозы и предыдущее нулевые, тогда генерируем следующее врем¤
		if (m_ulTimeBoltNext == 0 && m_ulTimeBoltLast == 0)
			m_ulTimeBoltNext = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER)) + (rand() % m_aTimeSections[m_iSectionCurr].m_DataSection.m_ulThunderPeriod);
		//иначе если предыдущее врем¤ грозы нулевое и врем¤ следующей грозы наступило
		else if (m_ulTimeBoltLast == 0 && TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER)) >= m_ulTimeBoltNext)
		{
			static const bool * env_default_thunderbolt = GET_PCVAR_BOOL("env_default_thunderbolt");
			//если предусмотерна молни¤ то показываем
			if (m_aTimeSections[m_iSectionCurr].m_DataSection.m_hasThunderbolt && (!env_default_thunderbolt || (env_default_thunderbolt && *env_default_thunderbolt)))
			{
				static float3 campos;
				Core_RFloat3Set(G_RI_FLOAT3_OBSERVER_POSITION, &campos);

				m_vBoltMin = float3_t(campos.x - WEATHER_THUNDERBOLT_WIDTH * 0.5f, m_fLevelMaxY, campos.z - WEATHER_THUNDERBOLT_LENGTH * 0.5f);
				m_vBoltMax = float3_t(campos.x + WEATHER_THUNDERBOLT_WIDTH * 0.5f, m_fLevelMaxY + WEATHER_THUNDERBOLT_HEIGHT, campos.z + WEATHER_THUNDERBOLT_LENGTH * 0.5f);

				float3 tpos = float3(randf(m_vBoltMin.x, m_vBoltMax.x), randf(m_vBoltMin.y, m_vBoltMax.y), randf(m_vBoltMin.z, m_vBoltMax.z));
				SPE_EffectSetPos(m_idEffThunderbolt, &tpos);
				SPE_EffectSetEnable(m_idEffThunderbolt, true);
				if(m_pLightThunderbolt)
				{
					m_pLightThunderbolt->setPosition(tpos);
					m_pLightThunderbolt->setEnabled(true);
				}
				m_ulTimeBoltLight = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER));
			}
			else
			{
				SSCore_SndSetPosPlay(m_idSndThunder, 0);
				SSCore_SndSetVolume(m_idSndThunder, (env_weather_snd_volume ? (*env_weather_snd_volume) : 1.f));

				if (m_isPlaying)
					SSCore_SndPlay(m_idSndThunder);
				else
					SSCore_SndPause(m_idSndThunder);
			}

			m_ulTimeBoltNext = 0;	//обнул¤ем следующее врем¤

			//и устанавливаем прошлому времени грозы текущее значение времени
			m_ulTimeBoltLast = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER));
			
		}
		//иначе если предыдцщее врем¤ грозы не нулевое и оно уже было достаточно давно чтобы дать возможность генерации следующей грозы
		else if (m_ulTimeBoltLast > 0 && TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - m_ulTimeBoltLast >= m_aTimeSections[m_iSectionCurr].m_DataSection.m_ulThunderPeriod)
			m_ulTimeBoltLast = 0;

		//если врем¤ света от молнии не нулевое и прошло достаточно времени чтобы выключить свет
		if (m_ulTimeBoltLight > 0 && TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - m_ulTimeBoltLight > WEATHER_THUNDERBOLT_LIGHT_TIME)
		{
			m_ulTimeBoltLight = 0;
			if(m_pLightThunderbolt)
			{
				m_pLightThunderbolt->setEnabled(false);
			}

			//и заодно проиграть звук молнии
			SSCore_SndSetPosPlay(m_idSndThunder, 0);
			SSCore_SndSetVolume(m_idSndThunder, clampf(m_fRainVolume*2.f, 0.f, 1.f));
			
			if (m_isPlaying)
				SSCore_SndPlay(m_idSndThunder);
			else
				SSCore_SndPause(m_idSndThunder);
		}
	}
	else
	{
		m_ulTimeBoltLast = 0;
		m_ulTimeBoltNext = 0;
		m_ulTimeBoltLight = 0;
	}
	mem_release(pSun);
}

void CWeather::updateRainSound()
{
	if (m_idEffRain < 0 || !m_isPlaying)
		return;

	static const float * env_weather_snd_volume = GET_PCVAR_FLOAT("env_weather_snd_volume");

	//если пришло врем¤ обновл¤ть громкость
	if (m_ulTimeRainVolSndLast == 0 || TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER)) - m_ulTimeRainVolSndLast >= WEATHER_RAIN_VOL_SND_UPDATE)
		m_ulTimeRainVolSndLast = TimeGetMls(Core_RIntGet(G_RI_INT_TIMER_RENDER));
	else
		return;

	//если внезапно количество оставл¤ющих след стало больше чем выделено
	if (SPE_EmitterGetTrackCount(m_idEffRain, 0) > m_iTrackPosCount)
	{
		mem_delete(m_aTrackPos);
		m_iTrackPosCount = SPE_EmitterGetCount(m_idEffRain, 0);
		m_aTrackPos = new float3[m_iTrackPosCount];
	}

	//получаем массив следов
	int tmpcount = SPE_EmitterGetTrackPos(m_idEffRain, 0, &m_aTrackPos, m_iTrackPosCount);
	m_fRainVolume = 0;
	float biger = 0.f;

	static float3 campos;
	Core_RFloat3Get(G_RI_FLOAT3_OBSERVER_POSITION, &campos);

	for (int i = 0; i < tmpcount; ++i)
	{
		float dist2 = 1.f - SMVector3Distance(campos, m_aTrackPos[i]) / WEATHER_RAIN_VOL_SND_DIST;
		m_fRainVolume += clampf(dist2, 0.f, 1.f);
	}

	m_fRainVolume /= tmpcount / 4;
	SSCore_SndSetVolume(m_idSndRain, (env_weather_snd_volume ? (*env_weather_snd_volume) : 1.f) *  m_fRainVolume);
}

float CWeather::getCurrRainDensity()
{
	if (m_iSectionCurr >= 0 && m_aTimeSections.size() > m_iSectionCurr)
		return m_aTimeSections[m_iSectionCurr].m_DataSection.m_fRainDensity;
	
	return 0.f;
}

void CWeather::sndPlay()
{
	if (m_isPlaying)
		return;

	m_isPlaying = true;
	m_RndSnd.play();

	if (m_idSndRain >= 0 && SSCore_SndGetState(m_idSndRain) == SOUND_OBJSTATE_PAUSE)
		SSCore_SndPlay(m_idSndRain);

	if (m_idSndThunder >= 0 && SSCore_SndGetState(m_idSndThunder) == SOUND_OBJSTATE_PAUSE)
		SSCore_SndPlay(m_idSndThunder);
}

void CWeather::sndPause()
{
	if (!m_isPlaying)
		return;

	m_isPlaying = false;
	m_RndSnd.pause();

	if (m_idSndRain >= 0 && SSCore_SndGetState(m_idSndRain) == SOUND_OBJSTATE_PLAY)
		SSCore_SndPause(m_idSndRain);

	if (m_idSndThunder >= 0 && SSCore_SndGetState(m_idSndThunder) == SOUND_OBJSTATE_PLAY)
		SSCore_SndPause(m_idSndThunder);
}

bool CWeather::sndGetPlaying()
{
	return m_isPlaying;
}
