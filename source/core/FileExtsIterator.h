/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, Ivan Dokunov, 2021
See the license in LICENSE
***********************************************************/

#ifndef __FILE_EXST_ITERATOR_H
#define __FILE_EXST_ITERATOR_H

#include "BaseFileIterator.h"

class CFileExtsIterator final : public CBaseFileIterator
{
private:
	Array<String> m_paths;
	char m_szPathStr[SIZE_PATH];
	char m_szBasePath[SIZE_PATH];
	Array<AAString> m_exts;
	AssotiativeArray<String, int> m_mapExistPath;

	int index = 0;
	HANDLE m_handle = nullptr;

	//Текущее расширение
	int m_currentExt = 0;

public:
	CFileExtsIterator::CFileExtsIterator(Array<String> &paths, const char *szBasePath, const char **szExt, int iExtSize);

	const char* XMETHODCALLTYPE next() override;

	void XMETHODCALLTYPE reset() override;

	~CFileExtsIterator();
};

#endif