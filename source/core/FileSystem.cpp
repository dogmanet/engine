#include "FileSystem.h"
#include "FileExtsIterator.h"
#include "FolderPathsIterator.h"
#include "FileRecursiveExtPathsIterator.h"
#include "File.h"
#include <shellapi.h>
#include <ShlObj.h>

template <typename T>
IFileIterator *CFileSystem::getListIterator(const char *szPath, const char **szExts, int extsCount)
{
	Array<String> paths;
	String basePath(szPath);

	if (!isAbsolutePath(szPath))
	{
		getAllvariantsCanonizePath(szPath, paths);
	}
	else
	{
		paths.push_back(szPath);
	}

	return paths.size() ? new T(paths, basePath, szExts, extsCount) : nullptr;
}

void CFileSystem::addPathInPriorityArray(int id, int iPriority)
{
	Pair newElement{ iPriority, id };

	//Если приоритет по умолчанию и нет элементов - задаем значение 0 (потому что первый)
	//Если элементов больше чем 0 то тогда ставим самый большой приоритет из возможных
	if (iPriority == -1)
	{
		UINT size = m_priorityArray.size();
		newElement.priority = size > 0 ? m_priorityArray[0].priority + 1 : 0;
	}

	m_priorityArray.insert(newElement, [](const Pair &obj, const Pair &obj2) -> bool {return obj.priority <= obj2.priority; });
}

bool CFileSystem::isFileOrDirectory(const char *szPath, bool isFile)
{
    char path[SIZE_PATH];
    getAbsoluteCanonizePath(szPath, path, SIZE_PATH);

    if (CHECK_CORRECT_PATH(path))
    {
        return false;
    }

    DWORD flag = GetFileAttributesW(CMB2WC(path));

    //Проверка на то куда имено ведет путь - к файлу или папке
    return (flag != INVALID_FILE_ATTRIBUTES) && (isFile ? !(flag & FILE_ATTRIBUTE_DIRECTORY) : (flag & FILE_ATTRIBUTE_DIRECTORY));
}

void CFileSystem::getAllvariantsCanonizePath(const char *szPath, Array<String> &container)
{
	for (int i = 0, I = m_filePaths.size(); i < I; ++i)
	{
		String buff = m_filePaths[i];
		buff += '/';
		buff += szPath;
		buff += '/'; // <- оптимизация buffObj

		if (isDirectory(buff.c_str()))
		{
			container.push_back(buff);
		}
	}
}

void CFileSystem::getNormalPath(const char *szPath, char *outBuff, int iOutMax)
{
	if(szPath != outBuff)
	{
    size_t len = strlen(szPath) + 1;

		if(iOutMax < len)
    {
        MEMCCPY_ERROR(outBuff);
        return;
    }

    memcpy(outBuff, szPath, len);
	}

    do
    {
        *outBuff = *outBuff == '/' ? '\\' : *outBuff;
    } while (*outBuff++ != '\0');
}

bool CFileSystem::isAbsolutePathInRoot(const char *szPath)
{
    if (!isAbsolutePath(szPath))
    {
        return false;
    }

    char rootPath[SIZE_PATH];
    getFullPathToBuild(rootPath, SIZE_PATH);
    const char *pos = strstr(szPath, rootPath);

    return pos != nullptr;
}

void CFileSystem::getAbsoluteCanonizePath(const char *szPath, char *outPath, int iOutMax)
{
    bool absolute = isAbsolutePath(szPath);
    bool correctPath = true;

    size_t len = absolute ? strlen(szPath) + 1 : SIZE_PATH;

    if (absolute) 
    {
        memcpy(outPath, szPath, len);
    }
    else
    { 
        correctPath = resolvePath(szPath, outPath, len);
    }

    //Во время поиска пути могут произойти ошибки - путь может быть не найден, или слишком маленький буфер для записи
    if (correctPath)
    {
        //Если все корректно прошло, то путь можно канонизировать
        canonize_path(outPath);
    }
    else
    {
        //Если что то пошло не так записываем '\0' в память, потом это значение можно проверить
        MEMCCPY_ERROR(outPath);
    }
}

