#ifndef __EXPORTER_H
#define __EXPORTER_H

#include "stdafx.h"
#include "Extended.h"
#include <dseplugin/ModelFile.h>

#include "ExtendedPhys.h"
#include "ExtendedAnim.h"
#include "ExtendedSkin.h"
#include "ExtendedActs.h"
#include "ExtendedLods.h"

//#define SXPROP_EXP_GIBS "SXPROP/EXP_GIBS"
#define SXPROP_EXP_SCALE "SXPROP/EXP_SCALE"
//#define SXPROP_EXP_TABI "SXPROP/EXP_TABI"
//#define SXPROP_EXP_STAT "SXPROP/EXP_STAT"
//#define SXPROP_EXP_WGHT "SXPROP/EXP_WGHT"
//#define SXPROP_EXP_SCNT "SXPROP/EXP_SCNT"
//#define SXPROP_EXP_SKN_PREF "SXPROP/SKN/"

class CExporter;
class CActListener: public CExtendedActs::IListener
{
public:
	CActListener(CExporter *pExporter);
	void onCommitted() override;
	void onChanged() override;

private:
	CExporter *m_pExporter;
};

class CLodListener: public CExtendedLods::IListener
{
public:
	CLodListener(CExporter *pExporter);
	void onCommitted() override;

private:
	CExporter *m_pExporter;
};

class IProgress
{
public:
	virtual void setProgress(float fProgress) = 0;
};

class IExporterProvider
{
public:
	virtual bool canExportTB() = 0;

	virtual bool prepare(IProgress *pProgress, bool forStaticExport, bool bPrepareMesh, bool bPrepareTB) = 0;
	virtual bool preapareAnimationTrack(IProgress *pProgress, UINT uStartFrame, UINT uFrameCount) = 0;

	virtual UINT getLayerCount() = 0;
	virtual const char* getLayerName(UINT uLayer) = 0;
	virtual UINT getLayerObjectCount(UINT uLayer) = 0;

	virtual float3 getObjectPosition(UINT uLayer, UINT uObject) = 0;
	virtual SMQuaternion getObjectRotation(UINT uLayer, UINT uObject) = 0;

	virtual UINT getObjectSubsetCount(UINT uLayer, UINT uObject) = 0;
	virtual const char* getObjectName(UINT uLayer, UINT uObject) = 0;
	virtual const char* getObjectSubsetTexture(UINT uLayer, UINT uObject, UINT uSubset) = 0;

	virtual UINT getObjectSubsetVertexCount(UINT uLayer, UINT uObject, UINT uSubset) = 0;
	virtual UINT getObjectSubsetIndexCount(UINT uLayer, UINT uObject, UINT uSubset) = 0;

	virtual UINT* getObjectSubsetIndices(UINT uLayer, UINT uObject, UINT uSubset) = 0;

	virtual vertex_static* getObjectSubsetStaticVertices(UINT uLayer, UINT uObject, UINT uSubset) = 0;
	virtual vertex_static_ex* getObjectSubsetStaticExVertices(UINT uLayer, UINT uObject, UINT uSubset) = 0;
	virtual vertex_animated* getObjectSubsetAnimatedVertices(UINT uLayer, UINT uObject, UINT uSubset) = 0;
	virtual vertex_animated_ex* getObjectSubsetAnimatedExVertices(UINT uLayer, UINT uObject, UINT uSubset) = 0;

	virtual UINT getBonesCount() = 0;
	virtual const char* getBoneName(UINT uBone) = 0;
	virtual int getBoneParent(UINT uBone) = 0;
	virtual float3 getBoneLocalPos(UINT uBone) = 0;
	virtual SMQuaternion getBoneLocalRot(UINT uBone) = 0;

	virtual float3 getBonePositionAtFrame(UINT uBone, UINT uFrame) = 0;
	virtual SMQuaternion getBoneRotationAtFrame(UINT uBone, UINT uFrame) = 0;

	virtual bool hasBones() = 0;

	virtual bool getConfigBool(const char *szKey, bool bDefault) = 0;
	virtual void setConfigBool(const char *szKey, bool bvalue) = 0;
	virtual float getConfigFloat(const char *szKey, float fDefault) = 0;
	virtual void setConfigFloat(const char *szKey, float fvalue) = 0;
	virtual int getConfigInt(const char *szKey, int iDefault) = 0;
	virtual void setConfigInt(const char *szKey, int ivalue) = 0;
	virtual void getConfigStr(const char *szKey, char *szOut, int iMaxOut) = 0;
	virtual void setConfigStr(const char *szKey, const char *szValue) = 0;
};

class CExporter
{
public:
	CExporter(const char *szFile, bool bSuppressPrompts, IExporterProvider *pCallback);
	~CExporter();

	int execute();

	void setFramerate(int iFramerate)
	{
		m_iFramerate = iFramerate;
	}
	void setStartFrame(UINT uFrame)
	{
		m_uStartFrame = uFrame;
	}
	void setEndFrame(UINT uFrame)
	{
		m_uEndFrame = uFrame;
	}

	UINT addTexture(const char *szName);

public:
	void onActsCommitted();
	void onActsChanged();
	void onLodsCommitted();

	void logWarning(const char *szFormat, ...)
	{
		if(!m_isConsoleAlloced)
		{
			m_isConsoleAlloced = AllocConsole();
			freopen("CONOUT$", "wt", stdout);
			freopen("CONOUT$", "wt", stderr);
		}

		va_list va;
		va_start(va, szFormat);
		vprintf(szFormat, va);
		va_end(va);
	}

private:
	
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void updateSectionMesh();
	void updateSectionAnim();
	void updateSectionGibs();

