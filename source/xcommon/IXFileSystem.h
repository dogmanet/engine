#ifndef __IXFILESYSTEM_H
#define __IXFILESYSTEM_H

#include <xcommon/IXFile.h>

// Интерфейс для работы с файловой системой (в том числе с упакованными архивами в будущем)
// Может производить поиск по нескольким корневым путям

// {E9A7046B-2D72-4073-A8E7-E96B9E7A7B7A}
#define IXFILESYSTEM_GUID DEFINE_GUID(0xe9a7046b, 0x2d72, 0x4073, 0xa8, 0xe7, 0xe9, 0x6b, 0x9e, 0x7a, 0x7b, 0x7a)

#define FILE_NOT_FOUND ((size_t)-1)

//! типы списков для #getFileList
enum FILE_TYPE
{
	//! файлы
	FILE_TYPE_FILE,

	//! директории
	FILE_TYPE_DIR,

	//! все
	FILE_TYPE_ALL,
};

enum FILE_OPEN_MODE
{
	FILE_MODE_READ   = 0x00000001,
	FILE_MODE_WRITE  = 0x00000002,
	FILE_MODE_APPEND = 0x00000004
};
DEFINE_ENUM_FLAG_OPERATORS(FILE_OPEN_MODE);

class IXFileIterator: public IXUnknown
{
public:
	//! Возврат к первому элементу
	virtual void XMETHODCALLTYPE reset() = 0;
	//! Имя следующего файла, NULL если файлы закончились
	virtual const char* XMETHODCALLTYPE next() = 0;
};

class IXBaseFileSystem: public IXUnknown
{
public:
	//! Проверяет наличие файла или каталога по указанному пути
	virtual bool XMETHODCALLTYPE fileExists(const char *szPath) = 0;
	//! Получает размер файла в байтах, либо FILE_NOT_FOUND в случае, если файл не существует, либо не является файлом
	virtual size_t XMETHODCALLTYPE fileGetSize(const char *szPath) = 0;
	//! Проверяет, что путь существует, и является файлом
	virtual bool XMETHODCALLTYPE isFile(const char *szPath) = 0;
	//! Проверяет, что путь существует, и является каталогом
	virtual bool XMETHODCALLTYPE isDirectory(const char *szPath) = 0;
	//! возвращает время последнего изменения файла
	virtual time_t XMETHODCALLTYPE getFileModifyTime(const char *szPath) = 0;

	//! возвращает массив со всеми данными находящимися по пути szPath
	virtual IXFileIterator* XMETHODCALLTYPE getDirEntries(const char *szPath) = 0;
};

class IXMountableFileSystem: public IXBaseFileSystem
{
	//! Получить имя файловой системы
	virtual const char* XMETHODCALLTYPE getName() = 0;

	//! Истина, если эта файловая система поддерживает только чтение
	virtual bool XMETHODCALLTYPE isReadOnly() = 0;

	//! Открыть файл.
	virtual IXFile* XMETHODCALLTYPE openFile(const char *szPath, FILE_OPEN_MODE mode = FILE_MODE_READ) = 0;
	//! Удаляет файл
	virtual bool XMETHODCALLTYPE unlink(const char *szPath) = 0;
	//! Перемещает файл или каталог
	virtual bool XMETHODCALLTYPE rename(const char *szPath, const char *szNewPath) = 0;
	
	//! Создает директорию по указанному пути
	virtual bool XMETHODCALLTYPE mkdir(const char *szPath) = 0;
	//! Удаляет пустую директорую
	virtual bool XMETHODCALLTYPE rmdir(const char *szPath) = 0;
};

struct XFileSystemMountInfo
{
	const char *szMountPoint;
	IXMountableFileSystem *pMFS;
	bool isWritable;
	int iPriority;
};

struct XFileSystemWriteHints
{
	const char *szTarget = NULL; // семантика желаемой точки монтирования
	bool bFailIfReadOnly = false; // вернуть ошибку в случае невозможности прямой записи, иначе использовать другой корень
	void *pReserved; // I don't know yet, must be null
};

