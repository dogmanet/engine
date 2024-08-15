
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Заголовочный файл sxlevel, библиотека управления уровнем
*/

#ifndef __SXLEVEL_H
#define __SXLEVEL_H

#include <gdefines.h>

#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllimport)

#if defined(_DEBUG)
#pragma comment(lib, "sxscore_d.lib")
#pragma comment(lib, "sxgreen_d.lib")
#pragma comment(lib, "sxaigrid_d.lib")
#pragma comment(lib, "sxparticles_d.lib")
#pragma comment(lib, "sxpp_d.lib")
#pragma comment(lib, "sxdecals_d.lib")
#else
#pragma comment(lib, "sxscore.lib")
#pragma comment(lib, "sxgreen.lib")
#pragma comment(lib, "sxaigrid.lib")
#pragma comment(lib, "sxparticles.lib")
#pragma comment(lib, "sxpp.lib")
#pragma comment(lib, "sxdecals.lib")
#endif

#include <gcore/sxgcore.h>
#include <green/sxgreen.h>
#include <aigrid/sxaigrid.h>
#include <particles/sxparticles.h>
#include <pp/sxpp.h>
//#include <physics/sxphysics.h>

#ifdef SX_DLL
#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllexport)
#endif

//#############################################################################

#define MAX_LEVEL_STRING 128
struct CLevelInfo
{
	char m_szName[MAX_LEVEL_STRING]; //!< имя папки уровня
	char m_szLocalName[MAX_LEVEL_STRING]; //!< Отображаемое имя уровня
	bool m_bHasPreview;

	HANDLE m_hFind; //!< для внутреннего использования
};

//! Возвращает информацию об следующем уровне
SX_LIB_API BOOL SLevel_EnumLevels(CLevelInfo *pInfo);

//#############################################################################

/*! \defgroup sxlevel sxlevel - библиотека управления уровнем
@{*/

//! версия подсистемы
SX_LIB_API long SLevel_0GetVersion();

//! установка функции вывода сообщений
SX_LIB_API void SLevel_Dbg_Set(report_func fnFunc);

//! инициализация подсистемы
SX_LIB_API void SLevel_0Create(
	const char *szName,			//!< передваваемое имя подсистемы
	bool isUnic = true,			//!< должна ли подсистема быть уникальной на основе имени
	bool isServerMode = false	//!< серверный режим (без графики)
	);

//! уничтожение либы
SX_LIB_API void SLevel_AKill();

//#############################################################################

//! очистка уровня
SX_LIB_API void SLevel_Clear();					

//! загрузка уровня
SX_LIB_API void SLevel_Load(const char *szName, bool isGame);	

//! сохранение уровня
SX_LIB_API void SLevel_Save(const char *szName);

//! возвращает и записывает имя уровня
SX_LIB_API const char* SLevel_GetName(char *szName=0);


//! загрузка файла с инфой о частицах
SX_LIB_API void SLevel_LoadParticles();			

//! сохранение файла с инфой о частицах
SX_LIB_API void SLevel_SaveParticles();			

//#############################################################################

/*! \defgroup sxlevel_ambient_snd Фоновые звуки окружения
\ingroup sxlevel
\note Звуки проигрывают поочередно
\note Путь до звука относительно директории со звуками
@{
*/

//! добавить звук
SX_LIB_API void SLevel_AmbientSndAdd(const char *szPath);

//! возвращает общее количество загруженных звуков
SX_LIB_API UINT SLevel_AmbientSndGetCount();

//! в path записывает путь до звука по его id
SX_LIB_API void SLevel_AmbientSndGet(ID id, char *szPath);

//! очистка списка звуков
SX_LIB_API void SLevel_AmbientSndClear();

//! начать проигрывание звуков
SX_LIB_API void SLevel_AmbientSndPlay();

//! обновление проигрывания
SX_LIB_API void SLevel_AmbientSndUpdate();

//! пауза проигрывания
SX_LIB_API void SLevel_AmbientSndPause();

//! возвращает "проигрываются ли звуки"
SX_LIB_API bool SLevel_AmbientSndGetPlaying();

//!@} sxlevel_ambient_snd

//#############################################################################

/*! \defgroup sxlevel_weather Погода
\ingroup sxlevel
\note Смена погоды происходит на основании игрового времени
\note При смене погоды для некоторых параметров возможна интерполяция данных от прошлой погоды к текущей на основании игрового времени, поэтому #SLevel_WeatherUpdate необходимо вызывать в основном цикле (в рендере) для планой смены

Файл-конфиг состоит из:
- основной секции (sections) с набором ключей содержащих имена секций по времени, к примеру имя ключа 01:11:12 это значит что ключ содержит имя секции с погодой для 1 часа ночи 11 минут и 12 секунд
- секции с параметрами погоды (для каждого указанного времени)
- секции с параметрами рандомных звуков, если таковые имеются \n

Секции погоды состоят из:
- sky_texture - имя текстуры для скайбокса
- sky_rotation - угол поворота скайбокса (в радианах)
- sky_color - цвет неба float4 [0-1] где первые три компоненты это сам цвет, а четвертая компонента на сколько будет применен цвет
- clouds_texture - текстура облаков
- clouds_color - цвет облаков float4 [0-1] где первые три компоненты это сам цвет, а четвертая компонента на сколько будет применен цвет
- clouds_rotate - поворот облаков (в радианах)
- clouds_transparency - прозрачность облаков [0-1]
- clouds_speed - скорость движения облаков
- sun_texture - текстура солнца, если указать 0 то солнце будет выключено
- sun_color - цвет солнца и освещения от солнца float3 [0-1]
- sun_pos - позиция сонлца в градусах по оси X и Z
- far - дальность видимости
- rain_density плотность дождя [0-1]
- rain_color - цвет дождя float4 [0-1] где первые три компоненты это сам цвет, а четвертая компонента на сколько будет применен цвет
- thunderbolt - будет ли молния
- thunder_period - перидичность звуков ударов молнии
- fog_color - цвет тумана float3 [0-1]
- fog_density - плотность тумана
- env_ambient - имя секции с рандомными звуками для данной погоды \n

Все ключи в секции погоды обязательны, за исключением env_ambient \n
Все компоненты векторов обязательны \n

Секция с рандомными звуками состоит из:
- period - промежуток (в млсек) между воспроизведением UINT2 (от и до), выбирается случайно
- volume - громкость (в процентах) воспроизведения UINT2 (от и до), выбирается случайно
- sounds - список звуков через запятую

@{
*/

//! загрузка конфига погоды
SX_LIB_API void SLevel_WeatherLoad(const char* path);

//! обновление погоды
SX_LIB_API void SLevel_WeatherUpdate();

//! возвращает текущую плотность дождя
SX_LIB_API float SLevel_WeatherGetCurrRainDensity();

//! начать воспроизведение звуков погоды
SX_LIB_API void SLevel_WeatherSndPlay();

//! приостановить воспроизведение звуков погоды
SX_LIB_API void SLevel_WeatherSndPause();

//! возвращает "воспроизводятся ли звуки погоды"
SX_LIB_API bool SLevel_WeatherSndIsPlaying();

//!@} sxlevel_weather

//!@} sxlevel

#endif