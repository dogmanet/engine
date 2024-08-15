#include "EffectLoader.h"

CEffectLoader::CEffectLoader(IXCore *pCore):
	m_pCore(pCore)
{
}

static char* WriteCurve(char* szCur, IXAnimationCurve *pCurve)
{
	// {"keys":[[fTime,fValue,fInTangent,fOutTangent],...],"pre":mode,"post":mode}
	szCur += sprintf(szCur, "{\"keys\":[");
	const XKeyframe *pFrame;
	for(UINT i = 0, l = pCurve->getKeyframeCount(); i < l; ++i)
	{
		pFrame = pCurve->getKeyAt(i);
		szCur += sprintf(szCur, "%s[%g,%g,%g,%g,%u,%g,%g]", i == 0 ? "" : ",", pFrame->fTime, pFrame->fValue, pFrame->fInTangent, pFrame->fOutTangent, pFrame->weightedMode, pFrame->fInWeight, pFrame->fOutWeight);
	}
	szCur += sprintf(szCur, "],\"pre\":%d,\"post\":%d}", pCurve->getPreWrapMode(), pCurve->getPostWrapMode());

	return(szCur);
}

static void ReadCurve(IXJSONObject *pObj, IXAnimationCurve *pCurve)
{
	if(!pObj)
	{
		return;
	}

	for(UINT i = 0, l = pObj->size(); i < l; ++i)
	{
		const char *szKey = pObj->getKey(i);
		if(!strcmp(szKey, "keys"))
		{
			IXJSONArray *pArr = pObj->at(i)->asArray();
			if(pArr)
			{
				pCurve->setKeyCount(pArr->size());
				XKeyframe keyFrame;
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					IXJSONArray *pKeyArr = pArr->at(j)->asArray();
					if(pKeyArr)
					{
						keyFrame = {};
						pKeyArr->at(0) && pKeyArr->at(0)->getFloat(&keyFrame.fTime);
						pKeyArr->at(1) && pKeyArr->at(1)->getFloat(&keyFrame.fValue);
						pKeyArr->at(2) && pKeyArr->at(2)->getFloat(&keyFrame.fInTangent);
						pKeyArr->at(3) && pKeyArr->at(3)->getFloat(&keyFrame.fOutTangent);
						int64_t i64WeightedMode = 0;
						pKeyArr->at(4) && pKeyArr->at(4)->getInt64(&i64WeightedMode);
						keyFrame.weightedMode = (XKEYFRAME_WEIGHTED_MODE)i64WeightedMode;
						pKeyArr->at(5) && pKeyArr->at(5)->getFloat(&keyFrame.fInWeight);
						pKeyArr->at(6) && pKeyArr->at(6)->getFloat(&keyFrame.fOutWeight);

						pCurve->setKey(j, keyFrame);
					}
				}
			}
		}
		else if(!strcmp(szKey, "pre"))
		{
			int64_t i64Val;
			if(pObj->at(i)->getInt64(&i64Val))
			{
				pCurve->setPreWrapMode((XCURVE_WRAP_MODE)i64Val);
			}
		}
		else if(!strcmp(szKey, "post"))
		{
			int64_t i64Val;
			if(pObj->at(i)->getInt64(&i64Val))
			{
				pCurve->setPostWrapMode((XCURVE_WRAP_MODE)i64Val);
			}
		}
	}
}

static void SaveMinMaxCurve(IXConfig *pConfig, const char *szSection, const char *szKey, IXMinMaxCurve *pCurve)
{
	char *szVal, *szCur;
	switch(pCurve->getMode())
	{
	case XMCM_CONSTANT:
		pConfig->setFloat(szSection, szKey, pCurve->getMax());
		break;
	case XMCM_TWO_CONSTANTS:
		pConfig->setVector2(szSection, szKey, float2_t(pCurve->getMin(), pCurve->getMax()));
		break;

	case XMCM_CURVE:
		szVal = szCur = (char*)alloca(sizeof(char) * (256 * (pCurve->getMaxCurve()->getKeyframeCount() + 1) + 192));
		// 256 * (frames + 1) + 64
		// {"max":{"keys":[[fTime,fValue,fInTangent,fOutTangent],...],"pre":mode,"post":mode},"m":fMax,"n":fMax}
		szCur += sprintf(szCur, "{\"max\":");
		szCur = WriteCurve(szCur, pCurve->getMaxCurve());
		szCur += sprintf(szCur, ",\"m\":%g,\"n\":%g}", pCurve->getMax(), pCurve->getMin());
		pConfig->set(szSection, szKey, szVal);
		break;

	case XMCM_TWO_CURVES:
		szVal = szCur = (char*)alloca(sizeof(char) * (256 * (pCurve->getMinCurve()->getKeyframeCount() + pCurve->getMaxCurve()->getKeyframeCount() + 1) + 192));
		// 256 * (frames + 1) + 128
		// {"min":{...},"max":{"keys":[[fTime,fValue,fInTangent,fOutTangent],...],"pre":mode,"post":mode},"m":fMax,"n":fMax}
		szCur += sprintf(szCur, "{\"min\":");
		szCur = WriteCurve(szCur, pCurve->getMinCurve());
		szCur += sprintf(szCur, ",\"max\":");
		szCur = WriteCurve(szCur, pCurve->getMaxCurve());
		szCur += sprintf(szCur, ",\"m\":%g,\"n\":%g}", pCurve->getMax(), pCurve->getMin());
		pConfig->set(szSection, szKey, szVal);
		break;
	}
}

