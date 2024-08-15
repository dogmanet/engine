
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*! \page general_info_libs Общая информация о библиотеках
\tableofcontents
\section general_info_libs_sec0 Общие правила библиотек

Каждая  функция в библиотеках имеет префикс с именем самой библиотеки которое формируется исходя из назначения. После следует символ _ после которого (желательно) указывается имя объекта/подсистемы/подмодуля если имеется.\n
Все функции имя которых после _ начинается с 0 можно вызывать без инициализации самой библиотеки, все остальные можно вызывать только после инициализации.\n
Набор стандартных функций с 0:
 - 0GetVersion – возвращает номер версии текущей библиотеки.
 - 0Create – инициализирует текущую библиотеку.

Функция Dbg_Set переназначает стандартную функцию вывода сообщений, и доступна в любой момент времени, независимо от инициализации библиотеки.\n
Функции имя которых после _ начинается с A являются начальными, и могут быть вызваны только после того как библиотека была инициализирована.\n
Набор стандартны функций с A:
 - AGetName  – получить имя текущей инициализированной библиотеки, которое было указано при инициализации.
 - AKill – уничтожить инициализированную библиотеку.

Каждая инициализированная библиотека должна иметь имя, уникальное или нет задается в момент вызова инициализации исходя из нужд программиста.\n
Каждая библиотека также может называться подсистема, в некоторых случаях ядро.\n
Для подключения любой библиотеки (в виде заголовочного файла либо исходного кода) осуществляется посредством подключения в проект всего одного файла с кодом:
 - для подключения внешней библиотеки (dll) нужно подключить заголовочный файл с расширением h а также файл с для экспорта функций с расширением lib.
 - для подключения исходного кода библиотеки достаточно подключить только cpp файл с исходным кодом.

Имена заголовочного и файла с исходным кодом одинаковые и отражают назначение самой библиотеки. К примеру sxgcore – графическое ядро, sxinput – система ввода (клавиатура/мышь), sxpp – библиотека с постпроцессом и т.д.

\section general_info_libs_sec1 Начальный базовый интерфейс IBaseObject
Начальным родителем всех интерфейсов является базовый интерфейс IBaseObject, который объявлен в файле gdefines.h и имеет следующий вид:\n
\code 
struct IBaseObject
{
	virtual ~IBaseObject(){};
	virtual void Release()=0;
};
\endcode

\section general_info_libs_sec2 Система вывода сообщений в библиотеках

Во всех библиотеках предусмотрена система вывода сообщений через единую функцию. По умолчанию все сообщения выводятся через стандартную функцию def_report, объявленную в gdefines.h.\n
Стандартная функция имеет ограничение на длину сообщения которое регулируется макроопределением REPORT_MSG_MAX_LEN объявленном в gdefines.h. При написании своего обработчика сообщений следует использовать данное ограничение.\n
Уровни критичности сообщений объявлены в gdefines.h: \n
 - #REPORT_MSG_LEVEL_NOTICE
 - #REPORT_MSG_LEVEL_WARNING
 - #REPORT_MSG_LEVEL_ERROR

В случае переназначения стандартной функции сообщений необходимо самостоятельно(в переназначаемой функции) обрабатывать уровни критичности, для примера можно использовать механизм стандартной функции.\n

Тип функции для сообщений: \n
 - #report_func

Каждая библиотека имеет функцию для переназначения стандартной функции сообщений на пользовательскую. Пример основного ядра: \n
 - #Core_Dbg_Set(report_func rf);

Данная функция может вызываться в любой момент времени, инициализация библиотеки необязательна.\n
Для формирования строки вида полный_путь_до_файла : номер_текущей_строки можно воспользоваться макроопределением gen_msg_location объявленной в gdefines.h.

\section general_info_libs_sec3 Ограничения на размеры
 - #CORE_NAME_MAX_LEN - максимальная длина имени объекта ядра/подсистемы 
 - #OBJECT_NAME_MAX_LEN - максимальная длина имени абстрактного объекта 
 - #CONFIG_SECTION_MAX_LEN - максимальная длина секции конфигурационного файла

