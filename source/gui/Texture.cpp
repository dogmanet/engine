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
		
	IXTexture* CTextureManager::getWhite()
	{
		if(!m_pWhite)
		{
			getTexture(TEX_WHITE, &m_pWhite);
		}
		
		return(m_pWhite);
	}
};