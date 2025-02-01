#ifndef __CBASE_FILE_ITERATOR_
#define __CBASE_FILE_ITERATOR_

#include "FileSystem.h"
#include <common/AAString.h>

class CBaseFileIterator : public IXUnknownImplementation<IFileIterator>
{
public:
	void canonizePath(char *szPath);

	void canonizePath(String *pPath);

	void canonizePaths(Array<String> &paths);

	void fillExtensionsArray(Array<AAString> &extsArray, const char **exts, int iExtsSize);

	bool findExtensionsInPath(const char *szPath, const Array<AAString> &exts);

	bool emptyOrRepeatPath(const char *szPath);
};

#endif 
