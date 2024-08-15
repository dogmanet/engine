#include <xcommon/IPluginManager.h>

#include "EffectEditorWindow.h"

CEffectEditorWindow::CEffectEditorWindow(const char *szEffect, IXRender *pRender, HWND hMainWnd, IXCore *pCore, IXParticleSystem *pParticleSystem):
	m_hMainWnd(hMainWnd),
	m_pCore(pCore),
	m_pParticleSystem(pParticleSystem),
	m_playerCallback(this)
{
	// keep itself until window is closed
	AddRef();

	m_pXUI = (IXUI*)pCore->getPluginManager()->getInterface(IXUI_GUID);

	char szTitle[256];
	sprintf(szTitle, "Effect editor [%s]", szEffect);
	XWINDOW_DESC wdesc;
	wdesc.iPosX = XCW_USEDEFAULT;
	wdesc.iPosY = XCW_USEDEFAULT;
	wdesc.iSizeX = 800;
	wdesc.iSizeY = 600;
	wdesc.szTitle = szTitle;
	wdesc.flags = XWF_BUTTON_CLOSE | XWF_BUTTON_MAXIMIZE | XWF_TRANSPARENT;

	
	m_pWindow = m_pXUI->createWindow(&wdesc, (IUIWindow*)hMainWnd); // temporary hack!
	m_pWindow->setCallback(EventProc, this);

	m_pViewport = m_pXUI->createViewport();
	m_pWindow->insertChild(m_pViewport);

	m_pPanel = m_pXUI->createPanel();
	m_pWindow->insertChild(m_pPanel);

	m_pEmittersGrid = m_pXUI->createGrid();
	m_pWindow->insertChild(m_pEmittersGrid);

	m_pEmittersGrid->setColumnCount(1);
	m_pEmittersGrid->setColumnHeader(0, "Emitter");
	m_pEmittersGrid->setColumnWidth(0, 250);

	//m_pEmittersGrid->setRowCount(3);
	//m_pEmittersGrid->setCell(0, 0, "Fire");
	//m_pEmittersGrid->setCell(0, 1, "Sparks");
	//m_pEmittersGrid->setCell(0, 2, "Smoke");

	m_pEmitterAddButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pEmitterAddButton);
	m_pEmitterAddButton->setLabel("Add emitter");

	m_pEmitterRemoveButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pEmitterRemoveButton);
	m_pEmitterRemoveButton->setLabel("Remove emitter");

	m_pEmitterNameTextbox = m_pXUI->createTextBox();
	m_pWindow->insertChild(m_pEmitterNameTextbox);
	m_pEmitterNameTextbox->setLabel("Emitter name");

	m_pPlayPauseButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pPlayPauseButton);
	//m_pPlayPauseButton->setLabel("Pause");

	m_pRestartButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pRestartButton);
	m_pRestartButton->setLabel("Restart");

	m_pStopButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pStopButton);
	m_pStopButton->setLabel("Stop");

	m_pSaveButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pSaveButton);
	m_pSaveButton->setLabel("Save");

	m_pCancelButton = m_pXUI->createButton();
	m_pWindow->insertChild(m_pCancelButton);
	m_pCancelButton->setLabel("Cancel");

	buildPropertiesUI();

	IXRenderTarget *pTextureTarget;
	m_pViewport->getRenderTarget(&pTextureTarget);

	pRender->newCamera(&m_pCamera);
	m_pCamera->setPosition(float3(0.0f, 2.0f, -2.0f));
	m_pCamera->setOrientation(SMQuaternion(-SM_PIDIV4, 'x'));
	updateCamera();
	m_pCamera->setLayerMask(0x2);

	pTextureTarget->setCamera(m_pCamera);

	IXRenderGraph *pGraph;
	pRender->getGraph("xParticlePreview", &pGraph);
	pTextureTarget->attachGraph(pGraph);
	mem_release(pGraph);

	mem_release(pTextureTarget);

	if(!m_pParticleSystem->getEffect(szEffect, &m_pEffect) && !m_pParticleSystem->newEffect(szEffect, &m_pEffect))
	{
		LogFatal("Unable to create effect '%s'\n", szEffect);
	}

	m_pParticleSystem->newEffectPlayer(m_pEffect, &m_pPlayer);

	m_pPlayer->setLayer(1);

	m_pPlayer->setCallback(&m_playerCallback);

	m_pPlayer->play();

	updateEmittersUI();
	
	onResize();

	updatePlayPauseLabel();
}

template<typename T>
void ReleaseControls(T &ui)
{
	IUIControl **ppControls = (IUIControl**)&ui;
	for(UINT i = 0, il = sizeof(ui) / sizeof(void*); i < il; ++i)
	{
		mem_release(ppControls[i]);
	}
}

CEffectEditorWindow::~CEffectEditorWindow()
{
	ReleaseControls(m_uiGenericData);
	ReleaseControls(m_uiEmissionData);
	ReleaseControls(m_uiShapeData);
	ReleaseControls(m_uiVelocityLifetimeData);
	ReleaseControls(m_uiLimitVelocityLifetimeData);
	ReleaseControls(m_uiForceLifetimeData);
	ReleaseControls(m_uiSizeLifetimeData);
	ReleaseControls(m_uiSizeSpeedData);
	ReleaseControls(m_uiRotationLifetimeData);
	ReleaseControls(m_uiRotationSpeedData);
	ReleaseControls(m_uiLifetimeEmitterSpeedData);
	ReleaseControls(m_uiColorLifetimeData);
	ReleaseControls(m_uiColorSpeedData);
	ReleaseControls(m_uiRenderData);

	mem_release(m_pEffect);
	mem_release(m_pPlayer);
	mem_release(m_pCamera);
	mem_release(m_pWindow);
	mem_release(m_pViewport);
	mem_release(m_pPanel);
	mem_release(m_pEmittersGrid);
	mem_release(m_pEmitterAddButton);
	mem_release(m_pEmitterRemoveButton);
	mem_release(m_pEmitterNameTextbox);
	mem_release(m_pPlayPauseButton);
	mem_release(m_pRestartButton);
	mem_release(m_pStopButton);
	mem_release(m_pSaveButton);
	mem_release(m_pCancelButton);
}

template<typename T>
void EnableControls(bool yesNo, T &ui, UINT uStartFrom = 0)
{
	IUIControl **ppControls = (IUIControl**)&ui;
	for(UINT i = uStartFrom, il = sizeof(ui) / sizeof(void*); i < il; ++i)
	{
		ppControls[i]->setEnabled(yesNo);
	}
}