static void ReadMinMaxCurve(IXConfig *pConfig, const char *szSection, const char *szKey, IXMinMaxCurve *pCurve)
{
	float2_t vec;
	float fVal;
	IXJSONObject *pObj;
	if(pConfig->tryGetVector2(szSection, szKey, &vec))
	{
		pCurve->setMode(XMCM_TWO_CONSTANTS);
		pCurve->setMin(vec.x);
		pCurve->setMax(vec.y);
		return;
	}

	if(pConfig->tryGetFloat(szSection, szKey, &fVal))
	{
		pCurve->setMode(XMCM_CONSTANT);
		pCurve->setMax(fVal);
		return;
	}

	if(pConfig->tryGetJsonObject(szSection, szKey, &pObj))
	{
		int iCurves = 0;
		for(UINT i = 0, l = pObj->size(); i < l; ++i)
		{
			const char *szKey = pObj->getKey(i);
			if(!strcmp(szKey, "m"))
			{
				if(pObj->at(i)->getFloat(&fVal))
				{
					pCurve->setMax(fVal);
				}
				continue;
			}

			if(!strcmp(szKey, "n"))
			{
				if(pObj->at(i)->getFloat(&fVal))
				{
					pCurve->setMin(fVal);
				}
				continue;
			}
			
			if(!strcmp(szKey, "min"))
			{
				ReadCurve(pObj->at(i)->asObject(), pCurve->getMinCurve());
				++iCurves;
			}
			else if(!strcmp(szKey, "max"))
			{
				ReadCurve(pObj->at(i)->asObject(), pCurve->getMaxCurve());
				++iCurves;
			}
		}

		if(iCurves == 1)
		{
			pCurve->setMode(XMCM_CURVE);
		}
		else if(iCurves == 2)
		{
			pCurve->setMode(XMCM_TWO_CURVES);
		}

		mem_release(pObj);
	}
}

static char* WriteGradient(char* szCur, IXColorGradient *pGradient)
{
	// {"c":[[fTime,vColor],...],"a":[[fTime,fAlpha],...]}
	szCur += sprintf(szCur, "{\"c\":[");
	const XColorKey *pColor;
	for(UINT i = 0, l = pGradient->getColorKeyCount(); i < l; ++i)
	{
		pColor = pGradient->getColorKeyAt(i);
		szCur += sprintf(szCur, "%s[%g,%g,%g,%g]", 
			i == 0 ? "" : ",", pColor->fTime, 
			pColor->vColor.x, pColor->vColor.y, pColor->vColor.z
		);
	}
	szCur += sprintf(szCur, "],\"a\":[");

	const XAlphaKey *pAlpha;
	for(UINT i = 0, l = pGradient->getAlphaKeyCount(); i < l; ++i)
	{
		pAlpha = pGradient->getAlphaKeyAt(i);
		szCur += sprintf(szCur, "%s[%g,%g]",
			i == 0 ? "" : ",", pAlpha->fTime,
			pAlpha->fAlpha
		);
	}

	szCur += sprintf(szCur, "]}");

	return(szCur);
}

static void ReadGradient(IXJSONObject *pObj, IXColorGradient *pGradient)
{
	if(!pObj)
	{
		return;
	}

	for(UINT i = 0, l = pObj->size(); i < l; ++i)
	{
		const char *szKey = pObj->getKey(i);
		if(!strcmp(szKey, "c"))
		{
			IXJSONArray *pArr = pObj->at(i)->asArray();
			if(pArr)
			{
				pGradient->setColorKeyCount(pArr->size());
				XColorKey keyFrame;
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					IXJSONArray *pKeyArr = pArr->at(j)->asArray();
					if(pKeyArr)
					{
						keyFrame = {};
						pKeyArr->at(0) && pKeyArr->at(0)->getFloat(&keyFrame.fTime);
						pKeyArr->at(1) && pKeyArr->at(1)->getFloat(&keyFrame.vColor.x);
						pKeyArr->at(2) && pKeyArr->at(2)->getFloat(&keyFrame.vColor.y);
						pKeyArr->at(3) && pKeyArr->at(3)->getFloat(&keyFrame.vColor.z);
						pGradient->setColorKey(j, keyFrame);
					}
				}
			}
		}
		else if(!strcmp(szKey, "a"))
		{
			IXJSONArray *pArr = pObj->at(i)->asArray();
			if(pArr)
			{
				pGradient->setAlphaKeyCount(pArr->size());
				XAlphaKey keyFrame;
				for(UINT j = 0, jl = pArr->size(); j < jl; ++j)
				{
					IXJSONArray *pKeyArr = pArr->at(j)->asArray();
					if(pKeyArr)
					{
						keyFrame = {};
						pKeyArr->at(0) && pKeyArr->at(0)->getFloat(&keyFrame.fTime);
						pKeyArr->at(1) && pKeyArr->at(1)->getFloat(&keyFrame.fAlpha);
						pGradient->setAlphaKey(j, keyFrame);
					}
				}
			}
		}
	}
}

