#include "ParticleEffectEmitterShapeData.h"

void CParticleEffectEmitterShapeData::evaluate(float fTime, float3 *pvPos, float3 *pvDir, UINT uCountInGen, UINT uIdInGen)
{
	switch(m_shape)
	{
	case XPS_SPHERE:
		evalSphere(m_fRadius, m_fRadiusThickness, false, pvPos, pvDir);
		break;

	case XPS_HEMISPHERE:
		evalSphere(m_fRadius, m_fRadiusThickness, true, pvPos, pvDir);
		break;

	case XPS_CONE:
		{
			float fThetha = evalArc(SM_2PI * m_fArc, fTime, uCountInGen, uIdInGen);
			float3 vTopPos = float3(cosf(fThetha), 0.0f, sinf(fThetha)) * randf(m_fRadius * (1.0f - m_fRadiusThickness), m_fRadius);
			vTopPos.y = m_fLength;

			bool isCylinder = SMIsZero(m_fAngle);

			float3 vStartPos;
			if(isCylinder)
			{
				// cylinder
				*pvDir = float3(0.0f, 1.0f, 0.0f);
			}
			else
			{
				// cone
				float fH0 = m_fRadius / tanf(SMToRadian(m_fAngle));

				float3 vBasePos(0.0f, m_fLength - fH0, 0.0f);

				*pvDir = SMVector3Normalize(vTopPos - vBasePos);

				if(fH0 > m_fLength)
				{
					vStartPos = (vTopPos - vBasePos) * ((fH0 - m_fLength) / fH0) + vBasePos;
				}
				else
				{
					vStartPos = float3(0.0f, m_fLength - fH0, 0.0f);
				}
			}

			switch(m_coneEmitFrom)
			{
			case XPSCEF_BASE:
				if(isCylinder)
				{
					*pvPos = vTopPos;
					pvPos->y = 0.0f;
				}
				else
				{
					*pvPos = vStartPos;
				}
				break;

			case XPSCEF_VOLUME:
				if(isCylinder)
				{
					*pvPos = vTopPos;
					pvPos->y *= randf(0.0f, 1.0f);
				}
				else
				{
					*pvPos = (vTopPos - vStartPos) * randf(0.0f, 1.0f) + vStartPos;
				}
				break;

			default:
				assert(!"Unknown cone emit from");
			}
		}
		break;

	case XPS_BOX:
		*pvDir = float3(0.0f, 1.0f, 0.0f);

		switch(m_boxEmitFrom)
		{
		case XPSBEF_EDGE:
			{
				float afLengths[12];
				afLengths[0] = afLengths[1] = afLengths[2] = afLengths[3] = m_vSize.x;
				afLengths[4] = afLengths[5] = afLengths[6] = afLengths[7] = m_vSize.y;
				afLengths[8] = afLengths[9] = afLengths[10] = afLengths[11] = m_vSize.z;

				float fMax = (m_vSize.x + m_vSize.y + m_vSize.z) * 4.0f;
				float fRand = randf(0.0f, fMax);
				int i = 0;
				for(; i < ARRAYSIZE(afLengths) - 1 && afLengths[i] < fRand; ++i)
				{
					fRand -= afLengths[i];
				}

				switch(i & 0xFC)
				{
				case 0: // x
					*pvPos = float3(randf(-m_vSize.x, m_vSize.x), (i & 1) ? m_vSize.y : -m_vSize.y, (i & 2) ? m_vSize.z : -m_vSize.z) * 0.5f;
					break;
				case 4: // y
					*pvPos = float3((i & 1) ? m_vSize.x : -m_vSize.x, randf(-m_vSize.y, m_vSize.y), (i & 2) ? m_vSize.z : -m_vSize.z) * 0.5f;
					break;
				case 8: // z
					*pvPos = float3((i & 1) ? m_vSize.x : -m_vSize.x, (i & 2) ? m_vSize.y : -m_vSize.y, randf(-m_vSize.z, m_vSize.z)) * 0.5f;
					break;
				}
			}
			break;

		case XPSBEF_SHELL:
			{
				float afAreas[6];
				afAreas[0] = afAreas[1] = m_vSize.x * m_vSize.y;
				afAreas[2] = afAreas[3] = m_vSize.z * m_vSize.y;
				afAreas[4] = afAreas[5] = m_vSize.x * m_vSize.z;

				float fMax = (afAreas[0] + afAreas[2] + afAreas[4]) * 2.0f;
				float fRand = randf(0.0f, fMax);
				int i = 0;
				for(; i < ARRAYSIZE(afAreas) - 1 && afAreas[i] < fRand; ++i)
				{
					fRand -= afAreas[i];
				}

				switch(i & 0xFE)
				{
				case 0:
					*pvPos = float3(randf(-m_vSize.x, m_vSize.x), (i & 1) ? m_vSize.y : -m_vSize.y, randf(-m_vSize.z, m_vSize.z)) * 0.5f;
					break;
				case 2:
					*pvPos = float3((i & 1) ? m_vSize.x : -m_vSize.x, randf(-m_vSize.y, m_vSize.y), randf(-m_vSize.z, m_vSize.z)) * 0.5f;
					break;
				case 4:
					*pvPos = float3(randf(-m_vSize.x, m_vSize.x), randf(-m_vSize.y, m_vSize.y), (i & 1) ? m_vSize.z : -m_vSize.z) * 0.5f;
					break;
				}
			}
			break;

		case XPSBEF_VOLUME:
			*pvPos = float3(randf(-m_vSize.x, m_vSize.x), randf(-m_vSize.y, m_vSize.y), randf(-m_vSize.z, m_vSize.z)) * 0.5f;
			break;

		default:
			assert(!"Unknown box emit from");
		}
		break;

	case XPS_CIRCLE:
		{
			float fThetha = evalArc(SM_2PI * m_fArc, fTime, uCountInGen, uIdInGen);

			*pvPos = float3(cosf(fThetha), 0.0f, sinf(fThetha)) * randf(m_fRadius * (1.0f - m_fRadiusThickness), m_fRadius);
			//*pvDir = SMVector3Normalize(*pvPos);
			*pvDir = float3(0.0f, 1.0f, 0.0f);
		}
		break;

	case XPS_EDGE:
		{
			float fZ = evalArc(m_fRadius, fTime, uCountInGen, uIdInGen);
						
			*pvPos = float3(0.0f, 0.0f, fZ);
			*pvDir = float3(0.0f, 1.0f, 0.0f);
		}
		break;

	case XPS_TORUS:
		{
			float fThetha = evalArc(SM_2PI * m_fArc, fTime, uCountInGen, uIdInGen);

			float3 vSliceOrigin = float3(cosf(fThetha), 0.0f, sinf(fThetha)) * m_fRadius;

			float fInnerAngle = randf(0.0f, SM_2PI);
			float2 vInnerPos = float2(cosf(fInnerAngle), sinf(fInnerAngle)) * randf(m_fDonutRadius * (1.0f - m_fRadiusThickness), m_fDonutRadius);

			*pvPos = (vInnerPos.x * SMVector3Normalize(vSliceOrigin)) + float3(0.0f, vInnerPos.y, 0.0f);
			*pvDir = SMVector3Normalize(*pvPos);
			*pvPos += vSliceOrigin;
		}
		break;

	case XPS_RECTANGLE:
		*pvDir = float3(0.0f, 1.0f, 0.0f);
		*pvPos = float3(randf(-m_vSize.x, m_vSize.x), 0.0f, randf(-m_vSize.z, m_vSize.z)) * 0.5f;
		break;

	default:
		assert(!"Unknown shape");
		*pvPos = 0.0f;
		*pvDir = float3(0.0f, 1.0f, 0.0f);
	}

	if(m_fRandomizeDirection > SM_EPSILON)
	{
		float3_t vec = randv();
		*pvDir = SMVector3Normalize(vlerp(*pvDir, vec, m_fRandomizeDirection));
	}

	if(m_fSpherizeDirection > SM_EPSILON)
	{
		float3_t vec = SMVector3Normalize(*pvPos);
		*pvDir = SMVector3Normalize(vlerp(*pvDir, vec, m_fSpherizeDirection));
	}

	if(m_fRandomizePosition > SM_EPSILON)
	{
		*pvPos += randv() * m_fRandomizePosition;
	}
}