void CEffectEditorWindow::EventProc(void *pCtx, IUIControl *pControl, gui::IEvent *ev)
{
	CEffectEditorWindow *pThis = (CEffectEditorWindow*)pCtx;

	pThis->eventProc(pControl, ev);
}
void CEffectEditorWindow::eventProc(IUIControl *pControl, gui::IEvent *ev)
{
	if(ev->type == gui::GUI_EVENT_TYPE_CLOSE)
	{
		if(m_isDirty)
		{
			//	messagebox confirm
			ev->preventDefault = true;

			if(!m_isPromptClose)
			{
				char szPrompt[256];
				sprintf(szPrompt, "Save changes to %s?", "untitled.eff");

				m_isPromptClose = true;
				m_pWindow->messageBox(szPrompt, NULL, XMBF_YESNOCANCEL | XMBF_ICONWARNING, [](void *pCtx, XMESSAGE_BOX_RESULT result){
					CEffectEditorWindow *pThis = (CEffectEditorWindow*)pCtx;
					pThis->m_isPromptClose = false;

					if(result == XMBR_CANCEL)
					{
						return;
					}
					if(result == XMBR_YES)
					{
						if(!pThis->save())
						{
							return;
						}
					}
					if(result == XMBR_NO)
					{
						pThis->m_isDirty = false;
					}
					pThis->m_pWindow->close();
					//mem_release(pThis);
				}, this);
			}
		}
		else
		{
			Release();
		}
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_RESIZE)
	{
		onResize();
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_MOUSEMOVE && pControl == m_pViewport)
	{
		if(m_isOrbiting)
		{
			float2_t v2CurPos = float2_t(ev->clientX, ev->clientY);
			float2_t v2Offset = v2CurPos - m_v2MouseLastPos;
			m_v2MouseLastPos = v2CurPos;

			// x yaw
			// y pitch
			m_fCamYaw += v2Offset.x * SMToRadian(0.5f);
			m_fCamPitch += v2Offset.y * SMToRadian(0.5f);

			m_fCamPitch = clampf(m_fCamPitch, -SM_PIDIV2, SM_PIDIV2);

			updateCamera();
		}
	}
	if((ev->type == gui::GUI_EVENT_TYPE_MOUSEWHEELDOWN || ev->type == gui::GUI_EVENT_TYPE_MOUSEWHEELUP) && pControl == m_pViewport)
	{
		float fOffset = (ev->type == gui::GUI_EVENT_TYPE_MOUSEWHEELUP ? -m_fCamRadius * 0.1f : m_fCamRadius / 0.9f * 0.1f);

		const float c_fMinOffset = 0.01f;
		if(ev->type == gui::GUI_EVENT_TYPE_MOUSEWHEELDOWN && fOffset < c_fMinOffset)
		{
			fOffset = c_fMinOffset;
		}

		m_fCamRadius += fOffset;

		updateCamera();
	}
	if((ev->type == gui::GUI_EVENT_TYPE_MOUSEDOWN || ev->type == gui::GUI_EVENT_TYPE_MOUSEUP) && pControl == m_pViewport)
	{
		if(ev->key == KEY_LBUTTON)
		{
			m_isOrbiting = ev->type == gui::GUI_EVENT_TYPE_MOUSEDOWN;
			TODO("m_pWindow->setCapture(ev->target);");
			m_v2MouseLastPos = float2_t(ev->clientX, ev->clientY);
		}
	}

	if(ev->type == gui::GUI_EVENT_TYPE_CHANGE)
	{
		if(pControl == m_pEmittersGrid)
		{
			int iSelectedEmitter = m_pEmittersGrid->getSelection();
			if(iSelectedEmitter >= 0)
			{
				m_pEffectEmitter = m_pEffect->getEmitterAt(iSelectedEmitter);
				
				updateControlsUIValues();
			}
			else
			{
				m_pEffectEmitter = NULL;

				m_uiEmissionData.pBurstsGrid->setSelection(-1);
				m_uiEmissionData.pBurstsGrid->setRowCount(0);
			}
			enableControls(m_pEffectEmitter != NULL);
		}
		else if(pControl == m_uiGenericData.pLoopingCheck)
		{
			m_pEffectEmitter->getGenericData()->setLooping(m_uiGenericData.pLoopingCheck->isChecked());
		}
		else if(pControl == m_uiGenericData.pPrewarmCheck)
		{
			m_pEffectEmitter->getGenericData()->setPrewarm(m_uiGenericData.pPrewarmCheck->isChecked());
		}
		else if(pControl == m_uiGenericData.pStartSizeSeparateCheck)
		{
			m_pEffectEmitter->getGenericData()->setStartSizeSeparate(m_uiGenericData.pStartSizeSeparateCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiGenericData.pStartRotationSeparateCheck)
		{
			m_pEffectEmitter->getGenericData()->setStartRotationSeparate(m_uiGenericData.pStartRotationSeparateCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiGenericData.pCullingModeCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_CULLING_MODE>().find(m_uiGenericData.pCullingModeCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getGenericData()->setCullingMode((XPARTICLE_CULLING_MODE)val.getValue());
			}
		}
		else if(pControl == m_uiGenericData.pRingBufferModeCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_RING_BUFFER_MODE>().find(m_uiGenericData.pRingBufferModeCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getGenericData()->setRingBufferMode((XPARTICLE_RING_BUFFER_MODE)val.getValue());
				updateControlsUI();
			}
		}
		else if(pControl == m_uiGenericData.pSimulationSpaceCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(m_uiGenericData.pSimulationSpaceCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getGenericData()->setSimulationSpace((XPARTICLE_SIMULATION_SPACE)val.getValue());
			}
		}
		else if(pControl == m_uiEmissionData.pBurstsGrid)
		{
			int iSelectedBurst = m_uiEmissionData.pBurstsGrid->getSelection();
			if(iSelectedBurst >= 0)
			{
				m_pEmissionBurst = m_pEffectEmitter->getEmissionData()->getBurstAt(iSelectedBurst);

				char tmp[256];
				sprintf(tmp, "%g", m_pEmissionBurst->getTime());
				m_uiEmissionData.pBurstTimeTextbox->setValue(tmp);

				sprintf(tmp, "%u", m_pEmissionBurst->getCycles());
				m_uiEmissionData.pBurstCyclesTextbox->setValue(tmp);

				m_uiEmissionData.pBurstCountCurve->setCurve(m_pEmissionBurst->getCountCurve());

				m_uiEmissionData.pBurstIntervalCurve->setCurve(m_pEmissionBurst->getIntervalCurve());

				sprintf(tmp, "%g", m_pEmissionBurst->getProbability());
				m_uiEmissionData.pBurstProbabilityTextbox->setValue(tmp);
			}
			else
			{
				m_pEmissionBurst = NULL;
			}

			m_uiEmissionData.pBurstRemoveButton->setEnabled(m_pEmissionBurst != NULL);
			m_uiEmissionData.pBurstTimeTextbox->setEnabled(m_pEmissionBurst != NULL);
			m_uiEmissionData.pBurstCyclesTextbox->setEnabled(m_pEmissionBurst != NULL);
			m_uiEmissionData.pBurstCountCurve->setEnabled(m_pEmissionBurst != NULL);
			m_uiEmissionData.pBurstIntervalCurve->setEnabled(m_pEmissionBurst != NULL);
			m_uiEmissionData.pBurstProbabilityTextbox->setEnabled(m_pEmissionBurst != NULL);
		}
		else if(pControl == m_uiEmissionData.pBurstCountCurve || pControl == m_uiEmissionData.pBurstIntervalCurve)
		{
			updateBurstRow();
		}
		else if(pControl == m_uiShapeData.pEnabledCheck)
		{
			m_pEffectEmitter->getShapeData()->enable(m_uiShapeData.pEnabledCheck->isChecked());
			EnableControls(m_uiShapeData.pEnabledCheck->isChecked(), m_uiShapeData, 1);
		}
		else if(pControl == m_uiShapeData.pShapeCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SHAPE>().find(m_uiShapeData.pShapeCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getShapeData()->setShape((XPARTICLE_SHAPE)val.getValue());
				updateControlsUI();
			}
		}
		else if(pControl == m_uiShapeData.pArcModeCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SHAPE_ARC_MODE>().find(m_uiShapeData.pArcModeCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getShapeData()->setArcMode((XPARTICLE_SHAPE_ARC_MODE)val.getValue());
				updateControlsUI();
			}
		}
		else if(pControl == m_uiShapeData.pConeEmitFromCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SHAPE_CONE_EMIT_FROM>().find(m_uiShapeData.pConeEmitFromCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getShapeData()->setConeEmitFrom((XPARTICLE_SHAPE_CONE_EMIT_FROM)val.getValue());
			}
		}
		else if(pControl == m_uiShapeData.pBoxEmitFromCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SHAPE_BOX_EMIT_FROM>().find(m_uiShapeData.pBoxEmitFromCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getShapeData()->setBoxEmitFrom((XPARTICLE_SHAPE_BOX_EMIT_FROM)val.getValue());
			}
		}
		else if(pControl == m_uiShapeData.pAlignToDirectionCheck)
		{
			m_pEffectEmitter->getShapeData()->setAlignToDirection(m_uiShapeData.pAlignToDirectionCheck->isChecked());
		}
		else if(pControl == m_uiVelocityLifetimeData.pEnabledCheck)
		{
			m_pEffectEmitter->getVelocityLifetimeData()->enable(m_uiVelocityLifetimeData.pEnabledCheck->isChecked());
			EnableControls(m_uiVelocityLifetimeData.pEnabledCheck->isChecked(), m_uiVelocityLifetimeData, 1);
		}
		else if(pControl == m_uiVelocityLifetimeData.pSimulationSpaceCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(m_uiVelocityLifetimeData.pSimulationSpaceCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getVelocityLifetimeData()->setSimulationSpace((XPARTICLE_SIMULATION_SPACE)val.getValue());
			}
		}
		else if(pControl == m_uiLimitVelocityLifetimeData.pEnabledCheck)
		{
			m_pEffectEmitter->getLimitVelocityLifetimeData()->enable(m_uiLimitVelocityLifetimeData.pEnabledCheck->isChecked());
			EnableControls(m_uiLimitVelocityLifetimeData.pEnabledCheck->isChecked(), m_uiLimitVelocityLifetimeData, 1);
		}
		else if(pControl == m_uiLimitVelocityLifetimeData.pSeparateAxesCheck)
		{
			m_pEffectEmitter->getLimitVelocityLifetimeData()->setSeparateAxes(m_uiLimitVelocityLifetimeData.pSeparateAxesCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiLimitVelocityLifetimeData.pMultiplyBySizeCheck)
		{
			m_pEffectEmitter->getLimitVelocityLifetimeData()->setMultiplyBySize(m_uiLimitVelocityLifetimeData.pMultiplyBySizeCheck->isChecked());
		}
		else if(pControl == m_uiLimitVelocityLifetimeData.pMultiplyByVelocityCheck)
		{
			m_pEffectEmitter->getLimitVelocityLifetimeData()->setMultiplyBySize(m_uiLimitVelocityLifetimeData.pMultiplyByVelocityCheck->isChecked());
		}
		else if(pControl == m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getLimitVelocityLifetimeData()->setSimulationSpace((XPARTICLE_SIMULATION_SPACE)val.getValue());
			}
		}
		else if(pControl == m_uiForceLifetimeData.pEnabledCheck)
		{
			m_pEffectEmitter->getForceLifetimeData()->enable(m_uiForceLifetimeData.pEnabledCheck->isChecked());
			EnableControls(m_uiForceLifetimeData.pEnabledCheck->isChecked(), m_uiForceLifetimeData, 1);
		}
		else if(pControl == m_uiForceLifetimeData.pSimulationSpaceCombo)
		{
			EnumReflector::Enumerator val = EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(m_uiForceLifetimeData.pSimulationSpaceCombo->getValue());
			if(val.isValid())
			{
				m_pEffectEmitter->getForceLifetimeData()->setSimulationSpace((XPARTICLE_SIMULATION_SPACE)val.getValue());
			}
		}
		else if(pControl == m_uiForceLifetimeData.pRandomizeCheck)
		{
			m_pEffectEmitter->getForceLifetimeData()->setRandomize(m_uiForceLifetimeData.pRandomizeCheck->isChecked());
		}
		else if(pControl == m_uiSizeLifetimeData.pEnabledCheck)
		{
			m_pEffectEmitter->getSizeLifetimeData()->enable(m_uiSizeLifetimeData.pEnabledCheck->isChecked());
			EnableControls(m_uiSizeLifetimeData.pEnabledCheck->isChecked(), m_uiSizeLifetimeData, 1);
		}
		else if(pControl == m_uiSizeLifetimeData.pSeparateAxesCheck)
		{
			m_pEffectEmitter->getSizeLifetimeData()->setSeparateAxes(m_uiSizeLifetimeData.pSeparateAxesCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiSizeSpeedData.pEnabledCheck)
		{
			m_pEffectEmitter->getSizeSpeedData()->enable(m_uiSizeSpeedData.pEnabledCheck->isChecked());
			EnableControls(m_uiSizeSpeedData.pEnabledCheck->isChecked(), m_uiSizeSpeedData, 1);
		}
		else if(pControl == m_uiSizeSpeedData.pSeparateAxesCheck)
		{
			m_pEffectEmitter->getSizeSpeedData()->setSeparateAxes(m_uiSizeSpeedData.pSeparateAxesCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiRotationLifetimeData.pEnabledCheck)
		{
			m_pEffectEmitter->getRotationLifetimeData()->enable(m_uiRotationLifetimeData.pEnabledCheck->isChecked());
			EnableControls(m_uiRotationLifetimeData.pEnabledCheck->isChecked(), m_uiRotationLifetimeData, 1);
		}
		else if(pControl == m_uiRotationLifetimeData.pSeparateAxesCheck)
		{
			m_pEffectEmitter->getRotationLifetimeData()->setSeparateAxes(m_uiRotationLifetimeData.pSeparateAxesCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiRotationSpeedData.pEnabledCheck)
		{
			m_pEffectEmitter->getRotationSpeedData()->enable(m_uiRotationSpeedData.pEnabledCheck->isChecked());
			EnableControls(m_uiRotationSpeedData.pEnabledCheck->isChecked(), m_uiRotationSpeedData, 1);
		}
		else if(pControl == m_uiRotationSpeedData.pSeparateAxesCheck)
		{
			m_pEffectEmitter->getRotationSpeedData()->setSeparateAxes(m_uiRotationSpeedData.pSeparateAxesCheck->isChecked());
			updateControlsUI();
		}
		else if(pControl == m_uiLifetimeEmitterSpeedData.pEnabledCheck)
		{
			m_pEffectEmitter->getLifetimeEmitterSpeedData()->enable(m_uiLifetimeEmitterSpeedData.pEnabledCheck->isChecked());
			EnableControls(m_uiLifetimeEmitterSpeedData.pEnabledCheck->isChecked(), m_uiLifetimeEmitterSpeedData, 1);
		}
		else if(pControl == m_uiColorLifetimeData.pEnabledCheck)
		{
			m_pEffectEmitter->getColorLifetimeData()->enable(m_uiColorLifetimeData.pEnabledCheck->isChecked());
			EnableControls(m_uiColorLifetimeData.pEnabledCheck->isChecked(), m_uiColorLifetimeData, 1);
		}
		else if(pControl == m_uiColorSpeedData.pEnabledCheck)
		{
			m_pEffectEmitter->getColorSpeedData()->enable(m_uiColorSpeedData.pEnabledCheck->isChecked());
			EnableControls(m_uiColorSpeedData.pEnabledCheck->isChecked(), m_uiColorSpeedData, 1);
		}
		else if(pControl == m_uiRenderData.pMaterial)
		{
			m_pEffectEmitter->getRenderData()->setMaterial(m_uiRenderData.pMaterial->getValue());
		}
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
	{
		if(pControl == m_pEmitterAddButton)
		{
			UINT uCount = m_pEffect->getEmitterCount();
			m_pEffect->setEmitterCount(uCount + 1);
			updateEmittersUI();
			m_pEmittersGrid->setSelection(uCount);
		}
		else if(pControl == m_pEmitterRemoveButton)
		{
			int iSelected = m_pEmittersGrid->getSelection();
			if(iSelected >= 0)
			{
				m_pEffect->removeEmitterAt(iSelected);
				m_pEmittersGrid->setSelection(-1);
				updateEmittersUI();
			}
		}
		else if(pControl == m_uiEmissionData.pBurstAddButton)
		{
			UINT uBursts = m_pEffectEmitter->getEmissionData()->getBurstsCount();
			m_pEffectEmitter->getEmissionData()->setBurstsCount(uBursts + 1);
			updateControlsUIValues();
			m_uiEmissionData.pBurstsGrid->setSelection(uBursts);
		}
		else if(pControl == m_uiEmissionData.pBurstRemoveButton)
		{
			int iSelected = m_uiEmissionData.pBurstsGrid->getSelection();
			if(iSelected >= 0)
			{
				m_pEffectEmitter->getEmissionData()->removeBurstAt(iSelected);
				m_uiEmissionData.pBurstsGrid->setSelection(-1);
				updateControlsUIValues();
			}
		}
		else if(pControl == m_pPlayPauseButton)
		{
			if(m_pPlayer->isPlaying())
			{
				m_pPlayer->pause();
			}
			else
			{
				m_pPlayer->play();
			}
			updatePlayPauseLabel();
		}
		else if(pControl == m_pRestartButton)
		{
			m_pPlayer->stop(true);
			m_pPlayer->play();
			updatePlayPauseLabel();
		}
		else if(pControl == m_pStopButton)
		{
			m_pPlayer->stop(true);
		}
		else if(pControl == m_pSaveButton)
		{
			save();
		}
		else if(pControl == m_pCancelButton)
		{
			m_pWindow->close();
		}
		return;
	}

	if(ev->type == gui::GUI_EVENT_TYPE_KEYUP)
	{
		float fValue;
		if(pControl == m_pEmitterNameTextbox)
		{
			m_pEffectEmitter->setName(m_pEmitterNameTextbox->getValue());
			updateEmittersUI();
		}
		else if(pControl == m_uiGenericData.pDurationTextbox)
		{
			if(sscanf(m_uiGenericData.pDurationTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getGenericData()->setDuration(fValue);
			}
		}
		else if(pControl == m_uiGenericData.pStartDelayTextbox)
		{
			if(sscanf(m_uiGenericData.pStartDelayTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getGenericData()->setStartDelay(fValue);
			}
		}
		else if(pControl == m_uiGenericData.pFlipRotationTextbox)
		{
			if(sscanf(m_uiGenericData.pFlipRotationTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getGenericData()->setFlipRotation(fValue);
			}
		}
		else if(pControl == m_uiGenericData.pGravityModifierTextbox)
		{
			if(sscanf(m_uiGenericData.pGravityModifierTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getGenericData()->setGravityModifier(fValue);
			}
		}
		else if(pControl == m_uiGenericData.pMaxParticlesTextbox)
		{
			UINT uCount;
			if(sscanf(m_uiGenericData.pMaxParticlesTextbox->getValue(), "%u", &uCount) > 0)
			{
				m_pEffectEmitter->getGenericData()->setMaxParticles(uCount);
			}
		}
		else if(pControl == m_uiEmissionData.pBurstTimeTextbox)
		{
			if(sscanf(m_uiEmissionData.pBurstTimeTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEmissionBurst->setTime(fValue);
				updateBurstRow();
			}
		}
		else if(pControl == m_uiEmissionData.pBurstCyclesTextbox)
		{
			UINT uCount;
			if(sscanf(m_uiEmissionData.pBurstCyclesTextbox->getValue(), "%u", &uCount) > 0)
			{
				m_pEmissionBurst->setCycles(uCount);
				updateBurstRow();
			}
		}
		else if(pControl == m_uiEmissionData.pBurstProbabilityTextbox)
		{
			if(sscanf(m_uiEmissionData.pBurstProbabilityTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEmissionBurst->setProbability(fValue);
				updateBurstRow();
			}
		}
		else if(pControl == m_uiShapeData.pRadiusTextbox)
		{
			if(sscanf(m_uiShapeData.pRadiusTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setRadius(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pRadiusThicknessTextbox)
		{
			if(sscanf(m_uiShapeData.pRadiusThicknessTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setRadiusThickness(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pAngleTextbox)
		{
			if(sscanf(m_uiShapeData.pAngleTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setAngle(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pArcTextbox)
		{
			if(sscanf(m_uiShapeData.pArcTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setArc(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pArcSpreadTextbox)
		{
			if(sscanf(m_uiShapeData.pArcSpreadTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setArcSpread(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pLengthTextbox)
		{
			if(sscanf(m_uiShapeData.pLengthTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setLength(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pDonutRadiusTextbox)
		{
			if(sscanf(m_uiShapeData.pDonutRadiusTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setDonutRadius(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pRandomizeDirectionTextbox)
		{
			if(sscanf(m_uiShapeData.pRandomizeDirectionTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setRandomizeDirection(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pSpherizeDirectionTextbox)
		{
			if(sscanf(m_uiShapeData.pSpherizeDirectionTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setSpherizeDirection(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pRandomizePositionTextbox)
		{
			if(sscanf(m_uiShapeData.pRandomizePositionTextbox->getValue(), "%g", &fValue) > 0)
			{
				m_pEffectEmitter->getShapeData()->setRandomizePosition(fValue);
			}
		}
		else if(pControl == m_uiShapeData.pSizeXTextbox)
		{
			if(sscanf(m_uiShapeData.pSizeXTextbox->getValue(), "%g", &fValue) > 0)
			{
				float3_t vSize = m_pEffectEmitter->getShapeData()->getSize();
				vSize.x = fValue;
				m_pEffectEmitter->getShapeData()->setSize(vSize);
			}
		}
		else if(pControl == m_uiShapeData.pSizeYTextbox)
		{
			if(sscanf(m_uiShapeData.pSizeYTextbox->getValue(), "%g", &fValue) > 0)
			{
				float3_t vSize = m_pEffectEmitter->getShapeData()->getSize();
				vSize.y = fValue;
				m_pEffectEmitter->getShapeData()->setSize(vSize);
			}
		}
		else if(pControl == m_uiShapeData.pSizeZTextbox)
		{
			if(sscanf(m_uiShapeData.pSizeZTextbox->getValue(), "%g", &fValue) > 0)
			{
				float3_t vSize = m_pEffectEmitter->getShapeData()->getSize();
				vSize.z = fValue;
				m_pEffectEmitter->getShapeData()->setSize(vSize);
			}
		}
		else if(pControl == m_uiSizeSpeedData.pSpeedRangeMinTextbox)
		{
			if(sscanf(m_uiSizeSpeedData.pSpeedRangeMinTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getSizeSpeedData()->getSpeedRange();
				vRange.x = fValue;
				m_pEffectEmitter->getSizeSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiSizeSpeedData.pSpeedRangeMaxTextbox)
		{
			if(sscanf(m_uiSizeSpeedData.pSpeedRangeMaxTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getSizeSpeedData()->getSpeedRange();
				vRange.y = fValue;
				m_pEffectEmitter->getSizeSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiRotationSpeedData.pSpeedRangeMinTextbox)
		{
			if(sscanf(m_uiRotationSpeedData.pSpeedRangeMinTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getRotationSpeedData()->getSpeedRange();
				vRange.x = fValue;
				m_pEffectEmitter->getRotationSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiRotationSpeedData.pSpeedRangeMaxTextbox)
		{
			if(sscanf(m_uiRotationSpeedData.pSpeedRangeMaxTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getRotationSpeedData()->getSpeedRange();
				vRange.y = fValue;
				m_pEffectEmitter->getRotationSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiLifetimeEmitterSpeedData.pSpeedRangeMinTextbox)
		{
			if(sscanf(m_uiLifetimeEmitterSpeedData.pSpeedRangeMinTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getLifetimeEmitterSpeedData()->getSpeedRange();
				vRange.x = fValue;
				m_pEffectEmitter->getLifetimeEmitterSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiLifetimeEmitterSpeedData.pSpeedRangeMaxTextbox)
		{
			if(sscanf(m_uiLifetimeEmitterSpeedData.pSpeedRangeMaxTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getLifetimeEmitterSpeedData()->getSpeedRange();
				vRange.y = fValue;
				m_pEffectEmitter->getLifetimeEmitterSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiColorSpeedData.pSpeedRangeMinTextbox)
		{
			if(sscanf(m_uiColorSpeedData.pSpeedRangeMinTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getColorSpeedData()->getSpeedRange();
				vRange.x = fValue;
				m_pEffectEmitter->getColorSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiColorSpeedData.pSpeedRangeMaxTextbox)
		{
			if(sscanf(m_uiColorSpeedData.pSpeedRangeMaxTextbox->getValue(), "%g", &fValue) > 0)
			{
				float2_t vRange = m_pEffectEmitter->getColorSpeedData()->getSpeedRange();
				vRange.y = fValue;
				m_pEffectEmitter->getColorSpeedData()->setSpeedRange(vRange);
			}
		}
		else if(pControl == m_uiRenderData.pMaterial)
		{
			m_pEffectEmitter->getRenderData()->setMaterial(m_uiRenderData.pMaterial->getValue());
		}
		return;
	}
}

void CEffectEditorWindow::onResize()
{
	UINT uWidth = m_pWindow->getDesc()->iSizeX;
	UINT uHeight = m_pWindow->getDesc()->iSizeY;
	LogInfo("onResize(%i, %i)\n", m_pWindow->getDesc()->iSizeX, m_pWindow->getDesc()->iSizeY);
	
	float fExternalMargin = 20.0f;
	float fSpacing = 10.0f;
	float fEmitterButtonWidth = 100.0f;
	float fEmitterButtonHeight = 24.0f;
	float fEmitterTextboxHeight = 48.0f;
	float fEmitterWindowHeight = 120.0f;

	float fPlaybackButtonWidth = 50.0f;
	float fPlaybackButtonHeight = 24.0f;

	float fControlButtonHeight = 24.0f;

	float fViewportWidth = uWidth * 0.5f - fExternalMargin;
	float fViewportHeight = uHeight - fExternalMargin * 2.0f - fSpacing - fPlaybackButtonHeight;
	m_pViewport->setPosition(fExternalMargin, fExternalMargin);
	m_pViewport->setSize(fViewportWidth, fViewportHeight);

	float fPanelWidth = uWidth * 0.5f - fSpacing - fExternalMargin;
	float fPanelHeight = uHeight - fEmitterWindowHeight - fExternalMargin * 2.0f - fSpacing * 2.0f - fControlButtonHeight;

	m_pPanel->setPosition(uWidth * 0.5f + fSpacing, fExternalMargin + fEmitterWindowHeight + fSpacing);
	m_pPanel->setSize(fPanelWidth, fPanelHeight);

	m_pEmittersGrid->setPosition(uWidth * 0.5f + fSpacing, fExternalMargin);
	m_pEmittersGrid->setSize(uWidth * 0.5f - fSpacing * 2.0f - fEmitterButtonWidth - fExternalMargin, fEmitterWindowHeight);

	m_pEmitterAddButton->setPosition(uWidth - fExternalMargin - fEmitterButtonWidth, fExternalMargin);
	m_pEmitterAddButton->setSize(fEmitterButtonWidth, fEmitterButtonHeight);

	m_pEmitterRemoveButton->setPosition(uWidth - fExternalMargin - fEmitterButtonWidth, fExternalMargin + fEmitterButtonHeight + fSpacing);
	m_pEmitterRemoveButton->setSize(fEmitterButtonWidth, fEmitterButtonHeight);

	m_pEmitterNameTextbox->setPosition(uWidth - fExternalMargin - fEmitterButtonWidth, fExternalMargin + 2.0f * (fEmitterButtonHeight + fSpacing));
	m_pEmitterNameTextbox->setSize(fEmitterButtonWidth, fEmitterTextboxHeight);

	float fColWidth = (uWidth * 0.5f - 5.0f * fSpacing - fExternalMargin) / 5.0f;
	for(UINT i = 0; i < 5; ++i)
	{
		m_uiEmissionData.pBurstsGrid->setColumnWidth(i, fColWidth);
	}


	fPlaybackButtonWidth = (fViewportWidth - fSpacing * 2.0f) / 3.0f;

	m_pPlayPauseButton->setPosition(fExternalMargin, fExternalMargin + fViewportHeight + fSpacing);
	m_pPlayPauseButton->setSize(fPlaybackButtonWidth, fPlaybackButtonHeight);

	m_pRestartButton->setPosition(fExternalMargin + fPlaybackButtonWidth + fSpacing, fExternalMargin + fViewportHeight + fSpacing);
	m_pRestartButton->setSize(fPlaybackButtonWidth, fPlaybackButtonHeight);

	m_pStopButton->setPosition(fExternalMargin + (fPlaybackButtonWidth + fSpacing) * 2.0f, fExternalMargin + fViewportHeight + fSpacing);
	m_pStopButton->setSize(fPlaybackButtonWidth, fPlaybackButtonHeight);

	float fControlButtonWidth = (fPanelWidth - fSpacing) / 2.0f;

	m_pSaveButton->setPosition(uWidth * 0.5f + fSpacing, fExternalMargin + fEmitterWindowHeight + fSpacing * 2.0f + fPanelHeight);
	m_pSaveButton->setSize(fControlButtonWidth, fControlButtonHeight);

	m_pCancelButton->setPosition(uWidth * 0.5f + fSpacing * 2.0f + fControlButtonWidth, fExternalMargin + fEmitterWindowHeight + fSpacing * 2.0f + fPanelHeight);
	m_pCancelButton->setSize(fControlButtonWidth, fControlButtonHeight);
}

void CEffectEditorWindow::buildPropertiesUI()
{
	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Generic data");
		pSpoiler->setCollapsed(true);

		m_uiGenericData.pDurationTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiGenericData.pDurationTextbox);
		m_uiGenericData.pDurationTextbox->setLabel("Duration");

		m_uiGenericData.pLoopingCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiGenericData.pLoopingCheck);
		m_uiGenericData.pLoopingCheck->setLabel("Looping");

		m_uiGenericData.pPrewarmCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiGenericData.pPrewarmCheck);
		m_uiGenericData.pPrewarmCheck->setLabel("Prewarm");
		m_uiGenericData.pPrewarmCheck->setVisible(false);

		m_uiGenericData.pStartDelayTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiGenericData.pStartDelayTextbox);
		m_uiGenericData.pStartDelayTextbox->setLabel("Start delay");

		m_uiGenericData.pStartLifetimeCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartLifetimeCurve);
		m_uiGenericData.pStartLifetimeCurve->setLabel("Start lifetime");

		m_uiGenericData.pStartSpeedCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartSpeedCurve);
		m_uiGenericData.pStartSpeedCurve->setLabel("Start speed");

		m_uiGenericData.pStartSizeSeparateCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiGenericData.pStartSizeSeparateCheck);
		m_uiGenericData.pStartSizeSeparateCheck->setLabel("Start size separate");

		m_uiGenericData.pStartSizeCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartSizeCurve);
		m_uiGenericData.pStartSizeCurve->setLabel("Start size");

		m_uiGenericData.pStartSizeXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartSizeXCurve);
		m_uiGenericData.pStartSizeXCurve->setLabel("Start size X");

		m_uiGenericData.pStartSizeYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartSizeYCurve);
		m_uiGenericData.pStartSizeYCurve->setLabel("Start size Y");

		m_uiGenericData.pStartSizeZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartSizeZCurve);
		m_uiGenericData.pStartSizeZCurve->setLabel("Start size Z");

		m_uiGenericData.pStartRotationSeparateCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiGenericData.pStartRotationSeparateCheck);
		m_uiGenericData.pStartRotationSeparateCheck->setLabel("Start rotation separate");

		m_uiGenericData.pStartRotationCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartRotationCurve);
		m_uiGenericData.pStartRotationCurve->setLabel("Start rotation");

		m_uiGenericData.pStartRotationXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartRotationXCurve);
		m_uiGenericData.pStartRotationXCurve->setLabel("Start rotation X");

		m_uiGenericData.pStartRotationYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartRotationYCurve);
		m_uiGenericData.pStartRotationYCurve->setLabel("Start rotation Y");

		m_uiGenericData.pStartRotationZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pStartRotationZCurve);
		m_uiGenericData.pStartRotationZCurve->setLabel("Start rotation Z");

		m_uiGenericData.pFlipRotationTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiGenericData.pFlipRotationTextbox);
		m_uiGenericData.pFlipRotationTextbox->setLabel("Flip rotation");

		m_uiGenericData.pCullingModeCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiGenericData.pCullingModeCombo);
		m_uiGenericData.pCullingModeCombo->setLabel("Culling mode");
		m_uiGenericData.pCullingModeCombo->addItem("Auto", "XPCM_AUTO");
		m_uiGenericData.pCullingModeCombo->addItem("Smart pause", "XPCM_SMART_PAUSE");
		m_uiGenericData.pCullingModeCombo->addItem("Pause", "XPCM_PAUSE");
		m_uiGenericData.pCullingModeCombo->addItem("Always run", "XPCM_ALWAYS_RUN");
		m_uiGenericData.pCullingModeCombo->setVisible(false);

		m_uiGenericData.pMaxParticlesTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiGenericData.pMaxParticlesTextbox);
		m_uiGenericData.pMaxParticlesTextbox->setLabel("Max particles");

		m_uiGenericData.pGravityModifierTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiGenericData.pGravityModifierTextbox);
		m_uiGenericData.pGravityModifierTextbox->setLabel("Gravity modifier");

		m_uiGenericData.pRingBufferModeCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiGenericData.pRingBufferModeCombo);
		m_uiGenericData.pRingBufferModeCombo->setLabel("Ring buffer mode");
		m_uiGenericData.pRingBufferModeCombo->addItem("Disabled", "XPRBM_DISABLED");
		m_uiGenericData.pRingBufferModeCombo->addItem("Pause", "XPRBM_PAUSE");
		m_uiGenericData.pRingBufferModeCombo->addItem("Loop", "XPRBM_LOOP");
		m_uiGenericData.pRingBufferModeCombo->setVisible(false);

		m_uiGenericData.pRingBufferLoopRangeCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiGenericData.pRingBufferLoopRangeCurve);
		m_uiGenericData.pRingBufferLoopRangeCurve->setLabel("Ring buffer loop range");
		m_uiGenericData.pRingBufferLoopRangeCurve->setVisible(false);

		m_uiGenericData.pSimulationSpaceCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiGenericData.pSimulationSpaceCombo);
		m_uiGenericData.pSimulationSpaceCombo->setLabel("Simulation space");
		m_uiGenericData.pSimulationSpaceCombo->addItem("Local", "XPSS_LOCAL");
		m_uiGenericData.pSimulationSpaceCombo->addItem("World", "XPSS_WORLD");

		m_uiGenericData.pStartColorGradient = m_pXUI->create2ColorGradient();
		pSpoiler->insertChild(m_uiGenericData.pStartColorGradient);
		m_uiGenericData.pStartColorGradient->setLabel("Start color");


		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Emission");
		pSpoiler->setCollapsed(true);

		m_uiEmissionData.pRatePerSecondCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiEmissionData.pRatePerSecondCurve);
		m_uiEmissionData.pRatePerSecondCurve->setLabel("Rate per second");

		m_uiEmissionData.pRatePerMeterCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiEmissionData.pRatePerMeterCurve);
		m_uiEmissionData.pRatePerMeterCurve->setLabel("Rate per meter");

		m_uiEmissionData.pBurstsGrid = m_pXUI->createGrid();
		pSpoiler->insertChild(m_uiEmissionData.pBurstsGrid);
		m_uiEmissionData.pBurstsGrid->setLabel("Bursts");

		m_uiEmissionData.pBurstsGrid->setColumnCount(5);
		m_uiEmissionData.pBurstsGrid->setColumnHeader(0, "Time");
		m_uiEmissionData.pBurstsGrid->setColumnHeader(1, "Cycles");
		m_uiEmissionData.pBurstsGrid->setColumnHeader(2, "Count");
		m_uiEmissionData.pBurstsGrid->setColumnHeader(3, "Interval");
		m_uiEmissionData.pBurstsGrid->setColumnHeader(4, "Prob.");

		m_uiEmissionData.pBurstAddButton = m_pXUI->createButton();
		pSpoiler->insertChild(m_uiEmissionData.pBurstAddButton);
		m_uiEmissionData.pBurstAddButton->setLabel("Add burst");

		m_uiEmissionData.pBurstRemoveButton = m_pXUI->createButton();
		pSpoiler->insertChild(m_uiEmissionData.pBurstRemoveButton);
		m_uiEmissionData.pBurstRemoveButton->setLabel("Remove burst");

		m_uiEmissionData.pBurstTimeTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiEmissionData.pBurstTimeTextbox);
		m_uiEmissionData.pBurstTimeTextbox->setLabel("Burst time");

		m_uiEmissionData.pBurstCyclesTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiEmissionData.pBurstCyclesTextbox);
		m_uiEmissionData.pBurstCyclesTextbox->setLabel("Burst cycles");

		m_uiEmissionData.pBurstCountCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiEmissionData.pBurstCountCurve);
		m_uiEmissionData.pBurstCountCurve->setLabel("Burst count");

		m_uiEmissionData.pBurstIntervalCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiEmissionData.pBurstIntervalCurve);
		m_uiEmissionData.pBurstIntervalCurve->setLabel("Burst interval");

		m_uiEmissionData.pBurstProbabilityTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiEmissionData.pBurstProbabilityTextbox);
		m_uiEmissionData.pBurstProbabilityTextbox->setLabel("Burst probability");


		m_uiEmissionData.pBurstRemoveButton->setEnabled(false);
		m_uiEmissionData.pBurstTimeTextbox->setEnabled(false);
		m_uiEmissionData.pBurstCyclesTextbox->setEnabled(false);
		m_uiEmissionData.pBurstCountCurve->setEnabled(false);
		m_uiEmissionData.pBurstIntervalCurve->setEnabled(false);
		m_uiEmissionData.pBurstProbabilityTextbox->setEnabled(false);

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Render");
		pSpoiler->setCollapsed(true);

		m_uiRenderData.pMaterial = m_pXUI->createMaterialBox();
		pSpoiler->insertChild(m_uiRenderData.pMaterial);
		m_uiRenderData.pMaterial->setLabel("Material");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Shape");
		pSpoiler->setCollapsed(true);

		m_uiShapeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiShapeData.pEnabledCheck);
		m_uiShapeData.pEnabledCheck->setLabel("Enabled");

		m_uiShapeData.pShapeCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiShapeData.pShapeCombo);
		m_uiShapeData.pShapeCombo->setLabel("Shape");
		m_uiShapeData.pShapeCombo->addItem("Sphere", "XPS_SPHERE");
		m_uiShapeData.pShapeCombo->addItem("Hemisphere", "XPS_HEMISPHERE");
		m_uiShapeData.pShapeCombo->addItem("Cone", "XPS_CONE");
		m_uiShapeData.pShapeCombo->addItem("Box", "XPS_BOX");
		m_uiShapeData.pShapeCombo->addItem("Circle", "XPS_CIRCLE");
		m_uiShapeData.pShapeCombo->addItem("Edge", "XPS_EDGE");
		m_uiShapeData.pShapeCombo->addItem("Torus", "XPS_TORUS");
		m_uiShapeData.pShapeCombo->addItem("Rectangle", "XPS_RECTANGLE");

		m_uiShapeData.pRadiusTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pRadiusTextbox);
		m_uiShapeData.pRadiusTextbox->setLabel("Radius");

		m_uiShapeData.pRadiusThicknessTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pRadiusThicknessTextbox);
		m_uiShapeData.pRadiusThicknessTextbox->setLabel("Radius thickness");

		m_uiShapeData.pAngleTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pAngleTextbox);
		m_uiShapeData.pAngleTextbox->setLabel("Angle");

		m_uiShapeData.pArcTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pArcTextbox);
		m_uiShapeData.pArcTextbox->setLabel("Arc");

		m_uiShapeData.pArcModeCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiShapeData.pArcModeCombo);
		m_uiShapeData.pArcModeCombo->setLabel("Arc mode");
		m_uiShapeData.pArcModeCombo->addItem("Random", "XPSAM_RANDOM");
		m_uiShapeData.pArcModeCombo->addItem("Loop", "XPSAM_LOOP");
		m_uiShapeData.pArcModeCombo->addItem("Ping-pong", "XPSAM_PING_PONG");
		m_uiShapeData.pArcModeCombo->addItem("Burst spread", "XPSAM_BURST_SPREAD");

		m_uiShapeData.pArcSpreadTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pArcSpreadTextbox);
		m_uiShapeData.pArcSpreadTextbox->setLabel("Arc spread");

		m_uiShapeData.pArcSpeedCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiShapeData.pArcSpeedCurve);
		m_uiShapeData.pArcSpeedCurve->setLabel("Arc speed");

		m_uiShapeData.pLengthTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pLengthTextbox);
		m_uiShapeData.pLengthTextbox->setLabel("Length");

		m_uiShapeData.pConeEmitFromCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiShapeData.pConeEmitFromCombo);
		m_uiShapeData.pConeEmitFromCombo->setLabel("Emit from");
		m_uiShapeData.pConeEmitFromCombo->addItem("Base", "XPSCEF_BASE");
		m_uiShapeData.pConeEmitFromCombo->addItem("Volume", "XPSCEF_VOLUME");

		m_uiShapeData.pBoxEmitFromCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiShapeData.pBoxEmitFromCombo);
		m_uiShapeData.pBoxEmitFromCombo->setLabel("Emit from");
		m_uiShapeData.pBoxEmitFromCombo->addItem("Edge", "XPSBEF_EDGE");
		m_uiShapeData.pBoxEmitFromCombo->addItem("Shell", "XPSBEF_SHELL");
		m_uiShapeData.pBoxEmitFromCombo->addItem("Volume", "XPSBEF_VOLUME");

		m_uiShapeData.pDonutRadiusTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pDonutRadiusTextbox);
		m_uiShapeData.pDonutRadiusTextbox->setLabel("Donut radius");

		m_uiShapeData.pAlignToDirectionCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiShapeData.pAlignToDirectionCheck);
		m_uiShapeData.pAlignToDirectionCheck->setLabel("Align to direction");

		m_uiShapeData.pRandomizeDirectionTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pRandomizeDirectionTextbox);
		m_uiShapeData.pRandomizeDirectionTextbox->setLabel("Randomize direction");

		m_uiShapeData.pSpherizeDirectionTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pSpherizeDirectionTextbox);
		m_uiShapeData.pSpherizeDirectionTextbox->setLabel("Spherize direction");

		m_uiShapeData.pRandomizePositionTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pRandomizePositionTextbox);
		m_uiShapeData.pRandomizePositionTextbox->setLabel("Randomize position");

		m_uiShapeData.pSizeXTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pSizeXTextbox);
		m_uiShapeData.pSizeXTextbox->setLabel("Size X");

		m_uiShapeData.pSizeYTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pSizeYTextbox);
		m_uiShapeData.pSizeYTextbox->setLabel("Size Y");

		m_uiShapeData.pSizeZTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiShapeData.pSizeZTextbox);
		m_uiShapeData.pSizeZTextbox->setLabel("Size Z");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Velocity over lifetime");
		pSpoiler->setCollapsed(true);

		m_uiVelocityLifetimeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pEnabledCheck);
		m_uiVelocityLifetimeData.pEnabledCheck->setLabel("Enabled");

		m_uiVelocityLifetimeData.pLinearXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pLinearXCurve);
		m_uiVelocityLifetimeData.pLinearXCurve->setLabel("Linear X");

		m_uiVelocityLifetimeData.pLinearYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pLinearYCurve);
		m_uiVelocityLifetimeData.pLinearYCurve->setLabel("Linear Y");

		m_uiVelocityLifetimeData.pLinearZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pLinearZCurve);
		m_uiVelocityLifetimeData.pLinearZCurve->setLabel("Linear Z");

		m_uiVelocityLifetimeData.pSimulationSpaceCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pSimulationSpaceCombo);
		m_uiVelocityLifetimeData.pSimulationSpaceCombo->setLabel("Simulation space");
		m_uiVelocityLifetimeData.pSimulationSpaceCombo->addItem("Local", "XPSS_LOCAL");
		m_uiVelocityLifetimeData.pSimulationSpaceCombo->addItem("World", "XPSS_WORLD");

		m_uiVelocityLifetimeData.pOrbitalXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pOrbitalXCurve);
		m_uiVelocityLifetimeData.pOrbitalXCurve->setLabel("Orbital X");

		m_uiVelocityLifetimeData.pOrbitalYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pOrbitalYCurve);
		m_uiVelocityLifetimeData.pOrbitalYCurve->setLabel("Orbital Y");

		m_uiVelocityLifetimeData.pOrbitalZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pOrbitalZCurve);
		m_uiVelocityLifetimeData.pOrbitalZCurve->setLabel("Orbital Z");

