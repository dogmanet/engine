#include "UICalcContainer.h"

UICalcContainer::UICalcContainer(IXUI* pXUI)
{
	XWINDOW_DESC wdesc;
	wdesc.iPosX = XCW_CENTER;
	wdesc.iPosY = XCW_CENTER;
	wdesc.iSizeX = 400;
	wdesc.iSizeY = 320;
	wdesc.szTitle = "Calculator";
	wdesc.flags = XWF_BUTTON_CLOSE | XWF_BUTTON_MINIMIZE | XWF_BUTTON_MAXIMIZE | XWF_TRANSPARENT;

	const char *aszButtonNames[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "+", "-", "*", "/", "="};

	pCalc = pXUI->createWindow(&wdesc);
	pCalc->setCallback([](void *pCtx, IUIControl *pControl, gui::IEvent *ev){
		UICalcContainer *pThis = (UICalcContainer*)pCtx;

		if(ev->type == gui::GUI_EVENT_TYPE_CLICK)
		{
			pThis->pressButton(pControl);
		}
	}, this);

	m_pTextBox = pXUI->createTextBox();
	pCalc->insertChild(m_pTextBox);

	for(int i = 0; i < ARRAYSIZE(aszButtonNames); ++i)
	{
		mButtons[i] = pXUI->createButton();
		pCalc->insertChild(mButtons[i]);
		mButtons[i]->setLabel(aszButtonNames[i]);
	}

	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < ARRAYSIZE(aszButtonNames) / 3; ++j)
		{
			mButtons[j * 3 + i]->setPosition(10 + i * 130, 55 + j * 50);
			mButtons[j * 3 + i]->setSize(120, 40);
		}
	}
	
	m_pTextBox->setPosition(10, 20);
	m_pTextBox->setSize(380, 30);
}

UICalcContainer::~UICalcContainer()
{
	for(UINT i = 0, l = mButtons.size(); i < l; ++i)
	{
		mem_release(mButtons[i]);
	}

	mem_release(pCalc);
}

void UICalcContainer::pressButton(IUIControl *pTarget)
{
	int iButton = mButtons.indexOf(pTarget, [](IUIButton *pA, IUIControl *pB){
		return(pA == pB);
	});
	if(iButton < 0)
	{
		return;
	}

	if(iButton <= 9)
	{
		iButton += 1;
		if(iButton == 10)
		{
			iButton = 0;
		}

		//char tmp[2];
		//sprintf(tmp, "%d", iButton);
		//m_pTextBox->setValue(tmp);

		int64_t *pCurOp = &m_i64FirstOp;
		if(m_bInputNumTwo)
		{
			m_hasSecondOp = true;
			pCurOp = &m_i64SecondOp;
		}
		
		if(m_bResetAtInput)
		{
			m_bResetAtInput = false;
			*pCurOp = 0;
		}

		*pCurOp = *pCurOp * 10 + iButton;

		char tmp[64];
		sprintf(tmp, "%lld", *pCurOp);
		m_pTextBox->setValue(tmp);
	}
	else
	{
		if(m_cOp && (m_bInputNumTwo || iButton == 14))
		{
			m_bInputNumTwo = false;

			if(!m_hasSecondOp)
			{
				m_i64SecondOp = m_i64FirstOp;
				m_hasSecondOp = true;
			}

			switch(m_cOp)
			{
			case '+':
				m_i64FirstOp += m_i64SecondOp;
				break;
			case '-':
				m_i64FirstOp -= m_i64SecondOp;
				break;
			case '*':
				m_i64FirstOp *= m_i64SecondOp;
				break;
			case '/':
				m_i64FirstOp /= m_i64SecondOp;
				break;
			}
			m_bResetAtInput = true;
			char tmp[64];
			sprintf(tmp, "%lld", m_i64FirstOp);
			m_pTextBox->setValue(tmp);
		}

		switch(iButton)
		{
		case 10:
			m_cOp = '+';
			m_bInputNumTwo = true;
			m_bResetAtInput = true;
			break;
		case 11:
			m_cOp = '-';
			m_bInputNumTwo = true;
			m_bResetAtInput = true;
			break;
		case 12:
			m_cOp = '*';
			m_bInputNumTwo = true;
			m_bResetAtInput = true;
			break;
		case 13:
			m_cOp = '/';
			m_bInputNumTwo = true;
			m_bResetAtInput = true;
			break;
		}
	}
}
