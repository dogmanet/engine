#ifndef _COMMAND_ROTATE_H_
#define _COMMAND_ROTATE_H_

#include <xcommon/editor/IXEditorExtension.h>
#include "terrax.h"
#include "CommandDuplicate.h"

class CCommandRotate final: public IXUnknownImplementation<IXEditorCommand>
{
public:
	CCommandRotate(bool bClone = false);
	~CCommandRotate();

	bool XMETHODCALLTYPE exec() override;
	bool XMETHODCALLTYPE undo() override;

	const char* XMETHODCALLTYPE getText() override
	{
		return("rotate");
	}

	bool XMETHODCALLTYPE isEmpty() override
	{
		return(false);
	}

	void addObject(IXEditorObject *pObj);
	void setStartOrigin(const float3 &vOrigin, const float3 &vAxis);
	void setStartPos(const float3 &vPos);

	void setCurrentPos(const float3 &vPos, bool useSnap=false);

protected:
	struct _rot_obj
	{
		IXEditorObject *pObj;
		float3_t vStartPos;
		float3_t vEndPos;
		SMQuaternion vStartOrient;
		SMQuaternion vEndOrient;
	};
	Array<_rot_obj> m_aObjects;
	float3_t m_vOrigin;
	float3_t m_vStartPos;
	float3_t m_vAxis;

	CCommandDuplicate *m_pDuplicateCommand = NULL;
};

#endif
