
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

/*!
\file
Модель
*/

#ifndef __PROP_STATIC_H
#define __PROP_STATIC_H

#include "BaseAnimating.h"

/*! Модель
\ingroup cbaseanimating
*/
class CPropStatic: public CBaseAnimating
{
	DECLARE_CLASS(CPropStatic, CBaseAnimating);
	DECLARE_PROPTABLE();
public:
	DECLARE_CONSTRUCTOR();
	~CPropStatic();
	
	void setPos(const float3 &pos) override
	{
		BaseClass::setPos(pos);

		if(m_pModel)
		{
			m_pModel->setPosition(pos);
		}
	}

	void setOrient(const SMQuaternion &q) override
	{
		BaseClass::setOrient(q);

		if(m_pModel)
		{
			m_pModel->setOrientation(q);
		}
	}

protected:
	void createPhysBody() override;
	void removePhysBody() override;

	void initPhysics() override;
	void releasePhysics() override;

	void onSetUseTrimesh(int iVal);
	int m_iTrimeshPhysics = 0;
};

#endif
