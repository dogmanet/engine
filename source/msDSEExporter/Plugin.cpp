#include <exporter_base/stdafx.h>
#include "Plugin.h"
#include <msLib/msLib.h>


#pragma comment(lib, "../sdks/msLib/lib/msModelLib.lib")

#include <stdio.h>
#include <process.h>

#include <exporter_base/Exporter.h>
#include "Provider.h"

cMsPlugIn* CreatePlugIn()
{
    return(new CPlugin());
}

CPlugin::CPlugin()
{
}

CPlugin::~CPlugin()
{
	if(m_pMdl)
	{
		msModel_Destroy(m_pMdl);
	}
}

int CPlugin::GetType()
{
    return(cMsPlugIn::eTypeExport);
}

const char* CPlugin::GetTitle()
{
	return("SkyXEngine DSE");
}


int CPlugin::Execute(msModel *pModel)
{
	if(!pModel)
	{
		return(-1);
	}

	m_pMdl = pModel;

	//
	// choose filename
	//
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));

	char szFile[MS_MAX_PATH];
	char szFileTitle[MS_MAX_PATH];
	char szDefExt[32] = "dse";
	char szFilter[128] = "SkyXEngine Model (*.dse)\0*.dse\0\0";
	szFile[0] = '\0';
	szFileTitle[0] = '\0';

	char szModelFile[MS_MAX_PATH];
	msModel_GetFileName(pModel, szModelFile, sizeof(szModelFile));
	char *ptr = szModelFile;
	int iDotPos = -1;
	while(*ptr)
	{
		if(*ptr == '.')
		{
			iDotPos = ptr - szModelFile;
		}
		++ptr;
	}
	if(iDotPos >= 0)
	{
		szModelFile[iDotPos] = 0;
	}
	strcpy(szFile, basename(szModelFile));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetActiveWindow();
	ofn.lpstrDefExt = szDefExt;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MS_MAX_PATH;
	ofn.lpstrFileTitle = szFileTitle;
	ofn.nMaxFileTitle = MS_MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrTitle = "Export DSE model";

	if(!::GetSaveFileName(&ofn))
	{
		return(0);
	}


	CProvider prov(pModel);
	CExporter exp(szFile, false, &prov);
	exp.setEndFrame(msModel_GetTotalFrames(pModel));

	char tmp[4096];
	for(int i = 0; i < msModel_GetMaterialCount(pModel); i++)
	{
		msMaterial_GetDiffuseTexture(msModel_GetMaterialAt(pModel, i), tmp, sizeof(tmp));
		exp.addTexture(tmp);
	}

	return(exp.execute());
}

