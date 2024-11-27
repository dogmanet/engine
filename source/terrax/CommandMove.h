#ifndef _COMMAND_MOVE_H_
#define _COMMAND_MOVE_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"
#include "CommandDuplicate.h"

class CCommandMove final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandMove(bool bClone = false);
	~CCommandMove();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override;

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(false);
	}

	void addObject(IXEditorObject *pObj);
	void setStartPos(const float3 &vPos);

	void setCurrentPos(const float3 &vPos);

	void setReferenceBound(const float3_t &vBoundMin, const float3_t &vBoundMax);
	void getReferenceBound(float3_t *pvBoundMin, float3_t *pvBoundMax);

protected:
	struct _move_obj
	{
		IXEditorObject *pObj;
		float3_t vStartPos;
		float3_t vEndPos;
	};
	Array<_move_obj> m_aObjects;
	float3_t m_vStartPos;

	CCommandDuplicate *m_pDuplicateCommand = NULL;


	float3_t m_vRefBoundMin;
	float3_t m_vRefBoundMax;
};

#endif
