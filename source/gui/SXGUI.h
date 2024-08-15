#ifndef _SXGUI_H_
#define _SXGUI_H_

#include <Windows.h>
#include <graphix/graphix.h>

#include <chrono>
typedef std::chrono::high_resolution_clock::time_point time_point;

#if 0
#ifndef SXGUI_LIB
#	ifdef _DEBUG
#		pragma comment(lib,"SXGUId.lib")
#	else
#		pragma comment(lib,"SXGUI.lib")
#	endif
#endif
#endif

#include "guimain.h"

namespace gui
{
	class CGUI;
	class CTextureManager;
	class CTexture;
	class IFontManager;
	class CDesktopStack: public IXUnknownImplementation<IDesktopStack>
	{
	public:
		SX_ALIGNED_OP_MEM();

		CDesktopStack(CGUI *pGUI, IXRender *pRender, const char *szResPath, UINT uWidth, UINT uHeight);
		~CDesktopStack();

		BOOL putMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void render() override;
		
		IDesktop* createDesktopA(const char *name, const char *file) override;
		IDesktop* createDesktopW(const WCHAR *name, const WCHAR *file) override;

		void setActiveDesktop(IDesktop *d, BOOL clearStack = TRUE) override;
		void setActiveDesktopA(const char *name) override;
		void setActiveDesktopW(const WCHAR *name) override;

		IDesktop* getActiveDesktop() override;

		IDesktop* getDesktopA(const char *name) override;
		IDesktop* getDesktopW(const WCHAR *name) override;

		void registerCallback(const char *cbName, GUI_CALLBACK cb, void *pUserData = NULL) override;

		void registerCallbackDefault(GUI_CALLBACK_WC cb, BOOL greedy = FALSE, void *pUserData = NULL) override;

		GUI_CALLBACK getCallbackByName(const char *cbName) override;
		GUI_CALLBACK getCallbackByName(const StringW &cbName) override;

		void updateScreenSize(UINT uWidth, UINT uHeight, bool doResize = true);
		void showCursor(BOOL bShow);

		WCHAR* getResourceDir();
		IGXDevice* getDevice();

		UINT getScreenWidth();
		UINT getScreenHeight();

		void messageBox(const WCHAR *title, const WCHAR *text, ...) override;

		void pushDesktop(IDesktop *dp) override;
		IDesktop* popDesktop() override;

		void destroyDesktop(IDesktop *dp);

		void execCallback(const StringW &cmd, IEvent *ev);


		void setTransformWorld(const SMMATRIX &mat)
		{
			m_mTransformWorld = mat;
		}
		void setTransformViewProj(const SMMATRIX &mat)
		{
			m_mTransformViewProj = mat;
		}

		CTextureManager* getTextureManager()
		{
			return(m_pTextureManager);
		}

		IFontManager* getFontManager()
		{
			return(m_pFontManager);
		}

		const SMMATRIX& getTransformWorld()
		{
			return(m_mTransformWorld);
		}
		const SMMATRIX& getTransformViewProj()
		{
			return(m_mTransformViewProj);
		}

		void updateTransformShader()
		{
			m_pVSTransformConstant->update(&SMMatrixTranspose(m_mTransformWorld * m_mTransformViewProj));
			m_pDevice->getThreadContext()->setVSConstant(m_pVSTransformConstant, 0);
		}

		IGXRenderBuffer* getQuadRenderBufferXYZ(float3_t *pVertices);
		IGXRenderBuffer* getQuadRenderBufferXYZTex16(float *pVertices);

		IFont* getFont(const WCHAR * szName, UINT size, IFont::STYLE style, int iBlurRadius) override;

	protected:
		SMMATRIX m_mTransformWorld;
		SMMATRIX m_mTransformViewProj;

		IXTexture *m_pDefaultWhite;

		CGUI *m_pGUI;
		IXRender *m_pRender;
		IGXDevice *m_pDevice;
		WCHAR *m_szResourceDir = NULL;

		CTextureManager *m_pTextureManager;
		IFontManager *m_pFontManager;

		bool m_isResizing = false;
		bool m_isScreenSizeChanged = false;
		UINT m_iScreenWidth = 0;
		UINT m_iScreenHeight = 0;

