
/*
struct.h
Файл включения в шейдеры (вершинные и пиксельные)
Определены все структуры которые используются
*/

#ifndef __STRUCT_H
#define __STRUCT_H


#include <gdefines.h>

/* \name ПОСТПРОЦЕСС
@{*/

//! выходная структура вершинного шейдера, восстановление позиции по глубине
struct VSO_ResPos
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
	
	float3 vEyeRay	:TEXCOORD1;
	float3 vWorldRay	:TEXCOORD2;
};

//! стандартная структура постпроцесса, на вход врешинному шейдеру
struct VSI_PP
{
	float3 vPosition	:POSITION;
	float2 vTexUV	:TEXCOORD0;
};

//! стандартная структура постпроцесса, на выход врешинного шейдера и на вход пиксельному
struct VSO_PP
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
};

//!@}


//! union alpha, структура результата работы объединения полупрозрачных слоев
struct PSO_UnionAlpha
{
	float4 vColor	: COLOR0;
	float4 vDepth0	: COLOR1;
	float4 vDepth1	: COLOR2;
};


//! растительность
struct VSI_Green 
{
	float3 vPosition		:POSITION0;
	float3 vNormal		:NORMAL0;
	float2 vTexUV		:TEXCOORD0;
	float3 vInstPos		:TEXCOORD1;
	float3 vInstTrans	:TEXCOORD2;
	float2 vInstSinCosRot:TEXCOORD3;
};


//! статическая геометрия
struct VSI_Geometry
{
	float3 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
	float3 vNormal	:NORMAL0;
	float3 vTangent	: TANGENT0;
	float3 vBinormal	: BINORMAL0;
};


//! анимационная модель
struct VSI_Animation
{
	float3 vPosition	: POSITION0;
	float2 vTexUV	: TEXCOORD0;
	float3 vNormal	: NORMAL0;
	float3 vTangent	: TANGENT0;
	float3 vBinormal	: BINORMAL0;
	uint4 vBones	: BLENDINDICES0;
	float4 vWeights	: BLENDWEIGHT0;
};


//! частицы
struct VSI_Particle
{
	float2 vTexUV    :TEXCOORD0;
	
	float4 vColor    :COLOR0;
	float4 qRotLocal :TEXCOORD1;
	float4 qRotGlobal:TEXCOORD2;
	float3 vPosition :POSITION0;
	float3 vSize     :TEXCOORD3;
};


//! вода
struct VSO_WaterPlane
{
	float4 vPosition:	POSITION;
	
	//! текстурные координаты для bump карты
	float2 vTexUV2:		TEXCOORD0;			
	
	//! текстурные координаты для du/dv карты
	float2 vTexUV3:		TEXCOORD1;			
	
	//! позиция (для расчета координат отражения)
	float4 vPos:			TEXCOORD2;			
	float3 ViewDir:		TEXCOORD3;
	
	//! нормальные текстурные координаты
	float2 vTexUV:		TEXCOORD4;			
	float3 vNormal:		TEXCOORD5;
};


//! общая структура выдаваемая вершинным шейдером (и передаваемая в пиксельный)
struct VSO_SceneCommon 
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
	float3 vNormal	:TEXCOORD1;
	float4 vPos		:TEXCOORD2;
	float3 vTangent	:TEXCOORD3;
	float3 vBinormal	:TEXCOORD4;
	float4 vColor	:COLOR0;
	uint4 uInstanceId	:BLENDINDICES0;
};

struct GSO_SceneShadows
{
	VSO_SceneCommon VSO;
	uint uRTIndex   :SV_RenderTargetArrayIndex;
};


//! выходная структура с для кубического отражения
struct VSO_RefCube 
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
	float3 vNormal	:TEXCOORD1;
	float4 vPos		:TEXCOORD2;
	float3 vCubePPos	:TEXCOORD3;
};


//! структура вывода всех данных для отложенного освещения при mrt
struct PSO_Gbuffer 
{
	float4 vColor	:COLOR0;
	float4 vNormal	:COLOR1;
	float4 vParam	:COLOR2;
	float4 vDepth	:COLOR3;
};


//! структура вывода базовой инфомрации при просчетах освещения (данные из #PSO_Gbuffer)
struct PSO_Lbuffer
{
	float4 vAmdient	:COLOR0;
	float4 vSpecular	:COLOR1;
};

//! структура вывода всех данных для RSM
struct PSO_RSMbuffer 
{
	float4 vDepth    :COLOR0;
#ifdef ENABLE_RSM
	float4 vNormal   :COLOR1;
	float4 vFlux     :COLOR2;
#endif
};

/*! \name SkyBox
@{*/

struct VSI_SkyBox
{
	float3 vPosition	:POSITION0;
	float3 vTexUV	:TEXCOORD0;
};

struct VSO_SkyBox
{
	float4 vPosition	:POSITION0;
	float3 vTexUV	:TEXCOORD0;
	
	float3 vNormal	:TEXCOORD1;
	float4 vPos		:TEXCOORD2;
};

//!@}


/*! \name SkyClouds
@{*/

struct VSI_SkyClouds
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
};

struct VSO_SkyClouds
{
	float4 vPosition	:POSITION;
	float2 vTexUV	:TEXCOORD0;
};

//!@}


/*! \name Particles
@{*/

struct VSI_Particles 
{
	float3 vPosition	:POSITION0;
	float2 vTexUV 	:TEXCOORD0;
	
