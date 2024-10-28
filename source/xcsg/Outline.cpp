#include "Outline.h"

COutline::COutline(IXEditor *pEditor, IXGizmoRenderer *pRenderer):
	m_pEditor(pEditor),
	m_pRenderer(pRenderer),
	m_callBack(this)
{
	
}

COutline::~COutline()
{
	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		mem_release(m_aPoints[i].pHandle);
	}
}

void COutline::addPoint()
{
	//if(isValid())
	if(m_isClosed)
	{
		bool isLineClicked = false;

		const float c_fClickThreshold = 4.0f; // 4 px

		float fViewportScale = m_pEditor->getViewScale(m_pEditor->getState()->activeWindow);
		float vBestD = FLT_MAX;
		float3 vWorldRayStart = m_pEditor->getState()->vWorldRayStart;
		float3 vWorldRayDir = m_pEditor->getState()->vWorldRayDir;
		float3 vBestPt;
		float3 vPt;
		UINT uInsertAfter = 0;
		for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
		{
			PathPoint &a = m_aPoints[i];
			PathPoint &b = m_aPoints[(i + 1) % l];

			float d = SMCrossLines(vWorldRayStart, vWorldRayDir, a.vPos, b.vPos - a.vPos, NULL, &vPt);
			d /= fViewportScale;

			if(d < vBestD && d <= c_fClickThreshold)
			{
				SMAABB aabb(SMVectorMin(a.vPos, b.vPos), SMVectorMax(a.vPos, b.vPos));
				if(
					aabb.vMin.x <= vPt.x &&
					aabb.vMin.y <= vPt.y &&
					aabb.vMin.z <= vPt.z &&
					aabb.vMax.x >= vPt.x &&
					aabb.vMax.y >= vPt.y &&
					aabb.vMax.z >= vPt.z
					)
				{
					vBestD = d;
					vBestPt = vPt;
					uInsertAfter = i;
					isLineClicked = true;
				}
			}
		}

		if(isLineClicked)
		{
			m_isDirty = true;
			IXEditorGizmoHandle *pHandle;
			m_pEditor->newGizmoHandle(&pHandle);
			pHandle->setPos(vBestPt);
			pHandle->lockInPlane(m_vNormal);
			pHandle->setCallback(&m_callBack);
			//pHandle->setCallback();
			m_aPoints.insert({vBestPt, false, pHandle}, uInsertAfter + 1);
		}
		else
		{
			for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
			{
				m_aPoints[i].isSelected = false;
				m_aPoints[i].pHandle->setColor(float4_t(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}
	}
	else
	{
		m_isDirty = true;
		IXEditorGizmoHandle *pHandle;
		m_pEditor->newGizmoHandle(&pHandle);
		pHandle->setPos(m_vNewPoint);
		pHandle->lockInPlane(m_vNormal);
		pHandle->setCallback(&m_callBack);
		//pHandle->setCallback();

		m_aPoints.push_back({m_vNewPoint, false, pHandle});
	}
}

void COutline::setPlane(const float3_t &vNormal)
{
	m_vNormal = SMVector3Normalize(vNormal);
}

void COutline::closePath()
{
	if(!m_isClosed && m_aPoints.size() >= 3)
	{
		m_isDirty = true;
		m_isClosed = true;
	}
}

void COutline::render(bool is3D)
{
	if(m_isDirty)
	{
		m_pRenderer->reset();

		if(m_isClosed && isValid())
		{
			buildContours();

			m_pRenderer->setColor(float4_t(0.2f, 0.8f, 0.2f, 0.6f));

			for(UINT i = 0, l = m_aContours.size(); i < l; ++i)
			{
				CContour &c = m_aContours[i];
				m_pRenderer->jumpTo(m_aPoints[c.getPoint(0)].vPos);
				for(UINT j = 1, jl = c.getPointCount(); j < jl; ++j)
				{
					m_pRenderer->lineTo(m_aPoints[c.getPoint(j)].vPos);
				}
				m_pRenderer->lineTo(m_aPoints[c.getPoint(0)].vPos);
			}
		}

		if(m_aPoints.size())
		{
			m_pRenderer->setColor(isValid() ? float4_t(0.6f, 0.8f, 1.0f, 1.0f) : float4_t(1.0f, 0.2f, 0.2f, 1.0f));
			m_pRenderer->jumpTo(m_aPoints[0].vPos);
			for(UINT i = 1, l = m_aPoints.size(); i < l; ++i)
			{
				m_pRenderer->lineTo(m_aPoints[i].vPos);
			}

			if(!m_isClosed)
			{
				m_pRenderer->setColor(isValid() ? float4_t(1.0f, 1.0f, 0.0f, 0.8f) : float4_t(1.0f, 0.2f, 0.2f, 0.8f));

				m_pRenderer->lineTo(m_vNewPoint);
			}

			m_pRenderer->lineTo(m_aPoints[0].vPos);
		}

		m_isDirty = false;
	}

	m_pRenderer->render(!is3D);
}

static bool SegmentIntersectSegment(const float3 &n, const float3 &a0, const float3 &a1, const float3 &b0, const float3 &b1)
{
	SMPLANE p0(SMVector3Normalize(SMVector3Cross(a0 - a1, n)), a0);
	float fp0 = SMVector4Dot(p0, float4(b0, 1.0f));
	float fp1 = SMVector4Dot(p0, float4(b1, 1.0f));

	if(SMIsZero(fp0) && SMIsZero(fp1)) // segments belongs to the same line
	{
		float3 vec = a0 - a1;
		float fa0 = SMVector3Dot(vec, a0);
		float fa1 = SMVector3Dot(vec, a1);
		float fb0 = SMVector3Dot(vec, b0);
		float fb1 = SMVector3Dot(vec, b1);

		return(!(min(fb0, fb1) > max(fa0, fa1) || min(fa0, fa1) > max(fb0, fb1)));
	}

	bool f0 = fp0 > 0.0f;
	bool f1 = fp1 > 0.0f;
	bool z0 = SMIsZero(fp0) || SMIsZero(fp1);
	

	SMPLANE p1(SMVector3Normalize(SMVector3Cross(b0 - b1, n)), b0);
	bool f2 = (fp0 = SMVector4Dot(p1, float4(a0, 1.0f))) > 0.0f;
	bool f3 = (fp1 = SMVector4Dot(p1, float4(a1, 1.0f))) > 0.0f;
	bool z1 = SMIsZero(fp0) || SMIsZero(fp1);

	return((f0 != f1 || z0) && (f2 != f3 || z1));
}

bool COutline::isValid()
{
	if(m_aPoints.size() < 2)
	{
		return(true);
	}

	if(m_isContoursFailed)
	{
		return(true);
	}

	if(!m_isClosed)
	{
		float3 va0 = m_vNewPoint;
		float3 va1 = m_aPoints[0].vPos;

		float3 vb0, vb1;
		for(UINT i = 2, l = m_aPoints.size(); i < l; ++i)
		{
			vb0 = m_aPoints[i - 1].vPos;
			vb1 = m_aPoints[i].vPos;
			if(SegmentIntersectSegment(m_vNormal, va0, va1, vb0, vb1))
			{
				return(false);
			}
		}

		va1 = m_aPoints[m_aPoints.size() - 1].vPos;
		for(UINT i = 0, l = m_aPoints.size() - 2; i < l; ++i)
		{
			vb0 = m_aPoints[i].vPos;
			vb1 = m_aPoints[i + 1].vPos;
			if(SegmentIntersectSegment(m_vNormal, va0, va1, vb0, vb1))
			{
				return(false);
			}
		}
	}

	UINT p0, p1, p2, p3;

	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		p0 = i;
		p1 = (i + 1) % l;

		for(UINT j = i + 1; j < l; ++j)
		{
			if(SMIsZero(SMVector3Length2(m_aPoints[i].vPos - m_aPoints[j].vPos)))
			{
				return(false);
			}

			p2 = j;
			p3 = (j + 1) % l;

			if(p2 == p1 || p0 == p3)
			{
				float fCos = fabsf(SMVector3Dot(
					SMVector3Normalize(m_aPoints[p0].vPos - m_aPoints[p1].vPos),
					SMVector3Normalize(m_aPoints[p2].vPos - m_aPoints[p3].vPos)
					));
				if(SMIsZero(fCos - 1.0f))
				{
					return(false);
				}
			}

			if(j > i + 1)
			{
				if(p0 == p3)
				{
					continue;
				}

				if(SegmentIntersectSegment(m_vNormal, m_aPoints[p0].vPos, m_aPoints[p1].vPos, m_aPoints[p2].vPos, m_aPoints[p3].vPos))
				{
					return(false);
				}
			}
		}
	}

	return(true);
}

void COutline::activate()
{
	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		m_aPoints[i].pHandle->enable(true);
	}
}

void COutline::deactivate()
{
	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		m_aPoints[i].pHandle->enable(false);
	}
}

