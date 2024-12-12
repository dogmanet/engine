#include "GameStates.h"

#include "GameData.h"
#include "Baseline.h"

#include <input/sxinput.h>

#include "Editable.h"

extern CEditable *g_pEditable;

CMainMenuGameState::CMainMenuGameState()
{
	m_pDesktop = GameData::m_pGUIStack->createDesktopA("menu_main", "menu/main.html");

	gui::dom::IDOMnode *pNode = m_pDesktop->getDocument()->getElementById(L"engine_version");
	static const char **pszVersion = GET_PCVAR_STRING("engine_version");
	static const char **pszBuild = GET_PCVAR_STRING("engine_build");
	if(pNode && pszVersion)
	{
		pNode->setText(StringW(L"SkyXEngine ") + StringW(String(*pszVersion)) + StringW(pszBuild ? (StringW(L"+") + String(*pszBuild)) : L""), TRUE);
	}
}

CMainMenuGameState::~CMainMenuGameState()
{
	mem_release(m_pDesktop);
}

void CMainMenuGameState::activate()
{
	printf("CMainMenuGameState::activate()\n");
	GameData::m_pGUIStack->setActiveDesktop(m_pDesktop);

	if(GameData::m_pGuiLayer)
	{
		GameData::m_pGuiLayer->play(true);

		if(GameData::m_pSoundPlayer)
		{
			GameData::m_pSoundPlayer->play();
		}
	}
}

void CMainMenuGameState::deactivate()
{
	printf("CMainMenuGameState::deactivate()\n");
	if(GameData::m_pGuiLayer)
	{
		GameData::m_pGuiLayer->play(false);

		if(GameData::m_pSoundPlayer)
		{
			GameData::m_pSoundPlayer->stop();
		}
	}
}

//##########################################################################

CIngameMenuGameState::CIngameMenuGameState()
{
	m_pDesktop = GameData::m_pGUIStack->createDesktopA("menu_ingame", "menu/ingame.html");

	gui::dom::IDOMnode *pNode = m_pDesktop->getDocument()->getElementById(L"engine_version");
	static const char **pszVersion = GET_PCVAR_STRING("engine_version");
	static const char **pszBuild = GET_PCVAR_STRING("engine_build");
	if(pNode && pszVersion)
	{
		pNode->setText(StringW(L"SkyXEngine ") + StringW(String(*pszVersion)) + StringW(pszBuild ? (StringW(L"+") + String(*pszBuild)) : L""), TRUE);
	}
}

CIngameMenuGameState::~CIngameMenuGameState()
{
	mem_release(m_pDesktop);
}

void CIngameMenuGameState::activate()
{
	printf("CIngameMenuGameState::activate()\n");
	GameData::m_pGUIStack->setActiveDesktop(m_pDesktop);

	if(GameData::m_pGuiLayer)
	{
		GameData::m_pGuiLayer->play(true);

		if(GameData::m_pSoundPlayer)
		{
			GameData::m_pSoundPlayer->play();
		}
	}
}

void CIngameMenuGameState::deactivate()
{
	printf("CIngameMenuGameState::deactivate()\n");
	GameData::m_pGUIStack->setActiveDesktop(m_pDesktop);

	if(GameData::m_pGuiLayer)
	{
		GameData::m_pGuiLayer->play(false);

		if(GameData::m_pSoundPlayer)
		{
			GameData::m_pSoundPlayer->stop();
		}
	}
}

//##########################################################################

void CIngameGameState::activate()
{
	printf("CIngameGameState::activate()\n");
	static const bool *dev_reset_world_on_run = GET_PCVAR_BOOL("dev_reset_world_on_run");

	if(*dev_reset_world_on_run)
	{
		m_pBaseLine = GameData::m_pMgr->createBaseline(m_pBaseLine ? m_pBaseLine->getId() : -1);
	}

	Core_0ConsoleExecCmd("cl_grab_cursor 1");
	GameData::m_pHUDcontroller->activate();

	ID idTimerRender = Core_RIntGet(G_RI_INT_TIMER_GAME);
	Core_TimeWorkingSet(idTimerRender, true);
//	SSCore_ChannelPlay(SX_SOUND_CHANNEL_GAME);
	SSInput_SetEnable(true);
	GetPhysWorld()->enableSimulation();

	Core_0ConsoleExecCmd("game_time_running 1");

	SAFE_CALL(GameData::m_pGameLayer, play, true);
}

void CIngameGameState::deactivate()
{
	printf("CIngameGameState::deactivate()\n");
	static const bool *dev_reset_world_on_run = GET_PCVAR_BOOL("dev_reset_world_on_run");

	Core_0ConsoleExecCmd("game_time_running 0");

	ID idTimerRender = Core_RIntGet(G_RI_INT_TIMER_GAME);
	Core_TimeWorkingSet(idTimerRender, false);
//	SSCore_ChannelStop(SX_SOUND_CHANNEL_GAME);
	SSInput_SetEnable(false);
	GetPhysWorld()->disableSimulation();

	SAFE_CALL(GameData::m_pGameLayer, play, false);

	if(*dev_reset_world_on_run)
	{
		GameData::m_pMgr->dispatchBaseline(m_pBaseLine);

		if(g_pEditable)
		{
			g_pEditable->resync();
		}
	}
}

//##########################################################################

void CEditorState::activate()
{
	printf("CEditorState::activate()\n");
//	ID idTimerRender = Core_RIntGet(G_RI_INT_TIMER_GAME);
//	Core_TimeWorkingSet(idTimerRender, true);
	//SSCore_ChannelPlay(SX_SOUND_CHANNEL_GAME);
//	SSInput_SetEnable(true);

	GameData::m_pMgr->setEditorMode(true);
}

void CEditorState::deactivate()
{
	printf("CEditorState::deactivate()\n");
	GameData::m_pMgr->setEditorMode(false);

//	ID idTimerRender = Core_RIntGet(G_RI_INT_TIMER_GAME);
//	Core_TimeWorkingSet(idTimerRender, false);
	//SSCore_ChannelStop(SX_SOUND_CHANNEL_GAME);
//	SSInput_SetEnable(false);
}

