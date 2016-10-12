
#include "Menu.h"
#undef LOG_TAG
#define	 LOG_TAG "Menu.cpp"
#include "debug.h"
#include <network/hotspot.h>
#include <network/hotspotSetting.h>
#include <network/wifiIf.h>
#include <gps/GpsController.h>
#ifdef MODEM_ENABLE
#include <cellular/CellularController.h>
#endif

#ifdef MESSAGE_FRAMEWORK
#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"
#endif


#define IDC_MENULIST			400

using namespace android;

int MenuProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	Menu* menu;
	ResourceManager* rm;
	switch(message) {
	case MSG_CREATE:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		if(menu->createSubWidgets(hWnd) < 0) {
			db_error("create subWidgets failed\n");
			break;
		}
		break;
	case MSG_FONTCHANGED:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->updateMenuTexts();
		menu->updateWindowFont();
		break;
	case MSG_SHOWWINDOW:
		if(wParam == SW_SHOWNORMAL) {
			/* 因为创建MenuList的时候没有指定WS_VISIBLE的属性(防止启动的时候闪屏)， 所以需要调用ShowWindow */
			ShowWindow(GetDlgItem(hWnd, IDC_MENULIST), SW_SHOWNORMAL);
			SetFocusChild(GetDlgItem(hWnd, IDC_MENULIST));
		}
		break;
	case MWM_CHANGE_FROM_WINDOW:
		SendMessage(GetDlgItem(hWnd, IDC_MENULIST), LB_SETCURSEL, 0, 0);
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->updateMenuTexts();
		menu->updateSwitchIcons();
		break;
	case MLM_NEW_SELECTED:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		menu->updateNewSelected(wParam, lParam);
		break;
	case MSG_CDR_KEY:
		menu = (Menu*)GetWindowAdditionalData(hWnd);
		return menu->keyProc(wParam, lParam);
	default:
		break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

Menu::Menu(CdrMain* pCdrMain)
	: mCdrMain(pCdrMain)
	, itemImageArray(NULL)
	, value1ImageArray(NULL)
	, unfoldImageLight()
	, unfoldImageDark()
{
	HWND hParent;
	RECT rect;
	CDR_RECT STBRect;
	ResourceManager* rm;

	itemImageArray = new BITMAP[MENU_LIST_COUNT];
	memset(itemImageArray, 0, sizeof(BITMAP) * MENU_LIST_COUNT);

	value1ImageArray = new BITMAP[MENU_LIST_COUNT];
	memset(value1ImageArray, 0, sizeof(BITMAP) * MENU_LIST_COUNT);

	hParent = pCdrMain->getHwnd();
	rm = ResourceManager::getInstance();
	rm->getResRect(ID_STATUSBAR, STBRect);

	GetWindowRect(hParent, &rect);
	rect.top += STBRect.h;

	mHwnd = CreateWindowEx(WINDOW_MENU, "",
			WS_NONE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			WINDOWID_MENU,
			rect.left, rect.top, RECTW(rect), RECTH(rect),
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create menu window failed\n");
		return;
	}

	ShowWindow(mHwnd, SW_HIDE);

}

Menu::~Menu()
{
	delete []itemImageArray;
	delete []value1ImageArray;
}

int Menu::createSubWidgets(HWND hWnd)
{
	HWND retWnd;
	RECT rect;
	menuListAttr_t menuListAttr;

	GetClientRect(hWnd, &rect);

	getMenuListAttr(menuListAttr);	

	retWnd = CreateWindowEx(CTRL_CDRMenuList, "", 
			WS_CHILD  | WS_VSCROLL | LBS_USEBITMAP | LBS_HAVE_VALUE,
			WS_EX_NONE | WS_EX_USEPARENTFONT,
			IDC_MENULIST,
			rect.left, rect.top, RECTW(rect), RECTH(rect),
			hWnd, (DWORD)&menuListAttr);
	if(retWnd == HWND_INVALID) {
		db_error("create Menu List failed\n");
		return -1;
	}

	createMenuListContents(retWnd);

	return 0;
}

int Menu::getMenuListAttr(menuListAttr_t &attr)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	attr.normalBgc = rm->getResColor(ID_MENU_LIST, COLOR_BGC);
	attr.normalFgc = rm->getResColor(ID_MENU_LIST, COLOR_FGC);
	attr.linec = rm->getResColor(ID_MENU_LIST, COLOR_LINEC_ITEM);
	attr.normalStringc = rm->getResColor(ID_MENU_LIST, COLOR_STRINGC_NORMAL);
	attr.normalValuec = rm->getResColor(ID_MENU_LIST, COLOR_VALUEC_NORMAL);
	attr.selectedStringc = rm->getResColor(ID_MENU_LIST, COLOR_STRINGC_SELECTED);
	attr.selectedValuec = rm->getResColor(ID_MENU_LIST, COLOR_VALUEC_SELECTED);
	attr.scrollbarc = rm->getResColor(ID_MENU_LIST, COLOR_SCROLLBARC);
	attr.itemHeight = 50;

	return 0;
}

/*
 * ----------------------------------------------------------------------------------
 * |   Image   |     itemString    |      first value			 |  second value    |
 * ----------------------------------------------------------------------------------
 * | itemImage | Example: 录像质量 | (sting or image) (optional) | image (optional) |
 * ----------------------------------------------------------------------------------
 * */
