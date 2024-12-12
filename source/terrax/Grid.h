#ifndef _GRID_H_
#define _GRID_H_

#include <xcommon/render/IXRender.h>

enum GRID_STEP
{
	GRID_STEP_MINIMAL = 0,
	GRID_STEP_1CM = 0, // hilite >= 5cm
	GRID_STEP_2CM,     // hilite >= 10cm
	GRID_STEP_5CM,     // hilite >= 50cm
	GRID_STEP_10CM,    // hilite >= 50cm
	GRID_STEP_20CM,    // hilite >= 1m
	GRID_STEP_50CM,    // hilite >= 5m

	GRID_STEP_1M,      // hilite >= 5m
	GRID_STEP_2M,      // hilite >= 10m
	GRID_STEP_5M,      // hilite >= 50m
	GRID_STEP_10M,     // hilite >= 50m
	GRID_STEP_20M,     // hilite >= 100m
	GRID_STEP_50M,     // hilite >= 100m
	GRID_STEP_100M,    // hilite all

	GRID_STEP_AXES,
	GRID_STEP_MAXIMAL = GRID_STEP_AXES,
};

#define GRID_COLOR_AXES float4_t(0.0f, 1.0f, 1.0f, 1.0f)

#define GRID_COLOR_MAIN float4_t(0.7f, 0.7f, 0.7f, 1.0f)
#define GRID_COLOR_MAIN_10M float4_t(0.0f, 0.7f, 0.7f, 1.0f)
#define GRID_COLOR_REGULAR float4_t(0.4f, 0.4f, 0.4f, 1.0f)
#define GRID_COLOR_REGULAR_10M float4_t(0.0f, 0.4f, 0.4f, 1.0f)

class CGrid
{
public:
	CGrid(IXRender *pRender, UINT uSize = 1000);
	~CGrid();

	void render(const SMMATRIX &mWorld, IXCamera *pCamera, GRID_STEP step = GRID_STEP_1CM);
	void setOpacity(float fOpacity);
	void setDottedMode(bool isDotted);

	//! Get minimum avaliable step by given viewport scale. Scale=1 means 1m = 1px
	static GRID_STEP GetMinStepForScale(float fScale);

protected:
	IXRender *m_pRender = NULL;
	IGXDevice *m_pDevice = NULL;
	IGXRenderBuffer *m_pRenderBuffer;
	struct
	{
		UINT m_uStartVertex;
		UINT m_uEndVertex;
	} m_uVertexPerStep[GRID_STEP_MAXIMAL + 1];

	ID m_idVS = -1;
	ID m_idPS = -1;
	ID m_idShaderKit = -1;

	float m_fOpacity = 1.0f;

	bool m_isDotted = false;

	struct vertex
	{
		float3_t vPos;
	};

	IGXBlendState *m_pBlendState = NULL;
	IGXBlendState *m_pBlendStateNoColor = NULL;
	IGXDepthStencilState *m_pStencilPass0 = NULL;
	IGXDepthStencilState *m_pStencilPass1 = NULL;
	IGXConstantBuffer *m_pVSConstantBuffer = NULL;
	IGXConstantBuffer *m_pPSConstantBuffer = NULL;
};

#endif
