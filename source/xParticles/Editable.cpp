#include "Editable.h"
#include <xcommon/IPluginManager.h>
#include <xcommon/IXModelWriter.h>

CEditable::CEditable(IXCore *pCore, IXParticleSystem *pParticleSystem):
	m_pCore(pCore)
{
	IXEditor *pEditor = (IXEditor*)pCore->getPluginManager()->getInterface(IXEDITOR_GUID);
	m_pEditorExtension = new CEditorExtension(this, pEditor, pCore, pParticleSystem);
}

CEditable::~CEditable()
{
	mem_delete(m_pEditorExtension);
}

void XMETHODCALLTYPE CEditable::startup(IGXDevice *pDevice)
{
	m_pDevice = pDevice;

	m_pEditorExtension->setRender((IXRender*)m_pCore->getPluginManager()->getInterface(IXRENDER_GUID));
}
void XMETHODCALLTYPE CEditable::shutdown()
{
}