int Menu::createMenuListContents(HWND hMenuList)
{
	MENULISTITEMINFO menuListII[MENU_LIST_COUNT];

	memset(&menuListII, 0, sizeof(menuListII));

	if(getItemImages(menuListII) < 0) {
		db_error("get item images failed\n");
		return -1;
	}
	if(getItemStrings(menuListII) < 0) {
		db_error("get item strings failed\n");
		return -1;
	}

	if(getFirstValueStrings(menuListII, -1) < 0) {
		db_error("get first value strings failed\n");
		return -1;
	}
	if(getFirstValueImages(menuListII, -1) < 0) {
		db_error("get first value images failed\n");
		return -1;
	}

	if(getSecondValueImages(menuListII, mlFlags) < 0) {
		db_error("get second value images failed\n");
		return -1;
	}

	for(unsigned int i = 0; i < MENU_LIST_COUNT; i++) {
		memcpy( &menuListII[i].flagsEX, &mlFlags[i], sizeof(MLFlags) );
	}

	db_msg("xxxxxxxxxxxxxxxxxx\n");
	SendMessage(hMenuList, LB_MULTIADDITEM, MENU_LIST_COUNT, (LPARAM)menuListII);
	db_msg("xxxxxxxxxxxxxxxxxx\n");
	//	SendMessage(hMenuList, LB_SETITEMHEIGHT,0,50);	/* set the height is 50 pixels*/
	SendMessage(hMenuList, MLM_HILIGHTED_SPACE, 8, 1);
	db_msg("height is %d\n", SendMessage(hMenuList, LB_GETITEMHEIGHT, 0, 0));

	return 0;
}

int Menu::getItemImages(MENULISTITEMINFO *mlii)
{
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	/* ++++++++++ load the itemImage bmp ++++++++++ */
	for(unsigned int i = 0; i < MENU_LIST_COUNT; i++) {
		retval = rm->getResBmp(menuResourceID[i], BMPTYPE_UNSELECTED, itemImageArray[i]);	
		if(retval < 0) {
			db_error("get bmp failed, i is %d, resID is %d\n", i, menuResourceID[i]);
			return -1;
		}
		mlii[i].hBmpImage = (DWORD)&itemImageArray[i];
	}
	/* ---------- load the itemImageArray bmp ---------- */

	return 0;
}

int Menu::getItemStrings(MENULISTITEMINFO* mlii)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	for(unsigned int i = 0; i < MENU_LIST_COUNT; i++) {
		mlii[i].string = (char*)rm->getResMenuItemString(menuResourceID[i]);
		if(mlii[i].string == NULL) {
			db_error("get the %d label string failed\n", i);
			return -1;
		}
	}

	return 0;
}

int Menu::getFirstValueStrings(MENULISTITEMINFO* mlii, int menuIndex)
{
	const char* ptr;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	if(menuIndex == -1) {
		for(unsigned int i = 0; i < MENU_LIST_COUNT ; i++) {
			if(haveValueString[i] == 1) {
				ptr = rm->getResSubMenuCurString(menuResourceID[i]);
				if(ptr == NULL) {
					db_error("get ResSubMenuString %d failed\n", i);
					return -1;
				}
				mlii[ i ].hValue[0] = (DWORD)ptr;
			}
		}
	} else if(menuIndex >=0 && menuIndex < MENU_LIST_COUNT ){
		if(haveSubMenu[menuIndex]) {
			ptr = rm->getResSubMenuCurString(menuResourceID[menuIndex]);
			if(ptr == NULL) {
				db_error("get ResSubMenuString %d failed\n", menuIndex);
				return -1;
			}
			mlii->hValue[0] = (DWORD)ptr;
		}
	}

	return 0;
}


int Menu::getFirstValueImages(MENULISTITEMINFO* mlii, int menuIndex)
{
	int retval;
	int current;
	ResourceManager* rm;
	HWND hMenuList;

	rm = ResourceManager::getInstance();

	if(menuIndex == -1) {
		for(unsigned int i = 0; i < MENU_LIST_COUNT; i++) {
			if(haveCheckBox[i] == 1) {
				retval = rm->getResBmpSubMenuCheckbox( menuResourceID[i], false, value1ImageArray[i] );
				if(retval < 0) {
					db_error("get first value images failed, i is %d\n", i);
					return -1;
				}
				mlii[i].hValue[0] = (DWORD)&value1ImageArray[i];
			}
		}
	} else if(menuIndex >= 0 && menuIndex < MENU_LIST_COUNT ) {
		if(mHwnd && mHwnd != HWND_INVALID) {
			hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);
			current = SendMessage(hMenuList, LB_GETCURSEL, 0, 0);
			rm->unloadBitMap(value1ImageArray[menuIndex]);
			if(current == menuIndex) {
				retval = rm->getResBmpSubMenuCheckbox( menuResourceID[menuIndex], true, value1ImageArray[menuIndex] );
			} else
				retval = rm->getResBmpSubMenuCheckbox( menuResourceID[menuIndex], false, value1ImageArray[menuIndex] );
			if(retval < 0) {
				db_error("get first value images failed, menuIndex is %d\n", menuIndex);
				return -1;
			}
		}
	}

	return 0;
}