void COutline::onHandleClick(IXEditorGizmoHandle *pHandle)
{
	int idx = m_aPoints.indexOf(pHandle, [](const PathPoint &p, IXEditorGizmoHandle *h){
		return(p.pHandle == h);
	});
	assert(idx >= 0);

	if(!m_isClosed && idx == 0 && m_aPoints.size() >= 3)
	{
		closePath();
	}
	
	if(m_pEditor->isKeyPressed(SIK_CONTROL))
	{
		m_aPoints[idx].isSelected = !m_aPoints[idx].isSelected;
		m_aPoints[idx].pHandle->setColor(float4_t(1.0f, 1.0f, m_aPoints[idx].isSelected ? 0.0f : 1.0f, 1.0f));
	}
	else
	{
		for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
		{
			m_aPoints[i].isSelected = false;
			m_aPoints[i].pHandle->setColor(float4_t(1.0f, 1.0f, 1.0f, 1.0f));
		}

		m_aPoints[idx].isSelected = true;
		m_aPoints[idx].pHandle->setColor(float4_t(1.0f, 1.0f, 0.0f, 1.0f));
	}


	m_isDirty = true;
}
void COutline::onHandleMove(const float3 &vNewPos, IXEditorGizmoHandle *pHandle)
{
	int idx = m_aPoints.indexOf(pHandle, [](const PathPoint &p, IXEditorGizmoHandle *h){
		return(p.pHandle == h);
	});
	assert(idx >= 0);

	float3 vDelta = vNewPos - m_aPoints[idx].vPos;

	if(!m_aPoints[idx].isSelected)
	{
		for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
		{
			m_aPoints[i].isSelected = false;
			m_aPoints[i].pHandle->setColor(float4_t(1.0f, 1.0f, 1.0f, 1.0f));
		}

		m_aPoints[idx].isSelected = true;
		m_aPoints[idx].pHandle->setColor(float4_t(1.0f, 1.0f, 0.0f, 1.0f));
	}

	m_aPoints[idx].vPos = vNewPos;
	pHandle->setPos(vNewPos);

	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		if(i != idx && m_aPoints[i].isSelected)
		{
			m_aPoints[i].vPos = (float3)(m_aPoints[i].vPos + vDelta);
			m_aPoints[i].pHandle->setPos(m_aPoints[i].vPos);
		}
	}
	
	m_isDirty = true;
}

