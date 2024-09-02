
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <SkyXEngine.h>
//#include <common/string_func.h>

#include <xWindow/IXWindowSystem.h>
#include <xEngine/IXEngine.h>

#ifdef _DEBUG
#	pragma comment(lib, "xWindow_d.lib")
#	pragma comment(lib, "xEngine_d.lib")
#else
#	pragma comment(lib, "xWindow.lib")
#	pragma comment(lib, "xEngine.lib")
#endif

GX_ENABLE_HIGH_PERFORMANCE_DUAL_GPU();

class CWindowCallback: public IXUnknownImplementation<IXWindowCallback>
{
public:
	CWindowCallback(IXEngine *pEngine):
		m_pEngine(pEngine)
	{
	}

	INT_PTR XMETHODCALLTYPE onMessage(UINT msg, WPARAM wParam, LPARAM lParam, IXWindow *pWindow) override
	{
		if(m_pEngine->onMessage(msg, wParam, lParam))
		{
			return(TRUE);
		}

		switch(msg)
		{
		case WM_CLOSE:
			PostQuitMessage(0);
			break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			// системная обработка F10 (вызов меню) не надо, останавливает главный цикл
			if(wParam == VK_F10)
			{
				return(0);
			}
			// системная обработка Alt (вызов меню) не надо, останавливает главный цикл
			if((wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU)
				&& GetKeyState(VK_TAB) < 0)
			{
				return(0);
			}
			break;

		default:
			return(pWindow->runDefaultCallback(msg, wParam, lParam));
		}

		return(0);
	}
protected:
	IXEngine *m_pEngine;
};

class CEngineCallback: public IXEngineCallback
{
public:
	CEngineCallback(IXWindowSystem *pWindowSystem, IXWindow *pWindow):
		m_pWindow(pWindow),
		m_pWindowSystem(pWindowSystem)
	{}
	void XMETHODCALLTYPE onGraphicsResize(UINT uWidth, UINT uHeight, bool isFullscreen, bool isBorderless, IXEngine *pEngine) override
	{
		XWINDOW_DESC wdesc = *m_pWindow->getDesc();
		/*if(isFullscreen || isBorderless)
		{
			wdesc.iPosX = 0;
			wdesc.iPosY = 0;
		}
		else if(wdesc.iPosX == 0 && wdesc.iPosY == 0)
		{*/
			wdesc.iPosX = XCW_CENTER;
			wdesc.iPosY = XCW_CENTER;
		//}
		wdesc.iSizeX = (int)uWidth;
		wdesc.iSizeY = (int)uHeight;
		wdesc.flags = XWF_BUTTON_CLOSE | XWF_BUTTON_MINIMIZE | XWF_NORESIZE;

		if(isFullscreen || isBorderless)
		{
			wdesc.flags |= XWF_NOBORDER;
		}
		else
		{
			wdesc.flags &= ~XWF_NOBORDER;
		}

		m_pWindow->update(&wdesc);
	}

	bool XMETHODCALLTYPE processWindowMessages() override
	{
		return(m_pWindowSystem->processMessages());
	}

	IXCamera* XMETHODCALLTYPE getCameraForFrame() override
	{
		return(SGame_GetActiveCamera());
	}

protected:
	IXWindowSystem *m_pWindowSystem;
	IXWindow *m_pWindow;
};

#if defined(_WINDOWS)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	int argc = 0;
	char **argv = CommandLineToArgvA(lpCmdLine, &argc);
#else
int main(int argc, char **argv)
{
#endif
	IXEngine *pEngine = XEngineInit(argc, argv, "build");
	INIT_OUTPUT_STREAM(pEngine->getCore());

	IXWindowSystem *pWindowSystem = XWindowInit();
	pEngine->getCore()->getPluginManager()->registerInterface(IXWINDOWSYSTEM_GUID, pWindowSystem);

	CWindowCallback cb(pEngine);
	XWINDOW_DESC wdesc;
	wdesc.iPosX = XCW_CENTER;
	wdesc.iPosY = XCW_CENTER;
	wdesc.iSizeX = 800;
	wdesc.iSizeY = 600;
	wdesc.szTitle = "SkyXEngine build";
	wdesc.flags = XWF_BUTTON_CLOSE | XWF_BUTTON_MINIMIZE | XWF_NORESIZE;
	IXWindow *pWindow = pWindowSystem->createWindow(&wdesc, &cb);
	CEngineCallback engineCb(pWindowSystem, pWindow);

	pEngine->initGraphics(pWindow->getOSHandle(), &engineCb);
	// pEngine->initServer();


	pEngine->getCore()->getConsole()->execCommand("exec ../config_game.cfg");
	pEngine->getCore()->getConsole()->execCommand("exec ../config_game_user.cfg");

#if 1

	IFileSystem *pFS = pEngine->getCore()->getFileSystem();
	IFile *pFile1 = pFS->openFile("dir/test.txt", FILE_MODE_READ);
	pFS->addRoot("fs_test", 20);
	pFS->addRoot("fs_test2", 20);

	printf(COLOR_LRED "################# FILESYTEM TEST ##################\n");

/*	IFile *pFile = pFS->openFile("dir/test.txt", FILE_MODE_READ);
	if(pFile)
	{
		char *pData = (char*)alloca(sizeof(char) * (pFile->getSize() + 1));
		size_t sizeRead = pFile->readBin(pData, sizeof(char) * pFile->getSize());
		pData[sizeRead] = 0;
		printf("File opened! Content: \n%s\n", pData);
	}
	else
	{
		printf("Unable to open file!\n");
	}

    printf("########## File iterator test ###########################\n");

    IFileIterator *itFile = pFS->getFileList("dir/dir1");

    while (const char *file = itFile->next())
    {
        printf("%s \n", file);
    } 

    printf("########## Folder iterator test ###########################\n");

    IFileIterator *itDirectory = pFS->getFolderList("dir/dir1");

    while (const char *file = itDirectory->next())
    {
        printf("%s \n", file);
    } */

	printf("########## Recursive iterator test ###########################\n");

	IFileIterator *itRecursive = pFS->getFileListRecursive("F:/engine/build/fs_test2");

	while (const char *file = itRecursive->next())
	{
		printf("%s \n", file); 
	}

	printf("###################################################\n" COLOR_RESET);

#endif

	int ret = pEngine->start();

	mem_release(pWindow);
	mem_release(pEngine);
	mem_release(pWindowSystem);
	return(ret);
}
