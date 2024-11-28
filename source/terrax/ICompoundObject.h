#ifndef __ICOMPOUNDOBJECT_H
#define __ICOMPOUNDOBJECT_H

#include <xcommon/editor/IXEditorObject.h>
#include <xcommon/editor/IXEditable.h>


// {F603FC8E-AF61-4328-BF65-5F4D51F852D0}
#define X_IS_COMPOUND_GUID DEFINE_XGUID(0xf603fc8e, 0xaf61, 0x4328, 0xbf, 0x65, 0x5f, 0x4d, 0x51, 0xf8, 0x52, 0xd0)

//#############################################################################

class ICompoundObject: public IXEditorObject
{
public:
	virtual void addChildObject(IXEditorObject *pObject) = 0;
	virtual void removeChildObject(IXEditorObject *pObject) = 0;

	virtual UINT getObjectCount() = 0;
	virtual IXEditorObject* getObject(UINT id) = 0;
};

#endif
