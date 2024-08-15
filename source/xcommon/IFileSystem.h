#ifndef __IFILESYSTEM_H
#define __IFILESYSTEM_H

#include <gdefines.h>
//#include <common/array.h>
#include <common/string.h>
#include <core/IFile.h>

// Интерфейс для работы с файловой системой (в том числе с упакованными архивами в будущем)
// Может производить поиск по нескольким корневым путям

// {38158042-092E-46F1-AD86-EA119E774D1F}
#define IFILESYSTEM_GUID DEFINE_GUID(0x38158042, 0x92e, 0x46f1, 0xad, 0x86, 0xea, 0x11, 0x9e, 0x77, 0x4d, 0x1f)

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

class IFileIterator: public IXUnknown
{
public:
	//! Возврат к первому элементу
	virtual void XMETHODCALLTYPE reset() = 0;
	//! Имя следующего файла, NULL если файлы закончились
	virtual const char* XMETHODCALLTYPE next() = 0;
};

//class IFile;
class IFileSystem: public IXUnknown
{
public:
	/*! Добавить корневой путь (позволяется как абсолютнй путь, так и относительно каталога build)
	 Так же, принимает путь к упакованному архиву с префиксом '@', например "@c:/engine/build/textures.ext" или "@textures.ext" (относительно build)
	*/
	virtual UINT addRoot(const char *szPath, int iPriority = -1) = 0;
	virtual UINT getRootCount() const = 0;
	virtual const char *getRoot(UINT id) const = 0;
	/*! Помечает корневой путь путем для записи. По этому пути будут осуществляться все операции создания файлов
		Путь для записи может быть только один, если явно не задан - используется первый добавленный путь, не являющийся архивом
	*/
	virtual void setWritableRoot(UINT id) = 0;

	//! Находит полный путь в файловой системе для заданного файла
    virtual bool resolvePath(const char *szPath, char *szOut, size_t iOutMax) = 0;

	//! Проверяет наличие файла или каталога по указанному пути
	virtual bool fileExists(const char *szPath) = 0;
	//! Получает размер файла в байтах, либо FILE_NOT_FOUND в случае, если файл не существует, либо не является файлом
	virtual size_t fileGetSize(const char *szPath) = 0;
	//! Проверяет, что путь существует, и является файлом
	virtual bool isFile(const char *szPath) = 0;
	//! Проверяет, что путь существует, и является каталогом
	virtual bool isDirectory(const char *szPath) = 0;
	//! возвращает время последнего изменения файла
	virtual time_t getFileModifyTime(const char *szPath) = 0;
	    
    //! Возвращает список всех папок
    virtual IFileIterator* getFolderList(const char *szPath) = 0;

	/*! возвращает массив со всеми данными находящимися по пути szPath,
	szPath может быть не канонизирован и необязательно должен заканчиваться слэшем,
	szPath уже может содержать фильтр (к примеру C:/*), но если его нет, тогда функция подставит сама
	*/
    virtual IFileIterator* getFileList(const char *szPath, const char *szExt = 0) = 0;

    //! То же, что и предыдущая, только для массива расширений
    virtual IFileIterator *getFileList(const char *szPath, const char **szExts, int extsCount) = 0;

	/*! возвращает список всех файлов или папок (в зависимости от extention),
	szPath не должен содержать фильтров, может быть не канонизирован и без последнего слэша,
	szExt - расширение файла без точки
	*/
	virtual IFileIterator* getFileListRecursive(const char *szPath, const char *szExt = 0) = 0;

	//! То же, что предыдущая, только позволяет использовать массив расширений для поиска. Последний элемент массива NULL
    virtual IFileIterator* getFileListRecursive(const char *szPath, const char **szExts, int extsCount) = 0;

	//! Создает директорию по указанному пути, рекурсивно
	virtual bool createDirectory(const char *szPath) = 0;

    //! Удаляет директорую со всеми вложенными файлами/папками
    virtual bool deleteDirectory(const char *szPath) = 0;

	//! Открыть файл. При открытии с возможностью записи файла, находящегося вне записываемого корня, файл копируется в записывающийся корень и открывается копия.
	virtual IFile* openFile(const char *szPath, FILE_OPEN_MODE mode = FILE_MODE_READ) = 0;
};

