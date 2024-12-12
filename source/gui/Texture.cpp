#include "Texture.h"

namespace gui
{
	void CTextureManager::getTexture(const StringW &szTexture, IXTexture **ppOut)
	{
		String path(szTexture[0] == '!' ? szTexture.substr(1) : getResourceDir() + L"textures/" + szTexture);

		GetGUI()->getMaterialSystem()->loadTexture(path.c_str(), ppOut);
	}
	
	void CTextureManager::bindTexture(IXTexture *tex)
	{
		GetGUI()->getMaterialSystem()->bindTexture(tex);
	}

	void CTextureManager::releaseTexture(IXTexture *pTexture)
	{
		if(pTexture)
		{
			m_aReleasedTextures.push_back(pTexture);
		}
	}
		
	IXTexture* CTextureManager::getWhite()
	{
		if(!m_pWhite)
		{
			getTexture(TEX_WHITE, &m_pWhite);
		}
		
		return(m_pWhite);
	}

	void CTextureManager::onNewFrame()
	{
		fora(i, m_aReleasedTextures)
		{
			mem_release(m_aReleasedTextures[i]);
		}
		m_aReleasedTextures.clearFast();
	}
};
