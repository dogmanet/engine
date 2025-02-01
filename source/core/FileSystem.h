/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, Ivan Dokunov, 2019
See the license in LICENSE
***********************************************************/

#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

#include <xcommon/IFileSystem.h>
//#include <cassert>
//#include <map>

#define CLOSE_HANDLE(handle) if (handle != INVALID_HANDLE_VALUE && handle != nullptr) \
{\
    CloseHandle(handle); \
    handle = nullptr; \
}

#define FIND_CLOSE(handle) if (handle != INVALID_HANDLE_VALUE && handle != nullptr) \
{\
    FindClose(handle); \
    handle = nullptr; \
}

//! Сравнение длинны массива и буфера записи
#define CHECK_SIZE(len, buffSize) \
{ \
    if (len > buffSize) \
    { \
        return false; \
    } \
}

//! Проверка существования пути с таким ID 
#define FILEID_CHECKED(size) if (size - 1 < id) \
{\
    assert(size - 1 < id && "The path ID you entered does not exist"); \
}

#define CHECK_CORRECT_PATH(path) path[0] == '\0'

#define NUM_BITS_IN_BYTE 8

#define MEMCCPY_ERROR(buff) memcpy(buff, "\0", 1);

#define SIZE_PATH 2048

#define INVALID_OR_NULL(handle) ((handle) == NULL || (handle) == INVALID_HANDLE_VALUE)

struct Pair
{
    int priority;
    int pathId;
};

class CFileSystem final: public IXUnknownImplementation<IFileSystem>
{
public:
    CFileSystem();

	UINT addRoot(const char *szPath, int iPriority = -1) override;

	UINT getRootCount() const override;

	const char *getRoot(UINT id) const override;

    void setWritableRoot(UINT id) override;

    bool resolvePath(const char *szPath, char *szOut, size_t iOutMax) override;

    bool fileExists(const char *szPath) override;

    size_t fileGetSize(const char *szPath) override;

    bool isFile(const char *szPath) override;

    bool isDirectory(const char *szPath) override;

    //! Данная функция возвращает время в UTC (Coordinated Universal Time)
    time_t getFileModifyTime(const char *szPath) override;

    IFileIterator *getFolderList(const char *szPath) override;

    IFileIterator *getFileList(const char *szPath, const char *szExt = 0) override;

    IFileIterator *getFileList(const char *szPath, const char **szExts, int extsCount) override;

    //! No implementation
     IFileIterator *getFileListRecursive(const char *szPath,  const char *szExt = 0)  override;
    
    //! No implementation
     IFileIterator *getFileListRecursive(const char *szPath, const char **szExts, int extsCount) override;

     bool createDirectory(const char *szPath) override;

     bool deleteDirectory(const char *szPath) override;

     IFile *openFile(const char *szPath, FILE_OPEN_MODE) override;

private:
	template <typename T>
	IFileIterator *getListIterator(const char *szPath, const char **szExts, int extsCount);

	void addPathInPriorityArray(int id, int iPriority);

	//! Метод делает проверку, ведет ли путь к файлу или папке
	bool isFileOrDirectory(const char *szPath, bool isFile);

	void getAllvariantsCanonizePath(const char *szPath, Array<String> &container);

	//!Превращает канонизированный путь в неканонизированный
	void getNormalPath(const char *szPath, char *outBuff, int iOutMax);

	//!Сравнивает пути с корнем, и возвращает true если путь - абсолютный и в корне
	bool isAbsolutePathInRoot(const char *szPath);

	//! Возвращает абсолютный канонизированный путь
	void getAbsoluteCanonizePath(const char *szPath, char *outPath, int iOutMax);

	void getFullPathToBuild(char *buff, int iSize);

	void getFileName(const char *name, char *outName, int iOutBuff);

	//! Вспомогательная функция для конвертирования FILETIME в time_t
	time_t filetimeToTime_t(const FILETIME& ft);

	HANDLE getFileHandle(const char *szPath);

	bool isAbsolutePath(const char* szPath);

	void copyFile(const char* szPath, char *szOut);

	//!корневые пути и приоритет
	Array<String> m_filePaths;

	//!Полный путь к build
	char m_pathToBuild[SIZE_PATH];

	Array<Pair> m_priorityArray;

	//!Наш текущий ID корневого пути для записи
	//! -1 - значит не установлен
	int m_writableRoot = -1;
};

#endif