int Menu::getSecondValueImages(MENULISTITEMINFO* mlii, MLFlags* mlFlags)
{
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	retval = rm->getResBmp(ID_MENU_LIST_UNFOLD_PIC, BMPTYPE_UNSELECTED, unfoldImageLight);
	if(retval < 0) {
		db_error("get secondValue image failed\n");
		return -1;
	}
	retval = rm->getResBmp(ID_MENU_LIST_UNFOLD_PIC, BMPTYPE_SELECTED, unfoldImageDark);
	if(retval < 0) {
		db_error("get secondValue image failed\n");
		return -1;
	}

	for(unsigned int i = 0; i < MENU_LIST_COUNT; i++) {
		if(mlFlags[i].valueCount == 2) {
			mlii[i].hValue[1] = (DWORD)&unfoldImageLight;
		}
	}

	return 0;
}


int Menu::updateNewSelected(int oldSel, int newSel)
{
	HWND hMenuList;
	MENULISTITEMINFO mlii;
	ResourceManager* rm;
	int retval;

	db_msg("xxxxxoldSel is %d, newSel is %d\n", oldSel, newSel);

	if(oldSel >= MENU_LIST_COUNT || newSel >= MENU_LIST_COUNT)
		return -1;

	if(oldSel == newSel)
		return 0;

	if(mHwnd == 0 || mHwnd == HWND_INVALID)
		return -1;

	db_msg("xxxxxxxxxxx\n");

	rm = ResourceManager::getInstance();
	hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);

	if(oldSel >= 0) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, oldSel, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed\n");
			return -1;
		}
		rm->unloadBitMap(itemImageArray[oldSel]);
		retval = rm->getResBmp(menuResourceID[oldSel] ,BMPTYPE_UNSELECTED, itemImageArray[oldSel]);
		if(retval < 0) {
			db_error("get item image failed, oldSel is %d\n", oldSel);
			return -1;
		}
		mlii.hBmpImage = (DWORD)&itemImageArray[oldSel];

		if(haveCheckBox[oldSel] == 1) {
			rm->unloadBitMap(value1ImageArray[oldSel]);
			retval = rm->getResBmpSubMenuCheckbox(menuResourceID[oldSel], false, value1ImageArray[oldSel]);
			if(retval < 0) {
				db_error("get resBmp SubMenu Check box failed, oldSel is %d\n", oldSel);
				return -1;
			}
			mlii.hValue[0] = (DWORD)&value1ImageArray[oldSel];
		} else if(mlii.flagsEX.valueCount == 2) {
			mlii.hValue[1] = (DWORD)&unfoldImageLight;
		}

		db_msg("xxxxxxxxx\n");
		SendMessage(hMenuList, LB_SETITEMDATA, oldSel, (LPARAM)&mlii);
		db_msg("xxxxxxxxx\n");
	}

	if(newSel >= 0) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, newSel, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed\n");
			return -1;
		}
		rm->unloadBitMap(itemImageArray[newSel]);
		retval = rm->getResBmp(menuResourceID[newSel],BMPTYPE_SELECTED, itemImageArray[newSel]);
		if(retval < 0) {
			db_error("get item image failed, oldSel is %d\n", oldSel);
			return -1;
		}
		mlii.hBmpImage = (DWORD)&itemImageArray[newSel];

		if(haveCheckBox[newSel] == 1) {
			rm->unloadBitMap(value1ImageArray[newSel]);
			retval = rm->getResBmpSubMenuCheckbox(menuResourceID[newSel], true, value1ImageArray[newSel]);
			if(retval < 0) {
				db_error("get resBmp SubMenu Check box failed, oldSel is %d\n", oldSel);
				return -1;
			}
			mlii.hValue[0] = (DWORD)&value1ImageArray[newSel];
		} else if(mlii.flagsEX.valueCount == 2) {
			mlii.hValue[1] = (DWORD)&unfoldImageDark;
		}

		SendMessage(hMenuList, LB_SETITEMDATA, newSel, (LPARAM)&mlii);
	}

	return 0;
}

