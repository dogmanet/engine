#include "GUIbase.h"
#include "IHTMLparser.h"
#include "IDOMdocument.h"

#include "IHTML.h"

#include "IRenderFrame.h"

#include "Font.h"

#include "IRenderBorder.h"

#include "VideoRenderer.h"

#include "Desktop.h"
#include "IVIDEO.h"
#include "ITEXT.h"

#include <core/sxcore.h>

namespace gui
{
	CGUI *g_pGUI;

	CGUI* GetGUI()
	{
		return(g_pGUI);
	}

	CGUI::CGUI(IXRender *pRender, IXMaterialSystem *pMaterialSystem, IFileSystem *pFileSystem):
		m_pRender(pRender),
		m_pDevice(pRender->getDevice()),
		m_pMaterialSystem(pMaterialSystem)
	{
		g_pGUI = this;

		gui::CKeyMap::init();

		m_shaders.m_baseTexturedColored.m_idVS = m_pRender->loadShader(SHADER_TYPE_VERTEX, "gui_main.vs");
		m_shaders.m_baseTexturedColored.m_idPS = m_pRender->loadShader(SHADER_TYPE_PIXEL, "gui_main.ps");
		m_shaders.m_baseTexturedColored.m_idShaderKit = m_pRender->createShaderKit(m_shaders.m_baseTexturedColored.m_idVS, m_shaders.m_baseTexturedColored.m_idPS);

		m_shaders.m_baseColored.m_idVS = m_pRender->loadShader(SHADER_TYPE_VERTEX, "gui_simple.vs");
		m_shaders.m_baseColored.m_idPS = m_pRender->loadShader(SHADER_TYPE_PIXEL, "gui_simple.ps");
		m_shaders.m_baseColored.m_idShaderKit = m_pRender->createShaderKit(m_shaders.m_baseColored.m_idVS, m_shaders.m_baseColored.m_idPS);

		m_shaders.m_baseTexturedTextransformColored.m_idVS = m_pRender->loadShader(SHADER_TYPE_VERTEX, "gui_main_textransform.vs");
		m_shaders.m_baseTexturedTextransformColored.m_idPS = m_shaders.m_baseTexturedColored.m_idPS;
		m_shaders.m_baseTexturedTextransformColored.m_idShaderKit = m_pRender->createShaderKit(m_shaders.m_baseTexturedTextransformColored.m_idVS, m_shaders.m_baseTexturedTextransformColored.m_idPS);

		GXDepthStencilDesc depthStencilDesc;
		depthStencilDesc.useDepthTest = false;
		depthStencilDesc.useDepthWrite = false;
		m_depthStencilStates.m_pDefault = m_pDevice->createDepthStencilState(&depthStencilDesc);

		depthStencilDesc.useStencilTest = true;
		depthStencilDesc.stencilTestFront.cmpFunc = GXCMP_EQUAL;
		depthStencilDesc.stencilTestFront.opPass = GXSTENCIL_OP_INCR;
		m_depthStencilStates.m_pStencilIncr = m_pDevice->createDepthStencilState(&depthStencilDesc);

		depthStencilDesc.stencilTestFront.opPass = GXSTENCIL_OP_KEEP;
		m_depthStencilStates.m_pStencilKeep = m_pDevice->createDepthStencilState(&depthStencilDesc);

		depthStencilDesc.stencilTestFront.opPass = GXSTENCIL_OP_DECR;
		m_depthStencilStates.m_pStencilDecr = m_pDevice->createDepthStencilState(&depthStencilDesc);

		GXBlendDesc blendDesc;
		blendDesc.renderTarget[0].useBlend = true;
		blendDesc.renderTarget[0].blendSrcAlpha = blendDesc.renderTarget[0].blendSrcColor = GXBLEND_SRC_ALPHA;
		blendDesc.renderTarget[0].blendDestAlpha = blendDesc.renderTarget[0].blendDestColor = GXBLEND_INV_SRC_ALPHA;

		blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_ONE;
		blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_SRC_ALPHA;

		m_blendStates.m_pDefault = m_pDevice->createBlendState(&blendDesc);

		blendDesc.renderTarget[0].u8RenderTargetWriteMask = 0;
		m_blendStates.m_pNoColorWrite = m_pDevice->createBlendState(&blendDesc);

		blendDesc.renderTarget[0].u8RenderTargetWriteMask = GXCOLOR_WRITE_ENABLE_ALL;
	//	blendDesc.renderTarget[0].blendSrcAlpha = GXBLEND_ONE;
	//	blendDesc.renderTarget[0].blendDestAlpha = GXBLEND_INV_SRC_ALPHA;
	//	blendDesc.renderTarget[0].blendOpAlpha = GXBLEND_OP_MAX;
		m_blendStates.m_pDesktop = m_pDevice->createBlendState(&blendDesc);

		GXRasterizerDesc rasterizerDesc;
		rasterizerDesc.useMultisample = true;
		rasterizerDesc.useAntialiasedLine = true;
		rasterizerDesc.cullMode = GXCULL_NONE;
		m_pDefaultRState = m_pDevice->createRasterizerState(&rasterizerDesc);

		GXVertexElement aVertexElementsXYZTex[] =
		{
			{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
			{0, 12, GXDECLTYPE_FLOAT2, GXDECLUSAGE_TEXCOORD, GXDECLSPEC_PER_VERTEX_DATA},
			GX_DECL_END()
		};
		m_vertexDeclarations.m_pXYZTex = m_pDevice->createVertexDeclaration(aVertexElementsXYZTex);

		GXVertexElement aVertexElementsXYZ[] =
		{
			{0, 0, GXDECLTYPE_FLOAT3, GXDECLUSAGE_POSITION, GXDECLSPEC_PER_VERTEX_DATA},
			GX_DECL_END()
		};
		m_vertexDeclarations.m_pXYZ = m_pDevice->createVertexDeclaration(aVertexElementsXYZ);

		USHORT pIdxQuad[] = {
			0, 1, 2, 0, 2, 3
		};
		m_pQuadIndexes = m_pDevice->createIndexBuffer(sizeof(USHORT) * 6, GXBUFFER_USAGE_STATIC, GXIT_UINT16, pIdxQuad);

		GXSamplerDesc samplerDesc;
		samplerDesc.filter = GXFILTER_ANISOTROPIC;
		m_pDefaultSamplerState = m_pDevice->createSamplerState(&samplerDesc);
	}

	CGUI::~CGUI()
	{
		g_pGUI = NULL;
		mem_release(m_pDefaultSamplerState);
		mem_release(m_pQuadIndexes);
		mem_release(m_vertexDeclarations.m_pXYZ);
		mem_release(m_vertexDeclarations.m_pXYZTex);
		mem_release(m_pDefaultRState);
		mem_release(m_blendStates.m_pDesktop);
		mem_release(m_blendStates.m_pNoColorWrite);
		mem_release(m_blendStates.m_pDefault);
		mem_release(m_depthStencilStates.m_pStencilDecr);
		mem_release(m_depthStencilStates.m_pStencilKeep);
		mem_release(m_depthStencilStates.m_pStencilIncr);
		mem_release(m_depthStencilStates.m_pDefault);
	}

	IGXDevice* CGUI::getDevice()
	{
		return(m_pDevice);
	}

	IXRender* CGUI::getRender()
	{
		return(m_pRender);
	}

	IDesktopStack* CGUI::newDesktopStack(const char *szResPath, UINT uWidth, UINT uHeight)
	{
		CDesktopStack *pStack = new CDesktopStack(this, m_pRender, szResPath, uWidth, uHeight);
		return(pStack);
	}

//##########################################################################

	CDesktopStack::CDesktopStack(CGUI *pGUI, IXRender *pRender, const char *szResPath, UINT uWidth, UINT uHeight):
		m_pGUI(pGUI),
		m_pRender(pRender),
		m_pDevice(pRender->getDevice())
	{
		add_ref(pGUI);
		updateScreenSize(uWidth, uHeight);

		StringW srp = String(szResPath);

		m_szResourceDir = new WCHAR[srp.length() + 1];
		memcpy(m_szResourceDir, srp.c_str(), sizeof(WCHAR)* (srp.length() + 1));

		m_pQuadVerticesXYZ = m_pDevice->createVertexBuffer(sizeof(float) * 3 * 4, GXBUFFER_USAGE_STREAM);
		m_pQuadVerticesXYZTex16 = m_pDevice->createVertexBuffer(sizeof(float) * 5 * 16, GXBUFFER_USAGE_STREAM);

		m_pQuadRenderXYZ = m_pDevice->createRenderBuffer(1, &m_pQuadVerticesXYZ, pGUI->getVertexDeclarations()->m_pXYZ);
		m_pQuadRenderXYZTex16 = m_pDevice->createRenderBuffer(1, &m_pQuadVerticesXYZTex16, pGUI->getVertexDeclarations()->m_pXYZTex);
		
		m_pVSTransformConstant = m_pDevice->createConstantBuffer(sizeof(SMMATRIX));

		//! @todo: make common cache of that!
		m_pTextureManager = new CTextureManager(m_szResourceDir);
		m_pFontManager = new IFontManager(m_szResourceDir, m_pTextureManager);

		m_pDefaultWhite = m_pTextureManager->getWhite();
	}

	CDesktopStack::~CDesktopStack()
	{
		mem_delete(m_pFontManager);
		mem_delete(m_pTextureManager);

		mem_release(m_pVSTransformConstant);
		mem_release(m_pQuadRenderXYZTex16);
		mem_release(m_pQuadRenderXYZ);
		mem_release(m_pQuadVerticesXYZTex16);
		mem_release(m_pQuadVerticesXYZ);

		mem_release(m_pGUI);

		mem_release(m_pActiveDesktop);

		Array<IDesktop*> aDesktops(m_mDesktops.Size());
		for(AssotiativeArray<StringW, IDesktop*>::Iterator i = m_mDesktops.begin(); i; ++i)
		{
			aDesktops.push_back(*(i.second));
		}
		fora(i, aDesktops)
		{
			mem_release(aDesktops[i]);
		}
	}

	IGXRenderBuffer* CDesktopStack::getQuadRenderBufferXYZ(float3_t *pVertices)
	{
		void *pData;
		if(m_pQuadVerticesXYZ->lock(&pData, GXBL_WRITE))
		{
			memcpy(pData, pVertices, sizeof(float3_t) * 4);
			m_pQuadVerticesXYZ->unlock();
		}
		return(m_pQuadRenderXYZ);
	}
	IGXRenderBuffer* CDesktopStack::getQuadRenderBufferXYZTex16(float *pVertices)
	{
		void *pData;
		if(m_pQuadVerticesXYZTex16->lock(&pData, GXBL_WRITE))
		{
			memcpy(pData, pVertices, sizeof(float) * 5 * 16);
			m_pQuadVerticesXYZTex16->unlock();
		}
		return(m_pQuadRenderXYZTex16);
	}

	BOOL CDesktopStack::putMessage(UINT message, WPARAM wParam, LPARAM lParam)
	{
		if(!m_pActiveDesktop)
		{
			return(TRUE);
		}
		IEvent ev;
		//const dom::IDOMnodeCollection * els;
#ifndef _UNICODE
		char src[] = {(char)wParam, 0};
		wchar_t dst[] = {0, 0};
#endif

		static int wheelDelta = 0;
		static int wheelCount = 0;
		static POINT mc;
		switch(message)
		{
		case WM_SIZE:
			updateScreenSize(LOWORD(lParam), HIWORD(lParam), !m_isResizing);
			//printf("%dx%d\n", (int)LOWORD(lParam), (int)HIWORD(lParam));
			m_isScreenSizeChanged = true;
			break;

		case WM_ENTERSIZEMOVE:
			m_isResizing = true;
			m_isScreenSizeChanged = false;
			break;

		case WM_EXITSIZEMOVE:
			m_isResizing = false;
			if(m_isScreenSizeChanged)
			{
				m_isScreenSizeChanged = false;
				updateScreenSize(m_iScreenWidth, m_iScreenHeight);
			}
			break;

		case WM_MOUSEWHEEL:
			wheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
			wheelCount = 0;
			for(; wheelDelta > WHEEL_DELTA; wheelDelta -= WHEEL_DELTA)
			{
				wheelCount++;
			}
			for(; wheelDelta < 0; wheelDelta += WHEEL_DELTA)
			{
				wheelCount--;
				ev.type = GUI_EVENT_TYPE_MOUSEWHEELDOWN;
			}
			if(wheelCount)
			{
				ev.type = wheelCount > 0 ? GUI_EVENT_TYPE_MOUSEWHEELUP : GUI_EVENT_TYPE_MOUSEWHEELDOWN;
				ev.clientX = GET_X_LPARAM(lParam);
				ev.clientY = GET_Y_LPARAM(lParam);

				for(; wheelCount != 0; wheelCount += (wheelCount > 0 ? -1 : 1))
				{
					m_pActiveDesktop->dispatchEvent(ev);
				}
			}
			break;

		case WM_SETCURSOR:
			break;
			return(FALSE);

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			switch(wParam)
			{
			case KEY_LEFT:
			case KEY_UP:
			case KEY_DOWN:
			case KEY_RIGHT:
			case KEY_HOME:
			case KEY_END:
			case KEY_DELETE:
			case KEY_PGUP:
			case KEY_PGDN:
				ev.type = GUI_EVENT_TYPE_KEYPRESS;
				ev.clientX = ev.clientY = 0;
				ev.key = wParam;
				ev.syskey = true;
				m_pActiveDesktop->dispatchEvent(ev);
				break;
			}
			if(CKeyMap::keyDown(wParam))
			{
				ev.type = GUI_EVENT_TYPE_KEYDOWN;
				ev.clientX = ev.clientY = 0;
				ev.key = wParam;
				m_pActiveDesktop->dispatchEvent(ev);

				if(wParam == KEY_ESCAPE && !ev.preventDefault)
				{
					popDesktop();
				}
			}
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			if(CKeyMap::keyUp(wParam))
			{
				ev.type = GUI_EVENT_TYPE_KEYUP;
				ev.clientX = ev.clientY = 0;
				ev.key = wParam;
				m_pActiveDesktop->dispatchEvent(ev);
			}
			break;

		case WM_CHAR:

			ev.type = GUI_EVENT_TYPE_KEYPRESS;
			ev.clientX = ev.clientY = 0;

#ifdef _UNICODE
			ev.key = wParam;
#else
			
			//MultiByteToWideChar(1251, CP_ACP, src, 1, dst, 1);
			MultiByteToWideChar(CP_ACP, 0, src, 1, dst, 1);
			ev.key = dst[0];
#endif
			ev.syskey = false;

			m_pActiveDesktop->dispatchEvent(ev);

			break;

		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
			ev.type = GUI_EVENT_TYPE_MOUSEDOWN;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = KEY_LBUTTON;
			CKeyMap::keyDown(KEY_LBUTTON);
			m_pActiveDesktop->dispatchEvent(ev);

			break;

		case WM_LBUTTONUP:
			ev.type = GUI_EVENT_TYPE_MOUSEUP;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = KEY_LBUTTON;
			CKeyMap::keyUp(KEY_LBUTTON);
			m_pActiveDesktop->dispatchEvent(ev);
			break;

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			ev.type = GUI_EVENT_TYPE_MOUSEDOWN;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = KEY_RBUTTON;
			CKeyMap::keyDown(KEY_RBUTTON);
			m_pActiveDesktop->dispatchEvent(ev); break;

		case WM_RBUTTONUP:
			ev.type = GUI_EVENT_TYPE_MOUSEUP;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = KEY_RBUTTON;
			CKeyMap::keyUp(KEY_RBUTTON);
			m_pActiveDesktop->dispatchEvent(ev);
			break;



		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
			ev.type = GUI_EVENT_TYPE_MOUSEDOWN;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = KEY_MBUTTON;
			CKeyMap::keyDown(KEY_MBUTTON);
			m_pActiveDesktop->dispatchEvent(ev); break;

		case WM_MBUTTONUP:
			ev.type = GUI_EVENT_TYPE_MOUSEUP;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = KEY_MBUTTON;
			CKeyMap::keyUp(KEY_MBUTTON);
			m_pActiveDesktop->dispatchEvent(ev);
			break;



		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
			ev.type = GUI_EVENT_TYPE_MOUSEDOWN;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = GET_XBUTTON_WPARAM(wParam);
			CKeyMap::keyDown(GET_XBUTTON_WPARAM(wParam));
			m_pActiveDesktop->dispatchEvent(ev); break;

		case WM_XBUTTONUP:
			ev.type = GUI_EVENT_TYPE_MOUSEUP;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			ev.key = GET_XBUTTON_WPARAM(wParam);
			CKeyMap::keyUp(GET_XBUTTON_WPARAM(wParam));
			m_pActiveDesktop->dispatchEvent(ev);
			break;

		case WM_MOUSEMOVE:
			ev.type = GUI_EVENT_TYPE_MOUSEMOVE;
			ev.clientX = GET_X_LPARAM(lParam);
			ev.clientY = GET_Y_LPARAM(lParam);
			m_pActiveDesktop->dispatchEvent(ev);
			break;
		}
		//OutputDebugStringA("putMessage():return(true);\n");
		return(TRUE);
	}

	void CDesktopStack::render()
	{
		if(!m_pActiveDesktop)
		{
			return;
		}

		time_point tNow = std::chrono::high_resolution_clock::now();
		long long mksdt = std::chrono::duration_cast<std::chrono::microseconds>(tNow - m_tPrev).count();
		m_tPrev = tNow;

		float fTimeDelta = (float)mksdt / 1000000.0f;

		//CVideoUpdateManager::update();

		IGXContext *pCtx = m_pDevice->getThreadContext();

		pCtx->clear(GX_CLEAR_STENCIL);

		pCtx->setRasterizerState(m_pGUI->getDefaultRasterizerState());

		pCtx->setSamplerState(m_pGUI->getDefaultSamplerState(), 0);

		pCtx->setPrimitiveTopology(GXPT_TRIANGLELIST);
		
		pCtx->setDepthStencilState(m_pGUI->getDepthStencilStates()->m_pDefault);
		pCtx->setBlendState(m_pGUI->getBlendStates()->m_pDefault);
		m_pTextureManager->bindTexture(m_pDefaultWhite);

		m_pRender->bindShader(pCtx, m_pGUI->getShaders()->m_baseTexturedColored.m_idShaderKit);
/*
		SMMATRIX m(
			2.0f / (float)m_iScreenWidth, 0.0f, 0.0f, 0.0f,
			0.0f, -2.0f / (float)m_iScreenHeight, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			-1.0f, 1.0f, 0.5f, 1.0f);
		m = SMMatrixTranslation(-0.5f, -0.5f, 0.0f) * m;
		*/

		////

		POINT pt = {0};
		//GetCursorPos(&pt);
		//ScreenToClient(m_hWnd, &pt);
		if(pt.x < 0)
		{
			pt.x = 0;
		}
		if(pt.x > (int)m_iScreenWidth)
		{
			pt.x = m_iScreenWidth;
		}
		if(pt.y < 0)
		{
			pt.y = 0;
		}
		if(pt.y > (int)m_iScreenHeight)
		{
			pt.y = m_iScreenHeight;
		}
		pt.x -= m_iScreenWidth / 2;
		pt.y -= m_iScreenHeight / 2;

		float fAngleFOV = SMToRadian(90.0f);

		float fParallaxFactor = m_pActiveDesktop->getParallaxFactor();

		SMMATRIX m = SMMatrixPerspectiveFovLH(fAngleFOV, (float)m_iScreenWidth / (float)m_iScreenHeight, -1.0f, 1.0f);
		SMMATRIX mi = SMMatrixLookAtLH(
			float3(fParallaxFactor * ((float)pt.x / (float)m_iScreenWidth), fParallaxFactor * ((float)pt.y / (float)m_iScreenHeight), 0.0f) +
			float3((float)m_iScreenWidth * 0.5f + 0.5f, (float)m_iScreenHeight * 0.5f + 0.5f, (float)m_iScreenHeight * tanf(fAngleFOV * 0.5f) * 0.5f),

			float3((float)m_iScreenWidth * 0.5f + 0.5f, (float)m_iScreenHeight * 0.5f + 0.5f, 0.0f),
			float3(0.0f, -1.0f, 0.0f)
		);

		setTransformViewProj(mi * m);

		for(UINT i = 0, l = m_mDesktopStack.size(); i < l; ++i)
		{
			if(m_mDesktopStack[i])
			{
				m_mDesktopStack[i]->render(fTimeDelta);
			}
		}
		m_pActiveDesktop->render(fTimeDelta);
#if 0
		struct point
		{
			float x;
			float y;
			float z;
			float tx;
			float ty;
		};


		point a[6] = {
			{TestRect.left, TestRect.top, 0, 0.0f, 1.0f},
			{TestRect.right, TestRect.top, 0, 1.0f, 1.0f},
			{TestRect.left, TestRect.bottom, 0, 0.0f, 0.0f},
			{TestRect.left, TestRect.bottom, 0, 0.0f, 0.0f},
			{TestRect.right, TestRect.top, 0, 1.0f, 1.0f},
			{TestRect.right, TestRect.bottom, 0, 1.0f, 0.0f}
		};




		ITextureManager::BindTexture(w);
		ITextureManager::BindShader(sh);
		pDevice->SetPixelShaderConstantF(0, (float*)&float4_t(1.0f, 1.0f, 1.0f, 0.5f), 1);
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &a, sizeof(point));
#endif
		//	brd.Render();


		//m_pDevice->EndScene();

		//		pDevice->SetDepthStencilSurface(pOldDepthStencilSurface);
	}
	
