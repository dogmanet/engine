#ifndef __DYNAMICMODELSHARED_H
#define __DYNAMICMODELSHARED_H

#include <xcommon/resource/IXModelProvider.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterial.h>

class CDynamicModelProvider;
class IXMaterialSystem;

#define MAX_INSTANCES 32

class CDynamicModelShared
{
public:
	SX_ALIGNED_OP_MEM();

	CDynamicModelShared(CDynamicModelProvider *pProvider);
	~CDynamicModelShared();
	void AddRef();
	void Release();

	bool init(IXResourceModelStatic *pResource);

	IXResourceModelStatic *getResource();

	UINT getPhysboxCount() const;
	const IModelPhysbox *getPhysBox(UINT id) const;

	float3 getLocalBoundMin() const;
	float3 getLocalBoundMax() const;
	SMAABB getLocalBound() const;

	void render(UINT uSkin, UINT uLod, XMODEL_FEATURE bmFeatures);
	void renderInstanced(const float3 &vPos, const SMQuaternion &qRot, float fScale, const float4_t &vColor);

	void initGPUresources();
	bool isReady() const;

	XMODEL_FEATURE getFeatures(UINT uSkin, UINT uLod);
	IXMaterial* getTransparentMaterial(UINT uSkin, UINT uLod);

	void onMaterialTransparencyChanged(const IXMaterial *pMaterial);
	void onMaterialEmissivityChanged(const IXMaterial *pMaterial);
	float3 getTransparentBoundMin(UINT uSkin, UINT uLod) const;
	float3 getTransparentBoundMax(UINT uSkin, UINT uLod) const;

	const Array<float4_t>& getPSPs(UINT uSkin, UINT uLod) const;

	void beginInstancing(UINT uSkin, UINT uLod, XMODEL_FEATURE bmWhat);
	void endInstancing();
	bool isInstancing();

protected:
	void buildPSPs();

protected:
	//! todo: make it atomic
	UINT m_uRefCount = 1;
	IXResourceModelStatic *m_pResource = NULL;

	IGXRenderBuffer **m_ppRenderBuffer = NULL;
	IGXIndexBuffer **m_ppIndexBuffer = NULL;

	UINT **m_ppTempIndices = NULL;
	USHORT **m_ppTempIndices16 = NULL;
	XResourceModelStaticVertexGPU **m_ppTempVertices = NULL;
	UINT *m_puTempTotalIndices = 0;
	UINT *m_puTempTotalVertices = 0;

	CDynamicModelProvider *m_pProvider;
	IGXDevice *m_pDevice;
	IXMaterialSystem *m_pMaterialSystem;

	struct
	{
		float4 vPosScale;
		SMQuaternion qRot;
	} m_instanceData[MAX_INSTANCES];
	struct
	{
		float4 vColor;
	} m_instanceColors[MAX_INSTANCES];
	IGXConstantBuffer *m_pInstanceBuffer = NULL;
	IGXConstantBuffer *m_pInstanceColorsBuffer = NULL;
	int m_iInstanceCount = 0;
	bool m_isInstancingEnabled = false;
	UINT m_uInstancingSkin = 0;
	UINT m_uInstancingLod = 0;
	XMODEL_FEATURE m_bmInstancingFeatures = MF_NONE;

	void **m_ppMaterialsBlob = NULL;
	IXMaterial ***m_pppMaterials = NULL;
	UINT m_uMaterialCount = 0;
	UINT m_uSkinCount = 0;
	XMODEL_FEATURE *m_bmFeatures = NULL; //!< По количеству скинов

	XPT_TOPOLOGY m_topology = XPT_TRIANGLELIST;

	struct physbox_s
	{
		const IModelPhysbox *pPhysbox;
		int iBone;
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

	Array<Array<subset_t>, 1> m_aLods;

	Array<Array<Array<float4_t>>, 1> m_aPSPs;

	Array<const IModelPhysbox*> m_apPhysboxes;
};

#endif