int Menu::keyProc(int keyCode, int isLongPress)
{
	switch(keyCode) {
	case CDR_KEY_MODE:
		{
			ResourceManager* rm;	
			rm = ResourceManager::getInstance();
			rm->syncConfigureToDisk();
			return WINDOWID_RECORDPREVIEW;
		}
	case CDR_KEY_OK:
		{
			HWND hMenuList;
			unsigned int selectedItem;
			int newSel, retval;
			bool isModified, isChecked;
			StorageManager *sm = StorageManager::getInstance();
			hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);
			if(isLongPress == SHORT_PRESS) {
				selectedItem = SendMessage(hMenuList, LB_GETCURSEL, 0, 0);
				db_msg("selectedItem is %d\n", selectedItem);
				switch(selectedItem) {
				case MENU_INDEX_VQ:	case MENU_INDEX_PQ:			case MENU_INDEX_VTL: 
				case MENU_INDEX_WB:	case MENU_INDEX_CONTRAST:	case MENU_INDEX_EXPOSURE:
				case MENU_INDEX_SS:	case MENU_INDEX_LANG:
					if( ( newSel = ShowSubMenu(selectedItem, isModified) ) < 0) {
						db_error("show submenu %d failed\n", selectedItem);
						break;
					}
					if(isModified == true)
						HandleSubMenuChange(selectedItem, newSel);
					break;
				case MENU_INDEX_POR:	case MENU_INDEX_SILENTMODE:		case MENU_INDEX_GPS:	
				case MENU_INDEX_TWM:	case MENU_INDEX_AWMD:
#ifdef MODEM_ENABLE
				case MENU_INDEX_CELLULAR:
#endif
					if(getCheckBoxStatus(selectedItem, isChecked) < 0) {
						db_error("get check box status failed\n");
						break;
					}
					HandleSubMenuChange(selectedItem, isChecked);
					if(selectedItem == MENU_INDEX_GPS) {
						if(isChecked) {
							mCdrMain->notify(EVENT_GPS, 1);
						#ifdef MESSAGE_FRAMEWORK
							{
								CommuReq req;
								req.cmdType = COMMU_TYPE_GPS;
								req.operType = 1;
								fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							}
						#else
							enableGps();
						#endif
						} else {
							mCdrMain->notify(EVENT_GPS, 0);
						#ifdef MESSAGE_FRAMEWORK
							{
								CommuReq req;
								req.cmdType = COMMU_TYPE_GPS;
								req.operType = 0;
								fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							}
						#else
							disableGps();
						#endif
						}
					}
#ifdef MODEM_ENABLE
					if(selectedItem == MENU_INDEX_CELLULAR) {
						if(isChecked){
							mCdrMain->notify(EVENT_CELLULAR, 1);
						#ifdef MESSAGE_FRAMEWORK
							{
								CommuReq req;
								req.cmdType = COMMU_TYPE_CELLULAR;
								req.operType = 1;
								fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							}
						#else
							enableCellular();
						#endif
						}else{
							mCdrMain->notify(EVENT_CELLULAR, 0);
						#ifdef MESSAGE_FRAMEWORK
							{
								CommuReq req;
								req.cmdType = COMMU_TYPE_CELLULAR;
								req.operType = 0;
								fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							}
						#else
							disableCellular();
						#endif
						}
					}
#endif
					break;
				case MENU_INDEX_DATE:
					ShowDateDialog();
					break;
				case MENU_INDEX_AP:
					ShowAPSettingDialog();
					break;
				case MENU_INDEX_WIFI:
					if(getCheckBoxStatus(selectedItem, isChecked) < 0) {
                                                db_error("get check box status failed\n");
                                                break;
                                        }
					HandleSubMenuChange(selectedItem, isChecked);
					if(isChecked) {
						if (isSoftapEnabled()) {
							mCdrMain->notify(EVENT_AP, 0);
							ResourceManager* rm = ResourceManager::getInstance();
							rm->setResBoolValue(ID_MENU_LIST_AP, 0);
							rm->syncConfigureToDisk();
						}
						#ifdef MESSAGE_FRAMEWORK
							{
								CommuReq req;
								req.cmdType = COMMU_TYPE_WIFI;
								req.operType = 1;
								fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							}
						#else
						if (!enable_wifi())
							mCdrMain->notify(EVENT_WIFI, 1);
						#endif
					} else {
						#ifdef MESSAGE_FRAMEWORK
							{
								CommuReq req;
								req.cmdType = COMMU_TYPE_WIFI;
								req.operType = 0;
								fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							}
						#else
						if (!disable_wifi())
							mCdrMain->notify(EVENT_WIFI, 0);
						#endif
					}
					break;
				case MENU_INDEX_FORMAT:
				case MENU_INDEX_FRESET:
					if(selectedItem == MENU_INDEX_FORMAT) {
						if (!sm->isInsert()) {
							showAlphaEffect();
							ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
							cancelAlphaEffect();
							break;
						}
					}
					retval = ShowMessageBox(selectedItem);
					if(retval < 0) {
						db_error("ShowMessageBox err, retval is %d\n", retval);
						break;
					}
					if(retval == IDC_BUTTON_OK) {
						if(selectedItem == MENU_INDEX_FRESET){
							#ifdef MESSAGE_FRAMEWORK
							CommuReq req;
							req.cmdType = COMMU_TYPE_FACTORY_RESET;
							req.operType = 1;
							db_msg("fwk_msg send factory reset \n");
							fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
							#else
							doFactoryReset();
							#endif
						}
						else if(selectedItem == MENU_INDEX_FORMAT) {
							StorageManager* sm;
							sm = StorageManager::getInstance();
							if(sm->isInsert() == false) {
								db_msg("tfcard not mount\n");
								ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
								break;
							}
							ShowFormattingTip();
						}
					}
					break;
				default:
					break;
				}
			} else {
				selectedItem = SendMessage(hMenuList, LB_GETCURSEL, 0, 0);
                                db_msg("LONG_PRESS:selectedItem is %d\n", selectedItem);
				switch (selectedItem) {
				case MENU_INDEX_WIFI:
					if (check_if_wifi_enabled())
						startWifiWpsPBC();
					break;
				default:
					break;
				}		
			}
		}
		break;
	default:
		break;
	}

	return WINDOWID_MENU;
}