	IDesktop* CDesktopStack::createDesktopA(const char *name, const char *file)
	{
		IDesktop *pOldDesk = getDesktopA(name);
		if(pOldDesk)
		{
			mem_release(pOldDesk);
		}

		IDesktop *d = new CDesktop(this, StringW(CMB2WC(name)));
		d->setWidth(m_iScreenWidth);
		d->setHeight(m_iScreenHeight);
		d->updateSize();
		d->loadFromFile(CMB2WC(file));
		m_mDesktops[String(name)] = d;
		return(d);
	}

	IDesktop* CDesktopStack::createDesktopW(const wchar_t *name, const wchar_t *file)
	{
		IDesktop *pOldDesk = getDesktopW(name);
		if(pOldDesk)
		{
			mem_release(pOldDesk);
		}

		IDesktop * d = new CDesktop(this, StringW(name));
		d->setWidth(m_iScreenWidth);
		d->setHeight(m_iScreenHeight);
		d->updateSize();
		d->loadFromFile(file);
		((dom::CDOMdocument*)(d->getDocument()))->reflow();
		m_mDesktops[name] = d;
		return(d);
	}

	void CDesktopStack::setActiveDesktop(IDesktop *d, BOOL clearStack)
	{
		IDesktop *pDp = m_pActiveDesktop;
		m_pActiveDesktop = d;
		add_ref(d);
		mem_release(pDp);

		CKeyMap::releaseKeys();
		if(d)
		{
			d->getDocument()->getElementsByTag(L"body")[0][0]->removePseudoclass(css::ICSSrule::PSEUDOCLASS_DISABLED);
		}
		if(clearStack)
		{
			fora(i, m_mDesktopStack)
			{
				mem_release(m_mDesktopStack[i]);
			}
			m_mDesktopStack.clear();
		}
	}

