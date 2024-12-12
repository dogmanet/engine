#ifndef __ANIMATEDMODEL_H
#define __ANIMATEDMODEL_H

#include <xcommon/resource/IXModel.h>
#include "AnimatedModelShared.h"

class CAnimatedModel final: public IXUnknownImplementation<IXAnimatedModel>
{
public:
	CAnimatedModel(CAnimatedModelProvider *pProvider, CAnimatedModelShared *pShared);
	~CAnimatedModel();

	bool XMETHODCALLTYPE isEnabled() const override;
	void XMETHODCALLTYPE enable(bool yesNo) override;

	IXAnimatedModel* XMETHODCALLTYPE asAnimatedModel() override;
	IXDynamicModel* XMETHODCALLTYPE asDynamicModel() override;
	IXStaticModel* XMETHODCALLTYPE asStaticModel() override;


	float3 XMETHODCALLTYPE getPosition() const override;
	void XMETHODCALLTYPE setPosition(const float3 &vPos) override;

	SMQuaternion XMETHODCALLTYPE getOrientation() const override;
	void XMETHODCALLTYPE setOrientation(const SMQuaternion &qRot) override;

	float XMETHODCALLTYPE getScale() const override;
	void XMETHODCALLTYPE setScale(float fScale) override;

	UINT XMETHODCALLTYPE getSkin() const override;
	void XMETHODCALLTYPE setSkin(UINT uSkin) override;

	float3 XMETHODCALLTYPE getLocalBoundMin() const override;
	float3 XMETHODCALLTYPE getLocalBoundMax() const override;
	SMAABB XMETHODCALLTYPE getLocalBound() const override;

	float4 XMETHODCALLTYPE getColor() const override;
	void XMETHODCALLTYPE setColor(const float4 &vColor) override;

	UINT XMETHODCALLTYPE getPhysboxCount(UINT uPartIndex = 0) const override;
	const IModelPhysbox* XMETHODCALLTYPE getPhysBox(UINT id, UINT uPartIndex = 0) const override;
	const IXResourceModel* XMETHODCALLTYPE getResource(UINT uIndex = 0) override;



	UINT XMETHODCALLTYPE getPartsCount() const override;
	const char* XMETHODCALLTYPE getPartName(UINT uIndex) const override;
	UINT XMETHODCALLTYPE getPartIndex(const char *szName) override;
	XMODEL_PART_FLAGS XMETHODCALLTYPE getPartFlags(UINT uIndex) const override;
	bool XMETHODCALLTYPE isPartEnabled(UINT uIndex) const override;
	void XMETHODCALLTYPE enablePart(UINT uIndex, bool yesNo) override;

	UINT XMETHODCALLTYPE getHitboxCount(UINT uPartIndex = 0) const override;
	const IModelPhysbox* XMETHODCALLTYPE getHitbox(UINT id, UINT uPartIndex = 0) const override;
	int XMETHODCALLTYPE getHitboxBone(UINT id, UINT uPartIndex = 0) const override;
	XHITBOXBODYPART XMETHODCALLTYPE getHitboxBodyPart(UINT id, UINT uPartIndex = 0) const override;

	void XMETHODCALLTYPE play(const char *szName, UINT uFadeTime = 0, UINT uSlot = 0, bool bReplaceActivity = true) override;
	void XMETHODCALLTYPE stop(UINT uSlot = 0) override;
	void XMETHODCALLTYPE resume(UINT uSlot = 0) override;
	void XMETHODCALLTYPE setProgress(float fProgress, UINT uSlot = 0) override;
	void XMETHODCALLTYPE startActivity(const char *szName, UINT uFadeTime = 0, UINT uSlot = 0) override;
	void XMETHODCALLTYPE stopAll() override;

	float3 XMETHODCALLTYPE getBoneTransformPos(UINT id, XMODEL_BONE_TRANSFORM boneTranform = XMBT_WORLD) override;
	SMQuaternion XMETHODCALLTYPE getBoneTransformRot(UINT id, XMODEL_BONE_TRANSFORM boneTranform = XMBT_WORLD) override;
	SMMATRIX XMETHODCALLTYPE getBoneTransform(UINT id) override;