#if 0
		m_uiVelocityLifetimeData.pOffsetXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pOffsetXCurve);
		m_uiVelocityLifetimeData.pOffsetXCurve->setLabel("Offset X");

		m_uiVelocityLifetimeData.pOffsetYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pOffsetYCurve);
		m_uiVelocityLifetimeData.pOffsetYCurve->setLabel("Offset Y");

		m_uiVelocityLifetimeData.pOffsetZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pOffsetZCurve);
		m_uiVelocityLifetimeData.pOffsetZCurve->setLabel("Offset Z");
#endif

		m_uiVelocityLifetimeData.pRadialCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pRadialCurve);
		m_uiVelocityLifetimeData.pRadialCurve->setLabel("Radial");

		m_uiVelocityLifetimeData.pSpeedModifierCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiVelocityLifetimeData.pSpeedModifierCurve);
		m_uiVelocityLifetimeData.pSpeedModifierCurve->setLabel("Speed modifier");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Limit velocity over lifetime");
		pSpoiler->setCollapsed(true);

		m_uiLimitVelocityLifetimeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pEnabledCheck);
		m_uiLimitVelocityLifetimeData.pEnabledCheck->setLabel("Enabled");

		m_uiLimitVelocityLifetimeData.pSeparateAxesCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pSeparateAxesCheck);
		m_uiLimitVelocityLifetimeData.pSeparateAxesCheck->setLabel("Separate axes");

		m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo);
		m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo->setLabel("Simulation space");
		m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo->addItem("Local", "XPSS_LOCAL");
		m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo->addItem("World", "XPSS_WORLD");

		m_uiLimitVelocityLifetimeData.pLimitCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pLimitCurve);
		m_uiLimitVelocityLifetimeData.pLimitCurve->setLabel("Limit");

		m_uiLimitVelocityLifetimeData.pLimitXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pLimitXCurve);
		m_uiLimitVelocityLifetimeData.pLimitXCurve->setLabel("Limit X");

		m_uiLimitVelocityLifetimeData.pLimitYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pLimitYCurve);
		m_uiLimitVelocityLifetimeData.pLimitYCurve->setLabel("Limit Y");

		m_uiLimitVelocityLifetimeData.pLimitZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pLimitZCurve);
		m_uiLimitVelocityLifetimeData.pLimitZCurve->setLabel("Limit Z");

		m_uiLimitVelocityLifetimeData.pDampenCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pDampenCurve);
		m_uiLimitVelocityLifetimeData.pDampenCurve->setLabel("Dampen");

		m_uiLimitVelocityLifetimeData.pDragCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pDragCurve);
		m_uiLimitVelocityLifetimeData.pDragCurve->setLabel("Drag");

		m_uiLimitVelocityLifetimeData.pMultiplyBySizeCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pMultiplyBySizeCheck);
		m_uiLimitVelocityLifetimeData.pMultiplyBySizeCheck->setLabel("Multiply by size");

		m_uiLimitVelocityLifetimeData.pMultiplyByVelocityCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiLimitVelocityLifetimeData.pMultiplyByVelocityCheck);
		m_uiLimitVelocityLifetimeData.pMultiplyByVelocityCheck->setLabel("Multiply by velocity");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Force over lifetime");
		pSpoiler->setCollapsed(true);

		m_uiForceLifetimeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiForceLifetimeData.pEnabledCheck);
		m_uiForceLifetimeData.pEnabledCheck->setLabel("Enabled");

		m_uiForceLifetimeData.pForceXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiForceLifetimeData.pForceXCurve);
		m_uiForceLifetimeData.pForceXCurve->setLabel("Force X");

		m_uiForceLifetimeData.pForceYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiForceLifetimeData.pForceYCurve);
		m_uiForceLifetimeData.pForceYCurve->setLabel("Force Y");

		m_uiForceLifetimeData.pForceZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiForceLifetimeData.pForceZCurve);
		m_uiForceLifetimeData.pForceZCurve->setLabel("Force Z");

		m_uiForceLifetimeData.pSimulationSpaceCombo = m_pXUI->createComboBox();
		pSpoiler->insertChild(m_uiForceLifetimeData.pSimulationSpaceCombo);
		m_uiForceLifetimeData.pSimulationSpaceCombo->setLabel("Simulation space");
		m_uiForceLifetimeData.pSimulationSpaceCombo->addItem("Local", "XPSS_LOCAL");
		m_uiForceLifetimeData.pSimulationSpaceCombo->addItem("World", "XPSS_WORLD");

		m_uiForceLifetimeData.pRandomizeCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiForceLifetimeData.pRandomizeCheck);
		m_uiForceLifetimeData.pRandomizeCheck->setLabel("Randomize");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Size over lifetime");
		pSpoiler->setCollapsed(true);

		m_uiSizeLifetimeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiSizeLifetimeData.pEnabledCheck);
		m_uiSizeLifetimeData.pEnabledCheck->setLabel("Enabled");

		m_uiSizeLifetimeData.pSeparateAxesCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiSizeLifetimeData.pSeparateAxesCheck);
		m_uiSizeLifetimeData.pSeparateAxesCheck->setLabel("Separate axes");

		m_uiSizeLifetimeData.pSizeCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeLifetimeData.pSizeCurve);
		m_uiSizeLifetimeData.pSizeCurve->setLabel("Size");

		m_uiSizeLifetimeData.pSizeXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeLifetimeData.pSizeXCurve);
		m_uiSizeLifetimeData.pSizeXCurve->setLabel("Size X");

		m_uiSizeLifetimeData.pSizeYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeLifetimeData.pSizeYCurve);
		m_uiSizeLifetimeData.pSizeYCurve->setLabel("Size Y");

		m_uiSizeLifetimeData.pSizeZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeLifetimeData.pSizeZCurve);
		m_uiSizeLifetimeData.pSizeZCurve->setLabel("Size Z");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Size by speed");
		pSpoiler->setCollapsed(true);

		m_uiSizeSpeedData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiSizeSpeedData.pEnabledCheck);
		m_uiSizeSpeedData.pEnabledCheck->setLabel("Enabled");

		m_uiSizeSpeedData.pSeparateAxesCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSeparateAxesCheck);
		m_uiSizeSpeedData.pSeparateAxesCheck->setLabel("Separate axes");

		m_uiSizeSpeedData.pSizeCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSizeCurve);
		m_uiSizeSpeedData.pSizeCurve->setLabel("Size");

		m_uiSizeSpeedData.pSizeXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSizeXCurve);
		m_uiSizeSpeedData.pSizeXCurve->setLabel("Size X");

		m_uiSizeSpeedData.pSizeYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSizeYCurve);
		m_uiSizeSpeedData.pSizeYCurve->setLabel("Size Y");

		m_uiSizeSpeedData.pSizeZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSizeZCurve);
		m_uiSizeSpeedData.pSizeZCurve->setLabel("Size Z");

		m_uiSizeSpeedData.pSpeedRangeMinTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSpeedRangeMinTextbox);
		m_uiSizeSpeedData.pSpeedRangeMinTextbox->setLabel("Speed range min");

		m_uiSizeSpeedData.pSpeedRangeMaxTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiSizeSpeedData.pSpeedRangeMaxTextbox);
		m_uiSizeSpeedData.pSpeedRangeMaxTextbox->setLabel("Speed range max");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Rotation over lifetime");
		pSpoiler->setCollapsed(true);

		m_uiRotationLifetimeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiRotationLifetimeData.pEnabledCheck);
		m_uiRotationLifetimeData.pEnabledCheck->setLabel("Enabled");

		m_uiRotationLifetimeData.pSeparateAxesCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiRotationLifetimeData.pSeparateAxesCheck);
		m_uiRotationLifetimeData.pSeparateAxesCheck->setLabel("Separate axes");

		m_uiRotationLifetimeData.pAngularVelocityCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationLifetimeData.pAngularVelocityCurve);
		m_uiRotationLifetimeData.pAngularVelocityCurve->setLabel("Angular velocity");

		m_uiRotationLifetimeData.pAngularVelocityXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationLifetimeData.pAngularVelocityXCurve);
		m_uiRotationLifetimeData.pAngularVelocityXCurve->setLabel("Angular velocity X");

		m_uiRotationLifetimeData.pAngularVelocityYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationLifetimeData.pAngularVelocityYCurve);
		m_uiRotationLifetimeData.pAngularVelocityYCurve->setLabel("Angular velocity Y");

		m_uiRotationLifetimeData.pAngularVelocityZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationLifetimeData.pAngularVelocityZCurve);
		m_uiRotationLifetimeData.pAngularVelocityZCurve->setLabel("Angular velocity Z");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Rotation by speed");
		pSpoiler->setCollapsed(true);

		m_uiRotationSpeedData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiRotationSpeedData.pEnabledCheck);
		m_uiRotationSpeedData.pEnabledCheck->setLabel("Enabled");

		m_uiRotationSpeedData.pSeparateAxesCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiRotationSpeedData.pSeparateAxesCheck);
		m_uiRotationSpeedData.pSeparateAxesCheck->setLabel("Separate axes");

		m_uiRotationSpeedData.pAngularVelocityCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationSpeedData.pAngularVelocityCurve);
		m_uiRotationSpeedData.pAngularVelocityCurve->setLabel("Angular velocity");

		m_uiRotationSpeedData.pAngularVelocityXCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationSpeedData.pAngularVelocityXCurve);
		m_uiRotationSpeedData.pAngularVelocityXCurve->setLabel("Angular velocity X");

		m_uiRotationSpeedData.pAngularVelocityYCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationSpeedData.pAngularVelocityYCurve);
		m_uiRotationSpeedData.pAngularVelocityYCurve->setLabel("Angular velocity Y");

		m_uiRotationSpeedData.pAngularVelocityZCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiRotationSpeedData.pAngularVelocityZCurve);
		m_uiRotationSpeedData.pAngularVelocityZCurve->setLabel("Angular velocity Z");

		m_uiRotationSpeedData.pSpeedRangeMinTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiRotationSpeedData.pSpeedRangeMinTextbox);
		m_uiRotationSpeedData.pSpeedRangeMinTextbox->setLabel("Speed range min");

		m_uiRotationSpeedData.pSpeedRangeMaxTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiRotationSpeedData.pSpeedRangeMaxTextbox);
		m_uiRotationSpeedData.pSpeedRangeMaxTextbox->setLabel("Speed range max");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Lifetime by emitter speed");
		pSpoiler->setCollapsed(true);

		m_uiLifetimeEmitterSpeedData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiLifetimeEmitterSpeedData.pEnabledCheck);
		m_uiLifetimeEmitterSpeedData.pEnabledCheck->setLabel("Enabled");

		m_uiLifetimeEmitterSpeedData.pMultiplierCurve = m_pXUI->createMinMaxCurve();
		pSpoiler->insertChild(m_uiLifetimeEmitterSpeedData.pMultiplierCurve);
		m_uiLifetimeEmitterSpeedData.pMultiplierCurve->setLabel("Multiplier");

		m_uiLifetimeEmitterSpeedData.pSpeedRangeMinTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiLifetimeEmitterSpeedData.pSpeedRangeMinTextbox);
		m_uiLifetimeEmitterSpeedData.pSpeedRangeMinTextbox->setLabel("Speed range min");

		m_uiLifetimeEmitterSpeedData.pSpeedRangeMaxTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiLifetimeEmitterSpeedData.pSpeedRangeMaxTextbox);
		m_uiLifetimeEmitterSpeedData.pSpeedRangeMaxTextbox->setLabel("Speed range max");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Color by lifetime");
		pSpoiler->setCollapsed(true);

		m_uiColorLifetimeData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiColorLifetimeData.pEnabledCheck);
		m_uiColorLifetimeData.pEnabledCheck->setLabel("Enabled");

		m_uiColorLifetimeData.pColorGradient = m_pXUI->create2ColorGradient();
		pSpoiler->insertChild(m_uiColorLifetimeData.pColorGradient);
		m_uiColorLifetimeData.pColorGradient->setLabel("Color");

		mem_release(pSpoiler);
	}

	{
		IUISpoiler *pSpoiler = m_pXUI->createSpoiler();
		m_pPanel->insertChild(pSpoiler);
		pSpoiler->setLabel("Color by speed");
		pSpoiler->setCollapsed(true);

		m_uiColorSpeedData.pEnabledCheck = m_pXUI->createCheckBox();
		pSpoiler->insertChild(m_uiColorSpeedData.pEnabledCheck);
		m_uiColorSpeedData.pEnabledCheck->setLabel("Enabled");

		m_uiColorSpeedData.pColorGradient = m_pXUI->create2ColorGradient();
		pSpoiler->insertChild(m_uiColorSpeedData.pColorGradient);
		m_uiColorSpeedData.pColorGradient->setLabel("Color");

		m_uiColorSpeedData.pSpeedRangeMinTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiColorSpeedData.pSpeedRangeMinTextbox);
		m_uiColorSpeedData.pSpeedRangeMinTextbox->setLabel("Speed range min");

		m_uiColorSpeedData.pSpeedRangeMaxTextbox = m_pXUI->createTextBox();
		pSpoiler->insertChild(m_uiColorSpeedData.pSpeedRangeMaxTextbox);
		m_uiColorSpeedData.pSpeedRangeMaxTextbox->setLabel("Speed range max");

		mem_release(pSpoiler);
	}

	enableControls(false);
}

