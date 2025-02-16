#ifndef __MODELOVERLAY_H
#define __MODELOVERLAY_H

#include <xcommon/resource/IXModel.h>
#include <graphix/graphix.h>
#include <mtrl/IXMaterial.h>

class IXMaterialSystem;
class CDecal;

class CModelOverlay final
{
public:
	CModelOverlay(CDecal *pDecal, IXMaterial *pMaterial, Array<XResourceModelStaticVertex> &aVertices, const float3_t &vNormal);

	const Array<XResourceModelStaticVertex>& getVertices();

	CModelOverlay* getNextOverlay();
	void setNextOverlay(CModelOverlay *pOverlay);

	IXMaterial* getMaterial();

	const float3_t& getNormal();

	void onModelRemoved();

private:
	// Model's overlays stored as linked list
	CModelOverlay *m_pNextOverlay = NULL;
	CDecal *m_pDecal;

	TODO("Use memory pool");
	Array<XResourceModelStaticVertex> m_aVertices;

	IXMaterial *m_pMaterial = NULL;

	float3_t m_vNormal;

	bool m_isTransparent = false;
};

#endif
