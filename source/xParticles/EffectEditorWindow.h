#ifndef __EFFECTEDITORWINDOW_H
#define __EFFECTEDITORWINDOW_H


#include <xUI/IXUI.h>
#include <xcommon/IXCore.h>

//#include <common/array.h>
//#include <common/assotiativearray.h>
//#include <common/string.h>
//#include <common/aastring.h>
//#include <xcommon/IFileSystem.h>
//#include <xcommon/editor/IXEditorExtension.h>
//#include <xcommon/render/IXRender.h>
#include <xcommon/particles/IXParticleSystem.h>

class CEffectEditorWindow;
class CEffectEditorPlayerCallback final: public IXParticlePlayerCallback
{
public:
	CEffectEditorPlayerCallback(CEffectEditorWindow *pWnd);

	void XMETHODCALLTYPE onEvent(XPARTICLE_EVENT ev) override;
private:
	CEffectEditorWindow *m_pWindow;
};

//#############################################################################

class CEffectEditorWindow final: public IXUnknownImplementation<IXUnknown>
{
	friend class CEffectEditorPlayerCallback;

private:
	static void EventProc(void *pCtx, IUIControl *pControl, gui::IEvent *ev);
	void eventProc(IUIControl *pControl, gui::IEvent *ev);

public:
	CEffectEditorWindow(const char *szEffect, IXRender *pRender, HWND hMainWnd, IXCore *pCore, IXParticleSystem *pParticleSystem);
	~CEffectEditorWindow();

private:
	HWND m_hMainWnd = NULL;
	IUIWindow *m_pWindow = NULL;
	IXCore *m_pCore = NULL;
	IXUI *m_pXUI = NULL;

	IXCamera *m_pCamera = NULL;
	IUIViewport *m_pViewport = NULL;
	IUIPanel *m_pPanel = NULL;
	IUIGrid *m_pEmittersGrid = NULL;
	IUIButton *m_pEmitterAddButton = NULL;
	IUIButton *m_pEmitterRemoveButton = NULL;

	IUIButton *m_pPlayPauseButton = NULL;
	IUIButton *m_pRestartButton = NULL;
	IUIButton *m_pStopButton = NULL;

	IUIButton *m_pSaveButton = NULL;
	IUIButton *m_pCancelButton = NULL;

	IXParticleSystem *m_pParticleSystem = NULL;
	IXParticleEffect *m_pEffect = NULL;
	IXParticlePlayer *m_pPlayer = NULL;
	IXParticleEffectEmitter *m_pEffectEmitter = NULL;
	IXParticleBurst *m_pEmissionBurst = NULL;
	IUITextBox *m_pEmitterNameTextbox;

	float3_t m_vLookPoint;

	float m_fCamPitch = 0.0f;
	float m_fCamYaw = 0.0f;
	float m_fCamRadius = 2.0f;

	bool m_isOrbiting = false;
	float2_t m_v2MouseLastPos;

	bool m_isDirty = true;
	bool m_isPromptClose = false;

	struct GenericDataUI
	{
		IUITextBox *pDurationTextbox;
		IUICheckbox *pLoopingCheck;
		IUICheckbox *pPrewarmCheck;
		IUITextBox *pStartDelayTextbox;
		IUIMinMaxCurve *pStartLifetimeCurve;
		IUIMinMaxCurve *pStartSpeedCurve;
		IUICheckbox *pStartSizeSeparateCheck;
		IUIMinMaxCurve *pStartSizeCurve;
		IUIMinMaxCurve *pStartSizeXCurve;
		IUIMinMaxCurve *pStartSizeYCurve;
		IUIMinMaxCurve *pStartSizeZCurve;
		IUICheckbox *pStartRotationSeparateCheck;
		IUIMinMaxCurve *pStartRotationCurve;
		IUIMinMaxCurve *pStartRotationXCurve;
		IUIMinMaxCurve *pStartRotationYCurve;
		IUIMinMaxCurve *pStartRotationZCurve;
		IUITextBox *pFlipRotationTextbox;
		IUIComboBox *pCullingModeCombo;
		IUITextBox *pGravityModifierTextbox;
		IUITextBox *pMaxParticlesTextbox;
		IUIComboBox *pRingBufferModeCombo;
		IUIMinMaxCurve *pRingBufferLoopRangeCurve;
		IUIComboBox *pSimulationSpaceCombo;
		IUI2ColorGradient *pStartColorGradient;
	} m_uiGenericData = {};

	struct EmissionDataUI
	{
		IUIMinMaxCurve *pRatePerSecondCurve;
		IUIMinMaxCurve *pRatePerMeterCurve;
		IUIGrid *pBurstsGrid;
		IUIButton *pBurstAddButton;
		IUIButton *pBurstRemoveButton;
		IUITextBox *pBurstTimeTextbox;
		IUITextBox *pBurstCyclesTextbox;
		IUIMinMaxCurve *pBurstCountCurve;
		IUIMinMaxCurve *pBurstIntervalCurve;
		IUITextBox *pBurstProbabilityTextbox;
	} m_uiEmissionData = {};