int Menu::ShowSubMenu(unsigned int menuIndex, bool &isModified)
{
	int retval;
	int oldSel;
	subMenuData_t subMenuData;

	if(getSubMenuData(menuIndex, subMenuData) < 0) {
		db_error("get submenu data failed\n");
		return -1;
	}
	oldSel = subMenuData.selectedIndex;

	showAlphaEffect();
	retval = showSubMenu(mHwnd, &subMenuData);
	db_msg("retval = %d\n", retval);
	cancelAlphaEffect();

	if(retval >= 0) {
		if(retval != oldSel) {
			db_msg("change from %d to %d\n", oldSel, retval);
			isModified = true;
		}
	}

	return retval;
}

int Menu::getCheckBoxStatus(unsigned int menuIndex, bool &isChecked)
{
	ResourceManager* rm;
	bool curStatus;

	rm = ResourceManager::getInstance();
	if(menuIndex >= MENU_LIST_COUNT )
		return -1;
	if(haveCheckBox[menuIndex] != 1) {
		db_error("menuIndex: %d, don not have checkbox\n", menuIndex);
		return -1;
	}

	curStatus = rm->getResBoolValue(menuResourceID[menuIndex]);

	isChecked = ( (curStatus == true) ? false : true );

	return 0;
}

int Menu::getSubMenuData(unsigned int subMenuIndex, subMenuData_t &subMenuData)
{
	int retval;
	int contentCount;
	const char* ptr;
	ResourceManager* rm;

	if(haveSubMenu[subMenuIndex] != 1) {
		db_error("invalid index %d\n", subMenuIndex);
		return -1;
	}

	rm = ResourceManager::getInstance();

	retval = rm->getResRect(ID_SUBMENU, subMenuData.rect);
	if(retval < 0) {
		db_error("get subMenu rect failed\n");
		return -1;
	}
	retval = rm->getResBmp(ID_SUBMENU_CHOICE_PIC, BMPTYPE_BASE, subMenuData.BmpChoice);
	if(retval < 0) {
		db_error("get subMenu choice bmp failed\n");
		return -1;
	}

	getMenuListAttr(subMenuData.menuListAttr);
	subMenuData.lincTitle = rm->getResColor(ID_SUBMENU, COLOR_LINEC_TITLE);
	subMenuData.pLogFont = rm->getLogFont();

	/* current submenu index */
	retval = rm->getResIntValue(menuResourceID[subMenuIndex], INTVAL_SUBMENU_INDEX );
	if(retval < 0) {
		db_error("get res submenu index failed\n");
		return -1;
	}
	subMenuData.selectedIndex = retval;

	/* submenu count */
	contentCount = rm->getResIntValue(menuResourceID[subMenuIndex], INTVAL_SUBMENU_COUNT);

	/* submenu title */
	ptr = rm->getResSubMenuTitle(menuResourceID[subMenuIndex]);
	if(ptr == NULL) {
		db_error("get video quality title failed\n");
		return -1;
	}
	subMenuData.title = ptr;

	/* submenu contents */
	for(int i = 0; i < contentCount; i++) {
		ptr = rm->getLabel(subMenuContent0Cmd[subMenuIndex] + i);
		if(ptr == NULL) {
			db_error("get video quality content %d failed", i);
			return -1;
		}
		subMenuData.contents.add(String8(ptr));
	}

	return 0;
}

void Menu::showAlphaEffect(void)
{
	RECT rect;
	HWND hMainWin;
	HDC secDC, MainWindowDC;

	hMainWin = GetMainWindowHandle(mHwnd);
	GetClientRect(hMainWin, &rect);
	secDC = GetSecondaryDC(hMainWin);
	MainWindowDC = GetDC(hMainWin);

	db_msg("rect.left is %d\n", rect.left);
	db_msg("rect.top is %d\n", rect.top);
	db_msg("rect.right is %d\n", rect.right);
	db_msg("rect.bottom is %d\n", rect.bottom);

	SetMemDCAlpha(secDC, MEMDC_FLAG_SRCALPHA, 180);
	ShowWindow(mHwnd, SW_HIDE);
	ShowWindow(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), SW_HIDE);

	BitBlt(secDC, 0, 0, RECTW(rect), RECTH(rect), MainWindowDC, 0, 0, 0);

	ReleaseDC(MainWindowDC);
}

void Menu::cancelAlphaEffect(void)
{
	RECT rect;
	HWND hMainWin;
	HDC secDC, MainWindowDC;

	hMainWin = GetMainWindowHandle(mHwnd);
	GetClientRect(hMainWin, &rect);
	secDC = GetSecondaryDC(hMainWin);
	MainWindowDC = GetDC(hMainWin);

	SetMemDCAlpha(secDC, 0, 0);
	BitBlt(secDC, 0, 0, RECTW(rect), RECTH(rect), secDC, 0, 0, 0);
	ShowWindow(mHwnd, SW_SHOWNORMAL);
	ShowWindow(mCdrMain->getWindowHandle(WINDOWID_STATUSBAR), SW_SHOWNORMAL);
	SetSecondaryDC(mHwnd, secDC, ON_UPDSECDC_DEFAULT);

	ReleaseDC(MainWindowDC);
}

