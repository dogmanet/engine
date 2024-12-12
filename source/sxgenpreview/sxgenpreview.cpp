
/******************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2018
See the license in LICENSE
******************************************************/

/*!
\file
Главный файл генератора превью игровых ресурсов sxgenpreview
*/

/*!
\page genpreview Генератор превью игровых ресурсов sxgenpreview
Основная задача - генерация превью для некоторых игровых ресурсов, таких как:
 - статическая геометрия
 - анимационные модели
 - текстуры
Основывается на основных данных движка, запускается и работает в скрытом режиме, показывает только консоль.
Поддерживает сравнение времени изменения файлов, то есть при запуске не будет создавать все первью заного, а только те которые более неактуальны по времени.
*/

#include <skyxengine.h>

UINT CreatePreviewTextures()
{
	UINT uStartTime = GetTickCount();
	String sGameTextures = Core_RStringGet(G_RI_STRING_PATH_GS_TEXTURES);
	String sPreviewTextures = Core_RStringGet(G_RI_STRING_PATH_GS_TEXTURES);
	sPreviewTextures.replace(SKYXENGINE_RELPATH_GAMESOURCE, SKYXENGINE_RELPATH_EDITOR_CACHE, 0);

	Array<String> aDirs = FileGetListDirs(sGameTextures.c_str());

	for (int i = 0, il = aDirs.size(); i < il; ++i)
	{
		Array<String> aFiles = FileGetListFiles((sGameTextures + aDirs[i]).c_str());

		FileCreateDir((sPreviewTextures + aDirs[i]).c_str());

		for (int k = 0, kl = aFiles.size(); k < kl; ++k)
		{
			String sFullPathFile = sGameTextures + aDirs[i] + "/" + aFiles[k];
			String sFullPathFileSave = sPreviewTextures + aDirs[i] + "/" + aFiles[k];
			sFullPathFileSave = FileSetStrExt(sFullPathFileSave.c_str(), "png");

			if (FileGetTimeLastModify(sFullPathFile.c_str()) >= FileGetTimeLastModify(sFullPathFileSave.c_str()))
			{
				IGXTexture2D *pTexture = SkyXEngine_LoadAsPreviewData(sFullPathFile.c_str());

				if (pTexture)
					SGCore_GetDXDevice()->saveTextureToFile(sFullPathFileSave.c_str(), pTexture);
				
				mem_release(pTexture);
			}
		}
	}

	return GetTickCount() - uStartTime;
}

UINT CreatePreviewModels()
{
	UINT uStartTime = GetTickCount();
	static IGXContext *pDXDevice = SGCore_GetDXDevice();

	if (!pDXDevice)
		return 1;
#ifdef _GRAPHIX_API
	pDXDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	pDXDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	pDXDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_TRUE);

	pDXDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDXDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pDXDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	pDXDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pDXDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pDXDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);

	pDXDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	pDXDevice->SetRenderState(D3DRS_ALPHAREF, 16);
	pDXDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