		IGXDepthStencilSurface *m_pOldDepthStencilSurface = NULL;
		IGXSurface *m_pDepthStencilSurface = NULL;

		IDesktop *m_pActiveDesktop = NULL;
		Array<IDesktop*> m_mDesktopStack;

		time_point m_tPrev = std::chrono::high_resolution_clock::now();;

		bool m_bShowCursor = true;
		AssotiativeArray<StringW, IDesktop*> m_mDesktops;

		struct SimpleCallback
		{
			GUI_CALLBACK fn;
			void *pUserData;
		};

		struct DefaultCallback
		{
			GUI_CALLBACK_WC fn;
			void *pUserData;
		};

		AssotiativeArray<StringW, /* Array< */SimpleCallback/* > */> m_mCallbacks;
		Array<DefaultCallback> m_mCallbacksDefaults;
		Array<DefaultCallback> m_mCallbacksDefaultsWC;
		
		IGXVertexBuffer *m_pQuadVerticesXYZ;
		IGXVertexBuffer *m_pQuadVerticesXYZTex16;

		IGXRenderBuffer *m_pQuadRenderXYZ;
		IGXRenderBuffer *m_pQuadRenderXYZTex16;

		IGXConstantBuffer *m_pVSTransformConstant = NULL;



		SimpleCallback* getFullCallbackByName(const char *cbName);
		SimpleCallback* getFullCallbackByName(const StringW &cbName);
	};

	class CGUI: public IXUnknownImplementation<IGUI>
	{
	public:
		SX_ALIGNED_OP_MEM();

		CGUI(IXRender *pRender, IXMaterialSystem *pMaterialSystem, IFileSystem *pFileSystem);
		~CGUI();

		IGXDevice* getDevice();
		IXRender* getRender();

		struct shader_s
		{
			ID m_idVS = -1;
			ID m_idPS = -1;
			ID m_idShaderKit = -1;
		};
		struct shaders_s
		{
			shader_s m_baseTexturedColored;
			shader_s m_baseTexturedTextransformColored;
			shader_s m_baseColored;
		};
		const shaders_s* getShaders()
		{
			return(&m_shaders);
		}

		struct dsstate_s
		{
			IGXDepthStencilState *m_pDefault;
			IGXDepthStencilState *m_pStencilIncr;
			IGXDepthStencilState *m_pStencilDecr;
			IGXDepthStencilState *m_pStencilKeep;
		};
		const dsstate_s *getDepthStencilStates()
		{
			return(&m_depthStencilStates);
		}

		struct bstate_s
		{
			IGXBlendState *m_pDefault;
			IGXBlendState *m_pDesktop;
			IGXBlendState *m_pNoColorWrite;
		};
		const bstate_s* getBlendStates()
		{
			return(&m_blendStates);
		}

		IGXRasterizerState* getDefaultRasterizerState()
		{
			return(m_pDefaultRState);
		}

		IGXSamplerState* getDefaultSamplerState()
		{
			return(m_pDefaultSamplerState);
		}

		struct vdecl_s
		{
			IGXVertexDeclaration *m_pXYZ;
			IGXVertexDeclaration *m_pXYZTex;
		};
		const vdecl_s* getVertexDeclarations()
		{
			return(&m_vertexDeclarations);
		}

		IGXIndexBuffer* getQuadIndexBuffer()
		{
			return(m_pQuadIndexes);
		}

		IDesktopStack* newDesktopStack(const char *szResPath, UINT uWidth, UINT uHeight) override;

		IXMaterialSystem* getMaterialSystem()
		{
			return(m_pMaterialSystem);
		}

	protected:
		IXRender *m_pRender;
		IGXDevice *m_pDevice;

		shaders_s m_shaders;
		dsstate_s m_depthStencilStates;
		bstate_s m_blendStates;
		IGXRasterizerState *m_pDefaultRState;
		vdecl_s m_vertexDeclarations;

		IGXIndexBuffer *m_pQuadIndexes;
		IGXSamplerState *m_pDefaultSamplerState;

		IXMaterialSystem *m_pMaterialSystem;
	};

	CGUI* GetGUI();
};

EXTERN_C __declspec(dllexport) gui::IGUI* InitInstance(IXRender *pRender, IXMaterialSystem *pMaterialSystem, IFileSystem *pFileSystem);

#endif