	void CDesktopStack::setActiveDesktopA(const char *name)
	{
		setActiveDesktop(getDesktopA(name));
	}
	void CDesktopStack::setActiveDesktopW(const WCHAR *name)
	{
		setActiveDesktop(getDesktopW(name));
	}

	IDesktop* CDesktopStack::getActiveDesktop()
	{
		return(m_pActiveDesktop);
	}

	IDesktop* CDesktopStack::getDesktopA(const char *_name)
	{
		StringW name = String(_name);
		if(m_mDesktops.KeyExists(name))
		{
			return(m_mDesktops[name]);
		}
		return(NULL);
	}
	IDesktop* CDesktopStack::getDesktopW(const wchar_t *name)
	{
		if(m_mDesktops.KeyExists(name))
		{
			return(m_mDesktops[name]);
		}
		return(NULL);
	}

	void CDesktopStack::registerCallback(const char *cbName, GUI_CALLBACK cb, void *pUserData)
	{
		if(cb)
		{
			//mCallbacks[StringW(String(cbName))].push_back(cb);
			m_mCallbacks[StringW(CMB2WC(cbName))] = {cb, pUserData};
		}
	}

	void CDesktopStack::registerCallbackDefault(GUI_CALLBACK_WC cb, BOOL greedy, void *pUserData)
	{
		if(cb)
		{
			(greedy ? m_mCallbacksDefaults : m_mCallbacksDefaultsWC).push_back({cb, pUserData});
		}
	}

