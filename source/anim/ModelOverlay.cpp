#include "ModelOverlay.h"
#include "Decal.h"

CModelOverlay::CModelOverlay(CDecal *pDecal, IXMaterial *pMaterial, Array<XResourceModelStaticVertex> &aVertices, const float3_t &vNormal):
	m_pDecal(pDecal),
	m_pMaterial(pMaterial),
	m_vNormal(vNormal)
{
	m_aVertices.swap(aVertices);
}

const Array<XResourceModelStaticVertex>& CModelOverlay::getVertices()
{
	return(m_aVertices);
}

CModelOverlay* CModelOverlay::getNextOverlay()
{
	return(m_pNextOverlay);
}
void CModelOverlay::setNextOverlay(CModelOverlay *pOverlay)
{
	m_pNextOverlay = pOverlay;
}

IXMaterial* CModelOverlay::getMaterial()
{
	return(m_pMaterial);
}

const float3_t& CModelOverlay::getNormal()
{
	return(m_vNormal);
}

void CModelOverlay::onModelRemoved()
{
	SAFE_CALL(m_pNextOverlay, onModelRemoved);

	//notify decal
	m_pDecal->onOverlayRemoved(this);
}