void CEffectEditorWindow::enableControls(bool yesNo)
{
	m_pEmitterRemoveButton->setEnabled(yesNo);
	m_pEmitterNameTextbox->setEnabled(yesNo);

	EnableControls(yesNo, m_uiGenericData);
	EnableControls(yesNo, m_uiRenderData);
	//EnableControls(yesNo, m_uiEmissionData);
	m_uiEmissionData.pRatePerSecondCurve->setEnabled(yesNo);
	m_uiEmissionData.pRatePerMeterCurve->setEnabled(yesNo);
	m_uiEmissionData.pBurstsGrid->setEnabled(yesNo);
	m_uiEmissionData.pBurstAddButton->setEnabled(yesNo);

	if(!yesNo || m_uiShapeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiShapeData);
	}
	else
	{
		m_uiShapeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiShapeData, 1);
	}

	if(!yesNo || m_uiVelocityLifetimeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiVelocityLifetimeData);
	}
	else
	{
		m_uiVelocityLifetimeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiVelocityLifetimeData, 1);
	}

	if(!yesNo || m_uiLimitVelocityLifetimeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiLimitVelocityLifetimeData);
	}
	else
	{
		m_uiLimitVelocityLifetimeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiLimitVelocityLifetimeData, 1);
	}

	if(!yesNo || m_uiForceLifetimeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiForceLifetimeData);
	}
	else
	{
		m_uiForceLifetimeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiForceLifetimeData, 1);
	}

	if(!yesNo || m_uiSizeLifetimeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiSizeLifetimeData);
	}
	else
	{
		m_uiSizeLifetimeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiSizeLifetimeData, 1);
	}

	if(!yesNo || m_uiSizeSpeedData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiSizeSpeedData);
	}
	else
	{
		m_uiSizeSpeedData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiSizeSpeedData, 1);
	}

	if(!yesNo || m_uiRotationLifetimeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiRotationLifetimeData);
	}
	else
	{
		m_uiRotationLifetimeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiRotationLifetimeData, 1);
	}

	if(!yesNo || m_uiRotationSpeedData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiRotationSpeedData);
	}
	else
	{
		m_uiRotationSpeedData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiRotationSpeedData, 1);
	}

	if(!yesNo || m_uiLifetimeEmitterSpeedData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiLifetimeEmitterSpeedData);
	}
	else
	{
		m_uiLifetimeEmitterSpeedData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiLifetimeEmitterSpeedData, 1);
	}

	if(!yesNo || m_uiColorLifetimeData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiColorLifetimeData);
	}
	else
	{
		m_uiColorLifetimeData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiColorLifetimeData, 1);
	}

	if(!yesNo || m_uiColorSpeedData.pEnabledCheck->isChecked())
	{
		EnableControls(yesNo, m_uiColorSpeedData);
	}
	else
	{
		m_uiColorSpeedData.pEnabledCheck->setEnabled(yesNo);
		EnableControls(false, m_uiColorSpeedData, 1);
	}
}

