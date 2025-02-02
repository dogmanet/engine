#include "FileExtsIterator.h"

CFileExtsIterator::CFileExtsIterator(Array<String> &paths, const char *szBasePath, const char **szExt, int iExtSize)
{
	m_paths = std::move(paths);
	strcpy(m_szBasePath, szBasePath);

	canonizePaths(m_paths);
	canonizePath(m_szBasePath);
	fillExtensionsArray(m_exts, szExt, iExtSize);
}

const char *CFileExtsIterator::next()
{
	WIN32_FIND_DATAW FindFileData;
	HANDLE hf;

	FindFileData.cFileName[0] = '\0';

	int size = m_paths.size();
	int sizeExt = m_exts.size();

	char szFileName[SIZE_PATH];

	while(index < size)
	{
		sprintf(szFileName, "%s*.*", m_paths[index].c_str());

		//Проверяем указатель, если m_handle пустой, то ищем первый файл с расширением szExts
		hf = INVALID_OR_NULL(m_handle) ? FindFirstFileW(CMB2WC(szFileName), &FindFileData) : m_handle;

		if(hf != INVALID_HANDLE_VALUE)
		{
			do {
				//Сохраняем HANDLE файла, что бы можно было продожлить с того места
				m_handle = hf;

				sprintf(m_szPathStr, "%s%s", m_paths[index].c_str(), (const char*)CWC2MB(FindFileData.cFileName));

				DWORD flag = GetFileAttributesW(CMB2WC(m_szPathStr));

				if(flag != INVALID_FILE_ATTRIBUTES && !(flag & FILE_ATTRIBUTE_DIRECTORY))
				{
					if(!findExtensionsInPath(CWC2MB(FindFileData.cFileName), m_exts))
					{
						continue;
					}

					sprintf(m_szPathStr, "%s%s", m_szBasePath, (const char*)CWC2MB(FindFileData.cFileName));

					if(m_mapExistPath.KeyExists(m_szPathStr))
					{
						continue;
					}
					else
					{
						//Возвращаем относительный путь, вместе с именем файла и расширением
						m_mapExistPath[m_szPathStr] = index;
						return(m_szPathStr);
					}
				}
				//Если указатель на файл валидный, то проверяем все отфильтрованные файлы по порядку
			}
			while(FindNextFileW(hf, &FindFileData) != 0);
		}
		++index;
		FIND_CLOSE(m_handle);
	}

	//Если вообще не нашли файлов возвращаем nullptr
	return(NULL);
}

void CFileExtsIterator::reset()
{
	index = 0;
	FIND_CLOSE(m_handle);
}

CFileExtsIterator::~CFileExtsIterator()
{
	FIND_CLOSE(m_handle);
}
