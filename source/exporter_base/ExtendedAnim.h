#ifndef __EXTENDED_ANIM_H
#define __EXTENDED_ANIM_H

#include "stdafx.h"
#include "IExtendedTab.h"
#include <dseplugin/ModelFile.h>

class CExtendedAnim final: public IExtendedTab
{
public:
	~CExtendedAnim();

	void init(HWND hWndParent) override;
	const char* getTitle() override
	{
		return("Animations");
	}

	void show(int X, int Y, int cx, int cy) override;
	void hide() override;

	void commit() override;
	void rollback() override;

	void addSequence(ModelSequence &ms);

	void setActivityList(const Array<String> &list);

	UINT getSequenceCount()
	{
		return(m_aSequences.size());
	}

	const ModelSequence& getSequence(UINT idx);

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	INT_PTR CALLBACK dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK EditDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void loadDefaults();
	
	bool invokeEditor(int iItem);

private:
	HWND m_hDlg = NULL;
	HWND m_hListWnd = NULL;

	int m_iEditRow = -1;

	struct Sequence
	{
		ModelSequence ms;
		String sActivity;
		bool toDelete;
	};

	Array<String> m_asActivities;
	Array<Sequence> m_aSequences;
};

#endif