void CEffectEditorWindow::updateEmittersUI()
{
	UINT uEmitters = m_pEffect->getEmitterCount();
	m_pEmittersGrid->setRowCount(uEmitters);
	for(UINT i = 0; i < uEmitters; ++i)
	{
		m_pEmittersGrid->setCell(0, i, m_pEffect->getEmitterAt(i)->getName());
	}
}

void CEffectEditorWindow::updateControlsUI()
{
	bool bCheck = m_uiGenericData.pStartSizeSeparateCheck->isChecked();
	m_uiGenericData.pStartSizeCurve->setVisible(!bCheck);
	m_uiGenericData.pStartSizeXCurve->setVisible(bCheck);
	m_uiGenericData.pStartSizeYCurve->setVisible(bCheck);
	m_uiGenericData.pStartSizeZCurve->setVisible(bCheck);

	bCheck = m_uiGenericData.pStartRotationSeparateCheck->isChecked();
	m_uiGenericData.pStartRotationCurve->setVisible(!bCheck);
	m_uiGenericData.pStartRotationXCurve->setVisible(bCheck);
	m_uiGenericData.pStartRotationYCurve->setVisible(bCheck);
	m_uiGenericData.pStartRotationZCurve->setVisible(bCheck);

	bCheck = m_pEffectEmitter->getGenericData()->getRingBufferMode() == XPRBM_LOOP;
	m_uiGenericData.pRingBufferLoopRangeCurve->setVisible(bCheck);

	XPARTICLE_SHAPE shape = m_pEffectEmitter->getShapeData()->getShape();
	m_uiShapeData.pRadiusTextbox->setVisible(
		shape == XPS_SPHERE ||
		shape == XPS_HEMISPHERE ||
		shape == XPS_CONE ||
		shape == XPS_CIRCLE ||
		shape == XPS_EDGE ||
		shape == XPS_TORUS
	);
	m_uiShapeData.pRadiusThicknessTextbox->setVisible(
		shape == XPS_SPHERE ||
		shape == XPS_HEMISPHERE ||
		shape == XPS_CONE ||
		shape == XPS_CIRCLE ||
		shape == XPS_TORUS
	);
	m_uiShapeData.pAngleTextbox->setVisible(shape == XPS_CONE);
	m_uiShapeData.pLengthTextbox->setVisible(shape == XPS_CONE);
	m_uiShapeData.pConeEmitFromCombo->setVisible(shape == XPS_CONE);
	m_uiShapeData.pArcTextbox->setVisible(
		shape == XPS_CONE ||
		shape == XPS_CIRCLE ||
		shape == XPS_TORUS
	);
	m_uiShapeData.pArcModeCombo->setVisible(
		shape == XPS_CONE ||
		shape == XPS_CIRCLE ||
		shape == XPS_EDGE ||
		shape == XPS_TORUS
	);
	m_uiShapeData.pArcSpreadTextbox->setVisible(
		shape == XPS_CONE ||
		shape == XPS_CIRCLE ||
		shape == XPS_EDGE ||
		shape == XPS_TORUS
	);
	m_uiShapeData.pArcSpeedCurve->setVisible((
		shape == XPS_CONE ||
		shape == XPS_CIRCLE ||
		shape == XPS_EDGE ||
		shape == XPS_TORUS
	) && m_pEffectEmitter->getShapeData()->getArcMode() != XPSAM_RANDOM);

	m_uiShapeData.pBoxEmitFromCombo->setVisible(shape == XPS_BOX);
	m_uiShapeData.pDonutRadiusTextbox->setVisible(shape == XPS_TORUS);

	m_uiShapeData.pSizeXTextbox->setVisible(shape == XPS_BOX || shape == XPS_RECTANGLE);
	m_uiShapeData.pSizeYTextbox->setVisible(shape == XPS_BOX);
	m_uiShapeData.pSizeZTextbox->setVisible(shape == XPS_BOX || shape == XPS_RECTANGLE);


	bCheck = m_uiLimitVelocityLifetimeData.pSeparateAxesCheck->isChecked();
	m_uiLimitVelocityLifetimeData.pLimitCurve->setVisible(!bCheck);
	m_uiLimitVelocityLifetimeData.pLimitXCurve->setVisible(bCheck);
	m_uiLimitVelocityLifetimeData.pLimitYCurve->setVisible(bCheck);
	m_uiLimitVelocityLifetimeData.pLimitZCurve->setVisible(bCheck);
	m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo->setVisible(bCheck);

	bCheck = m_uiSizeLifetimeData.pSeparateAxesCheck->isChecked();
	m_uiSizeLifetimeData.pSizeCurve->setVisible(!bCheck);
	m_uiSizeLifetimeData.pSizeXCurve->setVisible(bCheck);
	m_uiSizeLifetimeData.pSizeYCurve->setVisible(bCheck);
	m_uiSizeLifetimeData.pSizeZCurve->setVisible(bCheck);

	bCheck = m_uiSizeSpeedData.pSeparateAxesCheck->isChecked();
	m_uiSizeSpeedData.pSizeCurve->setVisible(!bCheck);
	m_uiSizeSpeedData.pSizeXCurve->setVisible(bCheck);
	m_uiSizeSpeedData.pSizeYCurve->setVisible(bCheck);
	m_uiSizeSpeedData.pSizeZCurve->setVisible(bCheck);

	bCheck = m_uiRotationLifetimeData.pSeparateAxesCheck->isChecked();
	m_uiRotationLifetimeData.pAngularVelocityCurve->setVisible(!bCheck);
	m_uiRotationLifetimeData.pAngularVelocityXCurve->setVisible(bCheck);
	m_uiRotationLifetimeData.pAngularVelocityYCurve->setVisible(bCheck);
	m_uiRotationLifetimeData.pAngularVelocityZCurve->setVisible(bCheck);

	bCheck = m_uiRotationSpeedData.pSeparateAxesCheck->isChecked();
	m_uiRotationSpeedData.pAngularVelocityCurve->setVisible(!bCheck);
	m_uiRotationSpeedData.pAngularVelocityXCurve->setVisible(bCheck);
	m_uiRotationSpeedData.pAngularVelocityYCurve->setVisible(bCheck);
	m_uiRotationSpeedData.pAngularVelocityZCurve->setVisible(bCheck);
}

