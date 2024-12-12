#ifndef __EDITABLE_H
#define __EDITABLE_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
#include "GameData.h"
#include "EditorExtension.h"

class CEditable final: public IXUnknownImplementation<IXEditable>
{
	friend class CEditorObject;

public:
	CEditable(IXCore *pCore);
	~CEditable();

	XIMPLEMENT_VERSION(IXEDITABLE_VERSION);

	UINT XMETHODCALLTYPE getObjectCount() override;
	IXEditorObject* XMETHODCALLTYPE getObject(UINT id) override;
	IXEditorObject* XMETHODCALLTYPE newObject(const char *szClassName) override;

	const char* XMETHODCALLTYPE getName() override
	{
		return("Entity");
	}
	UINT XMETHODCALLTYPE getClassCount() override
	{
		return(CEntityFactoryMap::GetInstance()->getListCount());
	}
	const char* XMETHODCALLTYPE getClass(UINT id) override
	{
		assert(id < getClassCount());
		return(m_pszClassList[id]);
	}

	void XMETHODCALLTYPE startup(IGXDevice *pDevice) override;
	void XMETHODCALLTYPE shutdown() override;
	
	IXEditorExtension* XMETHODCALLTYPE getEditorExtension() override
	{
		return(m_pEditorExtension);
	}

	void resync();

	void onSelectionChanged(CEditorObject *pObject);

	const Array<CEditorObject*>& getObjects() const
	{
		return(m_aObjects);
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

	bool XMETHODCALLTYPE canUseModel(const char *szClass) override;

	IGXDevice* getDevice()
	{
		return(m_pDevice);
	}

protected:
	IGXDevice *m_pDevice = NULL;
	IXCore *m_pCore = NULL;
	IXMaterialSystem *m_pMaterialSystem = NULL;

	IGXBlendState *m_pBlendColorFactor = NULL;
	IGXRasterizerState *m_pRSWireframe = NULL;
	IXTexture *m_pWhiteTexture = NULL;

	const char **m_pszClassList = NULL;

	Array<CEditorObject*> m_aObjects;

	void removeObject(CEditorObject *pObject);

	CEditorExtension *m_pEditorExtension = NULL;
};

#endif
