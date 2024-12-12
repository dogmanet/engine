#ifndef __PROXYOBJECT_H
#define __PROXYOBJECT_H

#include <xcommon/editor/IXEditorObject.h>
#include <xcommon/editor/IXEditable.h>
#include "ICompoundObject.h"


// {48989C9B-A44C-469D-A86F-D8F61AD3114C}
#define X_IS_PROXY_GUID DEFINE_XGUID(0x48989c9b, 0xa44c, 0x469d, 0xa8, 0x6f, 0xd8, 0xf6, 0x1a, 0xd3, 0x11, 0x4c)

class CProxyObject;
class CModelCallback: public IXEditorModelCallback
{
public:
	CModelCallback(CProxyObject *pObject):
		m_pObject(pObject)
	{}

	void XMETHODCALLTYPE onChanged(IXEditorModel *pModel, IXEditorObject *pObject) override;
	void XMETHODCALLTYPE onObjectAdded(IXEditorModel *pModel, IXEditorObject *pObject) override;
	void XMETHODCALLTYPE onObjectRemoved(IXEditorModel *pModel, IXEditorObject *pObject) override;
	//void XMETHODCALLTYPE onDestroyed(IXEditorModel *pModel) override;

private:
	CProxyObject *m_pObject;
};

//#############################################################################

class CProxyObject final: public IXUnknownImplementation<ICompoundObject>
{
	DECLARE_CLASS(CProxyObject, IXUnknownImplementation<ICompoundObject>);
public:
	CProxyObject();
	CProxyObject(const XGUID &guid);
	~CProxyObject();

	void XMETHODCALLTYPE setPos(const float3_t &pos) override;
	void XMETHODCALLTYPE setOrient(const SMQuaternion &orient) override;
	void XMETHODCALLTYPE setSize(const float3_t &vSize) override;

	void XMETHODCALLTYPE getBound(float3 *pvMin, float3 *pvMax) override;

	void XMETHODCALLTYPE render(bool is3D, bool bRenderSelection, IXGizmoRenderer *pGizmoRenderer) override;

	bool XMETHODCALLTYPE rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, ID *pidMtrl = NULL, bool bReturnNearestPoint = false) override;

	void XMETHODCALLTYPE remove() override;
	void XMETHODCALLTYPE create() override;
	void XMETHODCALLTYPE preSetup() override;
	void XMETHODCALLTYPE postSetup() override;

	void XMETHODCALLTYPE setKV(const char *szKey, const char *szValue) override;
	const char* XMETHODCALLTYPE getKV(const char *szKey) override;
	const X_PROP_FIELD* XMETHODCALLTYPE getPropertyMeta(UINT uKey) override;
	UINT XMETHODCALLTYPE getProperyCount() override;

	const char* XMETHODCALLTYPE getTypeName() override;
	const char* XMETHODCALLTYPE getClassName() override;

	float3_t XMETHODCALLTYPE getPos() override;

	SMQuaternion XMETHODCALLTYPE getOrient() override;

	bool XMETHODCALLTYPE isSelected() override
	{
		return(m_isSelected);
	}
	void XMETHODCALLTYPE setSelected(bool set) override;

	IXTexture* XMETHODCALLTYPE getIcon() override
	{
		return(NULL);
	}

	void XMETHODCALLTYPE setSimulationMode(bool set) override;

	bool XMETHODCALLTYPE hasVisualModel() override
	{
		return(true);
	}

	const XGUID* XMETHODCALLTYPE getGUID() override
	{
		return(&m_guid);
	}

	void XMETHODCALLTYPE getInternalData(const XGUID *pGUID, void **ppOut) override;

	bool setDstObject(const XGUID &guid);
	void addSrcModel(const XGUID &guid);
	void build();
	void reset();

	void onModelChanged(IXEditorModel *pModel, IXEditorObject *pObject);

	void addChildObject(IXEditorObject *pObject) override;
	void removeChildObject(IXEditorObject *pObject) override;

	IXEditorObject* getTargetObject();
	UINT getModelCount();
	IXEditorModel* getModel(UINT id);

	UINT getObjectCount() override;
	IXEditorObject* getObject(UINT id) override;

	void saveModel();

	bool isRemoved()
	{
		return(m_isRemoved);
	}

private:
	XGUID m_guid;

	bool m_isSelected = false;

	bool m_isRemoved = false;

	float3_t m_vPos;
	SMQuaternion m_qOrient;

	IXEditorObject *m_pTargetObject = NULL;

	IXResourceModel *m_pResource = NULL;

	struct SrcModel
	{
		IXEditorModel *pModel;
	};

	struct SrcObject
	{
		IXEditorObject *pObj;
		//CProxyObject *pFrom;
		float3_t vOffset;
		SMQuaternion qOffset;
	};

	Array<SrcModel> m_aModels;
	Array<SrcObject> m_aObjects;

	CModelCallback m_modelCallback;

	bool m_bSkipCallback = false;
};

#endif