class IXMountableFileSystem: public IXUnknown
{
public:
	//! Получить имя файловой системы
	virtual const char* XMETHODCALLTYPE getName() = 0;

	//! Истина, если эта файловая система поддерживает только чтение
	virtual bool XMETHODCALLTYPE isReadOnly() = 0;


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
	virtual IFileIterator* XMETHODCALLTYPE getDirEntries(const char *szPath) = 0;
	
	//! Создает директорию по указанному пути
	virtual bool XMETHODCALLTYPE mkdir(const char *szPath) = 0;
	//! Удаляет пустую директорую
	virtual bool XMETHODCALLTYPE rmdir(const char *szPath) = 0;

	//! Открыть файл.
	virtual IFile* XMETHODCALLTYPE openFile(const char *szPath, FILE_OPEN_MODE mode = FILE_MODE_READ) = 0;
	//! Удаляет файл
	virtual bool XMETHODCALLTYPE unlink(const char *szPath) = 0;
	//! Перемещает файл или каталог
	virtual bool XMETHODCALLTYPE rename(const char *szPath, const char *szNewPath) = 0;
};

struct XFileSystemMountInfo
{
	const char *szMountPoint;
	IXMountableFileSystem *pMFS;
	bool isWritable;
	int iPriority;
};

class IXFileSystem: public IXMountableFileSystem
{
public:
	/*! Смонтировать файловую систему в указанный каталог
		@param szPath путь к точке монтирования
		@param pMFS указатель на объект реализации файловой системы
		@param isWritable монтирование в режиме с поддержкой записи
		@param iPriority приоритет для данной точки монтирования. 
		
		При наличии нескольких смонтированных в одну точку ФС, работают как overlay.
		Просмотр путей идет в порядке возрастания приоритета.
		Запись осуществляется через ФС с наибольшим приоритетом. 
		Если ФС с наибольшим приоритетом смонтирована только для чтения, то запись невозможна.
		При монтировании в существующий непустой каталог, он считается ФС с наименьшим приоритетом в данной точке монтирования.
	*/
	virtual bool XMETHODCALLTYPE mount(const char *szPath, IXMountableFileSystem *pMFS, bool isWritable = false, int iPriority = -1) = 0;
	//! Отключает ФС от точки монтирования
	virtual bool XMETHODCALLTYPE umount(IXMountableFileSystem *pMFS) = 0;

	//! Получить количество монтирований
	virtual UINT XMETHODCALLTYPE getMountsCount() = 0;
	virtual const XFileSystemMountInfo* XMETHODCALLTYPE getMountInfo(UINT uIdx) = 0;

	virtual bool XMETHODCALLTYPE newDefaultMountableFileSystem(const char *szRootPath, IXMountableFileSystem **ppOut) = 0;

	//! Возвращает список всех папок
	virtual IFileIterator* XMETHODCALLTYPE getFolderList(const char *szPath) = 0;

	//! возвращает массив со всеми данными находящимися по пути szPath
	virtual IFileIterator* XMETHODCALLTYPE getFileList(const char *szPath, const char *szExt = 0) = 0;
	//! То же, что и предыдущая, только для массива расширений
	virtual IFileIterator* XMETHODCALLTYPE getFileList(const char *szPath, const char **szExts, int extsCount) = 0;

	/*! возвращает список всех файлов или папок (в зависимости от extention),
		@param szPath не должен содержать фильтров, может быть не канонизирован и без последнего слэша,
		@param szExt - расширение файла без точки
	*/
	virtual IFileIterator* XMETHODCALLTYPE getFileListRecursive(const char *szPath, const char *szExt = 0) = 0;
	//! То же, что предыдущая, только позволяет использовать массив расширений для поиска. Последний элемент массива NULL
	virtual IFileIterator* XMETHODCALLTYPE getFileListRecursive(const char *szPath, const char **szExts, int extsCount) = 0;


	//! Создает директорию по указанному пути, рекурсивно
	virtual bool XMETHODCALLTYPE createDirectory(const char *szPath) = 0;
	//! Удаляет директорую со всеми вложенными файлами/папками
	virtual bool XMETHODCALLTYPE deleteDirectory(const char *szPath) = 0;
};

#endif