class IXFileSystem: public IXBaseFileSystem
{
public:
	/*! Смонтировать файловую систему в указанный каталог
		@param szPath путь к точке монтирования
		@param pMFS указатель на объект реализации файловой системы
		@param szTargetSemantics семантика данной точки монтирования, используется при указании XFileSystemWriteHints
		@param isWritable монтирование в режиме с поддержкой записи
		@param iPriority приоритет для данной точки монтирования. 
		
		При наличии нескольких смонтированных в одну точку ФС, работают как overlay.
		Просмотр путей идет в порядке убывания приоритета. Таким образом, 
		при наличии нескольких версий одного файла,	доступ будет осуществлен к тому, 
		который находится в корне с большим приоритетом.
		Запись файлов:
			- при указании подсказок, руководствоваться ими;
			- если записываемый файл существует и находится в корне с поддержкой записи, записать в него:
			- иначе найти новый корень с наибольшим приоритетом и поддержкой записи:
				- если приоритет нового корня меньше, чем у того корня, 
				  в котором находится файл, то запись невозможна;
				- иначе записать в него;
		При монтировании в существующий непустой каталог, он считается ФС с наименьшим приоритетом в данной точке монтирования.
	*/
	virtual bool XMETHODCALLTYPE mount(const char *szPath, IXMountableFileSystem *pMFS, const char *szTargetSemantics = NULL, bool isWritable = false, int iPriority = -1) = 0;
	//! Отключает ФС от точки монтирования
	virtual bool XMETHODCALLTYPE umount(IXMountableFileSystem *pMFS) = 0;

	//! Получить количество монтирований
	virtual UINT XMETHODCALLTYPE getMountsCount() = 0;
	virtual const XFileSystemMountInfo* XMETHODCALLTYPE getMountInfo(UINT uIdx) = 0;

	//! Создать новую монтируемую файловую систему в стандартной реализации (основана на API операционной системы)
	virtual bool XMETHODCALLTYPE newDefaultMountableFileSystem(const char *szRootPath, IXMountableFileSystem **ppOut) = 0;

	//! Возвращает список всех папок
	virtual IXFileIterator* XMETHODCALLTYPE getFolderList(const char *szPath) = 0;

	//! возвращает массив со всеми данными находящимися по пути szPath
	virtual IXFileIterator* XMETHODCALLTYPE getFileList(const char *szPath, const char *szExt = 0) = 0;
	//! То же, что и предыдущая, только для массива расширений
	virtual IXFileIterator* XMETHODCALLTYPE getFileList(const char *szPath, const char **szExts, int extsCount) = 0;

	/*! возвращает список всех файлов или папок (в зависимости от extention),
		@param szPath не должен содержать фильтров, может быть не канонизирован и без последнего слэша,
		@param szExt - расширение файла без точки
	*/
	virtual IXFileIterator* XMETHODCALLTYPE getFileListRecursive(const char *szPath, const char *szExt = 0) = 0;
	//! То же, что предыдущая, только позволяет использовать массив расширений для поиска. Последний элемент массива NULL
	virtual IXFileIterator* XMETHODCALLTYPE getFileListRecursive(const char *szPath, const char **szExts, int extsCount) = 0;

	//! Открыть файл.
	virtual IXFile* XMETHODCALLTYPE openFile(const char *szPath, FILE_OPEN_MODE mode = FILE_MODE_READ, XFileSystemWriteHints *pHints = NULL) = 0;

	/*! Удаляет файл
		При попытке удаления файла, у которого есть версии в корнях, смонтированных в режиме "только для чтения", 
		необходимо создать файл-метку, которая будет свидетельствовать о необходимости игнорирования наличия этого файла
	*/
	virtual bool XMETHODCALLTYPE unlink(const char *szPath) = 0;

	/*! Перемещает файл или каталог
		При попытке перемещения файла, у которого есть версии в корнях, смонтированных в режиме "только для чтения", 
		необходимо создать файл-метку, которая будет свидетельствовать о необходимости игнорирования наличия этого файла
	*/
	virtual bool XMETHODCALLTYPE rename(const char *szPath, const char *szNewPath, XFileSystemWriteHints *pHints = NULL) = 0;

	//! Создает директорию по указанному пути, рекурсивно
	virtual bool XMETHODCALLTYPE createDirectory(const char *szPath, XFileSystemWriteHints *pHints = NULL) = 0;

	/*! Удаляет директорую со всеми вложенными файлами/папками
		При невозможности удаления директории из-за наличия корней, смонтированных в режиме "только для чтения", 
		необходимо создать файл-метку, которая будет свидетельствовать о необходимости игнорирования наличия этой директории
	*/
	virtual bool XMETHODCALLTYPE deleteDirectory(const char *szPath) = 0;
};

#endif
