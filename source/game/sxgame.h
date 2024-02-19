
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Заголовочный файл sxgame - библиотека игровых объектов
*/


/*!
	\page game_doc Игровая библиотека
	В основе игровой библиотеки лежит понятие "объект". Каждый объект обладает набором свойств, которые можно настраивать. Для взаимодействия объектов между собой используются связи.
	
	\subpage game_connections
	
	\par Подкатегории
	- \subpage ent_doc
*/

/*!
\page game_connections Соединения
Соединения используются для осуществления связи между различными объектами. Каждое соединение состоит из двух частей: выхода и входа. Выход - некий сигнал, генерируемый объектом. Вход - функция другого объекта, которая меняет его состояние. Любой выход может быть связан с любым входом с помощью редактора соединений в редакторе уровня.\n
Для создания соединения нужно выбрать выход, имя целевого объекта, его вход, задержку и параметр.\n
Любой выход может быть связан с любым количеством входов любых обхектов.\n
Задержка - определяет, через какое время выбранный вход будет активирован.\n
*/

/*! \page ent_doc Игровые объекты
	
	\subpage base_entity
	
	\par Подкатегории
	- \subpage entcat_point
	- \subpage entcat_prop
*/

/*! \page entcat_point Точечные объекты

	\subpage base_point
	
	\par Объекты
	- \subpage func_train
	- \subpage info_player_spawn
	- \subpage point_camera
	- \subpage path_corner
	
	\par Подкатегории
	- \subpage entcat_light
*/

/*! \page entcat_prop С моделью
	
	\subpage base_animating
	
	\par Объекты
	- \subpage player
	- \ref npc_base
	- \ref base_item
	
	\par Подкатегории
	- \subpage entcat_trigger
	- \subpage entcat_npc
	- \subpage entcat_items
*/

/*! \page entcat_items Для инвентаря
	
	\subpage base_item
	
	\par Объекты
	- \subpage base_ammobox
	- \ref base_supply
	- \ref base_tool
	- \ref base_wpn_addon
	
	\par Подкатегории
	- \subpage entcat_supply
	- \subpage entcat_tools
	- \subpage entcat_wpn_addons
*/

/*! \page entcat_tools Инструменты
	
	\subpage base_tool
	
	\par Объекты
	- \ref base_weapon
	
	\par Подкатегории
	- \subpage entcat_weapon
	
*/

/*! \page entcat_weapon Оружие
	
	\subpage base_weapon
	
	\par Объекты
	- \subpage base_riffle	
*/

/*! \page entcat_wpn_addons Навесы к оружию
	
	\subpage base_wpn_addon
	
	\par Объекты
	- \subpage base_silencer
	- \subpage base_scope
	- \subpage base_mag
	- \subpage base_handle
	
*/

/*! \page entcat_supply Припасы
	
	\subpage base_supply
	
	\par Объекты
	- \subpage base_ammo
	
*/

/*! \page entcat_npc NPC
	
	\subpage npc_base
	
	\par Объекты
	- \subpage npc_zombie
	
*/

/*! \page entcat_light Свет
	
	\par Объекты
	- \subpage light_directional
	- \subpage light_point
*/

/*! \page entcat_trigger Триггеры
	
	\par Объекты
	- \subpage trigger
*/

/*! \defgroup sxgame sxgame - библиотека игровых объектов
@{
*/

#ifndef __SXGAME_H
#define __SXGAME_H

#ifndef SXGAME_EXPORT
#define SXGAME_EXPORT _declspec(dllimport)
#endif

#include <gdefines.h>

#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllimport)

#ifdef SX_DLL
#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllexport)
#endif

#include "BaseEntity.h"
#include <gui/guimain.h>


/*! Инициализирует библиотеку
*/
SX_LIB_API void SGame_0Create(HWND hWnd, bool isGame);

/*! Деинициализирует библиотеку
*/
SX_LIB_API void SGame_AKill();

/*! Устанавливает функцию сообщений
*/
SX_LIB_API void SGame_Dbg_Set(report_func rf);

/*! Устанавливает количество потоков обновления
*/
SX_LIB_API void SGame_UpdateSetThreadNum(int num);

/*! Выполняет обновление
@param[in] thread Номер потока, выполняющего обновление
*/
SX_LIB_API void SGame_Update(int thread = 0);

/*! Выполняет синхронизацию. Для многопоточного обновления
*/
SX_LIB_API void SGame_Sync();

SX_LIB_API gui::IGUI* SGame_GetGUI();

/*! Помещает игрока в мир
*/
SX_LIB_API void SGame_PlayerSpawn();

SX_LIB_API void SGame_Render(IXRenderTarget *pFinalTarget);
SX_LIB_API void SGame_RenderHUD();

SX_LIB_API void SGame_EditorRender(ID id, ID id_sel_tex, const float3 *pvRenderPos=NULL);

SX_LIB_API IXCamera * SGame_GetActiveCamera();

/*! Получает количество зарегистрированных классов entity
*/
SX_LIB_API int SGame_EntGetClassListCount();

/*! Получает список зарегистрированных классов entity.
	Память под массив const char * должна быть выделена заранее
*/
SX_LIB_API void SGame_EntGetClassList(const char ** pszClassList, int count);

/*! Создает entity заданного класса
*/
SX_LIB_API CBaseEntity * SGame_CreateEntity(const char * classname);

/*! Уничтожает entity
*/
SX_LIB_API void SGame_RemoveEntity(CBaseEntity * pEnt);

/*! Получает таблицу свойств для заданного класса
*/
SX_LIB_API proptable_t * SGame_EntGetProptable(const char * classname);

/*! Получает значение свойства по умолчанию для заданного класса
*/
SX_LIB_API const char * SGame_EntGetDefault(const char * classname, const char * key);

/*! Количество всех entity
*/
SX_LIB_API int SGame_EntGetCount();

/*! Возвращает entity по его id
*/
SX_LIB_API CBaseEntity *SGame_EntGet(ID id);

SX_LIB_API CBaseEntity *SGame_EntGetByName(const char *szName, ID idStart = 0);

SX_LIB_API BOOL SGame_AddWMsg(UINT message, WPARAM wParam, LPARAM lParam);

#if 0
/*! Находит объект по пересечению с лучем, в режиме редактора так же находит точечные объекты, в режиме игры - нет
*/
SX_LIB_API ID SGame_EntGetByRay(const float3 &vStart, const float3 &vDir, float3 *pHitPos = NULL);
#endif

#endif

/*! @} sxgame*/


