#include "FileRecursiveExtPathsIterator.h"

CFileRecursiveExtPathsIterator::CFileRecursiveExtPathsIterator(Array<String> &paths, const char *szBasePath, const char **szExts, int iExtSize)
{
	strcpy(m_szBasePath, szBasePath);
	m_sPaths = std::move(paths);

	canonizePaths(m_sPaths);
	canonizePath(m_szBasePath);

	fillExtensionsArray(m_exts, szExts, iExtSize);
}

const char *CFileRecursiveExtPathsIterator::next()
{
	char szFileName[SIZE_PATH];
	char szFullName[SIZE_PATH];

	WIN32_FIND_DATAW FindFileData;
	HANDLE hf;
	UINT maxPathIndex = m_sPaths.size();

	FindFileData.cFileName[0] = '\0';

	while (pathIndex < maxPathIndex)
	{
		if(strlen(m_szCurrentFullPath) > 0)
		{
			strcpy(m_szCurrentFullPath, m_sPaths[pathIndex].c_str());
		}

		do {
			sprintf(szFileName, "%s*.*", m_sPaths[pathIndex].c_str());

			//Проверяем указатель, если m_handle пустой, то ищем первый файл с расширением szExts
			hf = INVALID_OR_NULL(m_handle) ? FindFirstFileW(CMB2WC(szFileName), &FindFileData) : m_handle;

			if (hf != INVALID_HANDLE_VALUE)
			{
				do {
					//Сохраняем HANDLE файла, что бы можно было продожлить с того места
					m_handle = hf;

					if (emptyOrRepeatPath(CWC2MB(FindFileData.cFileName)))
					{
						continue;
					}

					sprintf(szFullName, "%s%s", m_sPaths[pathIndex].c_str(), (const char*)CWC2MB(FindFileData.cFileName));

					DWORD flag = GetFileAttributesW(CMB2WC(szFullName));

					if (flag != INVALID_FILE_ATTRIBUTES && (flag & FILE_ATTRIBUTE_DIRECTORY))
					{
						strcat(szFullName, "/");
						m_folderList.push_back(szFullName);
						continue;
					}

					if (flag != INVALID_FILE_ATTRIBUTES && !(flag & FILE_ATTRIBUTE_DIRECTORY))
					{
						if (!findExtensionsInPath(CWC2MB(FindFileData.cFileName), m_exts))
						{
							continue;
						}
						//Если это файл - получаем относительный путь и ищем его в списке
						char *pos = strstr(szFullName, m_szBasePath);

						if(pos)
						{
							strcpy(m_szPathStr, pos);
						}

						if(m_mapExistPath.KeyExists(m_szPathStr))
						{
							continue;
						} 
						else
						{
							m_mapExistPath[m_szPathStr] = pathIndex;
							return m_szPathStr;
						}
					}
					//Если указатель на файл валидный, то проверяем все отфильтрованные файлы по порядку
				} while (FindNextFileW(hf, &FindFileData) != 0);

				if (m_folderList.size() != 0)
				{
					UINT index = 0;
					m_sPaths[pathIndex] = m_folderList[index];
					m_folderList.erase(index);	
					m_handle = NULL;
				}
				else
				{
					m_sPaths[pathIndex] = m_szCurrentFullPath;
				}
			}
		} while (m_sPaths[pathIndex] != m_szCurrentFullPath);
		++pathIndex;
		m_szCurrentFullPath[0] = 0;
		m_handle = NULL;
	}

	//Если вообще не нашли файлов возвращаем nullptr
	return nullptr;
}

void CFileRecursiveExtPathsIterator::reset()
{
	if (m_sPaths.size() < pathIndex) 
		m_sPaths[pathIndex] = m_szCurrentFullPath;

	m_szCurrentFullPath[0] = 0;
	m_mapExistPath.clear();
	pathIndex = 0;
	CLOSE_HANDLE(m_handle);
}

CFileRecursiveExtPathsIterator::~CFileRecursiveExtPathsIterator()
{
	CLOSE_HANDLE(m_handle);
}