	struct ShapeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUIComboBox *pShapeCombo;
		IUITextBox *pRadiusTextbox;
		IUITextBox *pRadiusThicknessTextbox;
		IUITextBox *pAngleTextbox;
		IUITextBox *pArcTextbox;
		IUIComboBox *pArcModeCombo;
		IUITextBox *pArcSpreadTextbox;
		IUIMinMaxCurve *pArcSpeedCurve;
		IUITextBox *pLengthTextbox;
		IUIComboBox *pConeEmitFromCombo;
		IUIComboBox *pBoxEmitFromCombo;
		IUITextBox *pDonutRadiusTextbox;
		IUICheckbox *pAlignToDirectionCheck;
		IUITextBox *pRandomizeDirectionTextbox;
		IUITextBox *pSpherizeDirectionTextbox;
		IUITextBox *pRandomizePositionTextbox;
		IUITextBox *pSizeXTextbox;
		IUITextBox *pSizeYTextbox;
		IUITextBox *pSizeZTextbox;
	} m_uiShapeData = {};

	struct VelocityLifetimeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUIMinMaxCurve *pLinearXCurve;
		IUIMinMaxCurve *pLinearYCurve;
		IUIMinMaxCurve *pLinearZCurve;
		IUIComboBox *pSimulationSpaceCombo;
		IUIMinMaxCurve *pOrbitalXCurve;
		IUIMinMaxCurve *pOrbitalYCurve;
		IUIMinMaxCurve *pOrbitalZCurve;
#if 0
		IUIMinMaxCurve *pOffsetXCurve;
		IUIMinMaxCurve *pOffsetYCurve;
		IUIMinMaxCurve *pOffsetZCurve;
#endif
		IUIMinMaxCurve *pRadialCurve;
		IUIMinMaxCurve *pSpeedModifierCurve;
	} m_uiVelocityLifetimeData = {};

	struct LimitVelocityLifetimeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUICheckbox *pSeparateAxesCheck;
		IUIComboBox *pSimulationSpaceCombo;
		IUIMinMaxCurve *pLimitCurve;
		IUIMinMaxCurve *pLimitXCurve;
		IUIMinMaxCurve *pLimitYCurve;
		IUIMinMaxCurve *pLimitZCurve;
		IUIMinMaxCurve *pDampenCurve;
		IUIMinMaxCurve *pDragCurve;
		IUICheckbox *pMultiplyBySizeCheck;
		IUICheckbox *pMultiplyByVelocityCheck;
	} m_uiLimitVelocityLifetimeData = {};

	struct ForceLifetimeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUIMinMaxCurve *pForceXCurve;
		IUIMinMaxCurve *pForceYCurve;
		IUIMinMaxCurve *pForceZCurve;
		IUIComboBox *pSimulationSpaceCombo;
		IUICheckbox *pRandomizeCheck;
	} m_uiForceLifetimeData = {};

	struct SizeLifetimeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUICheckbox *pSeparateAxesCheck;
		IUIMinMaxCurve *pSizeCurve;
		IUIMinMaxCurve *pSizeXCurve;
		IUIMinMaxCurve *pSizeYCurve;
		IUIMinMaxCurve *pSizeZCurve;
	} m_uiSizeLifetimeData = {};

	struct SizeSpeedDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUICheckbox *pSeparateAxesCheck;
		IUIMinMaxCurve *pSizeCurve;
		IUIMinMaxCurve *pSizeXCurve;
		IUIMinMaxCurve *pSizeYCurve;
		IUIMinMaxCurve *pSizeZCurve;
		IUITextBox *pSpeedRangeMinTextbox;
		IUITextBox *pSpeedRangeMaxTextbox;
	} m_uiSizeSpeedData = {};

	struct RotationLifetimeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUICheckbox *pSeparateAxesCheck;
		IUIMinMaxCurve *pAngularVelocityCurve;
		IUIMinMaxCurve *pAngularVelocityXCurve;
		IUIMinMaxCurve *pAngularVelocityYCurve;
		IUIMinMaxCurve *pAngularVelocityZCurve;
	} m_uiRotationLifetimeData = {};

	struct RotationSpeedDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUICheckbox *pSeparateAxesCheck;
		IUIMinMaxCurve *pAngularVelocityCurve;
		IUIMinMaxCurve *pAngularVelocityXCurve;
		IUIMinMaxCurve *pAngularVelocityYCurve;
		IUIMinMaxCurve *pAngularVelocityZCurve;
		IUITextBox *pSpeedRangeMinTextbox;
		IUITextBox *pSpeedRangeMaxTextbox;
	} m_uiRotationSpeedData = {};

	struct LifetimeEmitterSpeedDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUIMinMaxCurve *pMultiplierCurve;
		IUITextBox *pSpeedRangeMinTextbox;
		IUITextBox *pSpeedRangeMaxTextbox;
	} m_uiLifetimeEmitterSpeedData = {};

	struct ColorLifetimeDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUI2ColorGradient *pColorGradient;
	} m_uiColorLifetimeData = {};

	struct ColorSpeedDataUI
	{
		IUICheckbox *pEnabledCheck;
		IUI2ColorGradient *pColorGradient;
		IUITextBox *pSpeedRangeMinTextbox;
		IUITextBox *pSpeedRangeMaxTextbox;
	} m_uiColorSpeedData = {};

	struct RenderDataUI
	{
		//IUICheckbox *pEnabledCheck;
		IUIMaterialBox *pMaterial;
	} m_uiRenderData = {};

	CEffectEditorPlayerCallback m_playerCallback;

private:
	void onResize();

	void buildPropertiesUI();

	void enableControls(bool yesNo);

	void updateEmittersUI();

	void updateControlsUI();
	void updateControlsUIValues();

	void updateBurstRow();

	void onPlayerEvent(XPARTICLE_EVENT ev);

	void updatePlayPauseLabel();

	bool save();

	void updateCamera();
};

#endif