static void SaveTwoGradients(IXConfig *pConfig, const char *szSection, const char *szKey, IX2ColorGradients *pGradient)
{
	char *szVal, *szCur;
	size_t sizeMax;
	switch(pGradient->getMode())
	{
	case X2CGM_COLOR:
		pConfig->setVector4(szSection, szKey, pGradient->getColor());
		break;
	case X2CGM_TWO_COLORS:
		sizeMax = sizeof(char) * 256;
		szVal = szCur = (char*)alloca(sizeMax);
		{
			float4_t vC0 = pGradient->getColor0();
			float4_t vC1 = pGradient->getColor1();
			szCur += sprintf(szCur, "[[%g,%g,%g,%g],[%g,%g,%g,%g]]",
				vC0.x, vC0.y, vC0.z, vC0.w,
				vC1.x, vC1.y, vC1.z, vC1.w
			);
		}
		pConfig->set(szSection, szKey, szVal);
		assert(szCur - szVal < sizeMax);
		break;

	case X2CGM_GRADIENT:
		sizeMax = sizeof(char) * (128 * pGradient->getGradient0()->getColorKeyCount() + 32 * pGradient->getGradient0()->getAlphaKeyCount() + 128);
		szVal = szCur = (char*)alloca(sizeMax);
		// {"0":{"c":[[fTime,vColor],...],"a":[[fTime,fAlpha],...]}}
		szCur += sprintf(szCur, "{\"0\":");
		szCur = WriteGradient(szCur, pGradient->getGradient0());
		szCur += sprintf(szCur, "}");
		pConfig->set(szSection, szKey, szVal);
		assert(szCur - szVal < sizeMax);
		break;

	case X2CGM_TWO_GRADIENTS:
		sizeMax = sizeof(char) * (128 * pGradient->getGradient0()->getColorKeyCount() * 2 + 32 * pGradient->getGradient0()->getAlphaKeyCount() * 2 + 128);
		szVal = szCur = (char*)alloca(sizeMax);
		// {"0":{...},"1":{...}}
		szCur += sprintf(szCur, "{\"0\":");
		szCur = WriteGradient(szCur, pGradient->getGradient0());
		szCur += sprintf(szCur, ",\"1\":");
		szCur = WriteGradient(szCur, pGradient->getGradient1());
		szCur += sprintf(szCur, "}");
		pConfig->set(szSection, szKey, szVal);
		assert(szCur - szVal < sizeMax);
		break;
	}
}

static void ReadTwoGradients(IXConfig *pConfig, const char *szSection, const char *szKey, IX2ColorGradients *pGradient)
{
	float4_t vec;
	float fVal;
	IXJSONObject *pObj;
	IXJSONArray *pArr, *pArr2;
	if(pConfig->tryGetVector4(szSection, szKey, &vec))
	{
		pGradient->setMode(X2CGM_COLOR);
		pGradient->setColor(vec);
		return;
	}

	if(pConfig->tryGetJsonArray(szSection, szKey, &pArr))
	{
		pGradient->setMode((pArr->size() > 1) ? X2CGM_TWO_COLORS : X2CGM_COLOR);
		for(UINT i = 0, l = pArr->size(); i < l && i < 2; ++i)
		{
			if(pArr->at(i) && (pArr2 = pArr->at(i)->asArray()))
			{
				pArr2->at(0) && pArr2->at(0)->getFloat(&vec.x);
				pArr2->at(1) && pArr2->at(1)->getFloat(&vec.y);
				pArr2->at(2) && pArr2->at(2)->getFloat(&vec.z);
				pArr2->at(3) && pArr2->at(3)->getFloat(&vec.w);

				if(i == 0)
				{
					pGradient->setColor0(vec);
				}
				else
				{
					pGradient->setColor1(vec);
				}
			}
		}
		return;
	}

	if(pConfig->tryGetJsonObject(szSection, szKey, &pObj))
	{
		UINT uGrads = 0;
		for(UINT i = 0, l = pObj->size(); i < l && i < 2; ++i)
		{
			const char *szKey = pObj->getKey(i);

			if(!strcmp(szKey, "0"))
			{
				ReadGradient(pObj->at(i)->asObject(), pGradient->getGradient0());
				++uGrads;
			}
			else if(!strcmp(szKey, "1"))
			{
				ReadGradient(pObj->at(i)->asObject(), pGradient->getGradient1());
				++uGrads;
			}
		}

		if(uGrads == 1)
		{
			pGradient->setMode(X2CGM_GRADIENT);
		}
		else if(uGrads == 2)
		{
			pGradient->setMode(X2CGM_TWO_GRADIENTS);
		}

		mem_release(pObj);
	}
}