void CParticleEffectEmitterShapeData::evalSphere(float fRadius, float fRadiusThickness, bool isSemi, float3 *pvPos, float3 *pvDir)
{
	float3_t dir = randv();
	if(isSemi && dir.y < 0.0f)
	{
		dir.y = -dir.y;
	}
	*pvDir = dir;
	*pvPos = *pvDir * randf(fRadius - fRadius * fRadiusThickness, fRadius);
}

float CParticleEffectEmitterShapeData::evalArc(float fMaxAngle, float fTime, UINT uCountInGen, UINT uIdInGen)
{
	float fThetha = 0.0f;

	switch(m_arcMode)
	{
	case XPSAM_RANDOM:
		fThetha = randf(0.0f, fMaxAngle);
		break;

	case XPSAM_LOOP:
		// FIXME Кривую необходимо интегрировать для получения корректного значения
		fThetha = fmodf(fTime * m_curveArcSpeed.evaluate(fTime), fMaxAngle);
		break;

	case XPSAM_PING_PONG:
		// FIXME Кривую необходимо интегрировать для получения корректного значения
		fThetha = ((1.0f - cosf(fTime * m_curveArcSpeed.evaluate(fTime))) * 0.5f + 0.5f) * fMaxAngle;
		break;

	case XPSAM_BURST_SPREAD:
		// FIXME Кривую необходимо интегрировать для получения корректного значения
		fThetha = fmodf(fTime * m_curveArcSpeed.evaluate(fTime), fMaxAngle);
		assert(uCountInGen != 0);

		if(uCountInGen > 1)
		{
			float fStep = fMaxAngle / (float)(uCountInGen - 1);
			fThetha = fmodf(fThetha + fStep * (float)uIdInGen, fMaxAngle);
		}
		break;
	}

	if(!SMIsZero(m_fArcSpread))
	{
		fThetha -= fmodf(fThetha, m_fRadius * m_fArcSpread);
	}

	return(fThetha);
}
