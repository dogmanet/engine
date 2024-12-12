#include "MaterialLoader.h"

#define STR_VALIDATE(str) ((str) && (str)[0]!=0 && (str)[0]!='0')

inline bool StringToBool(const char *szValue)
{
	int iValue;
	return(!strcasecmp(szValue, "true") || (sscanf(szValue, "%d", &iValue) == 1 && iValue));
}

CMaterialLoader::CMaterialLoader(IXCore *pCore):
	m_pCore(pCore)
{
}

UINT XMETHODCALLTYPE CMaterialLoader::getExtCount() const
{
	return(1);
}
bool XMETHODCALLTYPE CMaterialLoader::canSave(UINT uIndex) const
{
	return(true);
}
const char* XMETHODCALLTYPE CMaterialLoader::getExt(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("mtl");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CMaterialLoader::getExtText(UINT uIndex) const
{
	assert(uIndex < getExtCount());
	switch(uIndex)
	{
	case 0:
		return("SkyXEngine material");
	}
	return(NULL);
}
const char* XMETHODCALLTYPE CMaterialLoader::getAuthor() const
{
	return("D-AIRY @ DogmaNet");
}
const char* XMETHODCALLTYPE CMaterialLoader::getCopyright() const
{
	return("Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2019");
}
const char* XMETHODCALLTYPE CMaterialLoader::getDescription() const
{
	return("SkyXEngine material loader");
}

bool XMETHODCALLTYPE CMaterialLoader::open(const char *szFileName, const char *szArg, bool forSave)
{
	assert(!m_pConfig && "File already opened!");

	if(forSave)
	{
		IFile *pFile = m_pCore->getFileSystem()->openFile(szFileName, FILE_MODE_WRITE);
		mem_release(pFile);
	}

	char szTmpPath[1024];
	if(m_pCore->getFileSystem()->resolvePath(szFileName, szTmpPath, sizeof(szTmpPath)))
	{
		m_pConfig = m_pCore->newConfig();
		if(!m_pConfig->open(szTmpPath))
		{
			mem_release(m_pConfig);
			return(false);
		}

		if(forSave)
		{
			m_pConfig->clear();
		}
	}
	
	return(true);
}

bool XMETHODCALLTYPE CMaterialLoader::load(IXMaterial *pMaterial)
{
	if(!m_pConfig)
	{
		return(false);
	}

	const char *szSection = m_pConfig->getSectionName(0);
	if(!szSection)
	{
		return(false);
	}

	if(m_pConfig->keyExists(szSection, "shader"))
	{
		pMaterial->setShader(m_pConfig->getKey(szSection, "shader"));
	}
	else if(m_pConfig->keyExists(szSection, "ps"))
	{
		const char *szShader = m_pConfig->getKey(szSection, "ps");
		if(!strcmp(szShader, "mtrltp_base.ps") || !strcmp(szShader, "mtrltp_refract.ps"))
		{
			pMaterial->setShader("Transparent");
		}
		// else ...
	}
	else
	{
		return(false);
	}

	if(m_pConfig->keyExists(szSection, "physmaterial"))
	{
		int iValue;
		if(sscanf(m_pConfig->getKey(szSection, "physmaterial"), "%d", &iValue))
		{
			pMaterial->setPhysicsType((MTLTYPE_PHYSIC)iValue);
		}
	}

	if(m_pConfig->keyExists(szSection, "transparent"))
	{
		pMaterial->setTransparent(StringToBool(m_pConfig->getKey(szSection, "transparent")));
	}

	if(m_pConfig->keyExists(szSection, "refractive"))
	{
		pMaterial->setRefractive(StringToBool(m_pConfig->getKey(szSection, "refractive")));
	}

	if(m_pConfig->keyExists(szSection, "alphatest"))
	{
		pMaterial->setFlag("has_alphatest", StringToBool(m_pConfig->getKey(szSection, "alphatest")));
	}

	if(m_pConfig->keyExists(szSection, "roughness"))
	{
		float fParam;
		if(sscanf(m_pConfig->getKey(szSection, "roughness"), "%f", &fParam) == 1)
		{
			pMaterial->setParam("pbr_roughness", float4_t(fParam, 0.0f, 0.0f, 0.0f));
		}
	}
	if(m_pConfig->keyExists(szSection, "thickness"))
	{
		float fParam;
		if(sscanf(m_pConfig->getKey(szSection, "thickness"), "%f", &fParam) == 1)
		{
			pMaterial->setParam("pbr_thickness", float4_t(fParam, 0.0f, 0.0f, 0.0f));
		}
	}
	if(m_pConfig->keyExists(szSection, "f0"))
	{
		float fParam;
		if(sscanf(m_pConfig->getKey(szSection, "f0"), "%f", &fParam) == 1)
		{
			pMaterial->setParam("pbr_f0", float4_t(fParam, 0.0f, 0.0f, 0.0f));
		}
	}

	const char *szTmp;

	if(m_pConfig->keyExists(szSection, "texture"))
	{
		szTmp = m_pConfig->getKey(szSection, "texture");
		if(STR_VALIDATE(szTmp))
		{
			pMaterial->setTexture("txBase", szTmp);
		}
	}

	if(m_pConfig->keyExists(szSection, "microrelief_r"))
	{
		szTmp = m_pConfig->getKey(szSection, "microrelief_r");
		if(STR_VALIDATE(szTmp))
		{
			pMaterial->setTexture("txNormals", szTmp);
		}
		
		if(m_pConfig->keyExists(szSection, "ps_userdata_z"))
		{
			float fParam;
			if(sscanf(m_pConfig->getKey(szSection, "ps_userdata_z"), "%f", &fParam) == 1)
			{
				pMaterial->setParam("nm_weight", float4_t(fParam, 0.0f, 0.0f, 0.0f));
			}
		}
	}
	else if(m_pConfig->keyExists(szSection, "detail_r"))
	{
		szTmp = m_pConfig->getKey(szSection, "detail_r");
		if(STR_VALIDATE(szTmp))
		{
			pMaterial->setTexture("txNormals", szTmp);
		}
	}

	//если есть ключ использования текстуры то грузим
	if(m_pConfig->keyExists(szSection, "use_texparamlight") && StringToBool(m_pConfig->getKey(szSection, "use_texparamlight")))
	{
		pMaterial->setFlag("has_parameter_texture", true);
	}

	if(m_pConfig->keyExists(szSection, "texparamligth"))
	{
		szTmp = m_pConfig->getKey(szSection, "texparamligth");
		if(STR_VALIDATE(szTmp))
		{
			pMaterial->setTexture("txParameters", szTmp);
		}
	}

	for(UINT i = 0, l = m_pConfig->getKeyCount(szSection); i < l; ++i)
	{
		const char *szKey = m_pConfig->getKeyName(szSection, i);

		if(!memcmp(szKey, "tex.", 4))
		{
			pMaterial->setTexture(szKey + 4, m_pConfig->getKey(szSection, szKey));
		}
		else if(!memcmp(szKey, "prop.", 5))
		{
			float4_t vParam(0.0f, 0.0f, 0.0f, 0.0f);
			if(sscanf(m_pConfig->getKey(szSection, szKey), "%f %f %f %f", &vParam.x, &vParam.y, &vParam.z, &vParam.w) >= 1)
			{
				pMaterial->setParam(szKey + 5, vParam);
			}
		}
		else if(!memcmp(szKey, "flag.", 5))
		{
			pMaterial->setFlag(szKey + 5, StringToBool(m_pConfig->getKey(szSection, szKey)));
		}
	}
	
	return(true);
}
bool XMETHODCALLTYPE CMaterialLoader::save(IXMaterial *pMaterial)
{
	if(!m_pConfig)
	{
		return(false);
	}

	const char *szSection = m_pConfig->getSectionName(0);
	if(!szSection)
	{
		szSection = "material";
	}

	m_pConfig->set(szSection, "shader", pMaterial->getShader());

	int iType = (int)pMaterial->getPhysicsType();
	char tmp[1024], tmp2[1024];
	sprintf(tmp, "%d", iType);
	m_pConfig->set(szSection, "physmaterial", tmp);

	const char *szKey;

	IKeyIterator *pIter = pMaterial->getFlagsIterator();
	while(!pIter->isEnd())
	{
		szKey = pIter->getCurrent();

		sprintf(tmp, "flag.%s", szKey);
		sprintf(tmp2, "%d", pMaterial->getFlag(szKey) ? 1 : 0);
		m_pConfig->set(szSection, tmp, tmp2);

		pIter->getNext();
	}
	mem_release(pIter);

	float4_t vTemp;

	pIter = pMaterial->getParamsIterator();
	while(!pIter->isEnd())
	{
		szKey = pIter->getCurrent();

		vTemp = pMaterial->getParam(szKey);

		sprintf(tmp, "prop.%s", szKey);
		sprintf(tmp2, "%f %f %f %f", vTemp.x, vTemp.y, vTemp.z, vTemp.w);
		m_pConfig->set(szSection, tmp, tmp2);

		pIter->getNext();
	}
	mem_release(pIter);

	pIter = pMaterial->getTexturesIterator();
	while(!pIter->isEnd())
	{
		szKey = pIter->getCurrent();

		sprintf(tmp, "tex.%s", szKey);
		m_pConfig->set(szSection, tmp, pMaterial->getTextureName(szKey));

		pIter->getNext();
	}
	mem_release(pIter);

	return(m_pConfig->save());
}
void XMETHODCALLTYPE CMaterialLoader::close()
{
	mem_release(m_pConfig);
}
