#ifndef __IXPARTICLESYSTEM_H
#define __IXPARTICLESYSTEM_H

#include <gdefines.h>
#include <xcommon/util/IXMinMaxCurve.h>
#include <xcommon/util/IX2ColorGradients.h>
#include <mtrl/IXMaterial.h>

//##########################################################################

// {811217A3-FEEB-468D-AF58-8640CF758683}
#define IXPARTICLESYSTEM_GUID DEFINE_XGUID(0x811217a3, 0xfeeb, 0x468d, 0xaf, 0x58, 0x86, 0x40, 0xcf, 0x75, 0x86, 0x83)
#define IXPARTICLESYSTEM_VERSION 1

//##########################################################################

// IXParticleEffect
// IXParticleEffectEmitter

// IXParticlePlayer
// IXParticleEmitter

//! Управление воспроизведением эффектов, находящихся за пределами видимой области
XENUM(XPARTICLE_CULLING_MODE,
	XPCM_AUTO,        //!< Зацикленные эффекты будут приостановлены, остальные продолжат воспроизведение
	XPCM_SMART_PAUSE, //!< Приостановка эффекта. При появлении будет произведена дополнительная симуляция для создания иллюзии, что эффект продолжал воспроизводиться все время
	XPCM_PAUSE,       //!< Приостановка эффекта
	XPCM_ALWAYS_RUN   //!< Продолжение воспроизведения
);

//! Управление удалением частиц
XENUM(XPARTICLE_RING_BUFFER_MODE,
	XPRBM_DISABLED, //!< Частица удаляется по истечении времени жизни
	XPRBM_PAUSE,    //!< После истечения времени жизни, старение частицы останавливается. Частица существует до тех пор, пока создание новых частиц возможно без превыщения лимита.
	XPRBM_LOOP      //!< После истечения времени жизни, части жизни частицы зацикливается. Частица существует до тех пор, пока создание новых частиц возможно без превыщения лимита.
);

//! Пространство симуляции
XENUM(XPARTICLE_SIMULATION_SPACE,
	XPSS_LOCAL, //!< Симуляция в локальном пространстве
	XPSS_WORLD  //!< Симуляция в мировом пространстве
);

//
XENUM(XPARTICLE_SHAPE,
	XPS_SPHERE,
	XPS_HEMISPHERE,
	XPS_CONE,
	XPS_BOX,
	XPS_CIRCLE,
	XPS_EDGE,
	XPS_TORUS,
	XPS_RECTANGLE
);

//! Откуда испускать частицы
XENUM(XPARTICLE_SHAPE_CONE_EMIT_FROM,
	XPSCEF_BASE,
	XPSCEF_VOLUME
);

//! Откуда испускать частицы
XENUM(XPARTICLE_SHAPE_BOX_EMIT_FROM,
	XPSBEF_EDGE,
	XPSBEF_SHELL,
	XPSBEF_VOLUME
);


//! Режим распределения частиц вдоль дуги формы.
XENUM(XPARTICLE_SHAPE_ARC_MODE,
	XPSAM_RANDOM,      //!< частицы генерируются случайно вдоль дуги
	XPSAM_LOOP,        //!< частицы генерируются последовательно вдоль дуги. При достижении конца дуги, генерация начинается сначала;
	XPSAM_PING_PONG,   //!< частицы генерируются последовательно вдоль дуги. При достижении конца дуги, генерация продолжается в обратном направлении;
	//XPSAM_PING_LOOP,
	XPSAM_BURST_SPREAD //!< частицы генерируются равномерно вдоль дуги.
);

class IXParticleEffectEmitterGenericData
{
public:
	//! Общее время работы эмиттера (в секундах)
	XMETHOD_GETSET(Duration, float, fValue);

	//! Если включено - эмиттер будет перезапускаться после истечения времени работы
	XMETHOD_GETSET(Looping, bool, yesNo);

	//! Если включено - эмиттер инициализируется так, как будто он уже совершил полный цикл (применяется только для зацикливаемых эмиттеров)
	/*-*/XMETHOD_GETSET(Prewarm, bool, yesNo);

	//! Задержка перед началом работы эмиттера после запуска эффекта (в секундах)
	XMETHOD_GETSET(StartDelay, float, fValue);

	//! Начальное время жизни частицы (в секундах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartLifetimeCurve);

	//! Начальная скорость частицы (м/с)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartSpeedCurve);

