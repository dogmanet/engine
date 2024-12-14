#ifndef _COMMAND_PASTE_H_
#define _COMMAND_PASTE_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"

#include <common/assotiativearray.h>
#include <common/string.h>
#include <xcommon/editor/IXEditable.h>
#include "CommandSelect.h"

class CCommandPaste final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	~CCommandPaste();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("paste");
	}

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(m_aObjects.size() == 0);
	}

	UINT addObject(const char *szTypeName, const char *szClassName, const float3 &vPos, const float3 &vScale, const SMQuaternion &qRotate, const XGUID &oldGUID);
	void setKV(UINT uId, const char *szKey, const char *szValue);

	UINT addProxy(const XGUID &guid);
	void addProxyObject(UINT uProxy, const XGUID &guid);

	UINT addGroup(const XGUID &guid);
	void addGroupObject(UINT uGroup, const XGUID &guid);

protected:
	struct _paste_obj
	{
	//	ID idObject;
		IXEditorObject *pObject;
		AssotiativeArray<String, String> mKeyValues;

		float3_t vPos;
		float3_t vScale;
		SMQuaternion qRotate;
	};
	Array<_paste_obj> m_aObjects;

	CCommandSelect *m_pCommandSelect = NULL;

	struct _proxy_obj
	{
		XGUID guid;
		CProxyObject *pProxy;
		Array<XGUID> aObjects;
	};
	Array<_proxy_obj> m_aProxies;

	Map<XGUID, XGUID> m_mapGuids;

	struct _group_obj
	{
		XGUID guid;
		CGroupObject *pGroup;
		Array<XGUID> aObjects;
	};
	Array<_group_obj> m_aGroups;
};

#endif
