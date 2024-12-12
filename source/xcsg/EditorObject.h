#ifndef __EDITOROBJECT_H
#define __EDITOROBJECT_H

#include <xcommon/editor/IXEditorObject.h>
//#include <common/string.h>
//#include <common/array.h>
#include "BrushMesh.h"
#include "Outline.h"


class CEditable;
class CEditorModel;
//class IXDynamicModel;
class CEditorObject final: public IXUnknownImplementation<IXEditorObject>
{
	DECLARE_CLASS(CEditorObject, IXEditorObject);
public:
	CEditorObject(CEditable *pEditable);
	//CEditorObject(CEditable *pEditable, CBaseEntity *pEntity);
	~CEditorObject();

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
	void setKV(const char *szKey, const char *szValue, bool bSkipFixPos);
	void setKV(const char *szKey, IXJSONItem *pValue, bool bSkipFixPos = false);
	const char* XMETHODCALLTYPE getKV(const char *szKey) override;
	const char* getKV(const char *szKey, bool forJSON);
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

	bool XMETHODCALLTYPE hasVisualModel() override;

	const XGUID* XMETHODCALLTYPE getGUID() override
	{
		return(&m_guid);
	}

	void addBrush(CBrushMesh *pBrushMesh);


	void fixPos();

	bool findFace(const float3 &vRayStart, const float3 &vRayDir, UINT *puFace, float3 *pvFacePoint = NULL);

	bool isVisible() const
	{
		return(m_isVisible);
	}

	UINT getFaceCount() const;

	void renderFace(IXGizmoRenderer *pRenderer, UINT uFace);

	void getFaceInfo(UINT uFace, BrushFace *pOut);
	void setFaceInfo(UINT uFace, const BrushFace &brushFace);

	void getFaceExtents(UINT uFace, Extents extents);

	int classify(const SMPLANE &plane);

	bool clip(const SMPLANE &plane);

	void setModel(CEditorModel *pModel);

	CEditorModel* getModel()
	{
		return(m_pModel);
	}

	void buildMesh(CMeshBuilder *pBuilder);
	void buildPhysbox(IXResourceModel *pResource);

	bool isRemoved()
	{
		return(m_isRemoved);
	}

	UINT getVertexCount();
	const float3_t& getVertexAt(UINT uVertex);

	//! сдвигает выбранные вертексы, возвращает количество удаленных вертексов и их прежние индексы в порядке убывания в puAffectedVertices
	UINT moveVertices(UINT *puAffectedVertices, UINT uVertexCount, const float3 &vDeltaPos);
	
private:
	void removeBrush(UINT idx);

private:
	CEditable *m_pEditable;

	CEditorModel *m_pModel = NULL;

	XGUID m_guid;

	String m_sGUID;

	Array<CBrushMesh*> m_aBrushes;

	bool m_isSelected = false;

	float3_t m_vPos;
	SMQuaternion m_qRot;

	Array<char> m_aSerializedState;

	bool m_isVisible = true;

	bool m_isRemoved = false;

	float3_t m_vColor;
	char m_szColor[48];

	String m_sName;
	String m_sQuotedName;
};

#endif
