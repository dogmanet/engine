
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Заголовочный файл sxdecals - библиотека декалей
*/

/*! \defgroup sxdecals sxdecals - библиотека декалей
@{
*/


#ifndef __SXDECALS_H
#define __SXDECALS_H

#if 1
#define SXDecals_ShootDecal(...)

#else
#include <gdefines.h>
#include <common/Math.h>

#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllimport)
#include <mtrl/sxmtrl.h>

#ifdef SX_DLL
#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllexport)
#endif
#endif

enum DECAL_TYPE
{
	DECAL_TYPE_CUSTOM = -1,
	DECAL_TYPE_CONCRETE = 0,
	DECAL_TYPE_METAL,
	DECAL_TYPE_GLASS,
	DECAL_TYPE_PLASTIC,
	DECAL_TYPE_WOOD,
	DECAL_TYPE_FLESH,
	DECAL_TYPE_EARTH,
	DECAL_TYPE_BLOOD_BIG,

	DECAL_TYPE_LAST
};

#if 0
/*! Инициализирует библиотеку
*/
SX_LIB_API void SXDecals_0Create();

/*! Деинициализирует библиотеку
*/
SX_LIB_API void SXDecals_AKill();

/*! Устанавливает функцию сообщений
*/
SX_LIB_API void SXDecals_Dbg_Set(report_func rf);


/*! Выполняет обновление физики
*/
SX_LIB_API void SXDecals_Update();

/*! Выполняет синхронизацию. Для многопоточного обновления
*/
SX_LIB_API void SXDecals_Sync();


/*! Выполняет отрисовку физических объектов
*/
SX_LIB_API void SXDecals_Render();

/*! Добавляет декаль
*/
SX_LIB_API void SXDecals_ShootDecal(DECAL_TYPE type, const float3 & fWorldPos, const float3 & normal);
SX_LIB_API void SXDecals_ShootDecalEx(DECAL_TYPE type, const float3 & fWorldPos, const float3 & normal, int flags, ID material = -1, float fScale = 1.0f, const float3 * saxis = NULL);

/*! Удаляет все декали
*/
SX_LIB_API void SXDecals_Clear();

#endif

#endif

/*! @} */