	void loadDefaults();

	void tryLoadModel();

	void loadPhysdata(FILE *pf);
	void loadMaterials(FILE *pf);
	void loadLoDs(FILE *pf);
	void loadDeps(FILE *pf);
	void loadActivities(FILE *pf);
	void loadBones(FILE *pf);
	void loadControllers(FILE *pf);
	void loadAnimations(FILE *pf);
	void loadHitboxes(FILE *pf);
	void loadChunks(FILE *pf);

	void writePhysdata(FILE *pf);
	void writeMaterials(FILE *pf);
	void writeLoDs(FILE *pf);
	void writeDeps(FILE *pf);
	void writeActivities(FILE *pf);
	void writeBones(FILE *pf);
	void writeControllers(FILE *pf);
	void writeAnimations(FILE *pf);
	void writeHitboxes(FILE *pf);
	void writeChunks(FILE *pf);

	void startExport();

	bool writeFile(const char *szFile, bool isForGib = false);

	static void ProcessThread(void *p);
	void doExport();

	void updateLodList();

	void performRetopology();

	void clearLods(int iTargetLod = -1);
	void clearPhysparts();
	void clearHitboxes();

	bool checkSettings();

	void preparePhysMesh(bool bIgnoreLayers = false, bool bHitbox = false);
	void prepareLodMesh(int iTargetLod = -1);

	bool prepareSkeleton(bool bOverwriteBindPose);

	void prepareAnimation();

	bool addBone(int iProviderParent, UINT uProviderId, bool bOverwriteBindPose);

	int getNewBoneId(int iProviderId);
	int getOldBoneId(int iId);

	template<typename T>
	void fixSubsetBoneIds(T *pVertices, UINT uVertexCount)
	{
		for(UINT i = 0; i < uVertexCount; ++i)
		{
			for(UINT j = 0; j < 4; ++j)
			{
				if(pVertices[i].BoneIndices[j] != 255)
				{
					pVertices[i].BoneIndices[j] = (byte)getNewBoneId(pVertices[i].BoneIndices[j]);
				}
				else
				{
					pVertices[i].BoneIndices[j] = 0;
				}
			}
		}
	}

	bool invokeEditor(HWND hWnd, LPCTSTR szDefault, LPCTSTR szTitle);


	class CProgress final: public IProgress
	{
	public:
		CProgress(HWND hProgressBar);
		void setProgress(float fProgress) override;

		void setRange(float fMin, float fMax);
		void setRange(float fMax);

	private:
		HWND m_hProgressBar;
		const int mc_iTopRange = 1000;

		float m_fMinRange = 0.0f;
		float m_fMaxRange = 1.0f;
	};

//	float3 getBonePosForFrame(UINT uBone, UINT uFrame);
//	SMQuaternion getBoneRotForFrame(UINT uBone, UINT uFrame);

	void enableChunk(const XGUID &guid, bool yesNo);

private:
	HWND m_hDlgWnd = NULL;
	CExtended *m_pExtended = NULL;

	const char *m_szFile = NULL;

	bool m_bSuppressPrompts = false;

	float m_fScale = 1.0f;

	int m_iFramerate = 30;
	UINT m_uStartFrame = 0;
	UINT m_uEndFrame = 1;
	bool m_isLooped = false;
	char m_szAnimationName[MODEL_MAX_NAME];
	char m_szAnimationActivity[MODEL_MAX_NAME];
	UINT m_uActivityChance = 1;

	IExporterProvider *m_pCallback = NULL;

	bool m_isSkeletonReady = false;
	Array<int> m_aSkeletonRemap; // m_aSkeletonRemap[oldId] = newId
	Array<int> m_aSkeletonRemapInv; // m_aSkeletonRemap[newId] = oldId


	const char *m_szEditDefault = NULL;
	const char *m_szEditTitle = NULL;

	CProgress *m_pProgress = NULL;

	BOOL m_isConsoleAlloced = FALSE;


	/// for gibs
	int m_iOnlyLayer = -1;
	int m_iOnlyObject = -1;
	const char *m_szOnlyLayer = NULL;
private:

	CExtendedPhys *m_pExtPhys = NULL;
	CExtendedAnim *m_pExtAnim = NULL;
	CExtendedSkin *m_pExtSkin = NULL;
	CExtendedActs *m_pExtActs = NULL;
	CExtendedLods *m_pExtLods = NULL;

	CActListener *m_pExtActsListener = NULL;
	CLodListener *m_pExtLodListener = NULL;


private:
	ModelHeader m_hdr;
	ModelHeader2 m_hdr2;
	ModelHeader3 m_hdr3;

	bool m_isLoaded = false;

	Array<ModelPhyspart> m_aPhysParts;

	Array<Array<String>> m_aasMaterials; // [skin][material]

	Array<ModelLoD> m_aLods;
	
	Array<ModelBoneName> m_aBones;

	Array<ModelBoneController> m_aControllers;

	Array<ModelHitbox> m_aHitboxes;
	Array<ModelHitboxEx> m_aHitboxesEx;

	Array<ModelPart> m_aParts;

	struct ModelChunkRaw
	{
		ModelChunkHeader hdr;
		Array<byte> aData;
	};

	Array<ModelChunkRaw> m_aChunks;

};

#endif
