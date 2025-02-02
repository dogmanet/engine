#include "BaseFileIterator.h"

void CBaseFileIterator::canonizePath(char *szPath)
{
	char symbol = szPath[strlen(szPath) - 1];

	/*Дело в том что абсолютный путь к файлу может не иметь символ "/"
	или "\\" на конце строки, и, если его не будет путь будет некорректен*/
	if(symbol != '\\' && symbol != '/')
	{
		strcat(szPath, "/");
	}
}

void CBaseFileIterator::canonizePath(String *pPath)
{
	char symbol = (*pPath)[pPath->length() - 1];

	/*Дело в том что абсолютный путь к файлу может не иметь символ "/"
	или "\\" на конце строки, и, если его не будет путь будет некорректен*/
	if(symbol != '\\' && symbol != '/')
	{
		(*pPath) += '/';
	}
}

void CBaseFileIterator::canonizePaths(Array<String> &paths)
{
	for(int i = 0, I = paths.size(); i < I; ++i)
	{
		canonizePath(&paths[i]);
	}
}

void CBaseFileIterator::fillExtensionsArray(Array<AAString> &extsArray, const char **exts, int iExtsSize)
{
	for(int i = 0; i < iExtsSize; ++i)
	{
		if(exts[i])
		{
			extsArray[i].setName(exts[i]);
		}
	}
}

bool CBaseFileIterator::findExtensionsInPath(const char *szPath, const Array<AAString> &exts)
{
	size_t len = strlen(szPath), extLen = 0;
	char szExt[32];

	for(int i = 0, I = exts.size(); i < I; ++i)
	{
		extLen = strlen(exts[i].getName()) + 1;
		sprintf(szExt, ".%s", exts[i].getName());
		if(!strcmp(szPath + len - extLen, szExt))
		{
			return(true);
		}
	}
	return(!exts.size());
}

bool CBaseFileIterator::emptyOrRepeatPath(const char * szPath)
{
	return(!strcmp(szPath, "..") || !strcmp(szPath, ".") || !strcmp(szPath, ""));
}