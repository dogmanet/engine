#ifndef __RENDERABLE_VISIBILITY_H
#define __RENDERABLE_VISIBILITY_H

#include <xcommon/IXRenderable.h>
#include <xcommon/resource/IXResourceModel.h>

class CAnimatedModelProvider;
class CDynamicModelProvider;
class CDecalProvider;
class CDynamicModel;
class CRenderableVisibility final: public IXUnknownImplementation<IXRenderableVisibility>
{
public:
	CRenderableVisibility(ID idPlugin, CAnimatedModelProvider *pProviderAnimated, CDynamicModelProvider *pProviderDynamic, CDecalProvider *pProviderDecal);
	~CRenderableVisibility();

	ID getPluginId() const override;

	void setOcclusionCuller(IXOcclusionCuller *pOcclusionCuller) override;

	void updateForCamera(IXCamera *pCamera, const IXRenderableVisibility *pReference = NULL) override;

	void updateForFrustum(const IXFrustum *pFrustum, UINT bmLayers, const IXRenderableVisibility *pReference = NULL) override;
	
	void append(const IXRenderableVisibility *pOther) override;

	void substract(const IXRenderableVisibility *pOther) override;

	struct item_s
	{
		bool isVisible = false;
		UINT uLod = 0;
		bool isTransparent = false;
		bool isEmissive = false;
	};

	struct TransparentModel
	{
		CDynamicModel *pReferenceMdl;
		bool hasPSP;
		SMPLANE psp;
		UINT uLod;
		IXMaterial *pMaterial;
	};
	
	struct OverlaySubset
	{
		IXMaterial *pMaterial;
		UINT uStartVertex;
		UINT uStartIndex;
		UINT uQuadCount;
	};

	struct OverlayData
	{
		Array<XResourceModelStaticVertexGPU> aVertices;
		Array<OverlaySubset> aSubsets;
		IGXVertexBuffer *pVB = NULL;
		IGXRenderBuffer *pRB = NULL;
		IGXIndexBuffer *pIB = NULL;
		UINT uVertexBufferAllocSize = 0;
	};

	void setItemCount(UINT uCount);
	item_s* getItem(UINT uIndex);

	void setItemTransparentCountDynamic(UINT uCount);
	void resetItemTransparentDynamic();
	void addItemTransparentDynamic(const TransparentModel &mdl);
	UINT getItemTransparentDynamicCount();
	TransparentModel* getItemTransparentDynamic(UINT uIndex);

	void setRenderList(void **ppData, UINT uCount);
	void setTransparentList(void **ppData, UINT uCount);
	void setSelfillumList(void **ppData, UINT uCount);

	Array<CDynamicModel*>& getRenderList();
	Array<CDynamicModel*>& getTransparentList();
	Array<CDynamicModel*>& getSelfillumList();
	OverlayData& getOverlayData();

	IXOcclusionCuller* getCuller()
	{
		return(m_pOcclusionCuller);
	}

private:
	ID m_idPlugin;
	CAnimatedModelProvider *m_pProviderAnimated;
	CDynamicModelProvider *m_pProviderDynamic;
	CDecalProvider *m_pProviderDecal;
	IXOcclusionCuller *m_pOcclusionCuller = NULL;

	Array<item_s> m_aItems;
	Array<TransparentModel> m_aItemsDynamicTransparent;

	Array<CDynamicModel*> m_aRenderList;
	Array<CDynamicModel*> m_aTransparentList;
	Array<CDynamicModel*> m_aSelfillumList;
	
	OverlayData m_overlayData;
};

#endif