#endif

	float4x4 mView;
	float4x4 mProjection = SMMatrixPerspectiveFovLH(SM_PIDIV2, 1.f, 0.025f, 100000.f);


	IXCamera *pCamera = SGCore_CrCamera();
	//pCamera->setFOV(SM_PIDIV2);


	String sGameMeshes = Core_RStringGet(G_RI_STRING_PATH_GS_MESHES);
	String sGameModels = Core_RStringGet(G_RI_STRING_PATH_GS_MODELS);

	Array<String> aFiles = FileGetListRec(sGameMeshes.c_str(), FILE_LIST_TYPE_FILES);

	for (int i = 0, il = aFiles.size(); i < il; ++i)
	{
		aFiles[i] = String(Core_RStringGet(G_RI_STRING_PATH_GS_MESHES)) + aFiles[i];
	}

	Array<String> aFilesModels = FileGetListRec(sGameModels.c_str(), FILE_LIST_TYPE_FILES);

	for (int i = 0, il = aFilesModels.size(); i < il; ++i)
	{
		aFiles.push_back(String(Core_RStringGet(G_RI_STRING_PATH_GS_MODELS)) + aFilesModels[i]);
	}


	for (int i = 0; i < aFiles.size(); ++i)
	{
		//printf("%s\n", aFiles[i].c_str());

		String sPreview = aFiles[i];
		sPreview.replace(SKYXENGINE_RELPATH_GAMESOURCE, SKYXENGINE_RELPATH_EDITOR_CACHE, 0);
		sPreview = FileSetStrExt(sPreview.c_str(), "jpg");

		if (FileGetTimeLastModify(sPreview.c_str()) >= FileGetTimeLastModify(aFiles[i].c_str()))
			continue;

		String sPath = aFiles[i];
		if (!SGCore_DSE_IsModel(sPath.c_str()))
			continue;

		FileCreateDir(FileGetPrevDir(sPreview.c_str()).c_str());

		SGCore_LoadTexClearLoaded();

		IAnimPlayer *pModel = SXAnim_CreatePlayer(sPath.c_str());

		const ISXBound *pBound = pModel->getBound();
		float3 vMin, vMax;
		pBound->getMinMax(&vMin, &vMax);
		float3 vCenter = (vMax + vMin) * 0.5f;
		float3 vDimensions = (vMax - vMin);
		float3 vBias;
		SMQuaternion qBias;

		if (
			(vDimensions.x >= vDimensions.y && vDimensions.x >= vDimensions.z && vDimensions.y >= vDimensions.z) ||
			(vDimensions.y >= vDimensions.x && vDimensions.y >= vDimensions.z && vDimensions.x >= vDimensions.z)
			)
		{
			vBias = float3(vCenter.x, vCenter.y, vCenter.z - 1.f);
		}
		else if (
			(vDimensions.x >= vDimensions.y && vDimensions.x >= vDimensions.z && vDimensions.z >= vDimensions.y) ||
			(vDimensions.z >= vDimensions.x && vDimensions.z >= vDimensions.y && vDimensions.y >= vDimensions.x)
			)
		{
			vBias = float3(vCenter.x, vCenter.y + 1, vCenter.z);
			qBias = SMQuaternion(-SM_PIDIV2, 'x');
		}
		else if (
			(vDimensions.y >= vDimensions.x && vDimensions.y >= vDimensions.z && vDimensions.z >= vDimensions.x) ||
			(vDimensions.z >= vDimensions.x && vDimensions.z >= vDimensions.y && vDimensions.y >= vDimensions.x)
			)
		{
			vBias = float3(vCenter.x + 1.f, vCenter.y, vCenter.z);
			qBias = SMQuaternion(-SM_PIDIV2, 'y');
		}


		float3 vCamPos(vBias);
		pCamera->setOrientation(&(qBias));

		float3 vCamLook;
		pCamera->getLook(&vCamLook);

		pCamera->getViewMatrix(&mView);
		pCamera->updateFrustum(&mProjection);

		float3 aBoxPoints[8];
		aBoxPoints[0] = vMin;
		aBoxPoints[1] = float3(vMin.x, vMin.y, vMax.z);
		aBoxPoints[2] = float3(vMin.x, vMax.y, vMin.z);
		aBoxPoints[3] = float3(vMax.x, vMin.y, vMin.z);
		aBoxPoints[4] = float3(vMax.x, vMax.y, vMin.z);
		aBoxPoints[5] = float3(vMax.x, vMin.y, vMax.z);
		aBoxPoints[5] = float3(vMin.x, vMax.y, vMax.z);
		aBoxPoints[7] = vMax;

		while (true)
		{
			if (
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[0]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[1]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[2]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[3]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[4]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[5]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[6]) ||
				!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[7])
				)
			{
				vCamPos += -(vCamLook);
				pCamera->setPosition(&vCamPos);

				pCamera->getViewMatrix(&mView);
				pCamera->updateFrustum(&mProjection);
			}
			else
				break;
		}


		SGCore_ShaderAllLoad();
		SGCore_LoadTexAllLoad();

		SXAnim_Update();
		SXAnim_Sync();

		pCamera->getViewMatrix(&mView);
		pCamera->updateFrustum(&mProjection);