int Menu::HandleSubMenuChange(unsigned int menuIndex, int newSel)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	switch(menuIndex) {
	case MENU_INDEX_LANG:	
	case MENU_INDEX_VQ:	
	case MENU_INDEX_PQ:	
	case MENU_INDEX_VTL:	
	case MENU_INDEX_WB:	
	case MENU_INDEX_CONTRAST:	
	case MENU_INDEX_EXPOSURE:	
	case MENU_INDEX_SS:	
		if(haveSubMenu[menuIndex] != 1) {
			db_error("invalid menuIndex %d\n", menuIndex);
			return -1;
		}
	#ifdef MESSAGE_FRAMEWORK
		if(MENU_INDEX_VQ == menuIndex)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_RESOLUTION;
			if(0 == newSel)
			{
				req.value = 1080;
			}
			else if(1 == newSel)
			{
				req.value = 720;
			}
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(MENU_INDEX_VTL== menuIndex)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_RECORD_TIME;
			db_debug("VTL newSel=%d", newSel);
			req.value = (newSel+1)*60;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(MENU_INDEX_EXPOSURE == menuIndex)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_EXPOSURE;
			db_debug("Exposure newSel=%d", newSel);
			req.value = newSel-3;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(MENU_INDEX_WB== menuIndex)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_WB;
			db_debug("WhiteBalance newSel=%d", newSel);
			req.value = newSel;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(MENU_INDEX_CONTRAST== menuIndex)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_CONTRAST;
			db_debug("Contrast newSel=%d", newSel);
			req.value = newSel;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else
		{
			if(rm->setResIntValue(menuResourceID[menuIndex], INTVAL_SUBMENU_INDEX, newSel) < 0) {
				db_error("set %d to %d failed\n", menuIndex, newSel);	
				return -1;
			}
		}
	#else
		if(rm->setResIntValue(menuResourceID[menuIndex], INTVAL_SUBMENU_INDEX, newSel) < 0) {
			db_error("set %d to %d failed\n", menuIndex, newSel);	
			return -1;
		}
	#endif
		break;
	default:
		db_error("invalid menuIndex %d\n", menuIndex);
		return -1;
	}

	HWND hMenuList;
	MENULISTITEMINFO mlii;
	hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);

	if(menuIndex != MENU_INDEX_LANG) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, menuIndex, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed, menuIndex is %d\n", menuIndex);
			return -1;
		}

		if(getFirstValueStrings(&mlii, menuIndex) < 0) {
			db_error("get first value strings failed\n");
			return -1;
		}
		db_msg("xxxxxxxx\n");
		SendMessage(hMenuList, LB_SETITEMDATA, menuIndex, (LPARAM)&mlii);
		db_msg("xxxxxxxx\n");
	}

	return 0;
}

int Menu::HandleSubMenuChange(unsigned int menuIndex, bool newValue)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	switch(menuIndex) {
	case MENU_INDEX_POR:
	case MENU_INDEX_SILENTMODE:
	case MENU_INDEX_TWM:
	case MENU_INDEX_AWMD:
	case MENU_INDEX_GPS:
#ifdef MODEM_ENABLE
	case MENU_INDEX_CELLULAR:
#endif
	case MENU_INDEX_WIFI:
		if(haveCheckBox[menuIndex] != 1) {
			db_error("invalid menuIndex %d\n", menuIndex);
			return -1;
		}
		if(rm->setResBoolValue(menuResourceID[menuIndex], newValue) < 0) {
			db_error("set %d to %d failed\n", menuIndex, newValue);
			return -1;
		}
		break;
	default:
		db_error("invalid menuIndex %d\n", menuIndex);
		return -1;
	}

	HWND hMenuList;
	MENULISTITEMINFO mlii;
	hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);
	memset(&mlii, 0, sizeof(mlii));
	if(SendMessage(hMenuList, LB_GETITEMDATA, menuIndex, (LPARAM)&mlii) != LB_OKAY) {
		db_error("get item info failed, menuIndex is %d\n", menuIndex);
		return -1;
	}
	if(getFirstValueImages(&mlii, menuIndex) < 0) {
		db_error("get first value images failed, menuIndex is %d\n", menuIndex);
		return -1;
	}

	db_msg("xxxxxxxx\n");
	SendMessage(hMenuList, LB_SETITEMDATA, menuIndex, (LPARAM)&mlii);
	db_msg("xxxxxxxx\n");

	return 0;
}


int Menu::updateMenuTexts(void)
{
	int itemCount;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[MENU_LIST_COUNT];

	hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get menu counts failed\n");
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	if(getItemStrings(menuListII) < 0) {
		db_error("get item strings failed\n");
		return -1;
	}
	if(getFirstValueStrings(menuListII, -1) < 0) {
		db_error("get item value strings failed\n");
		return -1;
	}

	db_msg("xxxxxxxxxx\n");
	for(int count = 0; count < itemCount; count++) {
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
		db_msg("xxxxxxxxxx\n");
	}

	return 0;
}

int Menu::updateSwitchIcons(void)
{
	int itemCount;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[MENU_LIST_COUNT];

	hMenuList = GetDlgItem(mHwnd, IDC_MENULIST);
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get menu counts failed\n");
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	getFirstValueImages(menuListII, -1);
	db_msg("xxxxxxxxxx\n");
	for(int count = 0; count < itemCount; count++) {
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
		db_msg("xxxxxxxxxx\n");
	}

	return 0;
}