static void CurveToString(const IXMinMaxCurve *pCurve, char *szBuf)
{
	switch(pCurve->getMode())
	{
	case XMCM_CONSTANT:
		sprintf(szBuf, "%g", pCurve->getMax());
		break;
	case XMCM_CURVE:
		strcpy(szBuf, "curve");
		break;
	case XMCM_TWO_CONSTANTS:
		sprintf(szBuf, "%g-%g", pCurve->getMin(), pCurve->getMax());
		break;
	case XMCM_TWO_CURVES:
		strcpy(szBuf, "2 curves");
		break;
	}
}

void CEffectEditorWindow::updateControlsUIValues()
{
	m_pEmitterNameTextbox->setValue(m_pEffectEmitter->getName());
	char szBuf[256];
	{
		IXParticleEffectEmitterGenericData *pData = m_pEffectEmitter->getGenericData();

		sprintf(szBuf, "%.3g", pData->getDuration());
		m_uiGenericData.pDurationTextbox->setValue(szBuf);

		m_uiGenericData.pLoopingCheck->setChecked(pData->getLooping());

		m_uiGenericData.pPrewarmCheck->setChecked(pData->getPrewarm());

		sprintf(szBuf, "%.3g", pData->getStartDelay());
		m_uiGenericData.pStartDelayTextbox->setValue(szBuf);

		m_uiGenericData.pStartLifetimeCurve->setCurve(pData->getStartLifetimeCurve());

		m_uiGenericData.pStartSpeedCurve->setCurve(pData->getStartSpeedCurve());

		m_uiGenericData.pStartSizeSeparateCheck->setChecked(pData->getStartSizeSeparate());

		m_uiGenericData.pStartSizeCurve->setCurve(pData->getStartSizeCurve());

		m_uiGenericData.pStartSizeXCurve->setCurve(pData->getStartSizeXCurve());
		m_uiGenericData.pStartSizeYCurve->setCurve(pData->getStartSizeYCurve());
		m_uiGenericData.pStartSizeZCurve->setCurve(pData->getStartSizeZCurve());

		m_uiGenericData.pStartRotationSeparateCheck->setChecked(pData->getStartRotationSeparate());

		m_uiGenericData.pStartRotationCurve->setCurve(pData->getStartRotationCurve());

		m_uiGenericData.pStartRotationXCurve->setCurve(pData->getStartRotationXCurve());
		m_uiGenericData.pStartRotationYCurve->setCurve(pData->getStartRotationYCurve());
		m_uiGenericData.pStartRotationZCurve->setCurve(pData->getStartRotationZCurve());

		sprintf(szBuf, "%.3g", pData->getFlipRotation());
		m_uiGenericData.pFlipRotationTextbox->setValue(szBuf);

		m_uiGenericData.pCullingModeCombo->setValue(EnumReflector::Get<XPARTICLE_CULLING_MODE>().find(pData->getCullingMode()).getFullName());

		sprintf(szBuf, "%.3g", pData->getGravityModifier());
		m_uiGenericData.pGravityModifierTextbox->setValue(szBuf);

		sprintf(szBuf, "%u", pData->getMaxParticles());
		m_uiGenericData.pMaxParticlesTextbox->setValue(szBuf);

		m_uiGenericData.pRingBufferModeCombo->setValue(EnumReflector::Get<XPARTICLE_RING_BUFFER_MODE>().find(pData->getRingBufferMode()).getFullName());

		m_uiGenericData.pRingBufferLoopRangeCurve->setCurve(pData->getRingBufferLoopRangeCurve());

		m_uiGenericData.pSimulationSpaceCombo->setValue(EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(pData->getSimulationSpace()).getFullName());

		m_uiGenericData.pStartColorGradient->setGradient(pData->getStartColorGradient());
	}

	{
		IXParticleEffectEmitterEmissionData *pData = m_pEffectEmitter->getEmissionData();

		m_uiEmissionData.pRatePerSecondCurve->setCurve(pData->getRatePerSecondCurve());
		m_uiEmissionData.pRatePerMeterCurve->setCurve(pData->getRatePerMeterCurve());

		
		m_uiEmissionData.pBurstsGrid->setSelection(-1); 
		UINT uBursts = pData->getBurstsCount();
		m_uiEmissionData.pBurstsGrid->setRowCount(uBursts);
		for(UINT i = 0; i < uBursts; ++i)
		{
			IXParticleBurst *pBurst = pData->getBurstAt(i);

			sprintf(szBuf, "%.3g", pBurst->getTime());
			m_uiEmissionData.pBurstsGrid->setCell(0, i, szBuf);

			sprintf(szBuf, "%u", pBurst->getCycles());
			m_uiEmissionData.pBurstsGrid->setCell(1, i, szBuf);

			CurveToString(pBurst->getCountCurve(), szBuf);
			m_uiEmissionData.pBurstsGrid->setCell(2, i, szBuf);

			CurveToString(pBurst->getIntervalCurve(), szBuf);
			m_uiEmissionData.pBurstsGrid->setCell(3, i, szBuf);

			sprintf(szBuf, "%.3g", pBurst->getProbability());
			m_uiEmissionData.pBurstsGrid->setCell(4, i, szBuf);
		}
	}

	{
		IXParticleEffectEmitterShapeData *pData = m_pEffectEmitter->getShapeData();

		m_uiShapeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiShapeData.pShapeCombo->setValue(EnumReflector::Get<XPARTICLE_SHAPE>().find(pData->getShape()).getFullName());

		sprintf(szBuf, "%.3g", pData->getRadius());
		m_uiShapeData.pRadiusTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getRadiusThickness());
		m_uiShapeData.pRadiusThicknessTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getAngle());
		m_uiShapeData.pAngleTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getArc());
		m_uiShapeData.pArcTextbox->setValue(szBuf);

		m_uiShapeData.pArcModeCombo->setValue(EnumReflector::Get<XPARTICLE_SHAPE_ARC_MODE>().find(pData->getArcMode()).getFullName());

		sprintf(szBuf, "%.3g", pData->getArcSpread());
		m_uiShapeData.pArcSpreadTextbox->setValue(szBuf);

		m_uiShapeData.pArcSpeedCurve->setCurve(pData->getArcSpeedCurve());

		sprintf(szBuf, "%.3g", pData->getLength());
		m_uiShapeData.pLengthTextbox->setValue(szBuf);

		m_uiShapeData.pConeEmitFromCombo->setValue(EnumReflector::Get<XPARTICLE_SHAPE_CONE_EMIT_FROM>().find(pData->getConeEmitFrom()).getFullName());

		m_uiShapeData.pBoxEmitFromCombo->setValue(EnumReflector::Get<XPARTICLE_SHAPE_BOX_EMIT_FROM>().find(pData->getBoxEmitFrom()).getFullName());

		sprintf(szBuf, "%.3g", pData->getDonutRadius());
		m_uiShapeData.pDonutRadiusTextbox->setValue(szBuf);

		m_uiShapeData.pAlignToDirectionCheck->setChecked(pData->getAlignToDirection());

		sprintf(szBuf, "%.3g", pData->getRandomizeDirection());
		m_uiShapeData.pRandomizeDirectionTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSpherizeDirection());
		m_uiShapeData.pSpherizeDirectionTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getRandomizePosition());
		m_uiShapeData.pRandomizePositionTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSize().x);
		m_uiShapeData.pSizeXTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSize().y);
		m_uiShapeData.pSizeYTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSize().z);
		m_uiShapeData.pSizeZTextbox->setValue(szBuf);
	}

	{
		IXParticleEffectEmitterVelocityLifetimeData *pData = m_pEffectEmitter->getVelocityLifetimeData();

		m_uiVelocityLifetimeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiVelocityLifetimeData.pLinearXCurve->setCurve(pData->getLinearXCurve());
		m_uiVelocityLifetimeData.pLinearYCurve->setCurve(pData->getLinearYCurve());
		m_uiVelocityLifetimeData.pLinearZCurve->setCurve(pData->getLinearZCurve());

		m_uiVelocityLifetimeData.pSimulationSpaceCombo->setValue(EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(pData->getSimulationSpace()).getFullName());

		m_uiVelocityLifetimeData.pOrbitalXCurve->setCurve(pData->getOrbitalXCurve());
		m_uiVelocityLifetimeData.pOrbitalYCurve->setCurve(pData->getOrbitalYCurve());
		m_uiVelocityLifetimeData.pOrbitalZCurve->setCurve(pData->getOrbitalZCurve());

#if 0
		m_uiVelocityLifetimeData.pOffsetXCurve->setCurve(pData->getOffsetXCurve());
		m_uiVelocityLifetimeData.pOffsetYCurve->setCurve(pData->getOffsetYCurve());
		m_uiVelocityLifetimeData.pOffsetZCurve->setCurve(pData->getOffsetZCurve());
#endif

		m_uiVelocityLifetimeData.pRadialCurve->setCurve(pData->getRadialCurve());

		m_uiVelocityLifetimeData.pSpeedModifierCurve->setCurve(pData->getSpeedModifierCurve());
	}

	{
		IXParticleEffectEmitterLimitVelocityLifetimeData *pData = m_pEffectEmitter->getLimitVelocityLifetimeData();

		m_uiLimitVelocityLifetimeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiLimitVelocityLifetimeData.pSeparateAxesCheck->setChecked(pData->getSeparateAxes());

		m_uiLimitVelocityLifetimeData.pSimulationSpaceCombo->setValue(EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(pData->getSimulationSpace()).getFullName());

		m_uiLimitVelocityLifetimeData.pLimitCurve->setCurve(pData->getLimitCurve());
		m_uiLimitVelocityLifetimeData.pLimitXCurve->setCurve(pData->getLimitXCurve());
		m_uiLimitVelocityLifetimeData.pLimitYCurve->setCurve(pData->getLimitYCurve());
		m_uiLimitVelocityLifetimeData.pLimitZCurve->setCurve(pData->getLimitZCurve());

		m_uiLimitVelocityLifetimeData.pDampenCurve->setCurve(pData->getDampenCurve());

		m_uiLimitVelocityLifetimeData.pDragCurve->setCurve(pData->getDragCurve());

		m_uiLimitVelocityLifetimeData.pMultiplyBySizeCheck->setChecked(pData->getMultiplyBySize());

		m_uiLimitVelocityLifetimeData.pMultiplyByVelocityCheck->setChecked(pData->getMultiplyByVelocity());
	}

	{
		IXParticleEffectEmitterForceLifetimeData *pData = m_pEffectEmitter->getForceLifetimeData();

		m_uiForceLifetimeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiForceLifetimeData.pSimulationSpaceCombo->setValue(EnumReflector::Get<XPARTICLE_SIMULATION_SPACE>().find(pData->getSimulationSpace()).getFullName());

		m_uiForceLifetimeData.pForceXCurve->setCurve(pData->getForceXCurve());
		m_uiForceLifetimeData.pForceYCurve->setCurve(pData->getForceYCurve());
		m_uiForceLifetimeData.pForceZCurve->setCurve(pData->getForceZCurve());

		m_uiForceLifetimeData.pRandomizeCheck->setChecked(pData->getRandomize());
	}

	{
		IXParticleEffectEmitterSizeLifetimeData *pData = m_pEffectEmitter->getSizeLifetimeData();

		m_uiSizeLifetimeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiSizeLifetimeData.pSeparateAxesCheck->setChecked(pData->getSeparateAxes());

		m_uiSizeLifetimeData.pSizeCurve->setCurve(pData->getSizeCurve());
		m_uiSizeLifetimeData.pSizeXCurve->setCurve(pData->getSizeXCurve());
		m_uiSizeLifetimeData.pSizeYCurve->setCurve(pData->getSizeYCurve());
		m_uiSizeLifetimeData.pSizeZCurve->setCurve(pData->getSizeZCurve());
	}

	{
		IXParticleEffectEmitterSizeSpeedData *pData = m_pEffectEmitter->getSizeSpeedData();

		m_uiSizeSpeedData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiSizeSpeedData.pSeparateAxesCheck->setChecked(pData->getSeparateAxes());

		m_uiSizeSpeedData.pSizeCurve->setCurve(pData->getSizeCurve());
		m_uiSizeSpeedData.pSizeXCurve->setCurve(pData->getSizeXCurve());
		m_uiSizeSpeedData.pSizeYCurve->setCurve(pData->getSizeYCurve());
		m_uiSizeSpeedData.pSizeZCurve->setCurve(pData->getSizeZCurve());

		sprintf(szBuf, "%.3g", pData->getSpeedRange().x);
		m_uiSizeSpeedData.pSpeedRangeMinTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSpeedRange().y);
		m_uiSizeSpeedData.pSpeedRangeMaxTextbox->setValue(szBuf);
	}

	{
		IXParticleEffectEmitterRotationLifetimeData *pData = m_pEffectEmitter->getRotationLifetimeData();

		m_uiRotationLifetimeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiRotationLifetimeData.pSeparateAxesCheck->setChecked(pData->getSeparateAxes());

		m_uiRotationLifetimeData.pAngularVelocityCurve->setCurve(pData->getAngularVelocityCurve());
		m_uiRotationLifetimeData.pAngularVelocityXCurve->setCurve(pData->getAngularVelocityXCurve());
		m_uiRotationLifetimeData.pAngularVelocityYCurve->setCurve(pData->getAngularVelocityYCurve());
		m_uiRotationLifetimeData.pAngularVelocityZCurve->setCurve(pData->getAngularVelocityZCurve());
	}

	{
		IXParticleEffectEmitterRotationSpeedData *pData = m_pEffectEmitter->getRotationSpeedData();

		m_uiRotationSpeedData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiRotationSpeedData.pSeparateAxesCheck->setChecked(pData->getSeparateAxes());

		m_uiRotationSpeedData.pAngularVelocityCurve->setCurve(pData->getAngularVelocityCurve());
		m_uiRotationSpeedData.pAngularVelocityXCurve->setCurve(pData->getAngularVelocityXCurve());
		m_uiRotationSpeedData.pAngularVelocityYCurve->setCurve(pData->getAngularVelocityYCurve());
		m_uiRotationSpeedData.pAngularVelocityZCurve->setCurve(pData->getAngularVelocityZCurve());

		sprintf(szBuf, "%.3g", pData->getSpeedRange().x);
		m_uiRotationSpeedData.pSpeedRangeMinTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSpeedRange().y);
		m_uiRotationSpeedData.pSpeedRangeMaxTextbox->setValue(szBuf);
	}

	{
		IXParticleEffectEmitterLifetimeEmitterSpeedData *pData = m_pEffectEmitter->getLifetimeEmitterSpeedData();

		m_uiLifetimeEmitterSpeedData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiLifetimeEmitterSpeedData.pMultiplierCurve->setCurve(pData->getMultiplierCurve());

		sprintf(szBuf, "%.3g", pData->getSpeedRange().x);
		m_uiLifetimeEmitterSpeedData.pSpeedRangeMinTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSpeedRange().y);
		m_uiLifetimeEmitterSpeedData.pSpeedRangeMaxTextbox->setValue(szBuf);
	}

	{
		IXParticleEffectEmitterColorLifetimeData *pData = m_pEffectEmitter->getColorLifetimeData();

		m_uiColorLifetimeData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiColorLifetimeData.pColorGradient->setGradient(pData->getColor());
	}

	{
		IXParticleEffectEmitterColorSpeedData *pData = m_pEffectEmitter->getColorSpeedData();

		m_uiColorSpeedData.pEnabledCheck->setChecked(pData->isEnabled());

		m_uiColorSpeedData.pColorGradient->setGradient(pData->getColor());

		sprintf(szBuf, "%.3g", pData->getSpeedRange().x);
		m_uiColorSpeedData.pSpeedRangeMinTextbox->setValue(szBuf);

		sprintf(szBuf, "%.3g", pData->getSpeedRange().y);
		m_uiColorSpeedData.pSpeedRangeMaxTextbox->setValue(szBuf);
	}

	{
		IXParticleEffectEmitterRenderData *pData = m_pEffectEmitter->getRenderData();

		m_uiRenderData.pMaterial->setValue(pData->getMaterial());
	}

	updateControlsUI();
}