	void CDesktopStack::updateScreenSize(UINT uWidth, UINT uHeight, bool doResize)
	{
		m_iScreenWidth = uWidth;
		m_iScreenHeight = uHeight;
		
		if(!m_iScreenHeight || !m_iScreenWidth || !doResize)
		{
			return;
		}

		for(auto i = m_mDesktops.begin(); i; i++)
		{
			CDesktop * pDsk = ((CDesktop*)(*i.second));
			pDsk->setWidth(m_iScreenWidth);
			pDsk->setHeight(m_iScreenHeight);
			pDsk->updateSize();
			((dom::CDOMdocument*)pDsk->getDocument())->calculateStyles();
			((dom::CDOMnode*)((dom::CDOMdocument*)pDsk->getDocument())->getRootNode())->updateLayout(true);
		}
	}

	void CDesktopStack::showCursor(BOOL bShow)
	{
		if(m_bShowCursor && !bShow)
		{
			m_bShowCursor = false;
			::ShowCursor(FALSE);
		}
		if(!m_bShowCursor && bShow)
		{
			m_bShowCursor = true;
			::ShowCursor(TRUE);
		}
	}

	WCHAR* CDesktopStack::getResourceDir()
	{
		return(m_szResourceDir);
	}

	IGXDevice* CDesktopStack::getDevice()
	{
		return(m_pDevice);
	}