	//! Начальный размер частицы (в метрах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartSizeCurve);

	//! Процент частиц, повернутых в обратном направлении (0-1)
	XMETHOD_GETSET(FlipRotation, float, fValue);

	//! Управление воспроизведением эффектов, находящихся за пределами видимой области
	/*-*/XMETHOD_GETSET(CullingMode, XPARTICLE_CULLING_MODE, mode);

	//! Множитель, применяемый к силе гравитации, действующей на частицы
	XMETHOD_GETSET(GravityModifier, float, fValue); // make it curve

	//! Максимальное количество частиц, которые могут существовать одновременно
	XMETHOD_GETSET(MaxParticles, UINT, uCount);

	//! Управление удалением частиц
	/*-*/XMETHOD_GETSET(RingBufferMode, XPARTICLE_RING_BUFFER_MODE, mode);

	//! Зацикливаемая доля жизни частицы. Применяется если RingBufferMode установлен в XPRBM_LOOP
	/*-*/XMETHOD_2CONST(IXMinMaxCurve*, getRingBufferLoopRangeCurve);

	//! Выбор пространства, в котором будет осуществлятся симуляция. Это может быть глобальное или локальное пространство
	XMETHOD_GETSET(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace);

	//! Начальный цвет частицы
	XMETHOD_2CONST(IX2ColorGradients*, getStartColorGradient);

	//! Начальный угол поворота частицы (в градусах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartRotationCurve);

	//! Если включено, то начальный угол поворота можно задавать отдельно по трем осям
	XMETHOD_GETSET(StartRotationSeparate, bool, yesNo);

	//! Начальный угол поворота частицы вокруг оси x (в градусах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartRotationXCurve);

	//! Начальный угол поворота частицы вокруг оси y (в градусах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartRotationYCurve);

	//! Начальный угол поворота частицы вокруг оси z (в градусах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartRotationZCurve);

	//! Если включено, то начальный размер частицы можно задавать отдельно по трем осям
	XMETHOD_GETSET(StartSizeSeparate, bool, yesNo);

	//! Начальный размер частицы по оси x (в метрах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartSizeXCurve);

	//! Начальный размер частицы по оси y (в метрах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartSizeYCurve);

	//! Начальный размер частицы по оси z (в метрах)
	XMETHOD_2CONST(IXMinMaxCurve*, getStartSizeZCurve);
};

class IXParticleBurst
{
public:
	//! Момент в секундах, с начала работы эффекта, в который произойдет эмиссия
	XMETHOD_GETSET(Time, float, fValue);

	//! Количество повторений. 0 - неограниченно
	XMETHOD_GETSET(Cycles, UINT, uValue);

	//! Вероятность порождения частиц (0-1)
	XMETHOD_GETSET(Probability, float, fValue);

	//! Количество порождаемых частиц
	XMETHOD_2CONST(IXMinMaxCurve*, getCountCurve);

	//! Время в секундах между повторениями
	XMETHOD_2CONST(IXMinMaxCurve*, getIntervalCurve);
};

class IXParticleEffectEmitterEmissionData
{
public:
	//! Количество частиц, порождаемых в секунду
	XMETHOD_2CONST(IXMinMaxCurve*, getRatePerSecondCurve);

	//! Количество частиц, порождаемых на метр перемещения эмиттера
	XMETHOD_2CONST(IXMinMaxCurve*, getRatePerMeterCurve);

	//! Burst - событие, которое порождает множество частиц. С помощью следующих настроек можно создавать частицы в нужные моменты времени:
	//!{
	XMETHOD_GETSET(BurstsCount, UINT, uCount);

	XMETHOD_2CONST(IXParticleBurst*, getBurstAt, UINT uIndex);

	virtual void XMETHODCALLTYPE removeBurstAt(UINT uIndex) = 0;
	//!}
};

class IXParticleEffectEmitterShapeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Форма испускающего объема
	XMETHOD_GETSET(Shape, XPARTICLE_SHAPE, shape);

	//! Радиус
	XMETHOD_GETSET(Radius, float, fValue);

	//! Пропорция объема, испускающего частицы. 0 - частицы испускаются только с поверхности, 1 - по всему объему. Промежуточные значения задают соотношение испускающего объема к всему объему формы.
	XMETHOD_GETSET(RadiusThickness, float, fValue);

	//! Угол наклона стороны конуса. При 0 - конус вырождается в цилиндр, при 90 - в плоский диск.
	XMETHOD_GETSET(Angle, float, fValue);

	//! Угловая пропорция дуги от полного круга, используемая для излучения.
	XMETHOD_GETSET(Arc, float, fValue);

	//! Режим распределения частиц вдоль дуги формы.
	XMETHOD_GETSET(ArcMode, XPARTICLE_SHAPE_ARC_MODE, mode);

	//! Интервалы вдоль дуги, на которых могут генерироваться частицы. 0 - генерация по всей дуге. 0.1 - на интервалах, длиной в 10% дуги.
	XMETHOD_GETSET(ArcSpread, float, fValue);

	//! Скорость, с которой точка испускания частиц движется по дуге. Этот параметр доступен только для режимов Mode отличных от Random
	XMETHOD_2CONST(IXMinMaxCurve*, getArcSpeedCurve);

	//! Длина конуса. Применяется только при режиме излучения Emit from установленным в Volume
	XMETHOD_GETSET(Length, float, fValue);

	//! Откуда испускать частицы
	XMETHOD_GETSET(ConeEmitFrom, XPARTICLE_SHAPE_CONE_EMIT_FROM, emitFrom);

	//! Откуда испускать частицы
	XMETHOD_GETSET(BoxEmitFrom, XPARTICLE_SHAPE_BOX_EMIT_FROM, emitFrom);

	//! Радиус внутреннего сечения тора.
	XMETHOD_GETSET(DonutRadius, float, fValue);

	//! Повернуть частицы по их начальному направлению. Например, это может использоваться для симуляции искр, вылетающих при столкновении объектов. При необходимости поворот можно переопределить с помощью параметра Start rotation из основных настроек.
	XMETHOD_GETSET(AlignToDirection, bool, yesNo);

	//! Коэффициент случайности направления. 0 - направление не меняется, 1 - направление полностью случайно.
	XMETHOD_GETSET(RandomizeDirection, float, fValue);

	//! Коэффициент сферичности направления. 0 - направление не меняется, 1 - частица движется от центра эмиттера.
	XMETHOD_GETSET(SpherizeDirection, float, fValue);

	//! Сместить частицу на случайное расстояние, не превыщающее заданное этим параметром. 0 - не применяет смещение.
	XMETHOD_GETSET(RandomizePosition, float, fValue);

	//! Размеры параллелепипеда
	XMETHOD_GETSET_REF(Size, float3_t, vSize);
};

class IXParticleEffectEmitterVelocityLifetimeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Линейная скорость по осям X, Y и Z.
	//!{
	XMETHOD_2CONST(IXMinMaxCurve*, getLinearXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getLinearYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getLinearZCurve);
	//!}

	//! В каком пространстве применяется линейная скорость: в локальном, или в глобальном.
	XMETHOD_GETSET(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace);

	//! Угловая скорость вокруг осей X, Y и Z.
	//!{
	XMETHOD_2CONST(IXMinMaxCurve*, getOrbitalXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getOrbitalYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getOrbitalZCurve);
	//!}

	//! Смещение центра вращения для вращающихся частиц.
	//!{
	/*-*/XMETHOD_2CONST(IXMinMaxCurve*, getOffsetXCurve);
	/*-*/XMETHOD_2CONST(IXMinMaxCurve*, getOffsetYCurve);
	/*-*/XMETHOD_2CONST(IXMinMaxCurve*, getOffsetZCurve);
	//!}

	//! Скорость удаления (сближения) от центра эффекта.
	XMETHOD_2CONST(IXMinMaxCurve*, getRadialCurve);

	//! Модификатор скорости без изменения направления.
	XMETHOD_2CONST(IXMinMaxCurve*, getSpeedModifierCurve);
};

class IXParticleEffectEmitterLimitVelocityLifetimeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Позволяет задать лимиты отдельно по каждой из осей.
	XMETHOD_GETSET(SeparateAxes, bool, yesNo);

	//! В каком пространстве применяются ограничения: в локальном, или в глобальном.
	XMETHOD_GETSET(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace);

	//! Ограничение скорости движения частиц.
	XMETHOD_2CONST(IXMinMaxCurve*, getLimitCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getLimitXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getLimitYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getLimitZCurve);

	//! Доля скорости частицы, которая используется для замедления при превышении заданного ограничения.
	XMETHOD_2CONST(IXMinMaxCurve*, getDampenCurve);

	//! Линейное замедление частицы.
	XMETHOD_2CONST(IXMinMaxCurve*, getDragCurve);

	//! Если включено, частицы большего размера будут сильнее замедляться параметром Drag.
	XMETHOD_GETSET(MultiplyBySize, bool, yesNo);

	//! Если включено, частицы с большей скоростью будут сильнее замедляться параметром Drag.
	XMETHOD_GETSET(MultiplyByVelocity, bool, yesNo);
};

class IXParticleEffectEmitterForceLifetimeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Сила, применяемая к частицам по каждой из осей.
	XMETHOD_2CONST(IXMinMaxCurve*, getForceXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getForceYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getForceZCurve);

	//! В каком пространстве применяются силы: в локальном, или в глобальном.
	XMETHOD_GETSET(SimulationSpace, XPARTICLE_SIMULATION_SPACE, simulationSpace);

	//! При случайном выборе значений между двумя кривыми или константами этот флаг заставляет систему выбирать новую случайную силу при каждом обновлении.
	XMETHOD_GETSET(Randomize, bool, yesNo);
};

class IXParticleEffectEmitterSizeLifetimeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Позволяет задать размеры отдельно по каждой из осей.
	XMETHOD_GETSET(SeparateAxes, bool, yesNo);

	//! Модификатор размера частицы.
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeZCurve);
};

class IXParticleEffectEmitterSizeSpeedData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Позволяет задать размеры отдельно по каждой из осей.
	XMETHOD_GETSET(SeparateAxes, bool, yesNo);

	//! Модификатор размера частицы.
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getSizeZCurve);

	//! Границы диапазона скоростей, отображаемого на кривой.
	XMETHOD_GETSET_REF(SpeedRange, float2_t, vRange);
};

class IXParticleEffectEmitterRotationLifetimeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Позволяет задать вращение отдельно по каждой из осей.
	XMETHOD_GETSET(SeparateAxes, bool, yesNo);

	//! Угловая скорость.
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityZCurve);
};

class IXParticleEffectEmitterRotationSpeedData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Позволяет задать размеры отдельно по каждой из осей.
	XMETHOD_GETSET(SeparateAxes, bool, yesNo);

	//! Угловая скорость вращения частицы.
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityXCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityYCurve);
	XMETHOD_2CONST(IXMinMaxCurve*, getAngularVelocityZCurve);

	//! Границы диапазона скоростей, отображаемого на кривой.
	XMETHOD_GETSET_REF(SpeedRange, float2_t, vRange);
};

class IXParticleEffectEmitterLifetimeEmitterSpeedData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Модификатор времени жизни частицы.
	XMETHOD_2CONST(IXMinMaxCurve*, getMultiplierCurve);

	//! Границы диапазона скоростей, отображаемого на кривой.
	XMETHOD_GETSET_REF(SpeedRange, float2_t, vRange);
};

class IXParticleEffectEmitterColorLifetimeData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Градиент цвета частицы.
	XMETHOD_2CONST(IX2ColorGradients*, getColor);
};

class IXParticleEffectEmitterColorSpeedData
{
public:
	virtual bool XMETHODCALLTYPE isEnabled() const = 0;
	virtual void XMETHODCALLTYPE enable(bool yesNo) = 0;

	//! Градиент цвета частицы.
	XMETHOD_2CONST(IX2ColorGradients*, getColor);

	//! Границы диапазона скоростей, отображаемого на градиенте.
	XMETHOD_GETSET_REF(SpeedRange, float2_t, vRange);
};

class IXParticleEffectEmitterRenderData
{
public:
	//! Материал, используемый при отрисовке частиц
	XMETHOD_GETSET(Material, const char*, szMaterial);
};

class IXParticleEffectEmitter: public IXUnknown
{
public:
	virtual const char* XMETHODCALLTYPE getName() const = 0;
	virtual void XMETHODCALLTYPE setName(const char *szName) = 0;

	XMETHOD_GETSET_REF(Pos, float3_t, vPos);
	XMETHOD_GETSET_REF(Orient, SMQuaternion, qOrient);

