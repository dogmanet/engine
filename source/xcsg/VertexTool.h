#ifndef __VERTEX_TOOL_H
#define __VERTEX_TOOL_H

#include <xcommon/editor/IXEditable.h>
#include <mtrl/IXMaterialSystem.h>
#include <xcommon/IXCore.h>
//#include "Outline.h"
//#include "BrushMesh.h"
//#include "FaceEdit.h"

class CEditable;
class CVertexTool;
class CCommandMorph;
//class CEditorObject;

class CVertexGizmoCallback: public IXEditorGizmoHandleCallback
{
public:
	CVertexGizmoCallback(CVertexTool *pTool):
		m_pTool(pTool)
	{
	}
	void XMETHODCALLTYPE moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onStart(IXEditorGizmoHandle *pGizmo) override;
	void XMETHODCALLTYPE onEnd(IXEditorGizmoHandle *pGizmo) override;
	
private:
	CVertexTool *m_pTool;
	bool m_isMoved = false;
	bool m_isFirstMov = false;
};

//##########################################################################

class CVertexTool final: public IXUnknownImplementation<IXEditorTool>
{
public:
	CVertexTool(CEditable *pEditable, IXEditor *pEditor);
	~CVertexTool();

	void* XMETHODCALLTYPE getIcon() override;
	const char* XMETHODCALLTYPE getTitle() override;
	XAccelItem XMETHODCALLTYPE getAccel() override;

	void XMETHODCALLTYPE activate() override;
	void XMETHODCALLTYPE deactivate() override;

	bool XMETHODCALLTYPE onMouseDown(bool isPrimary) override;
	bool XMETHODCALLTYPE onMouseMove() override;
	void XMETHODCALLTYPE onMouseUp(bool isPrimary) override;

	bool XMETHODCALLTYPE onKeyDown(UINT uKey) override;
	void XMETHODCALLTYPE onKeyUp(UINT uKey) override;

	void render(bool is3D);

	bool XMETHODCALLTYPE wantDrawSelection(bool is3D) override
	{
		return(false);
	}

	void XMETHODCALLTYPE onNextMode() override;

	bool XMETHODCALLTYPE wantUseClassSelector() override
	{
		return(false);
	}
	bool XMETHODCALLTYPE isRandomScaleOrYawSupported() override
	{
		return(false);
	}
	UINT XMETHODCALLTYPE getClassCount() override
	{
		return(0);
	}
	const char* XMETHODCALLTYPE getClassAt(UINT idx) override
	{
		return(NULL);
	}
	bool XMETHODCALLTYPE useClass(const char *szClassName) override
	{
		return(false);
	}

	bool XMETHODCALLTYPE allowUseCamera() override
	{
		return(false);
	}

	SX_ALIGNED_OP_MEM();

	void onHandleMoved(const float3 &vNewPos, IXEditorGizmoHandle *pHandle);
	void onHandleClick(IXEditorGizmoHandle *pHandle);
	void onHandleReleased();

	void onSelectionChanged(CEditorObject *pObject);

	void setSelectedVerticesForObject(CEditorObject *pObj, const Array<UINT> &aSelectedVertices);

	void onObjectTopologyChanged(CEditorObject *pObj);

private:
	void addObject(CEditorObject *pObj);
	void removeObject(CEditorObject *pObj);

	template<typename T>
	void enumHandles(const T &callback)
	{
		fora(i, m_aObjects)
		{
			ObjData &od = m_aObjects[i];
			fora(j, od.aHandles)
			{
				callback(i, j, od, od.aHandles[j]);
			}
		}
	}

private:
	CEditable *m_pEditable;
	IXEditor *m_pEditor;

	HBITMAP m_hBitmap;


	IXGizmoRenderer *m_pRenderer = NULL;

	bool m_isActive = false;

	CVertexGizmoCallback m_gizmoCallback;

	struct HandleData
	{
		IXEditorGizmoHandle *pHandle = NULL;
		bool isSelected = false;
		void select(bool yesNo)
		{
			isSelected = yesNo;
			if(yesNo)
			{
				pHandle->setColor(float4_t(1.0f, 0.0f, 0.0f, 1.0f));
			}
			else
			{
				pHandle->setColor(float4_t(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	};

	struct ObjData
	{
		CEditorObject *pObj = NULL;
		Array<HandleData> aHandles;
	};

	Array<ObjData> m_aObjects;

	CCommandMorph *m_pCommand = NULL;

	//int m_iMode = 0;
};

#endif
