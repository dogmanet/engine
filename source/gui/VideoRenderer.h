#ifndef _IVideoRenderer_H_
#define _IVideoRenderer_H_


#include "Texture.h"

#include <Vfw.h>


namespace gui
{

	class CVideoRenderer
	{
		enum STATE
		{
			STATE_STOP,
			STATE_PLAY,
			STATE_PAUSE
		};

	public:
		CVideoRenderer();
		~CVideoRenderer();

		void openFile(const WCHAR * szFile);
		void play();
		void pause();
		void stop();

		int getLength();
		int getPos();
		void setPos(int pos);

		IXTexture *getTexture();

		void update();

		bool isPlaying();
		bool isStopped();
		bool isPaused();
		bool isLooped();

		void release();

		UINT getInstanceId();

		void setLooped(bool l);

		bool isLoaded();

	protected:
		IXTexture *m_texture;
		UINT m_iWidth;
		UINT m_iHeight;

		STATE m_state;

		bool m_bLooped;

		bool m_bLoaded;

		UINT m_iInstanceId;

		PAVIFILE m_pAviFile;
		PAVISTREAM m_pAviStream;
		PGETFRAME m_pAviFrame;

		float m_fClipLength;
		float m_fFrameLength;
		float m_fLastDrawTime;

		int m_iFirstFrame;
		int m_iCurrentFrame;
		int m_iNumFrames;

		UINT * m_pImageData;
	};

	class CVideoUpdateManager
	{
	public:
		static void update();
		static void registerPlayer(CVideoRenderer * player);
		static void unregisterPlayer(CVideoRenderer * player);

	protected:

		static Array<CVideoRenderer*> m_pUpdateList;
	};
};

#endif