void COutline::setMouse(const float3_t &vPoint)
{
	if(!m_isClosed)
	{
		m_isDirty = true;
		m_vNewPoint = vPoint;
	}
}

void COutline::deleteSelected()
{
	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		if(m_aPoints[i].isSelected)
		{
			mem_release(m_aPoints[i].pHandle);
			m_aPoints.erase(i);
			--i;
			--l;
		}
	}

	if(m_aPoints.size() < 3)
	{
		m_isClosed = false;
	}

	m_isDirty = true;
}

void COutline::buildContours()
{
	assert(m_isClosed && m_aPoints.size() >= 3);

	m_aContours.clearFast();

	fixVertexOrder();

	Array<UINT> aIn;
	aIn.reserve(m_aPoints.size());
	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		aIn.push_back(i);
	}
	//aIn.push_back(0);

	buildContoursRecursive(aIn);
}

#if 0
bool COutline::canUsePoint(const Array<UINT> &aIn, UINT uPoint, UINT uPrevPoint0, UINT uPrevPoint1)
{
	float3 vFirstPt = m_aPoints[aIn[0]].vPos;
	float3 vSecondPt = m_aPoints[aIn[1]].vPos;

	float3 vPoint = m_aPoints[aIn[uPoint]].vPos;
	float3 vBack = m_aPoints[aIn[uPrevPoint0]].vPos - m_aPoints[aIn[uPrevPoint1]].vPos;
	float3 vFront = vPoint - m_aPoints[aIn[uPrevPoint1]].vPos;
	float3 vEnd = vFirstPt - vPoint;

	if(SMRightAngleBetweenVectors(vBack, vFront, m_vNormal) >= SM_PI
		|| SMRightAngleBetweenVectors(-vEnd, vSecondPt - vFirstPt, m_vNormal) >= SM_PI
		|| SMRightAngleBetweenVectors(-vFront, vFirstPt - vPoint, m_vNormal) >= SM_PI
		)
	{
		return(false);
	}

	for(UINT i = uPoint + 1, l = aIn.size() - 1; i < l; ++i)
	{
		if(SMVector3Length2(vEnd) > SMVector3Length2(m_aPoints[aIn[i]].vPos - vPoint) && SMRightAngleBetweenVectors(vEnd, m_aPoints[aIn[i]].vPos - vPoint, m_vNormal) >= SM_PI)
		{
			return(false);
		}
	}

	if(uPoint != uPrevPoint1 + 1 && uPoint != aIn[0])
	{
		// check for collisions
		// vFront
		for(UINT i = uPrevPoint1 + 1, l = aIn.size() - 1; i < l; ++i)
		{
			UINT a = i;
			UINT b = i + 1;
			if(a == uPoint || b == uPoint)
			{
				continue;
			}

			if(SegmentIntersectSegment(m_vNormal, m_aPoints[aIn[uPoint]].vPos, m_aPoints[aIn[uPrevPoint1]].vPos, m_aPoints[aIn[a]].vPos, m_aPoints[aIn[b]].vPos))
			{
				return(false);
			}
		}
	}

	return(true);
}
#endif

