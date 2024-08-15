#ifndef __RENDERABLE_VISIBILITY_H
#define __RENDERABLE_VISIBILITY_H

#include <xcommon/IXRenderable.h>

class CRenderableVisibility: public IXUnknownImplementation<IXRenderableVisibility>
{
public:
	CRenderableVisibility(ID idPlugin, UINT uRenderSystems);
	~CRenderableVisibility();

	ID getPluginId() const override;

	void setOcclusionCuller(IXOcclusionCuller *pOcclusionCuller) override;

	void updateForCamera(IXCamera *pCamera, const IXRenderableVisibility *pReference = NULL) override;

	void updateForFrustum(const IXFrustum *pFrustum, UINT bmLayers, const IXRenderableVisibility *pReference = NULL) override;

	IXRenderableVisibility *getVisibility(ID id);
	const IXRenderableVisibility *getVisibility(ID id) const;
	void setVisibility(ID id, IXRenderableVisibility *pVisibility);

	void append(const IXRenderableVisibility *pOther) override;

	void substract(const IXRenderableVisibility *pOther) override;

protected:
	IXRenderableVisibility **m_ppVisibilities;
	UINT m_uRenderSystems;
	ID m_idPlugin;
};

#endif
