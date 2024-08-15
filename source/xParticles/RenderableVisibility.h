#ifndef __RENDERABLE_VISIBILITY_H
#define __RENDERABLE_VISIBILITY_H

#include <xcommon/IXRenderable.h>

class CParticleSystem;
class CParticlePlayerEmitter;
class CRenderableVisibility final: public IXUnknownImplementation<IXRenderableVisibility>
{
public:
	CRenderableVisibility(ID idPlugin, CParticleSystem *pSystem);
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
		CParticlePlayerEmitter *pReferenceMdl;
		//bool hasPSP;
		//SMPLANE psp;
		UINT uLod;
		IXMaterial *pMaterial;
	};

	void setItemCount(UINT uCount);
	item_s* getItem(UINT uIndex);

	void setItemTransparentCountDynamic(UINT uCount);
	void resetItemTransparentDynamic();
	void addItemTransparentDynamic(const TransparentModel &mdl);
	UINT getItemTransparentDynamicCount();
	TransparentModel* getItemTransparentDynamic(UINT uIndex);

	void setRenderList(void **ppData, UINT uCount);
	//void setTransparentList(void **ppData, UINT uCount);
	void setSelfillumList(void **ppData, UINT uCount);

	Array<CParticlePlayerEmitter*>& getRenderList();
	//Array<CParticlePlayerEmitter*>& getTransparentList();
	Array<CParticlePlayerEmitter*>& getSelfillumList();

	IXOcclusionCuller* getCuller()
	{
		return(m_pOcclusionCuller);
	}

protected:
	ID m_idPlugin;
	CParticleSystem *m_pSystem;
	IXOcclusionCuller *m_pOcclusionCuller = NULL;

	Array<item_s> m_aItems;
	Array<TransparentModel> m_aItemsDynamicTransparent;

	Array<CParticlePlayerEmitter*> m_aRenderList;
	//Array<CParticlePlayerEmitter*> m_aTransparentList;
	Array<CParticlePlayerEmitter*> m_aSelfillumList;
};

#endif
