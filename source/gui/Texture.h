#ifndef _ITexture_H_
#define _ITexture_H_

#include "GUIbase.h"
#include "String.h"

namespace gui
{
	class CVideoRenderer;
	
	class CTextureManager
	{
	public:
		CTextureManager(const WCHAR *szResourceDir):
			m_wsResourceDir(szResourceDir)
		{
		}
		~CTextureManager()
		{
			mem_release(m_pWhite);
		}
		void getTexture(const StringW &szTexture, IXTexture **ppOut);
		void bindTexture(IXTexture *tex);
		void releaseTexture(IXTexture *pTexture);
		
		const StringW& getResourceDir()
		{
			return(m_wsResourceDir);
		}

		IXTexture* getWhite();

		void onNewFrame();

	protected:
		StringW m_wsResourceDir;
		IXTexture *m_pWhite = NULL;
		Array<IXTexture*> m_aReleasedTextures;
	};
};

#endif