	GUI_CALLBACK CDesktopStack::getCallbackByName(const char *cbName)
	{
		SimpleCallback *pCB = getFullCallbackByName(cbName);
		if(pCB)
		{
			return(pCB->fn);
		}
		return(NULL);
	}

	GUI_CALLBACK CDesktopStack::getCallbackByName(const StringW &cbName)
	{
		SimpleCallback *pCB = getFullCallbackByName(cbName);
		if(pCB)
		{
			return(pCB->fn);
		}
		return(NULL);
	}

	CDesktopStack::SimpleCallback* CDesktopStack::getFullCallbackByName(const char *_cbName)
	{
		StringW cbName = String(_cbName);
		if(m_mCallbacks.KeyExists(cbName))
		{
			return(&m_mCallbacks[cbName]);
		}
		return(NULL);
	}
	CDesktopStack::SimpleCallback* CDesktopStack::getFullCallbackByName(const StringW &cbName)
	{
		if(m_mCallbacks.KeyExists(cbName))
		{
			return(&m_mCallbacks[cbName]);
		}
		return(NULL);
	}

	UINT CDesktopStack::getScreenWidth()
	{
		return(m_iScreenWidth);
	}
	UINT CDesktopStack::getScreenHeight()
	{
		return(m_iScreenHeight);
	}

