
/*
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017
license MIT see in LICENSE or
https://s4g.su/
*/

/*!
\file
Заголовочный файл препроцессора
*/

/*! \page preprocessor Информация о препроцессоре
Препроцессор удаляет комментарии из исходного кода и обрабатывает директивы, такие как
\b #include <file> \b – Включает код из файла \b file \b производя поиск по путям включений \n
\b #define macro arg \b - обьявляет дефайн с именем macro, который будет заменен на arg в коде программы\n
\b #define macro(arg1, arg2, ..., argN) arg1 arg2 \b - обьявляет макрос с именем macro и параметрами, который будет заменен с учетом параметров в программе\n
\b #if условие \b - Условное включение, если \b условие \b истинно, то следующая часть кода будет включена\n
\b #else \b - Условное включение, выполняется, если условие в \b #if \b ложно\n
\b #endif \b - Заверщает условное включение, идет в паре с \b #if \b \n
\b #ifdef имя \b - Условное включение, если \b имя \b определено, то следующая часть кода будет включена. Работает с \b #else \b и требует \b #endif \b для завершения \n
\b #ifndef имя \b - То же, что \b #ifdef \b, только проверяет НЕ определенность \n

*/

/*! \defgroup pp Препроцессор
@{*/


#ifndef __SHADERPREPROCESSOR_H
#define __SHADERPREPROCESSOR_H

#include <xcommon/IXCore.h>

//! препроцессор
class CShaderPreprocessor final
{
public:
	CShaderPreprocessor(IFileSystem *pFileSystem):
		m_pFileSystem(pFileSystem)
	{
	}

	//! Объявлен ли дефайн?
	bool isDefined(const String &def);
	//! Обьявляет новый дефайн
	void define(const char *def, int len = 0, bool bTemp = false);
	//! Удаляет указанный дефайн
	void undef(const String &def);
	//! Добавляет путь поиска включаемых файлов
	void addIncPath(const String &s);

	//! Запускает препроцессор для кода
	String process(const char *src, const char *file=0);

	UINT getIncludesCount();
	void getIncludes(const char **pszIncludes);

	void clearIncludeCache();
	void reset();

	const char* getError();
	bool isError();

private:
	struct _define
	{
		Array<String> vArgs;
		bool isMacro;
		String sVal;
		bool isUndef;
		bool isTemp;
	};
	struct _include_path
	{
		String sPath;
	};
	struct _include
	{
		String sText;
		String sPath;
		bool isUsed;
	};
	AssotiativeArray<String, _define> m_mDefs;
	Array<_include_path> m_vIncludes;
	AssotiativeArray<String, _include> m_mIncludes;

	String m_sError;
	IFileSystem *m_pFileSystem;

	
	bool isTrue(const String &expr);
	
	//! Уничтожает временные дефайны
	void undefTemp();

	typedef AssotiativeArray<String, bool> ReplList;

	String makeExpansion(const String &expr);
	String makeExpansion(const String &expr, ReplList &rlist);

	String replArgs(const String &tok, const _define *pDef, const Array<String> &szArgs);

	String checkDefines(const String &expr);

	String stringify(const String &expr);

	String getInclude(const String &name, const char *szLocalPath, String *outPath = NULL, bool useLocalPath = false);
};

//!@} pp

#endif
