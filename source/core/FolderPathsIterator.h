/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, Ivan Dokunov, 2019
See the license in LICENSE
***********************************************************/

#ifndef __FOLDER_PATHS_ITERATOR_H
#define __FOLDER_PATHS_ITERATOR_H

#include "BaseFileIterator.h"

class CFolderPathsIterator final : public CBaseFileIterator
{
private:

    Array<String> m_paths;
	char m_szBasePath[SIZE_PATH];
	char m_szPathStr[SIZE_PATH];
	AssotiativeArray<String, int> m_mapExistPath;

    int index = 0;

    HANDLE m_handle = nullptr;

public:
	CFolderPathsIterator(Array<String> &paths, const char *szBasePath);

    const char* XMETHODCALLTYPE next() override;

    void XMETHODCALLTYPE reset() override;

    ~CFolderPathsIterator();
};

#endif