#ifdef _GRAPHIX_API
		pDXDevice->SetTransform(D3DTS_WORLD, &((D3DXMATRIX)SMMatrixIdentity()));
		pDXDevice->SetTransform(D3DTS_VIEW, &((D3DXMATRIX)mView));
		pDXDevice->SetTransform(D3DTS_PROJECTION, &((D3DXMATRIX)mProjection));
#endif

		pDXDevice->beginFrame();
		pDXDevice->clear(GX_CLEAR_COLOR | GX_CLEAR_DEPTH);

		SXAnim_Render();

		pDXDevice->endFrame();
		pDXDevice->swapBuffers();

		IGXSurface *pBackBuf;
		pBackBuf = pDXDevice->getColorTarget();
#ifdef _GRAPHIX_API
		D3DXSaveSurfaceToFile(sPreview.c_str(), D3DXIFF_JPG, pBackBuf, NULL, NULL);
#endif

		mem_release(pBackBuf);
		mem_release(pModel);
	}

	mem_release(pCamera);

	return GetTickCount() - uStartTime;
}

//##########################################################################

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	SkyXEngine_InitOutLog();
	SkyXEngine_InitPaths();
	HWND hWnd = SkyXEngine_CreateWindow("GenPreview", "GenPreview", SKYXENGINE_PREVIEW_SIZE, SKYXENGINE_PREVIEW_SIZE);

	/*if (!Core_0IsProcessRun("sxconsole.exe"))
		ShellExecute(0, "open", "sxconsole.exe", "0 GenPreview", Core_RStringGet(G_RI_STRING_PATH_EXE), SW_SHOWNORMAL);*/

	Core_0Create("sxcore", "GenPreview", false);
	Core_Dbg_Set(SkyXEngine_PrintfLog);
	Core_SetOutPtr();

	SkyXEngine_CreateLoadCVar();

	int *r_win_width = (int*)GET_PCVAR_INT("r_win_width");
	int *r_win_height = (int*)GET_PCVAR_INT("r_win_height");

	*r_win_width = SKYXENGINE_PREVIEW_SIZE;
	*r_win_height = SKYXENGINE_PREVIEW_SIZE;

	/*ID idTimerRender = Core_TimeAdd();
	Core_RIntSet(G_RI_INT_TIMER_RENDER, idTimerRender);

	Core_TimeWorkingSet(idTimerRender, true);*/

	SSInput_0Create("sxinput", hWnd, false);
	SSInput_Dbg_Set(SkyXEngine_PrintfLog);

	SGCore_0Create("sxgcore", hWnd, SKYXENGINE_PREVIEW_SIZE, SKYXENGINE_PREVIEW_SIZE, true, false);
	SGCore_Dbg_Set(SkyXEngine_PrintfLog);

	SXAnim_0Create();
	SXAnim_Dbg_Set(SkyXEngine_PrintfLog);

	//ShowWindow(hWnd, SW_SHOW);
	CreatePreviewModels();
	CreatePreviewTextures();

	return 0;

	//**********************************************************************

	/*static IDirect3DDevice9 *pDXDevice = SGCore_GetDXDevice();

	if (!pDXDevice)
		return 1;

	pDXDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	pDXDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	pDXDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_TRUE);

	pDXDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDXDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pDXDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	pDXDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pDXDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pDXDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);

	pDXDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	pDXDevice->SetRenderState(D3DRS_ALPHAREF, 16);
	pDXDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

	ShowWindow(hWnd, SW_SHOW);

	ISXCamera *pCamera = SGCore_CrCamera();
	pCamera->setFOV(1.57f);

	//IAnimPlayer *pModel = SXAnim_CreatePlayer("models/stalker_zombi/stalker_zombi.dse");
	IAnimPlayer *pModel = SXAnim_CreatePlayer("meshes/light/light_1.dse");

	float4x4 mView;
	float4x4 mProjection = SMMatrixPerspectiveFovLH(1.57f, 1.f, 0.025f, 100000.f);

	const ISXBound *pBound = pModel->getBound();
	float3 vMin, vMax;
	pBound->getMinMax(&vMin, &vMax);
	float3 vCenter = (vMax + vMin) * 0.5f;
	float3 vDimensions = (vMax - vMin);
	float3 vBias;
	SMQuaternion qBias;

	if (
		(vDimensions.x >= vDimensions.y && vDimensions.x >= vDimensions.z && vDimensions.y >= vDimensions.z) ||
		(vDimensions.y >= vDimensions.x && vDimensions.y >= vDimensions.z && vDimensions.x >= vDimensions.z)
		)
	{
		vBias = float3(vCenter.x, vCenter.y, vCenter.z - 1.f);
	}
	else if (
		(vDimensions.x >= vDimensions.y && vDimensions.x >= vDimensions.z && vDimensions.z >= vDimensions.y) ||
		(vDimensions.z >= vDimensions.x && vDimensions.z >= vDimensions.y && vDimensions.y >= vDimensions.x)
		)
	{
		vBias = float3(vCenter.x, vCenter.y + 1, vCenter.z);
		qBias = SMQuaternion(-SM_PIDIV2, 'x');
	}
	else if (
		(vDimensions.y >= vDimensions.x && vDimensions.y >= vDimensions.z && vDimensions.z >= vDimensions.x) ||
		(vDimensions.z >= vDimensions.x && vDimensions.z >= vDimensions.y && vDimensions.y >= vDimensions.x)
		)
	{
		vBias = float3(vCenter.x + 1.f, vCenter.y, vCenter.z);
		qBias = SMQuaternion(-SM_PIDIV2, 'y');
	}

	float3 vCamPos(vBias);
	pCamera->setOrientation(&(qBias));

	float3 vCamLook;
	pCamera->getLook(&vCamLook);

	pCamera->getViewMatrix(&mView);
	pCamera->updateFrustum(&mProjection);

	float3 aBoxPoints[8];
	aBoxPoints[0] = vMin;
	aBoxPoints[1] = float3(vMin.x, vMin.y, vMax.z);
	aBoxPoints[2] = float3(vMin.x, vMax.y, vMin.z);
	aBoxPoints[3] = float3(vMax.x, vMin.y, vMin.z);
	aBoxPoints[4] = float3(vMax.x, vMax.y, vMin.z);
	aBoxPoints[5] = float3(vMax.x, vMin.y, vMax.z);
	aBoxPoints[5] = float3(vMin.x, vMax.y, vMax.z);
	aBoxPoints[7] = vMax;

	while (true)
	{
		if (
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[0]) || 
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[1]) || 
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[2]) ||
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[3]) ||
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[4]) ||
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[5]) ||
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[6]) ||
			!pCamera->getFrustum()->pointInFrustum(&aBoxPoints[7])
			)
		{
			vCamPos += -(vCamLook);
			pCamera->setPosition(&vCamPos);

			pCamera->getViewMatrix(&mView);
			pCamera->updateFrustum(&mProjection);
		}
		else
			break;
	}

	MSG msg;
	::ZeroMemory(&msg, sizeof(MSG));

	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else
		{
			SGCore_ShaderAllLoad();
			SGCore_LoadTexAllLoad();

			pCamera->updateFrustum(&mProjection);

			SXAnim_Update();
			SXAnim_Sync();

			pCamera->getViewMatrix(&mView);

			pDXDevice->SetTransform(D3DTS_WORLD, &((D3DXMATRIX)SMMatrixIdentity()));
			pDXDevice->SetTransform(D3DTS_VIEW, &((D3DXMATRIX)mView));
			pDXDevice->SetTransform(D3DTS_PROJECTION, &((D3DXMATRIX)mProjection));

			pDXDevice->BeginScene();
			pDXDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
			
			SXAnim_Render();

			pDXDevice->EndScene();
			pDXDevice->Present(0, 0, 0, 0);
		}
	}

	SGCore_LoadTexClearLoaded();
	mem_release(pCamera);
	mem_release(pModel);

	return msg.message;*/
}