	void CDesktopStack::destroyDesktop(IDesktop *dp)
	{
		if(!dp)
		{
			return;
		}
		for(AssotiativeArray<StringW, IDesktop*>::Iterator i = m_mDesktops.begin(); i; ++i)
		{
			if(dp == *(i.second))
			{
				m_mDesktops.erase(*(i.first));
				break;
			}
		}
		if(getActiveDesktop() == dp)
		{
			setActiveDesktop(NULL);
		}
	}

	void CDesktopStack::messageBox(const WCHAR *title, const WCHAR *text, ...)
	{
		IDesktop *dp = getDesktopW(L"MessageBox");
		/*if(dp)
		{
			PopDesktop();
			DestroyDesktop(dp);
			dp = NULL;
		}*/
		if(!dp)
		{
			dp = createDesktopW(L"MessageBox", L"sys/messagebox.html");
		}
		dom::IDOMdocument * doc = dp->getDocument();
		doc->getElementById(L"mb_title")->setText(title, TRUE);
		dom::IDOMnode * pText = doc->getElementById(L"mb_text");// ->SetText(text, TRUE);

		while(pText->getChilds()->size())
		{
			pText->removeChild((*(pText->getChilds()))[0]);
		}

		dom::IDOMnodeCollection newText = dp->createFromText(text);
		for(UINT i = 0, l = newText.size(); i < l; i++)
		{
			pText->appendChild(newText[i]);
		}

		dom::IDOMnode *pBtns = doc->getElementById(L"mb_buttons");

		while(pBtns->getChilds()->size())
		{
			pBtns->removeChild((*(pBtns->getChilds()))[0]);
		}

		va_list ap;
		va_start(ap, text);

		WCHAR *cap;
		WCHAR *cmd;
		dom::IDOMnode * pFirst = NULL;
		while((cap = va_arg(ap, WCHAR*)))
		{
			cmd = va_arg(ap, WCHAR*);
			dom::IDOMnode * pNewNode = dp->createFromText(StringW(L"<button onclick=\"") + cmd + L"\">" + cap + L"</button>")[0];
			if(!pFirst)
			{
				pFirst = pNewNode;
			}
			pBtns->appendChild(pNewNode);
		}
		if(pFirst)
		{
			dp->requestFocus(pFirst);
		}
		va_end(ap);
		

		pushDesktop(dp);
	}