bool CEffectLoader::loadFromFile(const char *szFile, CParticleEffect *pEffect)
{
	IXConfig *pConfig = m_pCore->newConfig();
	bool isLoaded = false;
	if(pConfig->open(szFile))
	{
		UINT uEmitterCount;
		if(pConfig->tryGetUint("effect", "emitters", &uEmitterCount) && uEmitterCount < 1000)
		{
			pEffect->setEmitterCount(uEmitterCount);
			char szSection[64], szKey[64];
			float fVal;
			int iVal;
			UINT uVal;
			bool bVal;
			float2_t v2Val;
			float3_t v3Val;
			float4_t vVal;

#define READ_VAL(mod, what, data, type, var, etype) if(pConfig->tryGet##type(szSection, mod "." #what, &var)){data->set##what(etype var);}
#define READ_FLOAT(mod, what, data) READ_VAL(mod, what, data, Float, fVal, _VOID)
#define READ_INT(mod, what, data, type) READ_VAL(mod, what, data, Int, iVal, (type))
#define READ_UINT(mod, what, data) READ_VAL(mod, what, data, Uint, uVal, _VOID)
#define READ_BOOL(mod, what, data) READ_VAL(mod, what, data, Bool, bVal, _VOID)
#define READ_V2(mod, what, data) READ_VAL(mod, what, data, Vector2, v2Val, _VOID)
#define READ_V3(mod, what, data) READ_VAL(mod, what, data, Vector3, v3Val, _VOID)
#define READ_V4(mod, what, data) READ_VAL(mod, what, data, Vector4, vVal, _VOID)

			for(UINT i = 0; i < uEmitterCount; ++i)
			{
				sprintf(szSection, "emitter_%u", i);

				IXParticleEffectEmitter *pEmitter = pEffect->getEmitterAt(i);

				const char *szName = pConfig->getKey(szSection, "name");
				if(szName)
				{
					pEmitter->setName(szName);
				}

				auto *pGenericData = pEmitter->getGenericData();
				READ_FLOAT("generic", Duration, pGenericData);
				READ_BOOL("generic", Looping, pGenericData);
				READ_BOOL("generic", Prewarm, pGenericData);
				READ_FLOAT("generic", StartDelay, pGenericData);
				ReadMinMaxCurve(pConfig, szSection, "generic.StartLifetimeCurve", pGenericData->getStartLifetimeCurve());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartSpeedCurve", pGenericData->getStartSpeedCurve());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartSizeCurve", pGenericData->getStartSizeCurve());
				READ_FLOAT("generic", FlipRotation, pGenericData);
				READ_INT("generic", CullingMode, pGenericData, XPARTICLE_CULLING_MODE);
				READ_FLOAT("generic", GravityModifier, pGenericData);
				READ_UINT("generic", MaxParticles, pGenericData);
				READ_INT("generic", RingBufferMode, pGenericData, XPARTICLE_RING_BUFFER_MODE);
				ReadMinMaxCurve(pConfig, szSection, "generic.RingBufferLoopRangeCurve", pGenericData->getRingBufferLoopRangeCurve());
				READ_INT("generic", SimulationSpace, pGenericData, XPARTICLE_SIMULATION_SPACE);
				ReadTwoGradients(pConfig, szSection, "generic.StartColor", pGenericData->getStartColorGradient());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartRotationCurve", pGenericData->getStartRotationCurve());
				READ_BOOL("generic", StartRotationSeparate, pGenericData);
				ReadMinMaxCurve(pConfig, szSection, "generic.StartRotationXCurve", pGenericData->getStartRotationXCurve());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartRotationYCurve", pGenericData->getStartRotationYCurve());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartRotationZCurve", pGenericData->getStartRotationZCurve());
				READ_BOOL("generic", StartSizeSeparate, pGenericData);
				ReadMinMaxCurve(pConfig, szSection, "generic.StartSizeXCurve", pGenericData->getStartSizeXCurve());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartSizeYCurve", pGenericData->getStartSizeYCurve());
				ReadMinMaxCurve(pConfig, szSection, "generic.StartSizeZCurve", pGenericData->getStartSizeZCurve());

				auto *pEmissionData = pEmitter->getEmissionData();
				ReadMinMaxCurve(pConfig, szSection, "emission.RatePerSecondCurve", pEmissionData->getRatePerSecondCurve());
				ReadMinMaxCurve(pConfig, szSection, "emission.RatePerMeterCurve", pEmissionData->getRatePerMeterCurve());
				READ_UINT("emission", BurstsCount, pEmissionData);
				for(UINT j = 0, jl = pEmissionData->getBurstsCount(); j < jl; ++j)
				{
					IXParticleBurst *pBurst = pEmissionData->getBurstAt(j);
					sprintf(szKey, "emission.Burst_%u.Time", j);
					if(pConfig->tryGetFloat(szSection, szKey, &fVal))
					{
						pBurst->setTime(fVal);
					}
					sprintf(szKey, "emission.Burst_%u.Cycles", j);
					if(pConfig->tryGetUint(szSection, szKey, &uVal))
					{
						pBurst->setCycles(uVal);
					}
					sprintf(szKey, "emission.Burst_%u.Probability", j);
					if(pConfig->tryGetFloat(szSection, szKey, &fVal))
					{
						pBurst->setProbability(fVal);
					}
					sprintf(szKey, "emission.Burst_%u.IntervalCurve", j);
					ReadMinMaxCurve(pConfig, szSection, szKey, pBurst->getIntervalCurve());
					sprintf(szKey, "emission.Burst_%u.CountCurve", j);
					ReadMinMaxCurve(pConfig, szSection, szKey, pBurst->getCountCurve());
				}

				auto *pRenderData = pEmitter->getRenderData();
				const char *szRenderMaterial = pConfig->getKey(szSection, "render.Material");
				if(szRenderMaterial)
				{
					pRenderData->setMaterial(szRenderMaterial);
				}

				auto *pShapeData = pEmitter->getShapeData();
				if(pConfig->tryGetBool(szSection, "shape.Enabled", &bVal))
				{
					pShapeData->enable(bVal);
				}
				READ_INT("shape", Shape, pShapeData, XPARTICLE_SHAPE);
				READ_FLOAT("shape", Radius, pShapeData);
				READ_FLOAT("shape", RadiusThickness, pShapeData);
				READ_FLOAT("shape", Angle, pShapeData);
				READ_FLOAT("shape", Arc, pShapeData);
				READ_INT("shape", ArcMode, pShapeData, XPARTICLE_SHAPE_ARC_MODE);
				READ_FLOAT("shape", ArcSpread, pShapeData);
				ReadMinMaxCurve(pConfig, szSection, "shape.ArcSpeedCurve", pShapeData->getArcSpeedCurve());
				READ_FLOAT("shape", Length, pShapeData);
				READ_INT("shape", ConeEmitFrom, pShapeData, XPARTICLE_SHAPE_CONE_EMIT_FROM);
				READ_INT("shape", BoxEmitFrom, pShapeData, XPARTICLE_SHAPE_BOX_EMIT_FROM);
				READ_FLOAT("shape", DonutRadius, pShapeData);
				READ_BOOL("shape", AlignToDirection, pShapeData);
				READ_FLOAT("shape", RandomizeDirection, pShapeData);
				READ_FLOAT("shape", SpherizeDirection, pShapeData);
				READ_FLOAT("shape", RandomizePosition, pShapeData);
				READ_V3("shape", Size, pShapeData);

				auto *pVelocityLifetimeData = pEmitter->getVelocityLifetimeData();
				if(pConfig->tryGetBool(szSection, "velocityLifetime.Enabled", &bVal))
				{
					pVelocityLifetimeData->enable(bVal);
				}
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.LinearXCurve", pVelocityLifetimeData->getLinearXCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.LinearYCurve", pVelocityLifetimeData->getLinearYCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.LinearZCurve", pVelocityLifetimeData->getLinearZCurve());
				READ_INT("velocityLifetime", SimulationSpace, pVelocityLifetimeData, XPARTICLE_SIMULATION_SPACE);
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.OrbitalXCurve", pVelocityLifetimeData->getOrbitalXCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.OrbitalYCurve", pVelocityLifetimeData->getOrbitalYCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.OrbitalZCurve", pVelocityLifetimeData->getOrbitalZCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.OffsetXCurve", pVelocityLifetimeData->getOffsetXCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.OffsetYCurve", pVelocityLifetimeData->getOffsetYCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.OffsetZCurve", pVelocityLifetimeData->getOffsetZCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.RadialCurve", pVelocityLifetimeData->getRadialCurve());
				ReadMinMaxCurve(pConfig, szSection, "velocityLifetime.SpeedModifierCurve", pVelocityLifetimeData->getSpeedModifierCurve());

				auto *pLimitVelocityLifetimeData = pEmitter->getLimitVelocityLifetimeData();
				if(pConfig->tryGetBool(szSection, "limitVelocityLifetime.Enabled", &bVal))
				{
					pLimitVelocityLifetimeData->enable(bVal);
				}
				READ_BOOL("limitVelocityLifetime", SeparateAxes, pLimitVelocityLifetimeData);
				ReadMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.LimitCurveX", pLimitVelocityLifetimeData->getLimitXCurve());
				ReadMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.LimitCurveY", pLimitVelocityLifetimeData->getLimitYCurve());
				ReadMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.LimitCurveZ", pLimitVelocityLifetimeData->getLimitZCurve());
				ReadMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.DampenCurve", pLimitVelocityLifetimeData->getDampenCurve());
				ReadMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.DragCurve", pLimitVelocityLifetimeData->getDragCurve());
				READ_BOOL("limitVelocityLifetime", MultiplyBySize, pLimitVelocityLifetimeData);
				READ_BOOL("limitVelocityLifetime", MultiplyByVelocity, pLimitVelocityLifetimeData);

				IXParticleEffectEmitterForceLifetimeData *pForceLifetimeData = pEmitter->getForceLifetimeData();
				if(pConfig->tryGetBool(szSection, "forceLifetimeData.Enabled", &bVal))
				{
					pForceLifetimeData->enable(bVal);
				}
				ReadMinMaxCurve(pConfig, szSection, "forceLifetimeData.ForceXCurve", pForceLifetimeData->getForceXCurve());
				ReadMinMaxCurve(pConfig, szSection, "forceLifetimeData.ForceYCurve", pForceLifetimeData->getForceYCurve());
				ReadMinMaxCurve(pConfig, szSection, "forceLifetimeData.ForceZCurve", pForceLifetimeData->getForceZCurve());
				READ_INT("forceLifetimeData", SimulationSpace, pForceLifetimeData, XPARTICLE_SIMULATION_SPACE);
				READ_BOOL("forceLifetimeData", Randomize, pForceLifetimeData);

				IXParticleEffectEmitterSizeLifetimeData *pSizeLifetimeData = pEmitter->getSizeLifetimeData();
				if(pConfig->tryGetBool(szSection, "sizeLifetimeData.Enabled", &bVal))
				{
					pSizeLifetimeData->enable(bVal);
				}
				READ_BOOL("sizeLifetimeData", SeparateAxes, pSizeLifetimeData);
				ReadMinMaxCurve(pConfig, szSection, "sizeLifetimeData.SizeXCurve", pSizeLifetimeData->getSizeXCurve());
				ReadMinMaxCurve(pConfig, szSection, "sizeLifetimeData.SizeYCurve", pSizeLifetimeData->getSizeYCurve());
				ReadMinMaxCurve(pConfig, szSection, "sizeLifetimeData.SizeZCurve", pSizeLifetimeData->getSizeZCurve());

				IXParticleEffectEmitterSizeSpeedData *pSizeSpeedData = pEmitter->getSizeSpeedData();
				if(pConfig->tryGetBool(szSection, "sizeSpeedData.Enabled", &bVal))
				{
					pSizeSpeedData->enable(bVal);
				}
				READ_BOOL("sizeSpeedData", SeparateAxes, pSizeSpeedData);
				ReadMinMaxCurve(pConfig, szSection, "sizeSpeedData.SizeXCurve", pSizeSpeedData->getSizeXCurve());
				ReadMinMaxCurve(pConfig, szSection, "sizeSpeedData.SizeYCurve", pSizeSpeedData->getSizeYCurve());
				ReadMinMaxCurve(pConfig, szSection, "sizeSpeedData.SizeZCurve", pSizeSpeedData->getSizeZCurve());
				READ_V2("sizeSpeedData", SpeedRange, pSizeSpeedData);

				IXParticleEffectEmitterRotationLifetimeData *pRotationLifetimeData = pEmitter->getRotationLifetimeData();
				if(pConfig->tryGetBool(szSection, "rotationLifetimeData.Enabled", &bVal))
				{
					pRotationLifetimeData->enable(bVal);
				}
				READ_BOOL("rotationLifetimeData", SeparateAxes, pRotationLifetimeData);
				ReadMinMaxCurve(pConfig, szSection, "rotationLifetimeData.AngularVelocityXCurve", pRotationLifetimeData->getAngularVelocityXCurve());
				ReadMinMaxCurve(pConfig, szSection, "rotationLifetimeData.AngularVelocityYCurve", pRotationLifetimeData->getAngularVelocityYCurve());
				ReadMinMaxCurve(pConfig, szSection, "rotationLifetimeData.AngularVelocityZCurve", pRotationLifetimeData->getAngularVelocityZCurve());

				IXParticleEffectEmitterRotationSpeedData *pRotationSpeedData = pEmitter->getRotationSpeedData();
				if(pConfig->tryGetBool(szSection, "rotationSpeedData.Enabled", &bVal))
				{
					pRotationSpeedData->enable(bVal);
				}
				READ_BOOL("rotationSpeedData", SeparateAxes, pRotationSpeedData);
				ReadMinMaxCurve(pConfig, szSection, "rotationSpeedData.AngularVelocityXCurve", pRotationSpeedData->getAngularVelocityXCurve());
				ReadMinMaxCurve(pConfig, szSection, "rotationSpeedData.AngularVelocityYCurve", pRotationSpeedData->getAngularVelocityYCurve());
				ReadMinMaxCurve(pConfig, szSection, "rotationSpeedData.AngularVelocityZCurve", pRotationSpeedData->getAngularVelocityZCurve());
				READ_V2("rotationSpeedData", SpeedRange, pRotationSpeedData);

				IXParticleEffectEmitterLifetimeEmitterSpeedData *pLifetimeEmitterSpeedData = pEmitter->getLifetimeEmitterSpeedData();
				if(pConfig->tryGetBool(szSection, "lifetimeEmitterSpeedData.Enabled", &bVal))
				{
					pLifetimeEmitterSpeedData->enable(bVal);
				}
				ReadMinMaxCurve(pConfig, szSection, "lifetimeEmitterSpeedData.MultiplierCurve", pLifetimeEmitterSpeedData->getMultiplierCurve());
				READ_V2("lifetimeEmitterSpeedData", SpeedRange, pLifetimeEmitterSpeedData);

				IXParticleEffectEmitterColorLifetimeData *pColorLifetimeData = pEmitter->getColorLifetimeData();
				if(pConfig->tryGetBool(szSection, "colorLifetimeData.Enabled", &bVal))
				{
					pColorLifetimeData->enable(bVal);
				}
				ReadTwoGradients(pConfig, szSection, "colorLifetimeData.Color", pColorLifetimeData->getColor());

				IXParticleEffectEmitterColorSpeedData *pColorSpeedData = pEmitter->getColorSpeedData();
				if(pConfig->tryGetBool(szSection, "colorSpeedData.Enabled", &bVal))
				{
					pColorSpeedData->enable(bVal);
				}
				ReadTwoGradients(pConfig, szSection, "colorSpeedData.Color", pColorSpeedData->getColor());
				READ_V2("colorSpeedData", SpeedRange, pColorSpeedData);

				isLoaded = true;
			}

#undef READ_V4
#undef READ_V3
#undef READ_V2
#undef READ_BOOL
#undef READ_UINT
#undef READ_INT
#undef READ_FLOAT
#undef READ_VAL
		}
		else
		{
			LogError("Invalid effect/emitters value in %s\n", szFile);
		}
	}

	mem_release(pConfig);
	return(isLoaded);
}

