/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, Ivan Dokunov, 2021
See the license in LICENSE
***********************************************************/

#ifndef __FILE_RECURSIVE_EXT_PATHS_ITERATOR_H
#define __FILE_RECURSIVE_EXT_PATHS_ITERATOR_H

#include "BaseFileIterator.h"

class CFileRecursiveExtPathsIterator final : public CBaseFileIterator
{
private:
	Array<String> m_folderList;
	Array<String> m_sPaths;
	Array<AAString> m_exts;
	AssotiativeArray<String, int> m_mapExistPath;

	char m_szBasePath[SIZE_PATH];
	char m_szCurrentFullPath[SIZE_PATH];
	char m_szPathStr[SIZE_PATH];
	
    const char *m_szExt;
	UINT pathIndex = 0;

    HANDLE m_handle = nullptr;

public:
	CFileRecursiveExtPathsIterator(Array<String> &paths, const char *szBasePath, const char **szExts, int iExtSize);

    const char* XMETHODCALLTYPE next() override;

    void XMETHODCALLTYPE reset() override;

	~CFileRecursiveExtPathsIterator();
};

#endif