	XMETHOD_2CONST(IXParticleEffectEmitterGenericData*, getGenericData);
	XMETHOD_2CONST(IXParticleEffectEmitterEmissionData*, getEmissionData);
	XMETHOD_2CONST(IXParticleEffectEmitterShapeData*, getShapeData);
	XMETHOD_2CONST(IXParticleEffectEmitterVelocityLifetimeData*, getVelocityLifetimeData);
	XMETHOD_2CONST(IXParticleEffectEmitterLimitVelocityLifetimeData*, getLimitVelocityLifetimeData);
	XMETHOD_2CONST(IXParticleEffectEmitterForceLifetimeData*, getForceLifetimeData);
	XMETHOD_2CONST(IXParticleEffectEmitterSizeLifetimeData*, getSizeLifetimeData);
	XMETHOD_2CONST(IXParticleEffectEmitterSizeSpeedData*, getSizeSpeedData);
	XMETHOD_2CONST(IXParticleEffectEmitterRotationLifetimeData*, getRotationLifetimeData);
	XMETHOD_2CONST(IXParticleEffectEmitterRotationSpeedData*, getRotationSpeedData);
	XMETHOD_2CONST(IXParticleEffectEmitterLifetimeEmitterSpeedData*, getLifetimeEmitterSpeedData);
	XMETHOD_2CONST(IXParticleEffectEmitterColorLifetimeData*, getColorLifetimeData);
	XMETHOD_2CONST(IXParticleEffectEmitterColorSpeedData*, getColorSpeedData);
	XMETHOD_2CONST(IXParticleEffectEmitterRenderData*, getRenderData);
};

class IXParticleEffect: public IXUnknown
{
public:
	virtual UINT XMETHODCALLTYPE getEmitterCount() = 0;
	virtual void XMETHODCALLTYPE setEmitterCount(UINT uCount) = 0;
	virtual IXParticleEffectEmitter* XMETHODCALLTYPE getEmitterAt(UINT uIdx) = 0;
	virtual void XMETHODCALLTYPE removeEmitterAt(UINT uIdx) = 0;

	virtual bool XMETHODCALLTYPE save() = 0;
};

enum XPARTICLE_EVENT
{
	XPE_BURNOUT,
	XPE_FINISH,
};

class IXParticlePlayerCallback
{
public:
	virtual void XMETHODCALLTYPE onEvent(XPARTICLE_EVENT ev) = 0;
};

class IXParticlePlayer: public IXUnknown
{
public:
	virtual void XMETHODCALLTYPE play() = 0;
	virtual void XMETHODCALLTYPE pause() = 0;
	virtual void XMETHODCALLTYPE stop(bool bClear = false) = 0;
	virtual void XMETHODCALLTYPE clear() = 0;

	virtual void XMETHODCALLTYPE simulate(float fTime, bool bRestart = false) = 0;

	virtual bool XMETHODCALLTYPE isEmitting() = 0;
	virtual bool XMETHODCALLTYPE isPaused() = 0;
	virtual bool XMETHODCALLTYPE isPlaying() = 0;
	virtual bool XMETHODCALLTYPE isStopped() = 0;
	virtual bool XMETHODCALLTYPE isAlive() = 0;

	virtual UINT XMETHODCALLTYPE getParticleCount() = 0;
	virtual float XMETHODCALLTYPE getTime() = 0;

	virtual float3_t XMETHODCALLTYPE getPos() = 0;
	virtual void XMETHODCALLTYPE setPos(const float3_t &vPos) = 0;

	virtual SMQuaternion XMETHODCALLTYPE getOrient() = 0;
	virtual void XMETHODCALLTYPE setOrient(const SMQuaternion &qRot) = 0;

	virtual void XMETHODCALLTYPE setCallback(IXParticlePlayerCallback *pCallback) = 0;

	virtual void XMETHODCALLTYPE setLayer(UINT uLayer) = 0;
	virtual UINT XMETHODCALLTYPE getLayer() = 0;
};

class IXParticleSystem: public IXUnknown
{
public:
	virtual bool XMETHODCALLTYPE newEffect(const char *szName, IXParticleEffect **ppOut) = 0;
	virtual bool XMETHODCALLTYPE getEffect(const char *szName, IXParticleEffect **ppOut) = 0;

	virtual void XMETHODCALLTYPE newEffectPlayer(IXParticleEffect *pEffect, IXParticlePlayer **ppOut) = 0;
	// virtual void XMETHODCALLTYPE newEffectEmitter(IXParticleEffect *pEffect, IXParticleEmitter **ppOut) = 0;
};

#endif
