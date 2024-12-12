#ifndef __PROVIDER_H
#define __PROVIDER_H

#include <exporter_base/Exporter.h>

#include <msLib/msPlugIn.h>
#include <msLib/msLib.h>

class CProvider final: public IExporterProvider
{
public:
	CProvider(msModel *pModel);

	bool canExportTB() override;

	bool prepare(IProgress *pProgress, bool forStaticExport, bool bPrepareMesh, bool bPrepareTB) override;
	bool preapareAnimationTrack(IProgress *pProgress, UINT uStartFrame, UINT uFrameCount) override;

	UINT getLayerCount() override;
	const char* getLayerName(UINT uLayer) override;
	UINT getLayerObjectCount(UINT uLayer) override;

	float3 getObjectPosition(UINT uLayer, UINT uObject) override;
	SMQuaternion getObjectRotation(UINT uLayer, UINT uObject) override;

	UINT getObjectSubsetCount(UINT uLayer, UINT uObject) override;
	const char* getObjectName(UINT uLayer, UINT uObject) override;
	const char* getObjectSubsetTexture(UINT uLayer, UINT uObject, UINT uSubset) override;

	UINT getObjectSubsetVertexCount(UINT uLayer, UINT uObject, UINT uSubset) override;
	UINT getObjectSubsetIndexCount(UINT uLayer, UINT uObject, UINT uSubset) override;

	UINT* getObjectSubsetIndices(UINT uLayer, UINT uObject, UINT uSubset) override;

	vertex_static* getObjectSubsetStaticVertices(UINT uLayer, UINT uObject, UINT uSubset) override;
	vertex_static_ex* getObjectSubsetStaticExVertices(UINT uLayer, UINT uObject, UINT uSubset) override;
	vertex_animated* getObjectSubsetAnimatedVertices(UINT uLayer, UINT uObject, UINT uSubset) override;
	vertex_animated_ex* getObjectSubsetAnimatedExVertices(UINT uLayer, UINT uObject, UINT uSubset) override;

	bool hasBones() override;

	UINT getBonesCount() override;
	const char* getBoneName(UINT uBone) override;
	int getBoneParent(UINT uBone) override;
	float3 getBoneLocalPos(UINT uBone) override;
	SMQuaternion getBoneLocalRot(UINT uBone) override;

//	UINT getBonePositionKeysCount(UINT uBone) override;
//	UINT getBoneRotationKeysCount(UINT uBone) override;
//	float3 getBonePositionKey(UINT uBone, UINT uKey, float *pfTime) override;
//	SMQuaternion getBoneRotationKey(UINT uBone, UINT uKey, float *pfTime) override;

	float3 getBonePositionAtFrame(UINT uBone, UINT uFrame) override;
	SMQuaternion getBoneRotationAtFrame(UINT uBone, UINT uFrame) override;

	bool getConfigBool(const char *szKey, bool bDefault) override;
	void setConfigBool(const char *szKey, bool bvalue) override;
	float getConfigFloat(const char *szKey, float fDefault) override;
	void setConfigFloat(const char *szKey, float fvalue) override;
	int getConfigInt(const char *szKey, int iDefault) override;
	void setConfigInt(const char *szKey, int ivalue) override;
	void getConfigStr(const char *szKey, char *szOut, int iMaxOut) override;
	void setConfigStr(const char *szKey, const char *szValue) override;

protected:

	template<typename T>
	bool getConfigBool(T *pObj, const char *szKey, bool bDefault)
	{
		char tmp[32];
		getConfigStr(pObj, szKey, tmp, sizeof(tmp));
		int i = 0;
		if(sscanf(tmp, "%d", &i) != 1)
		{
			i = bDefault;
		}
		return(i != 0);
	}
	template<typename T>
	void setConfigBool(T *pObj, const char *szKey, bool bvalue)
	{
		char tmp[2];
		tmp[0] = bvalue ? '1' : '0';
		tmp[1] = 0;

		setConfigStr(pObj, szKey, tmp);
	}

	template<typename T>
	float getConfigFloat(T *pObj, const char *szKey, float fDefault)
	{
		char tmp[32];
		getConfigStr(pObj, szKey, tmp, sizeof(tmp));
		float f = 0;
		if(sscanf(tmp, "%f", &f) != 1)
		{
			f = fDefault;
		}
		return(f);
	}
	template<typename T>
	void setConfigFloat(T *pObj, const char *szKey, float fvalue)
	{
		char tmp[32];
		sprintf(tmp, "%f", fvalue);

		setConfigStr(pObj, szKey, tmp);
	}

	template<typename T>
	int getConfigInt(T *pObj, const char *szKey, int iDefault)
	{
		char tmp[32];
		getConfigStr(pObj, szKey, tmp, sizeof(tmp));
		int i = 0;
		if(sscanf(tmp, "%d", &i) != 1)
		{
			i = iDefault;
		}
		return(i);
	}
	template<typename T>
	void setConfigInt(T *pObj, const char *szKey, int ivalue)
	{
		char tmp[32];
		sprintf(tmp, "%d", ivalue);

		setConfigStr(pObj, szKey, tmp);
	}

