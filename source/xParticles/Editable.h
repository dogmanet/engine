#ifndef __EDITABLE_H
#define __EDITABLE_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
#include "EditorExtension.h"

class CEditorModel;

class CEditable final: public IXUnknownImplementation<IXEditable>
{
	friend class CEditorObject;

public:
	CEditable(IXCore *pCore, IXParticleSystem *pParticleSystem);
	~CEditable();

	XIMPLEMENT_VERSION(IXEDITABLE_VERSION);

	UINT XMETHODCALLTYPE getObjectCount() override
	{
		return(0);
	}
	IXEditorObject* XMETHODCALLTYPE getObject(UINT id) override
	{
		return(NULL);
	}
	IXEditorObject* XMETHODCALLTYPE newObject(const char *szClassName) override
	{
		return(NULL);
	}

	const char* XMETHODCALLTYPE getName() override
	{
		return("");
	}
	UINT XMETHODCALLTYPE getClassCount() override
	{
		return(0);
	}
	const char* XMETHODCALLTYPE getClass(UINT id) override
	{
		assert(id < getClassCount());
		return(NULL);
	}

	void XMETHODCALLTYPE startup(IGXDevice *pDevice) override;
	void XMETHODCALLTYPE shutdown() override;

	IXEditorExtension* XMETHODCALLTYPE getEditorExtension() override
	{
		return(m_pEditorExtension);
	}

	bool XMETHODCALLTYPE canUseModel(const char *szClass) override
	{
		return(false);
	}

	bool XMETHODCALLTYPE canProduceModel() override
	{
		return(false);
	}
	bool XMETHODCALLTYPE buildModelFromSelection(IXEditorModel **ppOut) override
	{
		return(false);
	}
	bool XMETHODCALLTYPE newModel(IXEditorModel **ppOut) override
	{
		return(false);
	}
	UINT XMETHODCALLTYPE getModelCount() override
	{
		return(0);
	}
	bool XMETHODCALLTYPE getModel(UINT id, IXEditorModel **ppOut) override
	{
		return(false);
	}

private:
	CEditorObject* getObjectByGUID(const XGUID &guid);

private:
	IGXDevice *m_pDevice = NULL;
	IXCore *m_pCore = NULL;
	//IXMaterialSystem *m_pMaterialSystem = NULL;

	CEditorExtension *m_pEditorExtension = NULL;
};

#endif