int Menu::getMessageBoxData(unsigned int menuIndex, MessageBox_t &messageBoxData)
{
	const char* ptr;
	unsigned int i;
	ResourceManager* rm;

	unsigned int indexTable [] = {
		MENU_INDEX_FORMAT,
		MENU_INDEX_FRESET
	};

	unsigned int resCmd[][2] = {
		{LANG_LABEL_SUBMENU_FORMAT_TITLE, LANG_LABEL_SUBMENU_FORMAT_TEXT},
		{LANG_LABEL_SUBMENU_FRESET_TITLE, LANG_LABEL_SUBMENU_FRESET_TEXT},
	};


	rm = ResourceManager::getInstance();

	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;
	messageBoxData.flag_end_key = 1; /* end the dialog when endkey keyup */

	for(i = 0; i < TABLESIZE(indexTable); i++) {
		if(indexTable[i] == menuIndex) {
			ptr = rm->getLabel(resCmd[i][0]);
			if(ptr == NULL) {
				db_error("get title failed, menuIndex is %d\n", menuIndex);
				return -1;
			}
			messageBoxData.title = ptr;

			ptr = rm->getLabel(resCmd[i][1]);
			if(ptr == NULL) {
				db_error("get text failed, menuIndex is %d\n", menuIndex);
				return -1;
			}
			messageBoxData.text = ptr;

			ptr = rm->getLabel(LANG_LABEL_SUBMENU_OK);
			if(ptr == NULL) {
				db_error("get ok string failed, menuIndex is %d\n", menuIndex);
				return -1;
			}
			messageBoxData.buttonStr[0] = ptr;

			ptr = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
			if(ptr == NULL) {
				db_error("get ok string failed, menuIndex is %d\n", menuIndex);
				return -1;
			}
			messageBoxData.buttonStr[1] = ptr;
		}
		db_msg("xxxxxxxx\n");
	}

	if(i > TABLESIZE(indexTable)) {
		db_error("invalid menuIndex: %d\n", menuIndex);
		return -1;
	}

	messageBoxData.pLogFont = rm->getLogFont();

	rm->getResRect(ID_MENU_LIST_MB, messageBoxData.rect);
	messageBoxData.fgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_FGC);
	messageBoxData.bgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_BGC);
	messageBoxData.linecTitle = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_ITEM);

	return 0;
}

static void formatCallback(HWND hDlg, void* data)
{
	Menu* menu;
	menu = (Menu*)data;
	db_msg("formatMessageBoxCallback\n");

#ifdef MESSAGE_FRAMEWORK
	{
		CommuReq req;
		req.cmdType = COMMU_TYPE_FORMAT;
		req.operType = 1;
		db_msg("fwk_msg send format \n");
		fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
	}
#else
	menu->doFormatTFCard();
#endif
	db_msg("formatMessageBoxCallback finish\n");
	EndDialog(hDlg, IDC_BUTTON_OK);
}

int Menu::ShowFormattingTip()
{
	tipLabelData_t tipLabelData;
	ResourceManager* rm;
	int retval;

	rm = ResourceManager::getInstance();
	memset(&tipLabelData, 0, sizeof(tipLabelData));
	if(getTipLabelData(&tipLabelData) < 0) {
		db_error("get TipLabel data failed\n");	
		return -1;
	}

	tipLabelData.title = rm->getLabel(LANG_LABEL_TIPS);
	if(!tipLabelData.title) {
		db_error("get FormattingTip titile failed\n");
		return -1;
	}

	tipLabelData.text = rm->getLabel(LANG_LABEL_SUBMENU_FORMATTING_TEXT);
		if(!tipLabelData.text) {
			db_error("get LANG_LABEL_LOW_POWER_TEXT failed\n");
			return -1;
	}

	tipLabelData.timeoutMs = TIME_INFINITE;
	tipLabelData.disableKeyEvent = true;
	tipLabelData.callback = formatCallback;
	tipLabelData.callbackData = this;

	showAlphaEffect();
	retval = showTipLabel(mHwnd, &tipLabelData);
	cancelAlphaEffect();

	return retval;
}

int Menu::ShowMessageBox(unsigned int menuIndex)
{
	int retval;
	MessageBox_t messageBoxData;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	memset(&messageBoxData, 0, sizeof(messageBoxData));
	if(getMessageBoxData(menuIndex, messageBoxData) < 0) {
		db_error("get messagebox data failed\n");
		return -1;
	}

	showAlphaEffect();
	retval = showMessageBox(mHwnd, &messageBoxData);
	cancelAlphaEffect();

	db_msg("retval is %d\n", retval);
	return retval;
}

