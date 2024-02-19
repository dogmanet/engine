#ifndef __PHYSICS_UTIL_H
#define __PHYSICS_UTIL_H

#include <xcommon/physics/IXPhysics.h>

IXPhysics* GetPhysics();
IXPhysicsWorld* GetPhysWorld();

class CClosestRayResultCallback: public IXRayCallback
{
public:
	CClosestRayResultCallback()
	{
		m_result.pCollisionObject = NULL;
	}
	float XMETHODCALLTYPE addSingleResult(const XRayResult &result) override
	{
		m_result = result;

		return(result.fHitFraction);
	}

	bool hasHit()
	{
		return(m_result.pCollisionObject != NULL);
	}

	XRayResult m_result;
};

//#############################################################################

class CClosestNotMeRayResultCallback: public IXRayCallback
{
public:
	CClosestNotMeRayResultCallback(IXCollisionObject* me)
	{
		m_me = me;
		m_result.pCollisionObject = NULL;
	}

	float XMETHODCALLTYPE addSingleResult(const XRayResult &result) override
	{
		if(result.pCollisionObject == m_me)
		{
			return(1.0f);
		}

		m_result = result;

		return(result.fHitFraction);
	}

	bool hasHit()
	{
		return(m_result.pCollisionObject != NULL);
	}

	IXCollisionObject *m_me;
	XRayResult m_result;
};

//#############################################################################

class CAllHitsNotMeRayResultCallback: public IXRayCallback
{
public:
	CAllHitsNotMeRayResultCallback(IXCollisionObject* me)
	{
		m_me = me;
	}

	float XMETHODCALLTYPE addSingleResult(const XRayResult &result) override
	{
		if(result.pCollisionObject == m_me)
		{
			return(1.0f);
		}

		m_aResults.push_back(result);

		return(1.0f);
	}

	bool hasHit()
	{
		return(m_aResults.size() != 0);
	}

	IXCollisionObject *m_me;
	Array<XRayResult> m_aResults;
};

//#############################################################################

class CAllHitsResultCallback: public IXRayCallback
{
public:
	float XMETHODCALLTYPE addSingleResult(const XRayResult &result) override
	{
		m_aResults.push_back(result);

		return(1.0f);
	}

	bool hasHit()
	{
		return(m_aResults.size() != 0);
	}

	IXCollisionObject *m_me;
	Array<XRayResult> m_aResults;
};

#endif
