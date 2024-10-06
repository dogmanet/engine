#ifndef __UIMENU_H
#define __UIMENU_H

#include "UIControl.h"
#include "IUIMenu.h"

class CUIMenu: public CUIControl<IUIMenu>
{
public:
	CUIMenu(ULONG uID);
	~CUIMenu();

	gui::dom::IDOMnode* createNode(gui::dom::IDOMdocument *pDomDocument) override;
	
	void XMETHODCALLTYPE dispatchEvent(gui::IEvent *ev) override;


	void XMETHODCALLTYPE show(int x, int y) override;
	void show(int x, int y, CUIMenu *pParentMenu);
	void XMETHODCALLTYPE hide() override;
	void hideTree();

	UINT XMETHODCALLTYPE addSeparator() override;
	void XMETHODCALLTYPE insertSeparator(UINT uIdx) override;

	UINT XMETHODCALLTYPE addItem(const char *szText, const char *szCommand = NULL, const char *szIcon = NULL, IUIMenu *pSubmenu = NULL) override;
	void XMETHODCALLTYPE insertItem(UINT uIdx, const char *szText, const char *szCommand = NULL, const char *szIcon = NULL, IUIMenu *pSubmenu = NULL) override;

	UINT XMETHODCALLTYPE getItemCount() override;

	const char* XMETHODCALLTYPE getItemText(UINT uIdx) override;
	const char* XMETHODCALLTYPE getItemIcon(UINT uIdx) override;
	const char* XMETHODCALLTYPE getItemCommand(UINT uIdx) override;
	bool XMETHODCALLTYPE isItemEnabled(UINT uIdx) override;
	IUIMenu* XMETHODCALLTYPE getItemSubmenu(UINT uIdx) override;

	void XMETHODCALLTYPE setItemText(UINT uIdx, const char *szText) override;
	void XMETHODCALLTYPE setItemIcon(UINT uIdx, const char *szIcon) override;
	void XMETHODCALLTYPE setItemCommand(UINT uIdx, const char *szCommand) override;
	void XMETHODCALLTYPE setItemEnabled(UINT uIdx, bool isEnabled) override;
	void XMETHODCALLTYPE setItemSubmenu(UINT uIdx, IUIMenu *pSubmenu) override;

	bool XMETHODCALLTYPE isSeparator(UINT uIdx) override;

private:
	struct MenuItem
	{
		gui::dom::IDOMnode *pNode = NULL;
		gui::dom::IDOMnode *pIconNode = NULL;
		gui::dom::IDOMnode *pTextNode = NULL;
		CUIMenu *pSubmenu = NULL;

		String sText;
		String sIcon;
		String sCommand;

		bool isSeparator = false;
		bool isEnabled = true;
	};

	gui::dom::IDOMdocument *m_pDoc = NULL;
	Array<MenuItem> m_aItems;
	gui::dom::IDOMnode *m_pPrevFocused = NULL;

	UINT m_uWidth = 0;
	UINT m_uHeight = 0;

	CUIMenu *m_pCurrentSubmenu = NULL;

	CUIMenu *m_pParentMenu = NULL;

	bool m_isVisible = false;

private:
	MenuItem* getItem(UINT uIdx);

	void update(MenuItem *pItem = NULL);

	void onChildMenuHidden(CUIMenu *pChildMenu);

protected:
	void cleanupNodes() override;
};

#endif