void CFileSystem::getFullPathToBuild(char *buff, int iSize)
{
    GetModuleFileName(nullptr, buff, iSize);
    dirname(buff);
    dirname(buff);
    canonize_path(buff);
}

void CFileSystem::getFileName(const char *name, char *outName, int iOutBuff)
{
    WIN32_FIND_DATAW wfd;
	HANDLE hFind = FindFirstFileW(CMB2WC(name), &wfd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        FIND_CLOSE(hFind);

        //Если размера буфера хватает - то записываем имя файла, если нет то записываем в [0] '\0'
		iOutBuff > MAX_PATH ? memcpy(outName, CWC2MB(wfd.cFileName), MAX_PATH) : MEMCCPY_ERROR(outName);
    }
}

time_t CFileSystem::filetimeToTime_t(const FILETIME& ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;

    return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

HANDLE CFileSystem::getFileHandle(const char *szPath)
{
	return CreateFileW(CMB2WC(szPath),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
}

bool CFileSystem::isAbsolutePath(const char *szPath)
{
    while (*szPath != '\0')
    {
        //Для корректности нужна проверка на разные слеши, ведь на вход может прийти путь не с /
        if (*szPath == ':' && (*(szPath + 1) == '/' || *(szPath + 1) == '\\'))
        {
            return true;
        }
        ++szPath;
    }
    return false;
}

String *CFileSystem::copyFile(const char* szPath)
{
    createDirectory(m_filePaths[m_writableRoot].c_str());

    char fn[MAX_PATH];
    getFileName(szPath, fn, MAX_PATH);
    String *newFilePath = new String(m_filePaths[m_writableRoot] + '/' + fn);
	CopyFileW(CMB2WC(szPath), CMB2WC(newFilePath->c_str()), false);

    return newFilePath;
}

CFileSystem::CFileSystem()
{
    getFullPathToBuild(m_pathToBuild, SIZE_PATH);
}

UINT CFileSystem::addRoot(const char *szPath, int iPriority)
{
    String str;

    //Если путь не абсолютный - то прибавляем к нему часть пути к папке build
    if (!isAbsolutePath(szPath))
    {
        str += m_pathToBuild;
    }

    str += szPath; // <--- Оптимизация для того что бы не создавать временных объектов

    m_filePaths.push_back(str);
    addPathInPriorityArray(m_filePaths.size() - 1, iPriority);

    //Если у нас некорректный путь для записи и путь не является архивным
    if (m_writableRoot == -1 && *szPath != '@')
    {
        m_writableRoot = m_filePaths.size() - 1;
    }

    return m_filePaths.size() - 1;
}

UINT CFileSystem::getRootCount() const 
{
    return m_filePaths.size();
}

const char *CFileSystem::getRoot(UINT id) const
{
    FILEID_CHECKED(m_filePaths.size());

    return m_filePaths[id].c_str();
}

void CFileSystem::setWritableRoot(UINT id)
{
    FILEID_CHECKED(m_filePaths.size());

    m_writableRoot = id;
}

bool CFileSystem::resolvePath(const char *szPath, char *szOut, size_t iOutMax)
{
    size_t len = 0;

    if (isAbsolutePath(szPath))
    {
        len = strlen(szPath) + 1;

        CHECK_SIZE(len, iOutMax);

        memcpy(szOut, szPath, len);
        return true;
    }
    
    String buff;

    for (UINT i = 0, l = m_priorityArray.size(); i < l; ++i)
    {
        int id = m_priorityArray[i].pathId;
        buff = (m_filePaths[id] + '/' + szPath);

        if (fileExists(buff.c_str())/* && isFile(buff.c_str())*/)
        {
            CHECK_SIZE(len, iOutMax);

            len = buff.length() + 1;
            memcpy(szOut, buff.c_str(), len);
            return true;
        }
    }

    return false;
}

bool CFileSystem::fileExists(const char *szPath)
{
    char path[SIZE_PATH];
    getAbsoluteCanonizePath(szPath, path, SIZE_PATH);

    if (CHECK_CORRECT_PATH(path))
    {
        //Если не удалось найти полный путь - на выход
        return false;
    }

    return fileGetSize(path) != FILE_NOT_FOUND;
}

size_t CFileSystem::fileGetSize(const char *szPath)
{
    char path[SIZE_PATH];
    getAbsoluteCanonizePath(szPath, path, SIZE_PATH);

    if (CHECK_CORRECT_PATH(path))
    {
        return FILE_NOT_FOUND;
    }

	WIN32_FILE_ATTRIBUTE_DATA lpFileInformation;

	int result = GetFileAttributesExW(CMB2WC(szPath), GetFileExInfoStandard, &lpFileInformation);

	//Преобразование размера из старших и младших бит
	ULONGLONG FileSize = (static_cast<ULONGLONG>(lpFileInformation.nFileSizeHigh) <<
        sizeof(lpFileInformation.nFileSizeLow) * NUM_BITS_IN_BYTE) |
		lpFileInformation.nFileSizeLow;

	//Если result != 0 то все хорошо, если 0 то файл не найден
	return result != 0 ? FileSize : FILE_NOT_FOUND;
}

bool CFileSystem::isFile(const char *szPath)
{
    return isFileOrDirectory(szPath, true);
}

bool CFileSystem::isDirectory(const char *szPath)
{
    return isFileOrDirectory(szPath, false);
}

time_t CFileSystem::getFileModifyTime(const char *szPath)
{
    char path[SIZE_PATH];
    getAbsoluteCanonizePath(szPath, path, SIZE_PATH);

    if (CHECK_CORRECT_PATH(path))
    {
        return 0;
    }

    WIN32_FILE_ATTRIBUTE_DATA lpFileInformation;

	GetFileAttributesExW(CMB2WC(path), GetFileExInfoStandard, &lpFileInformation);

	return filetimeToTime_t(lpFileInformation.ftLastWriteTime);
}

IFileIterator *CFileSystem::getFolderList(const char *szPath)
{
	Array<String> paths;
	String basePath(szPath);

	if (!isAbsolutePath(szPath)) 
	{
		getAllvariantsCanonizePath(szPath, paths);
	} 
	else
	{
		paths.push_back(szPath);
	}

	return paths.size() ? new CFolderPathsIterator(paths, basePath) : nullptr;
}

IFileIterator *CFileSystem::getFileList(const char *szPath, const char *szExt)
{
	const char *exts[] = { szExt };
	return getFileList(szPath, exts, 1);
}

IFileIterator *CFileSystem::getFileList(const char *szPath, const char **szExts, int extsCount)
{
	return getListIterator<CFileExtsIterator>(szPath, szExts, extsCount);
}

IFileIterator *CFileSystem::getFileListRecursive(const char *szPath, const char *szExt)
{
	const char *exts[] = { szExt };
	return getFileListRecursive(szPath, exts, 1);
}

IFileIterator *CFileSystem::getFileListRecursive(const char *szPath, const char **szExts, int extsCount)
{
	return getListIterator<CFileRecursiveExtPathsIterator>(szPath, szExts, extsCount);
}

bool CFileSystem::createDirectory(const char *szPath)
{
    char path[SIZE_PATH];
    getNormalPath(szPath, path, SIZE_PATH);
    if(!isAbsolutePath(path))
    {
        char szBuf[MAX_PATH];
        szBuf[0] = 0;
        strcat(szBuf, m_filePaths[m_writableRoot].c_str());
        strcat(szBuf, "/");
        strcat(szBuf, szPath);
        char *tmp = szBuf;
        while(*tmp)
        {
            if(*tmp == '/')
            {
                *tmp = '\\';
            }
            ++tmp;
        }
        return(SHCreateDirectoryExW(nullptr, CMB2WC(szBuf), nullptr) == NO_ERROR);
    }

	return(SHCreateDirectoryExW(nullptr, CMB2WC(path), nullptr) == NO_ERROR);
}

bool CFileSystem::deleteDirectory(const char *szPath)
{
    char path[SIZE_PATH];
	getAbsoluteCanonizePath(szPath, path, SIZE_PATH);
	getNormalPath(path, path, SIZE_PATH);

    CMB2WC wszPath(path);
    size_t len = wcslen(wszPath);
    wchar_t *pBuf = (wchar_t*)alloca(sizeof(wchar_t) * (len + 2));
    memcpy(pBuf, wszPath, sizeof(wchar_t) * (len + 1));
    pBuf[len + 1] = 0;

    SHFILEOPSTRUCTW file_op = {
        NULL,
        FO_DELETE,
        pBuf,
        L"",
        FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
        false,
        0,
        L"" 
    };

    // Если вернуло не 0, то все плохо
    return(SHFileOperationW(&file_op) == NO_ERROR);
}

IFile *CFileSystem::openFile(const char *szPath, FILE_OPEN_MODE mode = FILE_MODE_READ)
{
    //Выходим если режим открытия - не для чтения и нет пути для записи
    if (m_writableRoot == -1 && mode != FILE_MODE_READ)
    {
        return nullptr;
    }

    //Зарезервированная строка, если вдруг не хватит SIZE_PATH
    char reserveStr[SIZE_PATH * 2];
    bool useReserveStr = false;

    char fullPath[SIZE_PATH];
    getAbsoluteCanonizePath(szPath, fullPath, SIZE_PATH);

    //Если по каким либо причинам нельзя вернуть полный путь - на выход
    if (CHECK_CORRECT_PATH(fullPath) && mode == FILE_MODE_READ)
    {
        return nullptr;
    }

    IFile *file = new CFile;

    if (mode == FILE_MODE_READ)
    {
        //Если открываем только на чтение - то копирование не нужно (следовательно и выделение памяти тоже лишняя операция)
        if (file->open(fullPath, CORE_FILE_BIN) != 0)
        {
            mem_delete(file);
        }
        return file;
    }

    String *newFileName;

    if (CHECK_CORRECT_PATH(fullPath))
    {
        newFileName = new String(m_filePaths[m_writableRoot].c_str());
        *newFileName += '/';
        *newFileName += szPath;

        getAbsoluteCanonizePath(newFileName->c_str(), fullPath, SIZE_PATH);

        mem_delete(newFileName);
    }

    bool inRoot = isAbsolutePathInRoot(fullPath);

    //Если путь в корне, и файла не существует - создаем его
    if (inRoot && !fileExists(fullPath))
    {
        size_t len = strlen(fullPath) + 1;
        char dirName[SIZE_PATH];

        memcpy(dirName, fullPath, len);
        dirname(dirName);
        len = strlen(dirName);
        dirName[len - 1] = '\0';
        createDirectory(dirName);
    }
    //Если путь не в корне и его не существует - на выход
    else if (!fileExists(fullPath))
    {
        mem_delete(file);
        return nullptr;
    }
    //Если путь вне корня - тогда копируем в корень
    else if (!inRoot)
    {
        newFileName = copyFile(fullPath);
        size_t lenPath = strlen(fullPath) + 1;
        
        if (lenPath < newFileName->length())
        {
            memcpy(reserveStr, newFileName->c_str(), newFileName->length() + 1);
            useReserveStr = true;
        }

        memcpy(fullPath, newFileName->c_str(), newFileName->length() + 1);
        mem_delete(newFileName);
    }

    int res = 0;

    //Определяем использован ли дополнительный путь для записи в файловой системе
    char* correctPath = useReserveStr ? reserveStr : fullPath;

    switch (mode)
    {
    case FILE_MODE_WRITE:
        res = file->create(correctPath, CORE_FILE_BIN);
        break;

    case FILE_MODE_APPEND:
        res = file->add(correctPath, CORE_FILE_BIN);
        break;
    }

    if (res != 0)
    {
        mem_delete(file);
    }

    return file;
}