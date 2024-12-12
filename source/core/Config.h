

/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#ifndef __LOADER_CONFIG_H
#define __LOADER_CONFIG_H

#include "sxcore.h"
#include <gdefines.h>
#include <common/String.h>
#include <common/Array.h>
#include <common/AssotiativeArray.h>
#include <xcommon/IXConfig.h>

class CConfigString: public String
{
public:
	CConfigString() : String(){}
	CConfigString(const char * str) : String(str){}
	CConfigString(const String & str) : String(str){}
	CConfigString(const CConfigString & str) : String(str){}
	CConfigString(const CConfigString * str) : String(str){}
	CConfigString(const char sym) : String(sym){}
	~CConfigString() {}
	
	const char & operator[](const unsigned long & num) const
	{
		return(m_szString[num]);
	}

	char & operator[](const unsigned long & num)
	{
		return(m_szString[num]);
	}

	CConfigString & operator=(const CConfigString & str)
	{
		release();
		m_szString = new char[str.length() + 1];
		memcpy(m_szString, str.c_str(), str.length() + 1);
		return(*this);
	}

	CConfigString & operator=(const CConfigString * str)
	{
		release();
		m_szString = new char[str->length() + 1];
		memcpy(m_szString, str->c_str(), str->length() + 1);
		return(*this);
	}

	bool operator==(const CConfigString &str) const
	{
		return (strcasecmp(str.m_szString, m_szString) == 0);
	}

	bool operator==(const CConfigString * str) const
	{
		return (strcasecmp(str->m_szString, m_szString) == 0);
	}

	CConfigString & operator+=(const char &	sym)
	{
		char * newstring = new char[length() + 2];
		sprintf(newstring, "%s%c", m_szString, sym);
		mem_delete_a(m_szString);
		m_szString = newstring;
		return(*this);
	}
};

class CConfig: public ISXConfig
{
public:
	struct CValue
	{
		CConfigString val;
		bool isModified = false;
	};
	struct CSection
	{
		CConfigString parent;
		AssotiativeArray<CConfigString, CValue> mValues;
		bool native = false;
		CConfigString Include;
		bool isModified = false;
	};
	CConfig(IFileSystem *pFS);
	//SXLoaderConfig(const char* file);
	int open(const char* path);
	void New(const char* path);

	const char* getKey(const char* section, const char* key);
	const char* getKeyName(const char* section, int key);
	const char* getSectionName(int num);

	void set(const char* section, const char* key, const char* val);

	int save();

	int getSectionCount();
	int getKeyCount();
	int getKeyCount(const char* section);

	bool sectionExists(const char* section);
	bool keyExists(const char* section, const char* key);

	void Release();
	void clear();
	void clear2();

	AssotiativeArray<CConfigString, CSection> * getSections();

	//inline const char* GetPath();
	const char* getErrorFile();

protected:
	IFileSystem *m_pFS;

	CConfigString ErrorFile;
	
	struct CInclude
	{
		CConfigString name;
		CConfig *pParser;
	};

	CConfigString BaseFile;
	
//protected:
	AssotiativeArray<CConfigString, CValue> m_mFinalValues;

	AssotiativeArray<CConfigString, CSection> m_mSections;

	Array<CInclude> m_vIncludes;
//public:

	int writeFile(const CConfigString & name, CConfigString section, CConfigString key, const CConfigString & val);
	

	int parse(const char* file);

	int parseInclude(CConfigString & file, const CConfigString & dir);


	CConfigString getIncludeName(int i);

	CConfigString baseDir(CConfigString dir);
	CConfigString baseName(CConfigString dir);

	void modify(AssotiativeArray<CConfigString, CSection> & sections, AssotiativeArray<CConfigString, CValue> & values, CConfigString IncName);
};

class CXConfig: public IXUnknownImplementation<IXConfig>
{
public:
	CXConfig(IFileSystem *pFS);
	~CXConfig();

	bool XMETHODCALLTYPE open(const char *szPath) override;
	bool XMETHODCALLTYPE save() override;
	void XMETHODCALLTYPE clear() override;

	const char* XMETHODCALLTYPE getKey(const char *szSection, const char *szKey) override;
	const char* XMETHODCALLTYPE getKeyName(const char *szSection, UINT uIndex) override;
	const char* XMETHODCALLTYPE getSectionName(UINT uIndex) override;
	void XMETHODCALLTYPE set(const char *szSection, const char *szKey, const char *szValue) override;
	UINT XMETHODCALLTYPE getSectionCount() override;
	UINT XMETHODCALLTYPE getKeyCount() override;
	UINT XMETHODCALLTYPE getKeyCount(const char *szSection) override;
	bool XMETHODCALLTYPE sectionExists(const char *szSection) override;
	bool XMETHODCALLTYPE keyExists(const char *szSection, const char *szKey) override;

	bool XMETHODCALLTYPE tryGetBool(const char *szSection, const char *szKey, bool *pbOut) override;
	bool XMETHODCALLTYPE tryGetInt(const char *szSection, const char *szKey, int *piOut) override;
	bool XMETHODCALLTYPE tryGetUint(const char *szSection, const char *szKey, UINT *puOut) override;
	bool XMETHODCALLTYPE tryGetFloat(const char *szSection, const char *szKey, float *pfOut) override;
	bool XMETHODCALLTYPE tryGetVector2(const char *szSection, const char *szKey, float2_t *pvOut) override;
	bool XMETHODCALLTYPE tryGetVector3(const char *szSection, const char *szKey, float3_t *pvOut) override;
	bool XMETHODCALLTYPE tryGetVector4(const char *szSection, const char *szKey, float4_t *pvOut) override;
	bool XMETHODCALLTYPE tryGetJsonItem(const char *szSection, const char *szKey, IXJSONItem **ppOut) override;
	bool XMETHODCALLTYPE tryGetJsonObject(const char *szSection, const char *szKey, IXJSONObject **ppOut) override;
	bool XMETHODCALLTYPE tryGetJsonArray(const char *szSection, const char *szKey, IXJSONArray **ppOut) override;

	void XMETHODCALLTYPE setBool(const char *szSection, const char *szKey, bool bValue) override;
	void XMETHODCALLTYPE setInt(const char *szSection, const char *szKey, int iValue) override;
	void XMETHODCALLTYPE setUint(const char *szSection, const char *szKey, UINT uValue) override;
	void XMETHODCALLTYPE setFloat(const char *szSection, const char *szKey, float fValue) override;
	void XMETHODCALLTYPE setVector2(const char *szSection, const char *szKey, const float2_t &vValue) override;
	void XMETHODCALLTYPE setVector3(const char *szSection, const char *szKey, const float3_t &vValue) override;
	void XMETHODCALLTYPE setVector4(const char *szSection, const char *szKey, const float4_t &vValue) override;

protected:
	CConfig *m_pConfig = NULL;
};

#endif