	template<typename T>
	void getConfigStr(T *pObj, const char *szKey, char *szOut, int iMaxOut)
	{
		char tmp[4096];
		tmp[0] = 0;
		getObjectComment(pObj, tmp, 4096);
		szOut[0] = 0;

		int iRows = parse_str(tmp, NULL, 0, '\n');
		char **pszParts = (char**)alloca(sizeof(char*) * iRows);
		parse_str(tmp, pszParts, iRows, '\n');

		size_t l0 = strlen(szKey);

		for(int i = 0; i < iRows; ++i)
		{
			if(pszParts[i][0] == '#')
			{
				size_t l1 = strlen(pszParts[i]);
				if(l0 >= l1)
				{
					continue;
				}

				if(!memcmp(pszParts[i] + 1, szKey, l0) && pszParts[i][1 + l0] == ' ')
				{
					strncpy(szOut, pszParts[i] + 1 + l0 + 1, iMaxOut);
					szOut[iMaxOut - 1] = 0;
					return;
				}
			}
		}
	}
	template<typename T>
	void setConfigStr(T *pObj, const char *szKey, const char *szValue)
	{
		char tmp[4096], tmp2[4096];
		tmp[0] = 0;
		tmp2[0] = 0;

		getObjectComment(pObj, tmp, 4096);

		int iRows = parse_str(tmp, NULL, 0, '\n');
		char **pszParts = (char**)alloca(sizeof(char*) * iRows);
		parse_str(tmp, pszParts, iRows, '\n');

		size_t l0 = strlen(szKey);

		bool isFound = false;
		char *ptr = tmp2;
		for(int i = 0; i < iRows; ++i)
		{
			size_t l1 = strlen(pszParts[i]);
			if(pszParts[i][0] == '#' && l0 < l1 && !memcmp(pszParts[i] + 1, szKey, l0) && pszParts[i][1 + l0] == ' ')
			{
				ptr += sprintf(ptr, "#%s %s\r\n", szKey, szValue);
				isFound = true;
			}
			else
			{
				ptr += sprintf(ptr, "%s\r\n", pszParts[i]);
			}
		}

		if(!isFound)
		{
			ptr += sprintf(ptr, "#%s %s\r\n", szKey, szValue);
		}

		setObjectComment(pObj, tmp2);
	}

	float3 toPosition(msVec3 vec);
	SMQuaternion toRotation(msVec3 vec);

private:
	void setObjectComment(msModel *pMdl, const char *szComment)
	{
		msModel_SetComment(pMdl, szComment);
	}
	void setObjectComment(msMesh *pObj, const char *szComment)
	{
		msMesh_SetComment(pObj, szComment);
	}
	void setObjectComment(msBone *pObj, const char *szComment)
	{
		msBone_SetComment(pObj, szComment);
	}
	void setObjectComment(msMaterial *pObj, const char *szComment)
	{
		msMaterial_SetComment(pObj, szComment);
	}

	int getObjectComment(msModel *pModel, char *pszComment, int nMaxCommentLength)
	{
		return(msModel_GetComment(pModel, pszComment, nMaxCommentLength));
	}
	int getObjectComment(msMesh *pObj, char *pszComment, int nMaxCommentLength)
	{
		return(msMesh_GetComment(pObj, pszComment, nMaxCommentLength));
	}
	int getObjectComment(msBone *pObj, char *pszComment, int nMaxCommentLength)
	{
		return(msBone_GetComment(pObj, pszComment, nMaxCommentLength));
	}
	int getObjectComment(msMaterial *pObj, char *pszComment, int nMaxCommentLength)
	{
		return(msMaterial_GetComment(pObj, pszComment, nMaxCommentLength));
	}

	float3 getBonePosForFrame(UINT uBone, int iFrame);
	SMQuaternion getBoneRotForFrame(UINT uBone, int iFrame);

	UINT getBonePositionKeysCount(UINT uBone);
	UINT getBoneRotationKeysCount(UINT uBone);
	float3 getBonePositionKey(UINT uBone, UINT uKey, float *pfTime);
	SMQuaternion getBoneRotationKey(UINT uBone, UINT uKey, float *pfTime);


private:
	msModel *m_pModel;

	bool m_isStatic = false;

	struct Subset
	{
		String sTexture;
		Array<UINT> aIndices;
		Array<vertex_static> aVerticesStatic;
		Array<vertex_animated> aVerticesAnimated;
	};

	struct Object
	{
		float3 vPosition;
		SMQuaternion qRotation;
		Array<Subset> aSubsets;
		String sName;
	};
	struct Layer
	{
		String sName;
		Array<Object> aObjects;
	};
	Array<Layer> m_aLayers;

	struct Bone
	{
		int iParent;
		String sName;
		float3 vPos;
		SMQuaternion qRot;
	};
	Array<Bone> m_aBones;

	struct TrackBone
	{
		float3 vPos;
		SMQuaternion qRot;
	};
	Array<Array<TrackBone>> m_aaAnimationTrack;
};

#endif