int Menu::ShowAPSettingDialog(void)
{
	int retval;
	ResourceManager* rm;
	apData_t configData;

	rm = ResourceManager::getInstance();

	configData.mSwitch = rm->getLabel(LANG_LABEL_AP_SWITCH);
	configData.onStr = rm->getLabel(LANG_LABEL_AP_ON);
	configData.offStr = rm->getLabel(LANG_LABEL_AP_OFF);
	configData.ssid = rm->getLabel(LANG_LABEL_AP_SSID);
	configData.password = rm->getLabel(LANG_LABEL_AP_PASSWORD);
	if(configData.mSwitch == NULL)
		return -1;
	if(configData.ssid == NULL)
		return -1;
	if(configData.password == NULL)
		return -1;

	retval = rm->getResRect(ID_MENU_LIST_AP, configData.rect);
	if(retval < 0) {
		db_error("get ap rect failed\n");
		return -1;
	}

	//configData.titleRectH = rm->getResIntValue(ID_MENU_LIST_AP, INTVAL_TITLEHEIGHT);
	//if(configData.titleRectH < 0)
	//	return -1;

	configData.bgc_widget = rm->getResColor(ID_MENU_LIST_AP, COLOR_BGC);
	configData.fgc_label = rm->getResColor(ID_MENU_LIST_AP, COLOR_FGC_LABEL);
	configData.fgc_number = rm->getResColor(ID_MENU_LIST_AP, COLOR_FGC_NUMBER);
	configData.linec_title = rm->getResColor(ID_MENU_LIST_AP, COLOR_LINEC_TITLE);
	configData.borderc_selected = rm->getResColor(ID_MENU_LIST_AP, COLOR_BORDERC_SELECTED);
	configData.borderc_normal = rm->getResColor(ID_MENU_LIST_AP, COLOR_BORDERC_NORMAL);
	configData.pLogFont = rm->getLogFont();

	configData.rm = rm;
	configData.Cdr = mCdrMain;

	showAlphaEffect();
	retval = apSettingDialog(mHwnd, &configData);
	cancelAlphaEffect();
	return retval;
}

int Menu::ShowDateDialog(void)
{
	int retval;
	ResourceManager* rm;
	dateSettingData_t configData;

	rm = ResourceManager::getInstance();

	configData.title = rm->getLabel(LANG_LABEL_DATE_TITLE);
	configData.year = rm->getLabel(LANG_LABEL_DATE_YEAR);
	configData.month = rm->getLabel(LANG_LABEL_DATE_MONTH);
	configData.day = rm->getLabel(LANG_LABEL_DATE_DAY);
	configData.hour = rm->getLabel(LANG_LABEL_DATE_HOUR);
	configData.minute = rm->getLabel(LANG_LABEL_DATE_MINUTE);
	configData.second = rm->getLabel(LANG_LABEL_DATE_SECOND);
	if(configData.title == NULL)
		return -1;
	if(configData.year == NULL)
		return -1;
	if(configData.month == NULL)
		return -1;
	if(configData.day == NULL)
		return -1;
	if(configData.hour == NULL)
		return -1;
	if(configData.minute == NULL)
		return -1;
	if(configData.second == NULL)
		return -1;

	retval = rm->getResRect(ID_MENU_LIST_DATE, configData.rect);
	if(retval < 0) {
		db_error("get date rect failed\n");
		return -1;
	}

	configData.titleRectH = rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_TITLEHEIGHT);
	configData.hBorder	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_HBORDER);
	configData.yearW	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_YEARWIDTH);
	configData.numberW	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_NUMBERWIDTH);
	configData.dateLabelW = rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_LABELWIDTH);
	configData.boxH	= rm->getResIntValue(ID_MENU_LIST_DATE, INTVAL_BOXHEIGHT);
	if(configData.titleRectH < 0)
		return -1;
	if(configData.hBorder < 0)
		return -1;
	if(configData.yearW < 0)
		return -1;
	if(configData.numberW < 0)
		return -1;
	if(configData.dateLabelW < 0)
		return -1;
	if(configData.boxH < 0)
		return -1;

	configData.bgc_widget = rm->getResColor(ID_MENU_LIST_DATE, COLOR_BGC);
	configData.fgc_label = rm->getResColor(ID_MENU_LIST_DATE, COLOR_FGC_LABEL);
	configData.fgc_number = rm->getResColor(ID_MENU_LIST_DATE, COLOR_FGC_NUMBER);
	configData.linec_title = rm->getResColor(ID_MENU_LIST_DATE, COLOR_LINEC_TITLE);
	configData.borderc_selected = rm->getResColor(ID_MENU_LIST_DATE, COLOR_BORDERC_SELECTED);
	configData.borderc_normal = rm->getResColor(ID_MENU_LIST_DATE, COLOR_BORDERC_NORMAL);
	configData.pLogFont = rm->getLogFont();

	showAlphaEffect();
	retval = dateSettingDialog(mHwnd, &configData);
	cancelAlphaEffect();
	return retval;
}

void Menu::doFactoryReset(void)
{
	db_msg("ido factory reset\n");
	ResourceManager *rm = ResourceManager::getInstance();
	rm->resetResource();
	//resetDateTime();
	updateMenuTexts();
	updateSwitchIcons();
}

void Menu::finish(int what, int extra)
{
	db_error("-----formatfinish!|n");
	mCdrMain->forbidRecord(FORBID_NORMAL);
}

void Menu::doFormatTFCard(void)
{
	StorageManager *sm = StorageManager::getInstance();
	db_msg("do format TF card\n");
	sm->format(this);
	mCdrMain->forbidRecord(FORBID_FORMAT);
	finish(0, 0);
}