Под абстрактным объектом следует понимать обобщенные элементы движка, к примеру имя статической модели, имя источника света и прочее.

\section general_info_libs_sec4 Макроопределения
Данные макроопределения необходимо использовать только по назначению и они являются обязательными к использовнаию.
 - SX_EXE - данный проект исполняемый файл
 - SX_DLL - данный проект dll бибилиотека
 - SX_LEVEL_EDITOR - данный проект "редактор уровней"
 - SX_MATERIAL_EDITOR - данный проект "редактор материалов"
 - SX_PARTICLES_EDITOR - данный проект "редактор частиц"
 - SX_GAME - данный проект "билд"
*/

/*!
\file
Заголовочный файл для всех библиотек движка SkyXEngine
*/

//! макрос экспорта/импорта из библиотек
/*#if !defined(SX_LIB_API)
#if defined(SX_EXE)
#define SX_LIB_API extern "C" __declspec (dllimport)
#elif defined(SX_DLL)
#define SX_LIB_API extern "C" __declspec (dllexport)
#else
#define SX_LIB_API extern
#endif
#endif*/

#ifndef __GDEFINES_H
#define __GDEFINES_H

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdlib>

/*! \name Некоторые ограничения на размерности */
//!@{
#define CORE_NAME_MAX_LEN		32	/*!< максимальная длина имени объекта ядра/подсистемы */
#define OBJECT_NAME_MAX_LEN		64	/*!< максимальная длина имени абстрактного объекта */
#define CONFIG_SECTION_MAX_LEN	64	/*!< максимальная длина секции конфигурационного файла */
//!@}

/*! звуковой канал самой игры */
#define SX_SOUND_CHANNEL_GAME	0

/*! звуковой канал главного меню */
#define SX_SOUND_CHANNEL_MAIN_MENU	1

/*! Пустые дефайны для визуальной идентификации аргументов функций
@{
*/

//! выходной, будет произведена запись
#define _out

//! входящий, без записи 
#define _in

//! опциональный, значение может быть недействительным (к примеру вместо указателей можно отправить 0)
#define _opt

//!@}

//! for warning C4003: not enough actual parameters for macro
#define _VOID

#define ID_VALID(id) ((id) >= 0)

#define SAFE_CALL(obj, fn, ...) if(obj){obj->fn(__VA_ARGS__);}

#ifndef IFACEBASEOBJECT
#define IFACEBASEOBJECT

/*! Общий базовый интерфейс. Базовый интерфейс, от него происходит наследование во всех интерфейсах
 \note для удаления объекта вызвать Release, после сего объект будет уничтожен, но адрес не обнулится
 \note для удаления и обнуления использовать #mem_release
*/
struct IBaseObject 
{
	virtual ~IBaseObject(){};
	virtual void Release()=0;
};



#endif

#include <common/types.h>
#include <atomic>
#include <mutex>

class IKeyIterator: public IXUnknown
{
public:
	virtual const char* XMETHODCALLTYPE getCurrent() = 0;
	virtual const char* XMETHODCALLTYPE getNext() = 0;
	virtual bool XMETHODCALLTYPE isEnd() = 0;
};


//! Считывание неопределенного количества аргументов при форматированнии строки в buf на основании format
#define format_str(buf,format) va_list va; va_start(va, format); vsprintf_s(buf, sizeof(buf), format, va); va_end(va);

#define F3_BTVEC(xmf) (btVector3((xmf).x, (xmf).y, (xmf).z))
#define BTVEC_F3(btv) (float3((btv).x(), (btv).y(), (btv).z()))

#define Q4_BTQUAT(xmf) (btQuaternion((xmf).x, (xmf).y, (xmf).z, -(xmf).w))
#define BTQUAT_Q4(btv) (SMQuaternion((btv).x(), (btv).y(), (btv).z(), -(btv).w()))

#define MACRO_TEXT_(x) #x
#define MACRO_TEXT(x) MACRO_TEXT_(x)

#define MACRO_CAT_(a, b) a ## b
#define MACRO_CAT(a, b)  MACRO_CAT_(a, b)

//! Формирование строки вида file_name:string_num
#define GEN_MSG_LOCATION __FILE__ ## ": " ## MACRO_TEXT(__LINE__)

#define ASSERT_S(str,...) AllocConsole();freopen("CONOUT$", "wt", stdout); fprintf(stdout, str, ...); exit(1);
#define ASSERT(expr) if(!expr) ASSERT_S(#expr)

#define strdupa(str) strcpy((char*)alloca(sizeof(char) * (strlen(str) + 1)), str)

#define strdups(str) ((str) ? strdup(str) : NULL)

#define XWARN_(type) __FILE__ "(" MACRO_TEXT( __LINE__ ) ") : warning " ## type ## ": "
#define TODO(msg) __pragma(message(XWARN_("TODO") msg))
#define FIXME(msg) __pragma(message(XWARN_("FIXME") msg))

//! Тип функции вывода отладочной информации
typedef void(*report_func) (int iLevel, const char *szLibName, const char *szMessage);

#include <cstdio>
#include <cstdarg>
#if defined(_WINDOWS)
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#	include <common/MB2WC.h>
#endif

#include <common/math.h>
#include <common/array.h>
#include <common/assotiativearray.h>
#include <common/MemAlloc.h>
#include <common/EnumReflection.h>
#include <common/CWC2MB.h>

#include <xcommon/input/XKeyCodes.h>

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

/** \name Уровни критичности сообщений для функции репортов */
//! @{
#define REPORT_MSG_LEVEL_NOTICE		0	/*!< заметка */
#define REPORT_MSG_LEVEL_WARNING	1	/*!< предупреждение */
#define REPORT_MSG_LEVEL_ERROR		-1	/*!< ошибка, продолжаем */
#define REPORT_MSG_LEVEL_FATAL		-2	/*!< ошибка, желательно вылетать */

#define REPORT_MSG_MAX_LEN 4096		/*!< максимальный размер сообщения */
//! @}

/** \name Коды цветов для цветной консоли */
//! @{
#define COLOR_RESET "\033[0m" /*!< Сброс на стандартные цвета текста и фона */

#define COLOR_BLACK    "\033[38;5;0m"  /*!< Черный текст */
#define COLOR_RED      "\033[38;5;1m"  /*!< Красный текст */
#define COLOR_GREEN    "\033[38;5;2m"  /*!< Зеленый текст */
#define COLOR_BLUE     "\033[38;5;4m"  /*!< Синий текст */
#define COLOR_OLIVE    "\033[38;5;3m"  /*!< Темно-желтый текст */
#define COLOR_MAGENTA  "\033[38;5;5m"  /*!< Пурпурный текст */
#define COLOR_CYAN     "\033[38;5;6m"  /*!< Темно-бирюзовый текст */
#define COLOR_GRAY     "\033[38;5;7m"  /*!< Серый текст */

#define COLOR_DARKGRAY "\033[38;5;8m"  /*!< Темно-серый текст */
#define COLOR_LRED     "\033[38;5;9m"  /*!< Светло красный текст */
#define COLOR_LGREEN   "\033[38;5;10m" /*!< Зеленый текст */
#define COLOR_LBLUE    "\033[38;5;12m" /*!< Светло-синий текст */
#define COLOR_YELLOW   "\033[38;5;11m" /*!< Желтый текст */
#define COLOR_LMAGENTA "\033[38;5;13m" /*!< Розоывй текст */
#define COLOR_LCYAN    "\033[38;5;14m" /*!< Бирюзовый текст */
#define COLOR_WHITE    "\033[38;5;15m" /*!< Белый текст */

#define COLOR_BG_BLACK    "\033[48;5;0m"  /*!< Черный фон */
#define COLOR_BG_RED      "\033[48;5;1m"  /*!< Красный фон */
#define COLOR_BG_GREEN    "\033[48;5;2m"  /*!< Зеленый фон */
#define COLOR_BG_BLUE     "\033[48;5;4m"  /*!< Синий фон */
#define COLOR_BG_OLIVE    "\033[48;5;3m"  /*!< Темно-желтый фон */
#define COLOR_BG_MAGENTA  "\033[48;5;5m"  /*!< Пурпурный фон */
#define COLOR_BG_CYAN     "\033[48;5;6m"  /*!< Темно-бирюзовый фон */
#define COLOR_BG_GRAY     "\033[48;5;7m"  /*!< Серый фон */

#define COLOR_BG_DARKGRAY "\033[48;5;8m"  /*!< Темно-серый фон */
#define COLOR_BG_LRED     "\033[48;5;9m"  /*!< Светло красный фон */
#define COLOR_BG_LGREEN   "\033[48;5;10m" /*!< Зеленый фон */
#define COLOR_BG_LBLUE    "\033[48;5;12m" /*!< Светло-синий фон */
#define COLOR_BG_YELLOW   "\033[48;5;11m" /*!< Желтый фон */
#define COLOR_BG_LMAGENTA "\033[48;5;13m" /*!< Розоывй фон */
#define COLOR_BG_LCYAN    "\033[48;5;14m" /*!< Бирюзовый фон */
#define COLOR_BG_WHITE    "\033[48;5;15m" /*!< Белый фон */

//! @}

#define CONSOLE_TITLE "\033]0;"
#define CONSOLE_TITLE_END "\a"

#ifndef DEFAULT_FUNCTION_REPORT 
#define DEFAULT_FUNCTION_REPORT

#if 0
/*! Дефолтовая функция вывода отладочной информации ВМЕСТО НЕЕ В ЯДРО/ПОДСИСТЕМУ НУЖНО ОТПРАВЛЯТЬ СВОЮ */
inline void DefReport(int iLevel, const char *szLibName, const char *szFormat, ...)
{
#if 0 && defined(_WINDOWS)
	AllocConsole();
	freopen("CONOUT$", "wt", stdout);
#endif
	char buf[REPORT_MSG_MAX_LEN];
	int strl = sizeof(buf);
	format_str(buf, szFormat);
	fprintf(stdout, "!!! report function is not init !!!\n  LibName: %s\n  message: %s\n", szLibName, buf);
	fprintf(stdout, "work program will be stopped within 5 seconds ...");
	Sleep(5000);
	exit(1);
}
#endif
inline void DefReport(int iLevel, const char *szLibName, const char *szMessage)
{
	if(!isspace((unsigned char)szMessage[0]))
	{
		printf(COLOR_GREEN "%s" COLOR_RESET ": ", szLibName);
	}
	if(iLevel == REPORT_MSG_LEVEL_ERROR || iLevel == REPORT_MSG_LEVEL_FATAL)
	{
		fputs(COLOR_LRED, stdout);
	}
	else if(iLevel == REPORT_MSG_LEVEL_WARNING)
	{
		fputs(COLOR_YELLOW, stdout);
	}
	fputs(szMessage, stdout);
	if(iLevel == REPORT_MSG_LEVEL_ERROR || iLevel == REPORT_MSG_LEVEL_WARNING || iLevel == REPORT_MSG_LEVEL_FATAL)
	{
		fputs(COLOR_RESET, stdout);
	}

	if(iLevel == REPORT_MSG_LEVEL_FATAL)
	{
		exit(0);
	}
}

#endif


#ifndef SX_LIB_NAME
#define SX_LIB_NAME "User"
#endif

inline void LibReport(int iLevel, const char *szFormat, ...)
{
	va_list va;
	va_start(va, szFormat);
	size_t len = _vscprintf(szFormat, va) + 1;
	char * buf = (char*)alloca(len * sizeof(char));
	vsprintf(buf, szFormat, va);
	va_end(va);

	DefReport(iLevel, SX_LIB_NAME, buf);
}
#define LogInfo(...) LibReport(REPORT_MSG_LEVEL_NOTICE, __VA_ARGS__)
#define LogWarning(...) LibReport(REPORT_MSG_LEVEL_WARNING, __VA_ARGS__)
#define LogError(...) LibReport(REPORT_MSG_LEVEL_ERROR, __VA_ARGS__)
#define LogFatal(...) LibReport(REPORT_MSG_LEVEL_FATAL, __VA_ARGS__)


#define SX_MAX_THREAD_COUNT 64

#include <xcommon/IXProfiler.h>

#endif
