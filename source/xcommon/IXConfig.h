#ifndef __IXCONFIGLOADER_H
#define __IXCONFIGLOADER_H

#include <gdefines.h>
#include "IXJSON.h"

/*! интерфейс для работы с файлами конфигурации (ini)
\warning секции и ключи хранятся в виде дерева, и нет гарантии что может быть доступ по порядковому номеру,
можно получить общее количество секций/ключей, дальше плясать */
class IXConfig: public IXUnknown
{
public:
	// virtual void XMETHODCALLTYPE create(const char *szPath) = 0; //!< новый файл
	virtual bool XMETHODCALLTYPE open(const char *szPath) = 0; //!< открыть файл
	virtual bool XMETHODCALLTYPE save() = 0; //!< сохранить файл

	virtual const char* XMETHODCALLTYPE getKey(const char *szSection, const char *szKey) = 0; //!< получить значения ключа key который в секции section
	virtual const char* XMETHODCALLTYPE getKeyName(const char *szSection, UINT uIndex) = 0; //!< получить имя ключа по номеру
	virtual const char* XMETHODCALLTYPE getSectionName(UINT uIndex) = 0; //!< получить имя секции по номеру
	virtual void XMETHODCALLTYPE set(const char *szSection, const char *szKey, const char *szValue) = 0; //!< установить значение val ключа key котор в секции section
	virtual UINT XMETHODCALLTYPE getSectionCount() = 0; //!< количество секций в файле
	virtual UINT XMETHODCALLTYPE getKeyCount() = 0; //!< общее количество ключей
	virtual UINT XMETHODCALLTYPE getKeyCount(const char *szSection) = 0; //!< общее количество ключей в секции
	virtual bool XMETHODCALLTYPE sectionExists(const char *szSection) = 0; //!< существует ли секция section
	virtual bool XMETHODCALLTYPE keyExists(const char *szSection, const char *szKey) = 0; //!< существует ли ключ key в секции section

	virtual void XMETHODCALLTYPE clear() = 0; //!< очистить

	virtual bool XMETHODCALLTYPE tryGetBool(const char *szSection, const char *szKey, bool *pbOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetInt(const char *szSection, const char *szKey, int *piOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetUint(const char *szSection, const char *szKey, UINT *puOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetFloat(const char *szSection, const char *szKey, float *pfOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetVector2(const char *szSection, const char *szKey, float2_t *pvOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetVector3(const char *szSection, const char *szKey, float3_t *pvOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetVector4(const char *szSection, const char *szKey, float4_t *pvOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetJsonItem(const char *szSection, const char *szKey, IXJSONItem **ppOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetJsonObject(const char *szSection, const char *szKey, IXJSONObject **ppOut) = 0;
	virtual bool XMETHODCALLTYPE tryGetJsonArray(const char *szSection, const char *szKey, IXJSONArray **ppOut) = 0;

	virtual void XMETHODCALLTYPE setBool(const char *szSection, const char *szKey, bool bValue) = 0;
	virtual void XMETHODCALLTYPE setInt(const char *szSection, const char *szKey, int iValue) = 0;
	virtual void XMETHODCALLTYPE setUint(const char *szSection, const char *szKey, UINT uValue) = 0;
	virtual void XMETHODCALLTYPE setFloat(const char *szSection, const char *szKey, float fValue) = 0;
	virtual void XMETHODCALLTYPE setVector2(const char *szSection, const char *szKey, const float2_t &vValue) = 0;
	virtual void XMETHODCALLTYPE setVector3(const char *szSection, const char *szKey, const float3_t &vValue) = 0;
	virtual void XMETHODCALLTYPE setVector4(const char *szSection, const char *szKey, const float4_t &vValue) = 0;
};

#endif
