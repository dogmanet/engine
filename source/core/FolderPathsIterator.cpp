#include "FolderPathsIterator.h"

CFolderPathsIterator::CFolderPathsIterator(Array<String> &paths, const char *szBasePath)
{
	m_paths = std::move(paths);
	strcpy(m_szBasePath, szBasePath);

	canonizePaths(m_paths);
	canonizePath(m_szBasePath);
}

const char *CFolderPathsIterator::next()
{
	WIN32_FIND_DATAW FindFileData;
	HANDLE hf;

	FindFileData.cFileName[0] = '\0';

	UINT size = m_paths.size();

	while(index < size)
	{
		hf = INVALID_OR_NULL(m_handle) ? FindFirstFileW(CMB2WC((m_paths[index] + "*.*").c_str()), &FindFileData) : m_handle;

		if(hf != INVALID_HANDLE_VALUE)
		{
			do {
				m_handle = hf;

				sprintf(m_szPathStr, "%s%s", (m_paths)[index].c_str(), (const char*)CWC2MB(FindFileData.cFileName));

				DWORD flag = GetFileAttributesW(CMB2WC(m_szPathStr));

				if(emptyOrRepeatPath(CWC2MB(FindFileData.cFileName)))
				{
					continue;
				}

				//Берет только имена директорий
				if(flag != INVALID_FILE_ATTRIBUTES && flag & FILE_ATTRIBUTE_DIRECTORY)
				{
					sprintf(m_szPathStr, "%s%s", m_szBasePath, (const char*)CWC2MB(FindFileData.cFileName));

					if(m_mapExistPath.KeyExists(m_szPathStr))
					{
						continue;
					}
					else
					{
						//Возвращаем относительный путь к директории
						m_mapExistPath[m_szPathStr] = index;
						return(m_szPathStr);
					}
				}
			}
			while(FindNextFileW(hf, &FindFileData) != 0);
		}
		FIND_CLOSE(m_handle);
		++index;
	}

	//Если вообще не нашли файлов возвращаем nullptr
	return(NULL);
}

void CFolderPathsIterator::reset()
{
	index = 0;
	FIND_CLOSE(m_handle);
}

CFolderPathsIterator::~CFolderPathsIterator()
{
	FIND_CLOSE(m_handle);
}