bool COutline::checkPoint(const Array<UINT> &aIn, UINT uPoint, UINT uNextPoint)
{
	for(UINT i = 0, l = aIn.size(); i < l; ++i)
	{
		UINT a = i;
		UINT b = (i + 1) % l;
		if(a == uNextPoint || b == uPoint || a == uPoint || b == uNextPoint)
		{
			continue;
		}

		if(SegmentIntersectSegment(m_vNormal, m_aPoints[aIn[uPoint]].vPos, m_aPoints[aIn[uNextPoint]].vPos, m_aPoints[aIn[a]].vPos, m_aPoints[aIn[b]].vPos))
		{
			return(false);
		}
	}

	return(true);
}

void COutline::buildContoursRecursive(const Array<UINT> &aIn, UINT uCurDepth)
{
	if(uCurDepth == 0)
	{
		m_isContoursFailed = false;
	}
	else if(uCurDepth > 32)
	{
		m_isContoursFailed = true;
		return;
	}
	Array<UINT> aOut = aIn;

	UINT uStartPt = 0;
	UINT uCurPt = 0;
	UINT uNextPt, uPrevPt;
	//aOut.erase(aOut.size() - 1);
	bool isFirst = true;

	//bool wasTailMinimized = true;

	while(true)
	{
		uNextPt = (uCurPt + 1) % aOut.size();
		uPrevPt = (uCurPt > 0 ? uCurPt : aOut.size()) - 1;

		float3 vBack = m_aPoints[aOut[uPrevPt]].vPos - m_aPoints[aOut[uCurPt]].vPos;
		float3 vFront = m_aPoints[aOut[uNextPt]].vPos - m_aPoints[aOut[uCurPt]].vPos;

		if(!(SMRightAngleBetweenVectors(vBack, vFront, m_vNormal) < SM_PI && checkPoint(aIn, aIn.indexOf(aOut[uCurPt]), aIn.indexOf(aOut[uNextPt]))))
		{
			aOut.erase(uNextPt);
			if(uNextPt == 0)
			{
				uCurPt = aOut.size() - 1;
			}
			//wasTailMinimized = false;
		}
		else
		{
#if 0
			/*if(!wasTailMinimized)
			{
				wasTailMinimized = true;
				// step back to minimize distance
				float3 vNextPos = m_aPoints[aIn[i]].vPos;

				while(aOut.size() > 3)
				{
					float fPrevDist = SMVector3Length2(vNextPos - m_aPoints[aOut[aOut.size() - 1]].vPos);
					float fNextDist = SMVector3Length2(vNextPos - m_aPoints[aOut[aOut.size() - 2]].vPos);

					if(fPrevDist > fNextDist)
					{
						aOut.erase(aOut.size() - 1);
					}
					else
					{
						break;
					}
				}

				uLastPt = aOut[aOut.size() - 1];
			}*/
#endif
			
			if(uCurPt == 0 && !isFirst)
			{
				break;
			}

			uCurPt = uNextPt;
			isFirst = false;
		}
	}

	assert(aOut.size() > 2);
	if(aOut.size() <= 2)
	{
		m_isContoursFailed = true;
		return;
	}

	// build the rest

	UINT uStart = aIn.indexOf(aOut[aOut.size() - 1]);
	UINT i = uStart;
	Array<UINT> aTemp;
	aTemp.push_back(aIn[i]);
	do
	{
		i = (i + 1) % aIn.size();

		if(aOut.indexOf(aIn[i]) >= 0)
		{
			if(aTemp.size() < 2)
			{
				aTemp.clearFast();
			}
			aTemp.push_back(aIn[i]);

			if(aTemp.size() > 2)
			{
				aTemp.quickSort();
				buildContoursRecursive(aTemp, uCurDepth + 1);
				if(m_isContoursFailed)
				{
					return;
				}
				aTemp.clearFast();
				aTemp.push_back(aIn[i]);
			}
		}
		else
		{
			aTemp.push_back(aIn[i]);
		}
	}
	while(i != uStart);


	m_aContours[m_aContours.size()].setPoints(aOut);

#if 0
	return;

	// build aOut;
	// append aOut to m_aContours
	// buildContoursRecursive for rest contours

	aOut.push_back(aIn[0]);
	aOut.push_back(aIn[1]);

	float3 vFirst = m_aPoints[aIn[1]].vPos - m_aPoints[aIn[0]].vPos;

	for(UINT i = 2, l = aIn.size(); i < l; ++i)
	{
		float3 vBack = m_aPoints[aOut[aOut.size() - 2]].vPos - m_aPoints[aOut[aOut.size() - 1]].vPos;
		float3 vFront = m_aPoints[aIn[i]].vPos - m_aPoints[aOut[aOut.size() - 1]].vPos;

		//bool isGap = false;
		//if(SMRightAngleBetweenVectors(vBack, vFront, m_vNormal) < SM_PI && SMRightAngleBetweenVectors(m_aPoints[aIn[i]].vPos - m_aPoints[aIn[0]].vPos, vFirst, m_vNormal) < SM_PI)
		if(canUsePoint(aIn, i, aIn.indexOf(aOut[aOut.size() - 2]), aIn.indexOf(aOut[aOut.size() - 1])))
		{
			/*while(aOut.size() && SMRightAngleBetweenVectors(m_aPoints[aIn[i]].vPos - m_aPoints[aOut[aOut.size() - 1]].vPos, m_aPoints[aIn[i]].vPos - m_aPoints[aIn[0]].vPos, m_vNormal) >= SM_PI)
			{
				assert(!"Never goes here!");
				isGap = true;
				aOut.erase(aOut.size() - 1);
			}
			
			if(isGap)
			{
				vBack = m_aPoints[aOut[aOut.size() - 2]].vPos - m_aPoints[aOut[aOut.size() - 1]].vPos;
				--i;
			}
			else
			{*/
				aOut.push_back(aIn[i]);
			/*}*/
		}
		/*else // gap found
		{
			isGap = true;
		}*/

		/*if(isGap)*/
		{
			// let's find next point
			UINT uLastPt = aOut[aOut.size() - 1];
			bool isFound = false;
			UINT uFoundPt = UINT_MAX;

			bool wasTailMinimized = false;

			for(i = i + 1; i < l; ++i)
			{
				vFront = m_aPoints[aIn[i]].vPos - m_aPoints[uLastPt].vPos;
				//if(SMRightAngleBetweenVectors(vBack, vFront, m_vNormal) < SM_PI && SMRightAngleBetweenVectors(m_aPoints[aIn[i]].vPos - m_aPoints[aIn[0]].vPos, vFirst, m_vNormal) < SM_PI)
				if(canUsePoint(aIn, i, aIn.indexOf(aOut[aOut.size() - 2]), aIn.indexOf(aOut[aOut.size() - 1])))
				{
					isFound = true;
					uFoundPt = i;
					if(!wasTailMinimized)
					{
						wasTailMinimized = true;
						// step back to minimize distance
						float3 vNextPos = m_aPoints[aIn[i]].vPos;

						while(aOut.size() > 3)
						{
							float fPrevDist = SMVector3Length2(vNextPos - m_aPoints[aOut[aOut.size() - 1]].vPos);
							float fNextDist = SMVector3Length2(vNextPos - m_aPoints[aOut[aOut.size() - 2]].vPos);

							if(fPrevDist > fNextDist)
							{
								aOut.erase(aOut.size() - 1);
							}
							else
							{
								break;
							}
						}

						uLastPt = aOut[aOut.size() - 1];
					}

					if(i + 2 < l) // +1 ???
					{
						float fPrevDist = SMVector3Length2(m_aPoints[aIn[i]].vPos - m_aPoints[uLastPt].vPos);
						float fNextDist = SMVector3Length2(m_aPoints[aIn[i + 1]].vPos - m_aPoints[uLastPt].vPos);

						if(fPrevDist > fNextDist)
						{
							continue;
						}
					}

					break;
				}
				else if(isFound)
				{
					break;
				}
			}

			if(isFound)
			{
				aOut.push_back(aIn[uFoundPt]);
				i = uFoundPt;
			}
		}
	}

	assert(aOut[aOut.size() - 1] == aOut[0]);
	assert(aOut.size() > 3);

	// cycle minimization
	if(aOut.size() > 1 && aOut[aOut.size() - 1] - aOut[aOut.size() - 2] != 1 && !(aOut[aOut.size() - 1] == 0 && aOut[aOut.size() - 2] == aIn[aIn.size() - 2]))
	{
		aOut.erase(aOut.size() - 1);

		while(aOut.size() > 3)
		{
			float fPrevDist = SMVector3Length2(m_aPoints[aOut[0]].vPos - m_aPoints[aOut[aOut.size() - 1]].vPos);
			float fNextDist = SMVector3Length2(m_aPoints[aOut[1]].vPos - m_aPoints[aOut[aOut.size() - 1]].vPos);
			if(fPrevDist > fNextDist)
			{
				aOut.erase(0);
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		aOut.erase(aOut.size() - 1);
	}

	// build the rest

	m_aContours[m_aContours.size()].setPoints(aOut);
#endif
}

void COutline::fixVertexOrder()
{
	float3 vDir = SMVector3Normalize(m_aPoints[0].vPos - m_aPoints[1].vPos);

	UINT uBestIdx = UINT_MAX;
	float fBestProj = -FLT_MAX;
	for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
	{
		float fProj = SMVector3Dot(m_aPoints[i].vPos, vDir);
		if(fProj > fBestProj)
		{
			fBestProj = fProj;
			uBestIdx = i;
		}
	}
	assert(uBestIdx != UINT_MAX);

	UINT uNextIdx = (uBestIdx + 1) % m_aPoints.size();
	UINT uPrevIdx = (uBestIdx == 0 ? m_aPoints.size() : uBestIdx) - 1;

	float fAngNext = SMRightAngleBetweenVectors(vDir, m_aPoints[uNextIdx].vPos - m_aPoints[uBestIdx].vPos, m_vNormal);
	float fAngPrev = SMRightAngleBetweenVectors(vDir, m_aPoints[uPrevIdx].vPos - m_aPoints[uBestIdx].vPos, m_vNormal);

	if(fAngPrev > fAngNext)
	{
		Array<PathPoint> aPoints(m_aPoints.size());
		for(UINT i = 0, l = m_aPoints.size(); i < l; ++i)
		{
			aPoints[l - i - 1] = m_aPoints[i];
		}

		m_aPoints.swap(aPoints);
	}
}

UINT COutline::getContourCount() const
{
	assert(m_isClosed);

	return(m_aContours.size());
}
const CContour& COutline::getContour(UINT idx) const
{
	assert(m_isClosed && idx < m_aContours.size());

	return(m_aContours[idx]);
}

UINT COutline::getPointCount() const
{
	return(m_aPoints.size());
}
const float3_t& COutline::getPoint(UINT idx) const
{
	assert(idx < m_aPoints.size());

	return(m_aPoints[idx].vPos);
}

const float3_t& COutline::getNormal() const
{
	return(m_vNormal);
}

//##########################################################################

void XMETHODCALLTYPE COutlinePointCallback::moveTo(const float3 &vNewPos, IXEditorGizmoHandle *pGizmo)
{
	if(m_isFirstMov)
	{
		m_isFirstMov = false;
	}
	else
	{
		m_isMoved = true;
		m_pOutline->onHandleMove(vNewPos, pGizmo);
	}
}
void XMETHODCALLTYPE COutlinePointCallback::onStart(IXEditorGizmoHandle *pGizmo)
{
	m_isMoved = false;
	m_isFirstMov = true;
}
void XMETHODCALLTYPE COutlinePointCallback::onEnd(IXEditorGizmoHandle *pGizmo)
{
	if(!m_isMoved)
	{
		m_pOutline->onHandleClick(pGizmo);
	}
}