bool CEffectLoader::saveToFile(const char *szFile, CParticleEffect *pEffect)
{
	IXConfig *pConfig = m_pCore->newConfig();

	bool isSaved = false;

	pConfig->open(szFile);
	/*
	[effect]
	emitters = <int>

	[emitter_<n>]
	<module>.<key> = <val>
	*/
	pConfig->setUint("effect", "emitters", pEffect->getEmitterCount());

	char szSection[64], szKey[64];
	for(UINT i = 0, l = pEffect->getEmitterCount(); i < l; ++i)
	{
		sprintf(szSection, "emitter_%u", i);

		IXParticleEffectEmitter *pEmitter = pEffect->getEmitterAt(i);

		pConfig->set(szSection, "name", pEmitter->getName());

		auto *pGenericData = pEmitter->getGenericData();
		pConfig->setFloat(szSection, "generic.Duration", pGenericData->getDuration());
		pConfig->setBool(szSection, "generic.Looping", pGenericData->getLooping());
		pConfig->setBool(szSection, "generic.Prewarm", pGenericData->getPrewarm());
		pConfig->setFloat(szSection, "generic.StartDelay", pGenericData->getStartDelay());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartLifetimeCurve", pGenericData->getStartLifetimeCurve());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartSpeedCurve", pGenericData->getStartSpeedCurve());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartSizeCurve", pGenericData->getStartSizeCurve());
		pConfig->setFloat(szSection, "generic.FlipRotation", pGenericData->getFlipRotation());
		pConfig->setInt(szSection, "generic.CullingMode", pGenericData->getCullingMode());
		pConfig->setFloat(szSection, "generic.GravityModifier", pGenericData->getGravityModifier());
		pConfig->setUint(szSection, "generic.MaxParticles", pGenericData->getMaxParticles());
		pConfig->setInt(szSection, "generic.RingBufferMode", pGenericData->getRingBufferMode());
		SaveMinMaxCurve(pConfig, szSection, "generic.RingBufferLoopRangeCurve", pGenericData->getRingBufferLoopRangeCurve());
		pConfig->setInt(szSection, "generic.SimulationSpace", pGenericData->getSimulationSpace());
		SaveTwoGradients(pConfig, szSection, "generic.StartColor", pGenericData->getStartColorGradient());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartRotationCurve", pGenericData->getStartRotationCurve());
		pConfig->setBool(szSection, "generic.StartRotationSeparate", pGenericData->getStartRotationSeparate());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartRotationXCurve", pGenericData->getStartRotationXCurve());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartRotationYCurve", pGenericData->getStartRotationYCurve());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartRotationZCurve", pGenericData->getStartRotationZCurve());
		pConfig->setBool(szSection, "generic.StartSizeSeparate", pGenericData->getStartSizeSeparate());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartSizeXCurve", pGenericData->getStartSizeXCurve());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartSizeYCurve", pGenericData->getStartSizeYCurve());
		SaveMinMaxCurve(pConfig, szSection, "generic.StartSizeZCurve", pGenericData->getStartSizeZCurve());

		auto *pEmissionData = pEmitter->getEmissionData();
		SaveMinMaxCurve(pConfig, szSection, "emission.RatePerSecondCurve", pEmissionData->getRatePerSecondCurve());
		SaveMinMaxCurve(pConfig, szSection, "emission.RatePerMeterCurve", pEmissionData->getRatePerMeterCurve());
		pConfig->setUint(szSection, "emission.BurstsCount", pEmissionData->getBurstsCount());
		for(UINT j = 0, jl = pEmissionData->getBurstsCount(); j < jl; ++j)
		{
			IXParticleBurst *pBurst = pEmissionData->getBurstAt(j);
			sprintf(szKey, "emission.Burst_%u.Time", j);
			pConfig->setFloat(szSection, szKey, pBurst->getTime());
			sprintf(szKey, "emission.Burst_%u.Cycles", j);
			pConfig->setUint(szSection, szKey, pBurst->getCycles());
			sprintf(szKey, "emission.Burst_%u.Probability", j);
			pConfig->setFloat(szSection, szKey, pBurst->getProbability());
			sprintf(szKey, "emission.Burst_%u.IntervalCurve", j);
			SaveMinMaxCurve(pConfig, szSection, szKey, pBurst->getIntervalCurve());
			sprintf(szKey, "emission.Burst_%u.CountCurve", j);
			SaveMinMaxCurve(pConfig, szSection, szKey, pBurst->getCountCurve());
		}

		auto *pRenderData = pEmitter->getRenderData();
		pConfig->set(szSection, "render.Material", pRenderData->getMaterial());

		auto *pShapeData = pEmitter->getShapeData();
		pConfig->setBool(szSection, "shape.Enabled", pShapeData->isEnabled());
		pConfig->setInt(szSection, "shape.Shape", pShapeData->getShape());
		pConfig->setFloat(szSection, "shape.Radius", pShapeData->getRadius());
		pConfig->setFloat(szSection, "shape.RadiusThickness", pShapeData->getRadiusThickness());
		pConfig->setFloat(szSection, "shape.Angle", pShapeData->getAngle());
		pConfig->setFloat(szSection, "shape.Arc", pShapeData->getArc());
		pConfig->setInt(szSection, "shape.ArcMode", pShapeData->getArcMode());
		pConfig->setFloat(szSection, "shape.ArcSpread", pShapeData->getArcSpread());
		SaveMinMaxCurve(pConfig, szSection, "shape.ArcSpeedCurve", pShapeData->getArcSpeedCurve());
		pConfig->setFloat(szSection, "shape.Length", pShapeData->getLength());
		pConfig->setInt(szSection, "shape.ConeEmitFrom", pShapeData->getConeEmitFrom());
		pConfig->setInt(szSection, "shape.BoxEmitFrom", pShapeData->getBoxEmitFrom());
		pConfig->setFloat(szSection, "shape.DonutRadius", pShapeData->getDonutRadius());
		pConfig->setBool(szSection, "shape.AlignToDirection", pShapeData->getAlignToDirection());
		pConfig->setFloat(szSection, "shape.RandomizeDirection", pShapeData->getRandomizeDirection());
		pConfig->setFloat(szSection, "shape.SpherizeDirection", pShapeData->getSpherizeDirection());
		pConfig->setFloat(szSection, "shape.RandomizePosition", pShapeData->getRandomizePosition());
		pConfig->setVector3(szSection, "shape.Size", pShapeData->getSize());

		auto *pVelocityLifetimeData = pEmitter->getVelocityLifetimeData();
		pConfig->setBool(szSection, "velocityLifetime.Enabled", pVelocityLifetimeData->isEnabled());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.LinearXCurve", pVelocityLifetimeData->getLinearXCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.LinearYCurve", pVelocityLifetimeData->getLinearYCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.LinearZCurve", pVelocityLifetimeData->getLinearZCurve());
		pConfig->setInt(szSection, "velocityLifetime.SimulationSpace", pVelocityLifetimeData->getSimulationSpace());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.OrbitalXCurve", pVelocityLifetimeData->getOrbitalXCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.OrbitalYCurve", pVelocityLifetimeData->getOrbitalYCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.OrbitalZCurve", pVelocityLifetimeData->getOrbitalZCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.OffsetXCurve", pVelocityLifetimeData->getOffsetXCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.OffsetYCurve", pVelocityLifetimeData->getOffsetYCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.OffsetZCurve", pVelocityLifetimeData->getOffsetZCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.RadialCurve", pVelocityLifetimeData->getRadialCurve());
		SaveMinMaxCurve(pConfig, szSection, "velocityLifetime.SpeedModifierCurve", pVelocityLifetimeData->getSpeedModifierCurve());

		auto *pLimitVelocityLifetimeData = pEmitter->getLimitVelocityLifetimeData();
		pConfig->setBool(szSection, "limitVelocityLifetime.Enabled", pLimitVelocityLifetimeData->isEnabled());
		pConfig->setBool(szSection, "limitVelocityLifetime.SeparateAxes", pLimitVelocityLifetimeData->getSeparateAxes());
		SaveMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.LimitCurveX", pLimitVelocityLifetimeData->getLimitXCurve());
		SaveMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.LimitCurveY", pLimitVelocityLifetimeData->getLimitYCurve());
		SaveMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.LimitCurveZ", pLimitVelocityLifetimeData->getLimitZCurve());
		SaveMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.DampenCurve", pLimitVelocityLifetimeData->getDampenCurve());
		SaveMinMaxCurve(pConfig, szSection, "limitVelocityLifetime.DragCurve", pLimitVelocityLifetimeData->getDragCurve());
		pConfig->setBool(szSection, "limitVelocityLifetime.MultiplyBySize", pLimitVelocityLifetimeData->getMultiplyBySize());
		pConfig->setBool(szSection, "limitVelocityLifetime.MultiplyByVelocity", pLimitVelocityLifetimeData->getMultiplyByVelocity());

		IXParticleEffectEmitterForceLifetimeData *pForceLifetimeData = pEmitter->getForceLifetimeData();
		pConfig->setBool(szSection, "forceLifetimeData.Enabled", pForceLifetimeData->isEnabled());
		SaveMinMaxCurve(pConfig, szSection, "forceLifetimeData.ForceXCurve", pForceLifetimeData->getForceXCurve());
		SaveMinMaxCurve(pConfig, szSection, "forceLifetimeData.ForceYCurve", pForceLifetimeData->getForceYCurve());
		SaveMinMaxCurve(pConfig, szSection, "forceLifetimeData.ForceZCurve", pForceLifetimeData->getForceZCurve());
		pConfig->setInt(szSection, "forceLifetimeData.SimulationSpace", pForceLifetimeData->getSimulationSpace());
		pConfig->setBool(szSection, "forceLifetimeData.Randomize", pForceLifetimeData->getRandomize());

		IXParticleEffectEmitterSizeLifetimeData *pSizeLifetimeData = pEmitter->getSizeLifetimeData();
		pConfig->setBool(szSection, "sizeLifetimeData.Enabled", pSizeLifetimeData->isEnabled());
		pConfig->setBool(szSection, "sizeLifetimeData.SeparateAxes", pSizeLifetimeData->getSeparateAxes());
		SaveMinMaxCurve(pConfig, szSection, "sizeLifetimeData.SizeXCurve", pSizeLifetimeData->getSizeXCurve());
		SaveMinMaxCurve(pConfig, szSection, "sizeLifetimeData.SizeYCurve", pSizeLifetimeData->getSizeYCurve());
		SaveMinMaxCurve(pConfig, szSection, "sizeLifetimeData.SizeZCurve", pSizeLifetimeData->getSizeZCurve());

		IXParticleEffectEmitterSizeSpeedData *pSizeSpeedData = pEmitter->getSizeSpeedData();
		pConfig->setBool(szSection, "sizeSpeedData.Enabled", pSizeSpeedData->isEnabled());
		pConfig->setBool(szSection, "sizeSpeedData.SeparateAxes", pSizeSpeedData->getSeparateAxes());
		SaveMinMaxCurve(pConfig, szSection, "sizeSpeedData.SizeXCurve", pSizeSpeedData->getSizeXCurve());
		SaveMinMaxCurve(pConfig, szSection, "sizeSpeedData.SizeYCurve", pSizeSpeedData->getSizeYCurve());
		SaveMinMaxCurve(pConfig, szSection, "sizeSpeedData.SizeZCurve", pSizeSpeedData->getSizeZCurve());
		pConfig->setVector2(szSection, "sizeSpeedData.SpeedRange", pSizeSpeedData->getSpeedRange());

		IXParticleEffectEmitterRotationLifetimeData *pRotationLifetimeData = pEmitter->getRotationLifetimeData();
		pConfig->setBool(szSection, "rotationLifetimeData.Enabled", pRotationLifetimeData->isEnabled());
		pConfig->setBool(szSection, "rotationLifetimeData.SeparateAxes", pRotationLifetimeData->getSeparateAxes());
		SaveMinMaxCurve(pConfig, szSection, "rotationLifetimeData.AngularVelocityXCurve", pRotationLifetimeData->getAngularVelocityXCurve());
		SaveMinMaxCurve(pConfig, szSection, "rotationLifetimeData.AngularVelocityYCurve", pRotationLifetimeData->getAngularVelocityYCurve());
		SaveMinMaxCurve(pConfig, szSection, "rotationLifetimeData.AngularVelocityZCurve", pRotationLifetimeData->getAngularVelocityZCurve());

		IXParticleEffectEmitterRotationSpeedData *pRotationSpeedData = pEmitter->getRotationSpeedData();
		pConfig->setBool(szSection, "rotationSpeedData.Enabled", pRotationSpeedData->isEnabled());
		pConfig->setBool(szSection, "rotationSpeedData.SeparateAxes", pRotationSpeedData->getSeparateAxes());
		SaveMinMaxCurve(pConfig, szSection, "rotationSpeedData.AngularVelocityXCurve", pRotationSpeedData->getAngularVelocityXCurve());
		SaveMinMaxCurve(pConfig, szSection, "rotationSpeedData.AngularVelocityYCurve", pRotationSpeedData->getAngularVelocityYCurve());
		SaveMinMaxCurve(pConfig, szSection, "rotationSpeedData.AngularVelocityZCurve", pRotationSpeedData->getAngularVelocityZCurve());
		pConfig->setVector2(szSection, "rotationSpeedData.SpeedRange", pRotationSpeedData->getSpeedRange());

		IXParticleEffectEmitterLifetimeEmitterSpeedData *pLifetimeEmitterSpeedData = pEmitter->getLifetimeEmitterSpeedData();
		pConfig->setBool(szSection, "lifetimeEmitterSpeedData.Enabled", pLifetimeEmitterSpeedData->isEnabled());
		SaveMinMaxCurve(pConfig, szSection, "lifetimeEmitterSpeedData.MultiplierCurve", pLifetimeEmitterSpeedData->getMultiplierCurve());
		pConfig->setVector2(szSection, "lifetimeEmitterSpeedData.SpeedRange", pLifetimeEmitterSpeedData->getSpeedRange());

		IXParticleEffectEmitterColorLifetimeData *pColorLifetimeData = pEmitter->getColorLifetimeData();
		pConfig->setBool(szSection, "colorLifetimeData.Enabled", pColorLifetimeData->isEnabled());
		SaveTwoGradients(pConfig, szSection, "colorLifetimeData.Color", pColorLifetimeData->getColor());

		IXParticleEffectEmitterColorSpeedData *pColorSpeedData = pEmitter->getColorSpeedData();
		pConfig->setBool(szSection, "colorSpeedData.Enabled", pColorSpeedData->isEnabled());
		SaveTwoGradients(pConfig, szSection, "colorSpeedData.Color", pColorSpeedData->getColor());
		pConfig->setVector2(szSection, "colorSpeedData.SpeedRange", pColorSpeedData->getSpeedRange());
	}

	isSaved = pConfig->save();

	mem_release(pConfig);

	return(isSaved);
}
