#ifndef __UICALCCONTAINER_H
#define __UICALCCONTAINER_H

#include <xUI/UIWindow.h>
#include <xUI/IXUI.h>
#include <xUI/UICommand.h>
#include <xUI/UIClassCommand.h>

class UICalcContainer: public IXUnknownImplementation<IXUnknown>
{
public:
	UICalcContainer(IXUI* pXUI);
	~UICalcContainer();
private:
	void inputNumber(int iNum);
	void inputOperation(char op);

	void pressButton(IUIControl *pTarget);

	IXUI* pXUI = NULL;
	IUIWindow *pCalc = NULL;

	Array<IUIButton*> mButtons;
	IUITextBox *m_pTextBox = NULL;

	bool m_bInputNumTwo = false; //Если первая введена то ставим переключатель в true
	bool m_hasSecondOp = false; //Если вторая введена то ставим переключатель в true

	int64_t m_i64FirstOp = 0;
	int64_t m_i64SecondOp = 0;
	char m_cOp = 0;

	bool m_bResetAtInput = false;
};

#endif
