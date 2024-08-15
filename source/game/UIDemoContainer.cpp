#include "UIDemoContainer.h"

UIDemoContainer::UIDemoContainer(IXUI* pXUI)
{
	XWINDOW_DESC wdesc;
	wdesc.iPosX = XCW_CENTER;
	wdesc.iPosY = XCW_CENTER;
	wdesc.iSizeX = 800;
	wdesc.iSizeY = 600;
	wdesc.szTitle = "Demo window";
	wdesc.flags = XWF_BUTTON_CLOSE | XWF_BUTTON_MINIMIZE | XWF_BUTTON_MAXIMIZE | XWF_TRANSPARENT;

	pDemo = pXUI->createWindow(&wdesc);
	pDemo->setCallback([](void *pCtx, IUIControl *pControl, gui::IEvent *ev){
		LogInfo("%u\n", ev->type);
	}, this);

	IUIPanel *pPanel = pXUI->createPanel();
	pDemo->insertChild(pPanel);
	pPanel->setPosition(50.0, 50.0f);
	pPanel->setSize(300.0, 500.0f);

	IUISpoiler *pSpoiler = pXUI->createSpoiler();
	pPanel->insertChild(pSpoiler);
	pSpoiler->setLabel("Спойлер");

	IUIButton *pButton = pXUI->createButton();
	pSpoiler->insertChild(pButton);
	pButton->setLabel("Проверка");
	//pButton->setEnabled(false);

	pSpoiler = pXUI->createSpoiler();
	pPanel->insertChild(pSpoiler);
	pSpoiler->setLabel("Спойлер 2");

	pButton = pXUI->createButton();
	pSpoiler->insertChild(pButton);
	pButton->setLabel("Проверка 2");
	pButton->setEnabled(false);

	IUICheckbox *pCheckbox = pXUI->createCheckBox();
	pSpoiler->insertChild(pCheckbox);
	pCheckbox->setLabel("Проверка 3");
	pCheckbox->setChecked(true);
	//pCheckbox->setEnabled(false);

	IUITextBox *pTextbox = pXUI->createTextBox();
	pSpoiler->insertChild(pTextbox);
	pTextbox->setLabel("Проверка 4");
	//pTextbox->setEnabled(false);
	pTextbox->setValue("Содержимое текстбокса");

	IUIComboBox *pCombobox = pXUI->createComboBox();
	pSpoiler->insertChild(pCombobox);
	pCombobox->setLabel("Проверка 5");
	pCombobox->addItem("Варианта 1", "0");
	pCombobox->addItem("Вариант 2", "1");
	pCombobox->addItem("Вариант 3", "2");
	pCombobox->addItem("Вариант 4", "3");
	pCombobox->addItem("Вариант 5", "4");
	pCombobox->addItem("Вариант 5", "5");
	pCombobox->addItem("Вариант 5", "6");
	pCombobox->addItem("Вариант 5", "7");
	pCombobox->addItem("Вариант 5", "8");
	pCombobox->addItem("Вариант 5", "9");
	pCombobox->addItem("Вариант 5", "10");
	pCombobox->addItem("Вариант 6", "11");
	pCombobox->setValue("2");
	//pCombobox->setEnabled(false);

	//pButton = pXUI->createButton();
	//pSpoiler->insertChild(pButton);
	//pButton->setLabel("Проверка 6");

	// pButton = pXUI->createButton();
	// pPanel->insertChild(pButton);
	// pButton->setLabel("Проверка2");

	mem_release(pPanel);
}