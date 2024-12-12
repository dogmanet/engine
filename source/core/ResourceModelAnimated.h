#ifndef __RESOURCEMODELANIMATED_H
#define __RESOURCEMODELANIMATED_H

#include "ResourceModel.h"

#pragma warning(push)
#pragma warning(disable: 4250)

class CResourceModelAnimated: public CResourceModel, public virtual IXResourceModelAnimated
{
public:
	CResourceModelAnimated(CResourceManager *pMgr):
		CResourceModel(pMgr)
	{
	}
	~CResourceModelAnimated();

	XMODELTYPE XMETHODCALLTYPE getType() const override;

	UINT XMETHODCALLTYPE getSubsetCount(UINT uLod) const override;
	UINT XMETHODCALLTYPE addLod(UINT uSubsetCount, UINT *puVertexCount, UINT *puIndexCount) override;
	UINT XMETHODCALLTYPE getLodCount() const override;

	const XResourceModelAnimatedSubset * XMETHODCALLTYPE getSubset(UINT uLod, UINT uSubset) const override;
	XResourceModelAnimatedSubset * XMETHODCALLTYPE getSubset(UINT uLod, UINT uSubset) override;


	UINT XMETHODCALLTYPE getImportsCount() const override;
	const char * XMETHODCALLTYPE getImportName(UINT uIndex) const override;
	XMODEL_IMPORT XMETHODCALLTYPE getImportImportFlags(UINT uIndex) const override;
	UINT XMETHODCALLTYPE addImportName(const char *szFileName, XMODEL_IMPORT importFlags) override;

	const IXResourceModel * XMETHODCALLTYPE getImport(UINT uIndex) const override;
	IXResourceModel * XMETHODCALLTYPE getImport(UINT uIndex) override;
	void XMETHODCALLTYPE setImport(UINT uIndex, IXResourceModel *pResource) override;


	UINT XMETHODCALLTYPE getPartsCount() const override;
	const char * XMETHODCALLTYPE getPartFileName(UINT uIndex) const override;
	const char * XMETHODCALLTYPE getPartName(UINT uIndex) const override;
	XMODEL_IMPORT XMETHODCALLTYPE getPartImportFlags(UINT uIndex) const override;
	XMODEL_PART_FLAGS XMETHODCALLTYPE getPartFlags(UINT uIndex) const override;
	UINT XMETHODCALLTYPE addPartName(const char *szFileName, const char *szName, XMODEL_IMPORT importFlags, XMODEL_PART_FLAGS partFlags) override;

	const IXResourceModel * XMETHODCALLTYPE getPart(UINT uIndex) const override;
	IXResourceModel * XMETHODCALLTYPE getPart(UINT uIndex) override;
	void XMETHODCALLTYPE setPart(UINT uIndex, IXResourceModel *pResource) override;


	void XMETHODCALLTYPE setBoneCount(UINT uCount) override;
	UINT XMETHODCALLTYPE getBoneCount() const override;

	void XMETHODCALLTYPE setBoneInfo(int iBone, const char *szName, int iParent, const float3 &vTranslation, const SMQuaternion &vRotation) override;
	int XMETHODCALLTYPE getBoneParent(int iBone) const override;
	const char * XMETHODCALLTYPE getBoneName(int iBone) const override;
	float3 XMETHODCALLTYPE getBoneTranslation(int iBone) const override;
	SMQuaternion XMETHODCALLTYPE getBoneRotation(int iBone) const override;


	UINT XMETHODCALLTYPE getSequenceCount() const override;
	const XResourceModelSequence * XMETHODCALLTYPE getSequence(UINT uIndex) const override;

	void XMETHODCALLTYPE setSequenceCount(UINT uCount) override;
	XResourceModelSequence * XMETHODCALLTYPE getSequence(UINT uIndex) override;
	void XMETHODCALLTYPE setSequenceFrameCount(UINT uIndex, UINT uFrameCount) override;


	UINT XMETHODCALLTYPE addActivity(const char *szName) override;
	const char * XMETHODCALLTYPE getActivityName(UINT uIndex) const override;
	UINT XMETHODCALLTYPE getActivitiesCount() const override;


	UINT XMETHODCALLTYPE getControllersCount() const override;
	void XMETHODCALLTYPE setControllersCount(UINT uCount) override;
	void XMETHODCALLTYPE setControllerBoneCount(UINT uIndex, UINT uAffectedBonesCount) override;
	const XResourceModelController * XMETHODCALLTYPE getController(UINT uIndex) const override;
	XResourceModelController * XMETHODCALLTYPE getController(UINT uIndex) override;


	UINT XMETHODCALLTYPE getHitboxCount() const override;
	void XMETHODCALLTYPE setHitboxCount(UINT uCount) override;
	void XMETHODCALLTYPE setHitbox(UINT uIndex, IModelPhysbox *pPhysbox, int iBone, const char *szName, XHITBOXBODYPART part) override;
	IModelPhysbox* XMETHODCALLTYPE getHitbox(UINT uIndex) override;
	const IModelPhysbox* XMETHODCALLTYPE getHitbox(UINT uIndex) const override;
	const char* XMETHODCALLTYPE getHitboxName(UINT uIndex) const override;
	int XMETHODCALLTYPE getHitboxBone(UINT uIndex) const override;
	XHITBOXBODYPART XMETHODCALLTYPE getHitboxBodyPart(UINT uIndex) const override;

	bool XMETHODCALLTYPE validate() const;

protected:

	struct _subset_meta
	{
		UINT uSubsetCount;
		XResourceModelAnimatedSubset *pSubsets;
		XResourceModelAnimatedVertex *pVertices;
		UINT *pIndices;

		void clean()
		{
			mem_delete_a(pSubsets);
			mem_delete_a(pVertices);
			mem_delete_a(pIndices);
		}
	};
	Array<_subset_meta> m_aLods;

	struct _bone_hierarchy
	{
		int pid;         /*!< Номер родительской кости */
		char szName[XMODEL_MAX_NAME];
		XResourceModelBone bindPose;
	};

	_bone_hierarchy *m_pBones = NULL;
	UINT m_uBoneCount = 0;

	XResourceModelSequence *m_pSequences = NULL;
	UINT m_uSequenceCount = 0;

	Array<String> m_asActivities;

	XResourceModelController *m_pControllers = NULL;
	UINT m_uControllersCount = 0;

	struct Hitbox
	{
		IModelPhysbox *pPhysbox = NULL;
		int iBone = -1;
		String sName;
		XHITBOXBODYPART part = XHBP_DEFAULT;
	};
	Array<Hitbox> m_aHitBoxes;
	
	struct _import
	{
		IXResourceModel *pResource;
		XMODEL_IMPORT importFlags;
	};
	Array<String> m_asImportsNames;
	Array<_import> m_aImports;

	struct _part
	{
		IXResourceModel *pResource;
		XMODEL_IMPORT importFlags;
		XMODEL_PART_FLAGS flags;
		String sName;
	};
	Array<String> m_asPartsNames;
	Array<_part> m_aParts;
};

#pragma warning(pop)

#endif