	UINT XMETHODCALLTYPE getBoneId(const char *szName) override;
	UINT XMETHODCALLTYPE getBoneCount() const override;
	const char* XMETHODCALLTYPE getBoneName(UINT id) const override;

	bool XMETHODCALLTYPE isPlayingAnimations() override;
	bool XMETHODCALLTYPE isPlayingAnimation(const char *szName) override;
	
	void XMETHODCALLTYPE setController(UINT id, float fValue) override;

	UINT XMETHODCALLTYPE getControllersCount() const override;
	const char* XMETHODCALLTYPE getControllerName(UINT id) override;
	UINT XMETHODCALLTYPE getControllerId(const char *szName) override;

	void XMETHODCALLTYPE setCallback(IAnimationCallback *pCallback) override;

	int XMETHODCALLTYPE getBoneParent(UINT id) override;

	UINT XMETHODCALLTYPE getLayersCount() const override;
	void XMETHODCALLTYPE setLayersCount(UINT uCount) override;
	float XMETHODCALLTYPE getLayerBlendWeight(UINT uLayer) const override;
	void XMETHODCALLTYPE setLayerBlendWeight(UINT uLayer, float fWeight) override;

	void update(float fDT);

	UINT addLayer();
	void delLayer();

	void XMETHODCALLTYPE render(UINT uLod, XMODEL_FEATURE bmFeatures) override;
	void sync();

	void initGPUresources();

	bool XMETHODCALLTYPE rayTest(const float3 &vStart, const float3 &vEnd, float3 *pvOut = NULL, float3 *pvNormal = NULL, bool isRayInWorldSpace = true, bool bReturnNearestPoint = false) override;

	void XMETHODCALLTYPE setLayer(UINT uLayer) override;
	UINT XMETHODCALLTYPE getLayer() override;
protected:
	CAnimatedModelProvider *m_pProvider;
	CAnimatedModelShared *m_pShared;
	IGXDevice *m_pDevice;

	float3_t m_vPosition;
	SMQuaternion m_qRotation;
	UINT m_uSkin = 0;
	float4_t m_vColor{1.0f, 1.0f, 1.0f, 1.0f};
	bool m_isEnabled = true;
	float m_fScale = 1.0f;

	IGXConstantBuffer *m_pWorldBuffer = NULL;
	bool m_isWorldDirty = true;
	IGXConstantBuffer *m_pBoneConstantBuffer = NULL;

	IGXConstantBuffer *m_pColorBuffer = NULL;
	bool m_isColorDirty = true;

	IAnimationCallback *m_pCallback = NULL;

	struct ModelBoneShader
	{
		float4 position;   /*!< Позиция */
		SMQuaternion orient; /*!< Вращение */

		SX_ALIGNED_OP_MEM();
	};

	ModelBoneShader *m_pBonesBlob = NULL;
	ModelBoneShader *m_pRenderFrameBones = NULL; //! Используются в текущем кадре }
	ModelBoneShader *m_pNextFrameBones = NULL; //! Рассчет на следующий кадр      } -- переключаются каждый кадр
	
	XResourceModelBone *m_pBoneControllers = NULL;

	bool m_isBoneMatrixReFilled = false;

	struct layer_s
	{
		bool isPlaying = false;
		bool isNewPlaying = false; // for fading
		int iCurrentFrame = 0;
		UINT uFrameCount = 0;
		float fTime = 0;

		UINT uFadeTime = 0;
		UINT uFadeCurTime = 0;
		bool isInFade = false;
		bool bDoAdvance = true;

		XResourceModelBone *pLastFrameBones = NULL;
		XResourceModelBone *pCurrentBones = NULL;
		bool *isBoneWorld = NULL;

		UINT uAnimationId = 0;

		int iActivity = -1;
		UINT uActivityFadeTime = 0;

		bool isDirty = true;

		float fWeight = 1.0f;
	};
	Array<layer_s, 1> m_aLayers;


	void playActivityNext(UINT uLayer);
	bool validateLayer(UINT uLayer) const;
	void fillBoneMatrix();

	mutable bool m_isLocalAABBvalid = false;
	mutable float3_t m_vLocalMin;
	mutable float3_t m_vLocalMax;

	void _updateAABB() const;

	bool m_wasInitiated = false;

	UINT m_uLayer = 0;
};

#endif