	void CDesktopStack::pushDesktop(IDesktop *dp)
	{
		if(m_pActiveDesktop)
		{
			add_ref(m_pActiveDesktop);
			m_mDesktopStack.push_back(m_pActiveDesktop);
			m_pActiveDesktop->getDocument()->getElementsByTag(L"body")[0][0]->addPseudoclass(css::ICSSrule::PSEUDOCLASS_DISABLED);
		}
		setActiveDesktop(dp, FALSE);
	}
	IDesktop* CDesktopStack::popDesktop()
	{
		UINT s = m_mDesktopStack.size();
		if(s == 0)
		{
			return(NULL);
		}
		IDesktop * ret = m_pActiveDesktop;
		setActiveDesktop(m_mDesktopStack[s - 1], FALSE);
		m_pActiveDesktop->getDocument()->getElementsByTag(L"body")[0][0]->removePseudoclass(css::ICSSrule::PSEUDOCLASS_DISABLED);
		mem_release(m_mDesktopStack[s - 1]);
		m_mDesktopStack.erase(s - 1);
		return(ret);
	}

	void CDesktopStack::execCallback(const StringW &cmd, IEvent *ev)
	{
		DefaultCallback *pDefCB;
		SimpleCallback *cb = getFullCallbackByName(cmd);
		if(cb)
		{
			ev->pCallbackData = cb->pUserData;
			cb->fn(ev);
		}
		else
		{
			
			for(UINT i = 0, l = m_mCallbacksDefaults.size(); i < l; ++i)
			{
				pDefCB = &m_mCallbacksDefaults[i];
				ev->pCallbackData = pDefCB->pUserData;
				pDefCB->fn(cmd.c_str(), ev);
			}
		}
		for(UINT i = 0, l = m_mCallbacksDefaultsWC.size(); i < l; ++i)
		{
			pDefCB = &m_mCallbacksDefaultsWC[i];
			ev->pCallbackData = pDefCB->pUserData;
			pDefCB->fn(cmd.c_str(), ev);
		}
	}

	IFont* CDesktopStack::getFont(const WCHAR *szName, UINT size, IFont::STYLE style, int iBlurRadius)
	{
		return(m_pFontManager->getFont(szName, size, style, iBlurRadius));
	}
};


gui::IGUI* InitInstance(IXRender *pRender, IXMaterialSystem *pMaterialSystem, IFileSystem *pFileSystem)
{
	if(gui::g_pGUI)
	{
		return(gui::g_pGUI);
	}

	Core_SetOutPtr();

	gui::CGUI *pGui = new gui::CGUI(pRender, pMaterialSystem, pFileSystem);

	return(pGui);
}

#ifdef SX_STATIC_BUILD
gui::IGUI * GUI_InitInstance(IGXContext * pDev, IXMaterialSystem *pMaterialSystem, IFileSystem *pFileSystem)
{
	return(InitInstance(pDev, pMaterialSystem, pFileSystem));
}

#else

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{

	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif
