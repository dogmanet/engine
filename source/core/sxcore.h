
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file 
Заголовочный файл sxcore - основного ядра
*/

/*! \defgroup sxcore sxcore - основное ядро
@{
*/

#ifndef __SXCORE_H
#define __SXCORE_H

#include <fstream>
#include <gdefines.h>
#include <chrono>

#undef SX_LIB_API
#define SX_LIB_API extern "C" __declspec (dllexport)

#if defined(_WINDOWS)
#	if defined(SXCORE_EXPORTS)
#		define SXCORE_API __declspec(dllexport)
#	else
#		define SXCORE_API __declspec(dllimport)
#		if defined(_DEBUG)
#			pragma comment(lib, "sxcore_d.lib")
#		else
#			pragma comment(lib, "sxcore.lib")
#		endif
#	endif
#else
#	define SXCORE_API
#endif
#define EXTERN_C extern "C"



#include <io.h>
#include <fcntl.h>
#include <tlhelp32.h>

//#define SM_D3D_CONVERSIONS
#include <common/Math.h>
#include <xcommon/IXCore.h>
#include <xcommon/IPluginManager.h>
#include <GRegisterIndex.h>
#include <xcommon/IFileSystem.h>

//! тип функции для обработки в менеджере задач
typedef void(*THREAD_UPDATE_FUNCTION)();

//! значения определющие поведение задачи
enum CORE_TASK_FLAG
{
	CORE_TASK_FLAG_NONE        = 0x0000,

	CORE_TASK_FLAG_REPEATING   = 0x0001, //!< Задача будет повторяться, если не указано - выполнится только один раз
	CORE_TASK_FLAG_THREADSAFE  = 0x0002, //!< Задача может быть выполнена в любом потоке
	CORE_TASK_FLAG_FRAME_SYNC  = 0x0004, //!< Задаче необходима синхронизация по границе кадра
	CORE_TASK_FLAG_ON_SYNC     = 0x0008, //!< Это выполняется в момент синхронизации(выполняет необходимые действия для обмена данными во время синхронизации)
	CORE_TASK_FLAG_FOR_LOOP = 0x0010, //!< Для внутреннего использования

	CORE_TASK_FLAG_MAINTHREAD = CORE_TASK_FLAG_NONE, //!< Задача выполняется в главном потоке, один раз
	CORE_TASK_FLAG_MAINTHREAD_REPEATING = CORE_TASK_FLAG_REPEATING, //!< Задача выполняется в главном потоке, повторяется
	CORE_TASK_FLAG_BACKGROUND = CORE_TASK_FLAG_THREADSAFE, //!< Задача выполняется в фоне(не ожидает синхронизации) один раз
	CORE_TASK_FLAG_BACKGROUND_REPEATING = CORE_TASK_FLAG_THREADSAFE | CORE_TASK_FLAG_REPEATING,		//!< Задача выполняется в фоне(не ожидает синхронизации), по завершении повторяется
	CORE_TASK_FLAG_THREADSAFE_SYNC = CORE_TASK_FLAG_THREADSAFE | CORE_TASK_FLAG_FRAME_SYNC,			//!< Задача может выполняться в любом потоке, ожидает синхронизации, выполняется один раз
	CORE_TASK_FLAG_THREADSAFE_SYNC_REPEATING = CORE_TASK_FLAG_THREADSAFE | CORE_TASK_FLAG_REPEATING | CORE_TASK_FLAG_FRAME_SYNC,	//!< Задача может выполняться в любом потоке, ожидает синхронизации, выполняется многократно

	CORE_TASK_FLAG_ALL = ~0x0
};

/*! \name Базовые функции ядра 
!@{*/

//! @FIXME: Убрать это!
class ITask;
EXTERN_C SXCORE_API IXCore* XCoreInit(const char *szName);
EXTERN_C SXCORE_API void XCoreStart();
EXTERN_C SXCORE_API void XCoreStop();
EXTERN_C SXCORE_API void XCoreAddTask(ITask *pTask);

//! возвращает версию ядра
SX_LIB_API long Core_0GetVersion();

//! создание нового ядра, name - имя, is_unic - должно ли имя ядра быть уникальным
SX_LIB_API void Core_0Create(const char *szName, const char *szNameConsole, bool isUnic = true);

//! Загружает командную строку
SX_LIB_API void Core_0LoadCommandLine(int argc, char **argv);

//! Выполняет консольные команды из командной строки
SX_LIB_API void Core_0ExecCommandLine();

//! Выполняет консольные команды из командной строки
SX_LIB_API const char *Core_0GetCommandLineArg(const char *szArg, const char *szDefault = NULL);

SX_LIB_API XDEPRECATED IXCore *Core_GetIXCore();


//! установка своего обработчика вывода отладочной информации
SX_LIB_API void Core_Dbg_Set(report_func rf); 

//! копирует строку в буфер обмена
SX_LIB_API bool Core_0ClipBoardCopy(const char *szStr);		

//! запущен ли процесс
SX_LIB_API bool Core_0IsProcessRun(const char *szProcess);	

//! уничтожить ядро
SX_LIB_API void Core_AKill(); 

//! получить имя ядра
SX_LIB_API void Core_AGetName(char *szName); 

//!@}

//##########################################################################

/*! @name Менеджер задач
создает потоки по количеству ядер
@{*/

//! добавить задачу
SX_LIB_API void Core_MTaskAdd(
	THREAD_UPDATE_FUNCTION func, //!< функция обработки
	DWORD flag //!< флаг из #CoreTaskFlag 
	);

//! Запретить многопоточный режим
SX_LIB_API void Core_MForceSinglethreaded();

//! стартовать обрабатывать все задачи
SX_LIB_API void Core_MTaskStart();

//! остановить все задачи
SX_LIB_API void Core_MTaskStop();

//! получает идентификатор потока
SX_LIB_API ID Core_MGetThreadID();

//! получить количество потоков
SX_LIB_API int Core_MGetThreadCount();

#if 0
class IParallelForBody
{
public:
	virtual ~IParallelForBody(){}

	virtual void forLoop(int iStart, int iEnd) const = 0;
};
#endif

//! запускает в параллельную обработку pBody
SX_LIB_API ID Core_MForLoop(int iStart, int iEnd, const IParallelForBody *pBody, int iMaxChunkSize = 0);

//! ожидает завершения обработки, начатой Core_MForLoop
SX_LIB_API void Core_MWaitFor(ID id);

//! @}

//##########################################################################

/*! @name Регистры
Массивы данных, определенных типов, доступные в любой библиотеке включающей в себя ядро
@{*/

/*! размер массива регистров */
#define CORE_REGISTRY_SIZE 64	

//! установка значения в регистр bool типа
SX_LIB_API XDEPRECATED void Core_RBoolSet(int id, bool val);

//! получение значения из регистра bool типа
SX_LIB_API XDEPRECATED bool Core_RBoolGet(int id);


//! установка значения в регистр int32_t типа
SX_LIB_API XDEPRECATED void Core_RIntSet(int id, int32_t val);

//! получение значения из регистра int32_t типа
SX_LIB_API XDEPRECATED int32_t Core_RIntGet(int id);


//! установка значения в регистр float32_t типа
SX_LIB_API XDEPRECATED void Core_RFloatSet(int id, float32_t val);

//! получение значения из регистра float32_t типа
SX_LIB_API XDEPRECATED float32_t Core_RFloatGet(int id);


//! установка значения в регистр float3 типа
SX_LIB_API XDEPRECATED void Core_RFloat3Set(int id, float3* val);

//! получение значения из регистра float3 типа
SX_LIB_API XDEPRECATED void Core_RFloat3Get(int id, float3* val);


//! установка значения в регистр string типа
SX_LIB_API XDEPRECATED void Core_RStringSet(int id, const char *val);

//! получение значения из регистра string типа
SX_LIB_API XDEPRECATED const char* Core_RStringGet(int id);

//! @}

//##########################################################################

/*! \name Таймеры
 \note Для Unix даты рекомендуется использовать функции:
  - localtime или localtime_s (из структуры tm в общее количество секунд), 
  - mktime (из общего количества секунд в структуру tm).
 \note По умолчанию Unix дата используется time(NULL) в момент создания таймера
 \note Все возвращаемое время вычисляется в соответсвии с коэфициентом скорости
 \note Функции возвращающие время без постфикса U (update) требуют обновления, которое происходит в функции #Core_TimesUpdate
@{
*/

//! добавление таймера
SX_LIB_API ID Core_TimeAdd();		

//! обновление всех таймеров
SX_LIB_API void Core_TimesUpdate();	

//! установка скорости течения времени
SX_LIB_API void Core_TimeSpeedSet(ID id, float fSpeed);	

//!< возвращает скоротечность времени для таймера
SX_LIB_API float Core_TimeSpeedGet(ID id);				


//! установка состояния запуска таймера
SX_LIB_API void Core_TimeWorkingSet(ID id, bool isWorking);	

//! запущен ли таймер
SX_LIB_API bool Core_TimeWorkingGet(ID id);					


//! установить стартовую дату в Unix в секундах
SX_LIB_API void Core_TimeUnixStartSet(ID id, int64_t iStartTime);	

//! возвращает стартовую дату в Unix в секундах
SX_LIB_API int64_t Core_TimeUnixStartGet(ID id);					

//! возвращает текущюю дату в Unix в секундах
SX_LIB_API int64_t Core_TimeUnixCurrGet(ID id);						


//! возвращает общее время работы таймера в микросекундах (требует обновления)
SX_LIB_API int64_t Core_TimeTotalMcsGet(ID id);						


//! возвращает общее время работы таймера в миллисекундах (требует обновления)
#define Core_TimeTotalMlsGet(id) Core_TimeTotalMcsGet(id)/1000		


//!< возвращает общее время работы таймера в миллисекундах (требует обновления)
#define TimeGetMls(id) Core_TimeTotalMlsGet(id)						

//! возвращает общее время работы таймера в микросекундах (требует обновления)
#define TimeGetMcs Core_TimeTotalMcsGet								


//! возвращает общее время работы таймера в микросекундах (независимо от обновления)
SX_LIB_API int64_t Core_TimeTotalMcsGetU(ID id);					


//! возвращает общее время работы таймера в миллисекундах (независимо от обновления)
#define Core_TimeTotalMlsGetU(id) Core_TimeTotalMcsGetU(id)/1000	

//! возвращает общее время работы таймера в миллисекундах (независимо от обновления)
#define TimeGetMlsU(id) Core_TimeTotalMlsGetU(id)					

//! возвращает общее время работы таймера в микросекундах (независимо от обновления)
#define TimeGetMcsU Core_TimeTotalMcsGetU							

//!@}

//##########################################################################

#include "IFile.h"

/*! \name Создание экземпляров файлов 
!@{*/

//! создать экземпляр класса IFile
SX_LIB_API XDEPRECATED IFile* Core_CrFile();

//! открыть файл
SX_LIB_API XDEPRECATED IFile* Core_OpFile(const char* szPath, int iType);

//!@}

//##########################################################################

/*SX_LIB_API void Core_ResPathAdd(const char *szPath);
SX_LIB_API void Core_ResPathClear();

SX_LIB_API const char* Core_ResPathGetLast(int iRegisterPath=-1);

SX_LIB_API const char* Core_ResPathGetFullPathByRel(const char *szRelPath);
SX_LIB_API const char* Core_ResPathGetFullPathByRel2(const char *szRelPathPart1, const char *szRelPathPart2);
SX_LIB_API const char* Core_ResPathGetFullPathByRelIndex(int iRegisterPath, const char *szRelPath);
SX_LIB_API const char* Core_ResPathGetFullPathByRelIndex2(int iRegisterPath, const char *szRelPathPart1, const char *szRelPathPart2);*/

//##########################################################################

/*! интерфейс для работы с файлами конфигурации (ini) 
 \warning секции и ключи хранятся в виде дерева, и нет гарантии что может быть доступ по порядковому номеру, 
можно получить общее количество секций/ключей, дальше плясать */
struct ISXConfig : public IBaseObject
{
	virtual ~ISXConfig(){};
	virtual void New(const char *path) = 0;	//!< новый файл
	virtual int open(const char *path) = 0;	//!< открыть файл
	virtual const char* getKey(const char *section, const char *key) = 0;	//!< получить значения ключа key который в секции section
	virtual const char* getKeyName(const char *section, int key) = 0;		//!< получить имя ключа по номеру
	virtual const char* getSectionName(int num) = 0;						//!< получить имя секции по номеру
	virtual void set(const char* section, const char *key, const char * val)=0;	//!< установить значение val ключа key котор в секции section
	virtual int save()=0;				//!< сохранить файл
	virtual int getSectionCount()=0;	//!< количество секций в файле
	virtual int getKeyCount()=0;		//!< общее количество ключей
	virtual int getKeyCount(const char *section) = 0;				//!< общее количество ключей в секции
	virtual bool sectionExists(const char *section) = 0;			//!< существует ли секция section
	virtual bool keyExists(const char *section, const char *key) = 0;	//!< существует ли ключ key в секции section
};

/*! \name Создание экземпляров конфигурацииных файлов 
@{*/

//! создать файл экземпляр класса ISXLConfig
SX_LIB_API XDEPRECATED ISXConfig* Core_CrConfig();

//! открыть файл конфигов
SX_LIB_API XDEPRECATED ISXConfig* Core_OpConfig(const char* path);

//!@}

/*! \name Работа с консолью 
!@{*/

#pragma pointers_to_members(full_generality, virtual_inheritance)

typedef void(*SXCONCMD)(); /*!< Тип функции для регистрации команды без аргументов */
typedef void(*SXCONCMDARG)(int argc, const char ** argv); /*!< Тип функции для регистрации команды с аргументами */

class ConCmdStub{}; /*!< Класс-заглушка, для определения следующих типов */
typedef void(ConCmdStub::* SXCONCMDCLS)(); /*!< Тип метода для регистрации команды-члена класса без аргументов */
typedef void(ConCmdStub::* SXCONCMDCLSARG)(int argc, const char ** argv); /*!< Тип метода для регистрации команды-члена класса с аргументами */

SX_LIB_API XDEPRECATED void Core_0RegisterConcmd(const char * name, SXCONCMD cmd, const char * desc = NULL); //!< Регистрация консольной функции без аргументов
SX_LIB_API XDEPRECATED void Core_0RegisterConcmdArg(const char * name, SXCONCMDARG cmd, const char * desc = NULL); //!< Регистрация консольной функции с аргументами
SX_LIB_API XDEPRECATED void Core_0RegisterConcmdCls(const char * name, void * pObject, const SXCONCMDCLS &cmd, const char * desc = NULL); //!< Регистрация консольной функции-члена класса без аргументов
SX_LIB_API XDEPRECATED void Core_0RegisterConcmdClsArg(const char * name, void * pObject, const SXCONCMDCLSARG &cmd, const char * desc = NULL); //!< Регистрация консольной функции-члена класса с аргументами

SX_LIB_API void Core_0ConsoleUpdate(); //!< Обновление консоли, выполнение буфера команд
SX_LIB_API XDEPRECATED void Core_0ConsoleExecCmd(const char * format, ...); //!< Добавление команды на исполнение в буфер команд

SX_LIB_API UINT_PTR Core_ConsoleGetOutHandler();

class COutPtr
{
	friend void Core_SetOutPtr();
	COutPtr()
	{
		UINT_PTR sock = Core_ConsoleGetOutHandler();
		if(sock == ~0)
		{
			return;
		}
		int hOut = _open_osfhandle(sock, O_RDONLY | O_RDWR | O_WRONLY | _O_APPEND);
		m_fOut = ::_fdopen(_dup(hOut), "a+");
		::setvbuf(m_fOut, NULL, _IONBF, 0);

#ifdef _MSC_VER
		if(_fileno(stdout) < 0)
		{
			char szPipename[255];
			HANDLE hPipe = NULL;
			{
				sprintf(szPipename, "\\\\.\\pipe\\SkyXEngineConsoleStdout-%u-%u", GetCurrentProcessId(), GetCurrentThreadId());
				hPipe = CreateNamedPipe(szPipename, PIPE_ACCESS_DUPLEX, PIPE_NOWAIT | PIPE_READMODE_BYTE, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);
				freopen(szPipename, "w", stdout);
				_dup2(_fileno(m_fOut), _fileno(stdout));
				CloseHandle(hPipe);
			}

			{
				sprintf(szPipename, "\\\\.\\pipe\\SkyXEngineConsoleStderr-%u-%u", GetCurrentProcessId(), GetCurrentThreadId());
				hPipe = CreateNamedPipe(szPipename, PIPE_ACCESS_DUPLEX, PIPE_NOWAIT | PIPE_READMODE_BYTE, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);
				freopen(szPipename, "w", stderr);
				_dup2(_fileno(m_fOut), _fileno(stderr));
				CloseHandle(hPipe);
			}
		}
		else
		{
#endif
			_dup2(_fileno(m_fOut), _fileno(stdout));
			_dup2(_fileno(m_fOut), _fileno(stderr));
#ifdef _MSC_VER
	}
#endif
		::setvbuf(stdout, NULL, _IONBF, 0);
		::setvbuf(stderr, NULL, _IONBF, 0);
		//close(hOut);
	}
	~COutPtr()
	{
		if(m_fOut)
		{
			fclose(m_fOut);
		}
	}

	FILE *m_fOut;
};

/*! Устанавливает поток вывода. Для работы консоли
	\warning Должна быть инлайнова, чтобы выполняться в контексте вызывающей библиотеки
*/
__inline void Core_SetOutPtr()
{
	static COutPtr s_optr;
}

//!@}

#define DECLARE_PROFILER_INTERNAL() IXProfiler *g_pProfiler = NULL
#define INIT_PROFILER_INTERNAL() g_pProfiler = (IXProfiler*)Core_GetIXCore()->getPluginManager()->getInterface(IXPROFILER_GUID)

/*! \name Работа с реестром конфигурации (cvars) 
!@{*/

//! Флаги кваров
/*enum CVAR_FLAG
{
	FCVAR_NONE       = 0x00, //!< нет
	FCVAR_CHEAT      = 0x01, //!< Изменение этой переменной с дефолтного значения разрешено только в режиме разработки
	FCVAR_READONLY   = 0x02,  //!< Только для чтения
	FCVAR_NOTIFY_OLD = 0x04, //!< Оповещать об изменениях
	FCVAR_NOTIFY     = 0x08  //!< Оповещать об изменениях
};*/

//! Регистрирует строковую переменную
SX_LIB_API XDEPRECATED void Core_0RegisterCVarString(
	const char * name, //!< Имя квара
	const char * value,  //!< значение по умолчанию
	const char * desc=NULL, //!< краткое описание
	int flags = 0 //!< флаги из CVAR_FLAG
	);

//! Регистрирует целую переменную
SX_LIB_API XDEPRECATED void Core_0RegisterCVarInt(
	const char * name, //!< Имя квара
	int value,  //!< значение по умолчанию
	const char * desc = NULL, //!< краткое описание
	int flags = 0 //!< флаги из CVAR_FLAG
	);

//! Регистрирует дробную переменную
SX_LIB_API XDEPRECATED void Core_0RegisterCVarFloat(
	const char * name, //!< Имя квара
	float value,  //!< значение по умолчанию
	const char * desc = NULL, //!< краткое описание
	int flags = 0 //!< флаги из CVAR_FLAG
	);

//! Регистрирует логическую переменную
SX_LIB_API XDEPRECATED void Core_0RegisterCVarBool(
	const char * name, //!< Имя квара
	bool value,  //!< значение по умолчанию
	const char * desc = NULL, //!< краткое описание
	int flags = 0 //!< флаги из CVAR_FLAG
	);

//! Регистрирует указатель
SX_LIB_API XDEPRECATED void Core_0RegisterCVarPointer(
	const char * name, //!< Имя квара
	UINT_PTR value  //!< значение по умолчанию
	);

//! Получает указатель на значение строкового квара. При отсутствии квара запрошенного типа возвращает NULL
SX_LIB_API XDEPRECATED const char ** Core_0GetPCVarString(const char * name);
#define GET_PCVAR_STRING(k) Core_0GetPCVarString(k)

//! Получает указатель на значение целочисленного квара. При отсутствии квара запрошенного типа возвращает NULL
SX_LIB_API XDEPRECATED const int * Core_0GetPCVarInt(const char * name);
#define GET_PCVAR_INT(k) Core_0GetPCVarInt(k)

//! Получает указатель на значение дробного квара. При отсутствии квара запрошенного типа возвращает NULL
SX_LIB_API XDEPRECATED const float * Core_0GetPCVarFloat(const char * name);
#define GET_PCVAR_FLOAT(k) Core_0GetPCVarFloat(k)

//! Получает указатель на значение логического квара. При отсутствии квара запрошенного типа возвращает NULL
SX_LIB_API XDEPRECATED const bool * Core_0GetPCVarBool(const char * name);
#define GET_PCVAR_BOOL(k) Core_0GetPCVarBool(k)

//! Получает указатель по имени. При отсутствии квара запрошенного типа возвращает NULL
SX_LIB_API XDEPRECATED UINT_PTR * Core_0GetPCVarPointer(const char * name);
#define GET_PCVAR_POINTER(k) Core_0GetPCVarPointer(k)

//! Устанавливает новое значение квара. Должен существовать
SX_LIB_API XDEPRECATED void Core_0SetCVarString(const char * name, const char * value);

//! Устанавливает новое значение квара. Должен существовать
SX_LIB_API XDEPRECATED void Core_0SetCVarInt(const char * name, int value);

//! Устанавливает новое значение квара. Должен существовать
SX_LIB_API XDEPRECATED void Core_0SetCVarFloat(const char * name, float value);

//! Устанавливает новое значение квара. Должен существовать
SX_LIB_API XDEPRECATED void Core_0SetCVarBool(const char * name, bool value);

//! Получает значение квара в виде строки
SX_LIB_API XDEPRECATED void Core_0GetCVarAsString(const char * name, char * szOut, int iMaxLength);


//!@}

#endif

/*! @} */
