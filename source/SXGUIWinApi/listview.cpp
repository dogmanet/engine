
/***********************************************************
Copyright � Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "listview.h"


CGUIListView::CGUIListView()
{

}

CGUIListView::CGUIListView(WORD x, WORD y, WORD width, WORD heigth, DWORD exstyle, DWORD style, HWND parent, WNDPROC handler, ID idObj)
{
	m_hWindow = CreateWindowEx(
		exstyle,
		WC_LISTVIEW,
		0,
		style,
		x, y, width, heigth,
		parent,
		(HMENU)idObj,
		GetModuleHandle(0),
		0);

	//ListView_SetExtendedListViewStyleEx(m_hWindow, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_FLATSB | LVS_EX_ONECLICKACTIVATE);
	this->init(this->getHWND(), parent, handler, idObj);
	SetWindowLong(getHWND(), GWL_USERDATA, (LONG)dynamic_cast<ISXGUIComponent*>(this));
	this->initComponent();
}

CGUIListView::CGUIListView(WORD x, WORD y, WORD width, WORD heigth, HWND parent, WNDPROC handler, ID idObj)
{
	m_hWindow = CreateWindowEx(
		0,
		WC_LISTVIEW,
		0,
		(parent != 0 ? WS_CHILD : 0) | WS_VISIBLE | /*WS_VSCROLL | WS_HSCROLL | LVS_ALIGNLEFT |*/ LVS_SHOWSELALWAYS | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
		x, y, width, heigth,
		parent,
		(HMENU)idObj,
		GetModuleHandle(0),
		0);

	ListView_SetExtendedListViewStyleEx(m_hWindow, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_FLATSB | LVS_EX_ONECLICKACTIVATE);
	this->init(this->getHWND(), parent, handler, idObj);
	SetWindowLong(getHWND(), GWL_USERDATA, (LONG)dynamic_cast<ISXGUIComponent*>(this));
	this->initComponent();
}

int CGUIListView::insertColumn(const char* text, int col, int width)
{
	RECT rcl;
	::GetClientRect(m_hWindow, &rcl);

	/*char *szIStr = new char[strlen(text)];
	sprintf(szIStr, "%s", text);*/

	LVCOLUMN lvc;
	memset(&lvc, 0, sizeof(LVCOLUMN));
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.cx = width;
	lvc.pszText = (char*)text;

	int iRes =  ListView_InsertColumn(m_hWindow, col, &lvc);

	//delete[] szIStr;

	if (col < m_aStrings.size() - 1)
	{
		m_aStrings[m_aStrings.size()] = m_aStrings[col];
		Array<String> aNewStrings;
		m_aStrings[col] = aNewStrings;
	}
	else
		m_aStrings[m_aStrings.size()];

	return iRes;
}

int CGUIListView::addColumn(const char* text, int width)
{
	m_aStrings[m_aStrings.size()];
	return insertColumn(text, getColumnsCount(), width);
}

void CGUIListView::deleteColumn(int col)
{
	m_aStrings.erase(col);
	ListView_DeleteColumn(m_hWindow, col);
}

int CGUIListView::getColumnsCount()
{
	HWND hwndHeader = ListView_GetHeader(m_hWindow);
	return ::SendMessage(hwndHeader, HDM_GETITEMCOUNT, 0, 0);
}

int CGUIListView::addString(LPARAM data)
{
	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVCOLUMN));
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = getStringCount();
	lvi.iSubItem = 0;
	lvi.lParam = data;
	m_aStringData.push_back(data);
	return ListView_InsertItem(m_hWindow, &lvi);
}

void CGUIListView::deleteString(int str)
{
	ListView_DeleteItem(m_hWindow, str);
}

int CGUIListView::getStringCount()
{
	return ListView_GetItemCount(m_hWindow);
}

void CGUIListView::setItemText(const char* text, int col, int str)
{
	m_aStrings[col][str] = text;
	ListView_SetItemText(m_hWindow, str, col, (char*)m_aStrings[col][str].c_str());
}

void CGUIListView::getItemText(char* text, int col, int str, int sizebuff)
{
	ListView_GetItemText(m_hWindow, str, col, text, sizebuff);
}

void CGUIListView::setItemData(int str, LPARAM data)
{
	LVITEM lvi;
	lvi.iItem = str;
	lvi.iSubItem = 0;

	ListView_GetItem(m_hWindow, &lvi);

	lvi.mask = lvi.mask | LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.lParam = data;
	BOOL res = ListView_SetItem(m_hWindow, &lvi);
	m_aStringData[str] = data;
	int qwerty = 0;
}

LPARAM CGUIListView::getItemData(int str)
{
	LVITEM lvi;
	lvi.iItem = str;
	lvi.iSubItem = 0;

	BOOL res = ListView_GetItem(m_hWindow, &lvi);
	return m_aStringData[str];// lvi.lParam;
}

int CGUIListView::getSelString(int start)
{
	return ::SendMessage(m_hWindow, LVM_GETNEXTITEM, start, LVNI_SELECTED);
}

void CGUIListView::setSelString(int str)
{
	if (str < 0)
	{
		for (int i = 0; i < this->getStringCount(); ++i)
			ListView_SetItemState(m_hWindow, i, 0x0, LVIS_SELECTED | LVIS_FOCUSED);
	}
	else
		ListView_SetItemState(m_hWindow, str, 0xF, LVIS_SELECTED | LVIS_FOCUSED);
	this->setFocus();
}

int CGUIListView::getSelColumn()
{
	return ListView_GetSelectedColumn(m_hWindow);
}

void CGUIListView::setSelColumn(int col)
{
	ListView_SetSelectedColumn(m_hWindow, col);
}

void CGUIListView::clear()
{
	clearStrings();
	ListView_DeleteAllItems(m_hWindow);

	while (getColumnsCount() > 0)
	{
		deleteColumn(0);
	}
}

void CGUIListView::clearStrings()
{
	int countstr = getStringCount();
	for (int i = 0; i < countstr; ++i)
		deleteString(0);

	for (int i = 0; i < m_aStrings.size(); ++i)
		m_aStrings[i].clear();

	m_aStringData.clear();
}