#ifndef __ANIMATEDMODELSHARED_H
#define __ANIMATEDMODELSHARED_H

#include <xcommon/resource/IXModelProvider.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterial.h>

class CAnimatedModelProvider;
class IXMaterialSystem;

class CAnimatedModelShared
{
public:
	CAnimatedModelShared(CAnimatedModelProvider *pProvider);
	~CAnimatedModelShared();
	void AddRef();
	void Release();

	bool init(UINT uResourceCount, IXResourceModelAnimated **ppResources);
	bool isSame(UINT uResourceCount, IXResourceModelAnimated **ppResources);

	const Array<IXResourceModelAnimated*> &getResources();

	UINT getPhysboxCount(UINT uPartIndex = 0) const;
	const IModelPhysbox *getPhysBox(UINT id, UINT uPartIndex = 0) const;
	const int getPhysBoxBone(UINT id, UINT uPartIndex = 0) const;
	const IXResourceModel *getResource(UINT uIndex = 0);


	UINT getPartsCount() const;
	const char* getPartName(UINT uIndex) const;
	UINT getPartIndex(const char *szName);
	XMODEL_PART_FLAGS getPartFlags(UINT uIndex) const;


	UINT getHitboxCount(UINT uPartIndex = 0) const;
	const IModelPhysbox* getHitbox(UINT id, UINT uPartIndex = 0) const;
	int getHitboxBone(UINT id, UINT uPartIndex = 0) const;
	XHITBOXBODYPART getHitboxBodyPart(UINT id, UINT uPartIndex = 0) const;


	int getBoneId(const char *szName);
	UINT getBoneCount() const;
	const char *getBoneName(UINT id) const;
	int getBoneParent(UINT id) const;
	const XResourceModelBone *getBone(UINT id) const;
	const XResourceModelBone *getInvertedBindPose() const;

	UINT getControllersCount() const;
	const char *getControllerName(UINT id) const;
	UINT getControllerId(const char *szName) const;
	const XResourceModelController *getController(UINT uIndex) const;

	int getActivityId(const char *szName) const;
	const char *getActivityName(UINT id) const;

	UINT getSequenceCount() const;
	const XResourceModelSequence *getSequence(UINT uIndex) const;
	int getSequenceId(const char *szName) const;

	float3 getLocalBoundMin() const;
	float3 getLocalBoundMax() const;
	SMAABB getLocalBound() const;

	void render(UINT uSkin, UINT uLod);

	void initGPUresources();

protected:
	UINT m_uRefCount = 1;
	Array<IXResourceModelAnimated*> m_apResources;

	IGXRenderBuffer **m_ppRenderBuffer = NULL;
	IGXIndexBuffer **m_ppIndexBuffer = NULL;

	UINT **m_ppTempIndices = NULL;
	XResourceModelAnimatedVertexGPU **m_ppTempVertices = NULL;
	UINT *m_puTempTotalIndices = 0;
	UINT *m_puTempTotalVertices = 0;

	CAnimatedModelProvider *m_pProvider;
	IGXDevice *m_pDevice;
	IXMaterialSystem *m_pMaterialSystem;

	Array<const char*> m_aszActivities;

	struct bone_s
	{
		const char *szName;
		int iParent;
		XResourceModelBone bone;
	};
	bone_s *m_pBones = NULL;
	XResourceModelBone *m_pInvBindPose = NULL;
	UINT m_uBoneCount = 0;

	void **m_ppMaterialsBlob = NULL;
	IXMaterial ***m_pppMaterials = NULL;
	UINT m_uMaterialCount = 0;
	UINT m_uSkinCount = 0;

	struct physbox_s
	{
		const IModelPhysbox *pPhysbox;
		int iBone;
	};

	struct Hitbox
	{
		const IModelPhysbox *pPhysbox = NULL;
		int iBone = -1;
		XHITBOXBODYPART part = XHBP_DEFAULT;
	};

	float3_t m_vLocalMin;
	float3_t m_vLocalMax;

	struct subset_t
	{
		UINT uStartIndex = 0;
		UINT uIndexCount = 0;
		UINT uStartVertex = 0;
		UINT uVertexCount = 0;
	};

	struct part_s
	{
		const char *szName;
		XMODEL_PART_FLAGS flags;
		Array<Hitbox> aHitboxes;
		Array<physbox_s> aPhysboxes;

		Array<Array<subset_t>> aLods;
	};
	Array<part_s> m_aParts;

	UINT m_uLodCount = 0;

	Array<XResourceModelController> m_aControllers;

	Array<XResourceModelSequence> m_aSequences;

private:
	struct bone_node
	{
		UINT uResource;
		UINT uBoneIdx;
		bone_node *pParent;
		const char *szName;
		bool bLooseTransform;
	};

	struct mtl_node
	{
		const char *szName = NULL;
		UINT uResource;
	};

	struct merge_subset_s
	{
		XResourceModelAnimatedSubset data;
		const IXResourceModelAnimated *pResource;
	};
	
	void _collectResources(IXResourceModelAnimated *pResource, Array<IXResourceModelAnimated*> &aResources);
	void _mergeByParent(bone_node *pParent, bone_node *pNodes, UINT uTotalBones);
	int _buildBoneListByParent(bone_node *pParent, bone_node *pNodes, UINT uTotalBones, bone_s *pList, IXResourceModelAnimated **ppResources, int iParent);

	void _initPart(Array<IXResourceModelAnimated*> &aPart, part_s *pPart, Array<Array<merge_subset_s>> &partLods, Array<Array<mtl_node>> &aaMaterials, XMODEL_IMPORT importFlags = XMI_ALL);
};

#endif
