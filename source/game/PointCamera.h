
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Камера игровая
*/

/*! \ingroup cpointentity
@{
*/

#ifndef __POINT_CAMERA_H
#define __POINT_CAMERA_H

#include "PointEntity.h"

//! Класс камеры
class CPointCamera: public CPointEntity
{
	DECLARE_CLASS(CPointCamera, CPointEntity);
	DECLARE_PROPTABLE();
public:
	DECLARE_CONSTRUCTOR();
	~CPointCamera();

	//! Возвращает объект камеры из графической либы
	IXCamera* getCamera();

	void setPos(const float3 &pos) override;
	void setOrient(const SMQuaternion &q) override;

private:
	void inActivate(inputdata_t *pInputdata);
	void inDeactivate(inputdata_t *pInputdata);

private:
	IXCamera *m_pSXC;
};

#endif

/*! @} */
