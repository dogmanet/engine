#include "RenderableVisibility.h"

CRenderableVisibility::CRenderableVisibility(ID idPlugin, UINT uRenderSystems):
	m_uRenderSystems(uRenderSystems),
	m_idPlugin(idPlugin)
{
	m_ppVisibilities = new IXRenderableVisibility*[uRenderSystems];
	memset(m_ppVisibilities, 0, sizeof(IXRenderableVisibility*) * uRenderSystems);
}

CRenderableVisibility::~CRenderableVisibility()
{
	for(UINT i = 0; i < m_uRenderSystems; ++i)
	{
		mem_release(m_ppVisibilities[i]);
	}
	mem_delete_a(m_ppVisibilities);
}

ID CRenderableVisibility::getPluginId() const
{
	return(m_idPlugin);
}

void CRenderableVisibility::setOcclusionCuller(IXOcclusionCuller *pOcclusionCuller)
{
	for(UINT i = 0; i < m_uRenderSystems; ++i)
	{
		if(m_ppVisibilities[i])
		{
			m_ppVisibilities[i]->setOcclusionCuller(pOcclusionCuller);
		}
	}
}

void CRenderableVisibility::updateForCamera(IXCamera *pCamera, const IXRenderableVisibility *pReference)
{
	XPROFILE_FUNCTION();
	CRenderableVisibility *pRef = NULL;
	if(pReference)
	{
		assert(((IXRenderableVisibility*)pReference)->getPluginId() == getPluginId());
		pRef = (CRenderableVisibility*)pReference;
	}

	for(UINT i = 0; i < m_uRenderSystems; ++i)
	{
		if(m_ppVisibilities[i])
		{
			m_ppVisibilities[i]->updateForCamera(pCamera, pRef ? pRef->getVisibility(i) : NULL);
		}
	}
}

void CRenderableVisibility::updateForFrustum(const IXFrustum *pFrustum, UINT bmLayers, const IXRenderableVisibility *pReference)
{
	CRenderableVisibility *pRef = NULL;
	if(pReference)
	{
		assert(((IXRenderableVisibility*)pReference)->getPluginId() == -1);
		pRef = (CRenderableVisibility*)pReference;
	}

	for(UINT i = 0; i < m_uRenderSystems; ++i)
	{
		if(m_ppVisibilities[i])
		{
			m_ppVisibilities[i]->updateForFrustum(pFrustum, bmLayers, pRef ? pRef->getVisibility(i) : NULL);
		}
	}
}

IXRenderableVisibility *CRenderableVisibility::getVisibility(ID id)
{
	assert(ID_VALID(id) && (UINT)id < m_uRenderSystems);

	return(m_ppVisibilities[id]);
}

const IXRenderableVisibility *CRenderableVisibility::getVisibility(ID id) const
{
	assert(ID_VALID(id) && (UINT)id < m_uRenderSystems);

	return(m_ppVisibilities[id]);
}

void CRenderableVisibility::setVisibility(ID id, IXRenderableVisibility *pVisibility)
{
	assert(ID_VALID(id) && (UINT)id < m_uRenderSystems);

	mem_release(m_ppVisibilities[id]);

	m_ppVisibilities[id] = pVisibility;

	if(pVisibility)
	{
		pVisibility->AddRef();
	}
}

void CRenderableVisibility::append(const IXRenderableVisibility *pOther_)
{
	assert(pOther_->getPluginId() == getPluginId());
	const CRenderableVisibility *pOther = (const CRenderableVisibility*)pOther_;

	for(UINT i = 0; i < m_uRenderSystems; ++i)
	{
		if(m_ppVisibilities[i])
		{
			m_ppVisibilities[i]->append(pOther->getVisibility(i));
		}
	}
}

void CRenderableVisibility::substract(const IXRenderableVisibility *pOther_)
{
	assert(pOther_->getPluginId() == getPluginId());
	const CRenderableVisibility *pOther = (const CRenderableVisibility*)pOther_;

	for(UINT i = 0; i < m_uRenderSystems; ++i)
	{
		if(m_ppVisibilities[i])
		{
			m_ppVisibilities[i]->substract(pOther->getVisibility(i));
		}
	}
}
