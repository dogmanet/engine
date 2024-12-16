#ifndef __GROUPOBJECT_H
#define __GROUPOBJECT_H

#include <xcommon/editor/IXEditorObject.h>
#include <xcommon/editor/IXEditable.h>
#include "ICompoundObject.h"
#include <common/string.h>


// {B92F6791-0F82-4A47-BA46-6319149FEFED}
#define X_IS_GROUP_GUID DEFINE_XGUID(0xb92f6791, 0xf82, 0x4a47, 0xba, 0x46, 0x63, 0x19, 0x14, 0x9f, 0xef, 0xed)


//#############################################################################

class CGroupObject final: public IXUnknownImplementation<ICompoundObject>
{
	DECLARE_CLASS(CGroupObject, IXUnknownImplementation<ICompoundObject>);
public:
	CGroupObject();
	CGroupObject(const XGUID &guid);
	~CGroupObject();

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

	void addChildObject(IXEditorObject *pObject) override;
	void removeChildObject(IXEditorObject *pObject) override;

	UINT getObjectCount() override;
	IXEditorObject* getObject(UINT id) override;

	/*bool isRemoved()
	{
		return(m_isRemoved);
	}
	*/
private:
	XGUID m_guid;

	bool m_isSelected = false;

	bool m_isRemoved = false;

	float3_t m_vPos;
	SMQuaternion m_qOrient;

	struct SrcObject
	{
		IXEditorObject *pObj;
		float3_t vOffset;
		SMQuaternion qOffset;
	};

	Array<SrcObject> m_aObjects;

	String m_sGUID;
	String m_sName;
};

#endif