	float3 vInstPos	:TEXCOORD1;
	float4 vInstTex	:TEXCOORD2;
	float vInstSize	:TEXCOORD3;
	float vInstAlpha	:TEXCOORD4;
	
	float2 vInstRot1	:TEXCOORD5;
	float vInstLight	:TEXCOORD6;
};

struct VSO_Particles 
{
	float4 vPosition	:POSITION0;
	float2 vTexUV 	:TEXCOORD0;
	float Alpha		:TEXCOORD1;
	float4 vPos		:TEXCOORD2;
	float Light		:TEXCOORD3;
	float2 vTexUV2	:TEXCOORD4;
};

//!@}


/*! \name AI grid
@{*/

struct VSI_AIgrid 
{
	float3 vPosition	:POSITION0;
	float2 vTexUV 	:TEXCOORD0;
	
	float3 vInstPos	:TEXCOORD1;
	float4 vInstTex	:TEXCOORD2;
	float4 vInstColor:COLOR0;
};

struct VSO_AIgrid 
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
	float4 vPos		:TEXCOORD1;
	float4 vColor	:TEXCOORD2;
};

//!@}

/*! \name LightBound
@{*/

struct VSI_LightBound
{
	float3 vPosition	:POSITION0;
};

struct VSO_LightBound
{
	float4 vPosition	:POSITION;
};

//!@}

/*! \name GUI
@{*/

struct VSI_GUITextured
{
	float3 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
};

struct VSO_GUITextured
{
	float4 vPosition	:POSITION;
	float2 vTexUV	:TEXCOORD0;
};

struct VSI_GUISimple
{
	float3 vPosition	:POSITION0;
};

struct VSO_GUISimple
{
	float4 vPosition	:POSITION;
};

//!@}

/*! \name TerraX editor
@{*/

struct VSI_TXGrid
{
	float4 vPosition	:POSITION0;
};

struct VSO_TXGrid
{
	float4 vPosition	:POSITION;
};

struct VSI_TXTextured
{
	float4 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
};

struct VSI_DevLines
{
	float4 vPosition :POSITION0;
	float4 vColor :TEXCOORD0;
	float3 vDir :TEXCOORD1;
	float2 vTexUV :TEXCOORD2;
};

struct VSO_DevLines
{
	float4 vPosition :POSITION;
	float4 vColor :TEXCOORD1;
	float2 vTexUV :TEXCOORD0;
};

struct VSI_DevPoints
{
	float4 vPosition :POSITION0;
	float4 vColor :TEXCOORD0;
	float3 vTexUVMode :TEXCOORD2;
};

struct VSO_DevPoints
{
	float4 vPosition :POSITION;
	float4 vColor :TEXCOORD1;
	float3 vTexUVMode :TEXCOORD0;
};

struct VSI_DevText
{
	float4 vPosTexUV :POSITION0;
	float4 vColor :TEXCOORD0;
	float3 vRefPos :TEXCOORD2;
};

struct VSO_DevText
{
	float4 vPosition :POSITION;
	float4 vColor :TEXCOORD1;
	float2 vTexUV :TEXCOORD0;
};

struct VSO_TXTextured
{
	float4 vPosition	:POSITION;
	float2 vTexUV	:TEXCOORD0;
};

struct VSI_TXColored
{
	float4 vPosition	:POSITION0;
};

struct VSO_TXColored
{
	float4 vPosition	:POSITION;
};

struct VSO_TXColored2
{
	float4 vPosition	:POSITION;
	float4 vColor	:TEXCOORD0;
};

struct VSI_TXHandler 
{
	float3 vPosition	:POSITION0;
	
	float3 vInstPos	:TEXCOORD0;
};

struct VSI_TXBound 
{
	float3 vPosition  :POSITION0;
	
	float3 vInstPos   :TEXCOORD0;
	float3 vInstSize  :TEXCOORD1;
};

struct VSI_TXIcon 
{
	float3 vPosition	:POSITION0;
	float2 vTexUV	:TEXCOORD0;
	
	float3 vInstPos	:TEXCOORD1;
};

struct VSI_TXMatFrame
{
	float2 vPosition :POSITION0;
	float2 vScaleFactor :TEXCOORD0;
	float4 vColor :TEXCOORD1;
	float fHighlightFactor :TEXCOORD2;
	float fTitleFactor :TEXCOORD3;
};

struct VSI_TXMatInner
{
	float2 vPosition :POSITION0;
	float2 vScaleFactor :TEXCOORD0;
	float2 vTexUV :TEXCOORD1;
};

struct VSI_GUIText
{
	float2 vPosition :POSITION0;
	float2 vTexUV :TEXCOORD0;
};

struct VSI_TXScrollbar
{
	float2 vPosition :POSITION0;
	float2 vScaleFactor :TEXCOORD0;
	float4 vMarker :TEXCOORD1;
};

struct VSI_DbgColorvertex
{
	float3 vPosition	:POSITION0;
	float4 vColor	:COLOR0;
};

struct VSO_DbgColorvertex
{
	float4 vPosition	:POSITION;
	float4 vColor	:COLOR0;
};

//!@}

struct VSI_GICubes
{
	float3 vPosition	:POSITION0;
};

struct VSO_GICubes
{
	float4 vPosition	:POSITION0;
};

struct GSO_GICubes
{
	float4 vPosition  :POSITION0;
	float3 vNormal    :NORMAL0;
	float3 vCenterPos :TEXCOORD0;
};


#endif