void CEffectEditorWindow::updateBurstRow()
{
	if(m_pEmissionBurst)
	{
		int i = m_uiEmissionData.pBurstsGrid->getSelection();
		char szBuf[256];

		sprintf(szBuf, "%g", m_pEmissionBurst->getTime());
		m_uiEmissionData.pBurstsGrid->setCell(0, i, szBuf);

		sprintf(szBuf, "%u", m_pEmissionBurst->getCycles());
		m_uiEmissionData.pBurstsGrid->setCell(1, i, szBuf);

		CurveToString(m_pEmissionBurst->getCountCurve(), szBuf);
		m_uiEmissionData.pBurstsGrid->setCell(2, i, szBuf);

		CurveToString(m_pEmissionBurst->getIntervalCurve(), szBuf);
		m_uiEmissionData.pBurstsGrid->setCell(3, i, szBuf);

		sprintf(szBuf, "%g", m_pEmissionBurst->getProbability());
		m_uiEmissionData.pBurstsGrid->setCell(4, i, szBuf);
	}
}

void CEffectEditorWindow::onPlayerEvent(XPARTICLE_EVENT ev)
{
	if(ev == XPE_FINISH)
	{
		updatePlayPauseLabel();
	}
}

void CEffectEditorWindow::updatePlayPauseLabel()
{
	m_pPlayPauseButton->setLabel(m_pPlayer->isPlaying() ? "Pause" : "Play");
}

bool CEffectEditorWindow::save()
{
	if(m_pEffect->save())
	{
		m_isDirty = false;
		m_pWindow->messageBox("Effect saved!", NULL, XMBF_ICONINFORMATION, [](void *pCtx, XMESSAGE_BOX_RESULT result){});
		return(true);
	}

	m_pWindow->messageBox("Unable to save effect!", NULL, XMBF_ICONERROR, [](void *pCtx, XMESSAGE_BOX_RESULT result){});
	
	return(false);
}

void CEffectEditorWindow::updateCamera()
{
	SMQuaternion q = (SMQuaternion(m_fCamYaw, 'y') * SMQuaternion(m_fCamPitch, 'x')).Conjugate();
	m_pCamera->setPosition(q * float3(0.0f, 0.0f, -m_fCamRadius));
	m_pCamera->setOrientation(q);
}

//#############################################################################

CEffectEditorPlayerCallback::CEffectEditorPlayerCallback(CEffectEditorWindow *pWnd):
	m_pWindow(pWnd)
{}

void XMETHODCALLTYPE CEffectEditorPlayerCallback::onEvent(XPARTICLE_EVENT ev)
{
	m_pWindow->onPlayerEvent(ev);
}
