
#include "NewMenu.h"
#undef LOG_TAG
#define	 LOG_TAG "NewMenu.cpp"
#include "debug.h"
#include <network/hotspot.h>
#include <network/hotspotSetting.h>
#include <network/wifiIf.h>
#include <gps/GpsController.h>
#ifdef MODEM_ENABLE
#include <cellular/CellularController.h>
#endif
#include "../widget/include/ctrlclass.h"

#ifdef MESSAGE_FRAMEWORK
#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"
#endif


using namespace android;

static pic_t icon_win_pic[] = {
	{ID_MENUBAR_ICON_WIN0,	"IconWin0"},
	{ID_MENUBAR_ICON_WIN1,	"IconWin1"},
	{ID_MENUBAR_ICON_WIN2,	"IconWin2"},
	{ID_MENUBAR_ICON_WIN3,	"IconWin3"},
};

int MenuProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	Menu* menu = (Menu*)GetWindowAdditionalData(hWnd);
 	ResourceManager* rm = ResourceManager::getInstance();
	//db_msg("message is %s\n", Message2Str(message));
	switch(message) {
	case MSG_CREATE:
		{
			HDC hdc = GetClientDC(hWnd);
			SetWindowBkColor(hWnd, RGBA2Pixel(hdc, 0x00, 0x00, 0x00, 0x00));
			ReleaseDC(hdc);
			if(menu->createSubWidgets(hWnd) < 0) {
				db_error("create menubar widgets failed\n");	
			}
		}
		break;
	case MSG_PAINT:
		{
			HDC hdc;
			RECT rcClient;
			CDR_RECT iconRect, mbRect, menuRect, tmpRect;
			PBITMAP pBgPic;
			PBITMAP pVlinePic;
			PBITMAP pBgLightPic;
			PBITMAP pMlBgPic;
			unsigned int curActiveWin;
			unsigned int curIconCount;

			curActiveWin = menu->getCurActiveIcon();
			SendMessage(GetDlgItem(hWnd, rpMenuResourceID[curActiveWin]), MSG_PAINT, 0, 0);
			hdc = BeginPaint (hWnd);

			db_info("paint ----------menubar----------\n");

			rm->getResRect(ID_MENUBAR, mbRect);
			rm->getResRect(ID_MENU, menuRect);

			// 1.draw menu bar bg
			//以下是实际图片相关操作
			//头部不可拉伸部分
			pBgPic = menu->getBgBmp();
			tmpRect.x = mbRect.x;
			tmpRect.y = mbRect.y - menuRect.y;
			tmpRect.w = pBgPic->bmWidth/2;
			tmpRect.h = mbRect.h;
			FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
									pBgPic, 0, 0, pBgPic->bmWidth/2, pBgPic->bmHeight-2);
			//中间可拉伸部分
			tmpRect.x = tmpRect.x + tmpRect.w;
			tmpRect.w = mbRect.w - pBgPic->bmWidth + (pBgPic->bmWidth % 2);
			FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
									pBgPic, 20, 0, pBgPic->bmWidth-40, pBgPic->bmHeight-2);

			//尾部不可拉伸部分
			tmpRect.x = tmpRect.x + tmpRect.w;
			tmpRect.w = pBgPic->bmWidth / 2;
			FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
							pBgPic, pBgPic->bmWidth/2, 0, pBgPic->bmWidth/2, pBgPic->bmHeight-2);
			
			// 2.draw menu bar icon line
			pVlinePic = menu->getVerticalLineBmp();
			curIconCount = menu->getCurIconCount();
			for (unsigned int i=0; i<curIconCount; i++) {
				rm->getResRect(icon_win_pic[i].id, iconRect);
				FillBoxWithBitmap(hdc, iconRect.x+iconRect.w, iconRect.y-menuRect.y, 2, iconRect.h, pVlinePic);
			}

			// 3.draw menu bar light icon bg
			pBgLightPic = menu->getLightBgBmp();

			rm->getResRect(icon_win_pic[curActiveWin].id, iconRect);
			FillBoxWithBitmap(hdc, iconRect.x, iconRect.y-menuRect.y, iconRect.w, iconRect.h, pBgLightPic);

			EndPaint (hWnd, hdc);
		}
		break;
	case MSG_SHOWWINDOW:
		if(wParam == SW_SHOWNORMAL) {
			unsigned int curIconCount = menu->getCurIconCount();
			unsigned int curActiveWin = menu->getCurActiveIcon();
			MenuWindow_t curMenuWindow = menu->getCurMenuWindow();
			//show icon windows
			for (unsigned int i=0; i<curIconCount; i++) {
				ShowWindow(GetDlgItem(hWnd, icon_win_pic[i].id), SW_SHOWNORMAL);
			}

			//show menulist windows
			if (curMenuWindow == MENU_RECORDPREVIEW) {
				SendMessage(GetDlgItem(hWnd, rpMenuResourceID[curActiveWin]), LB_SETCURSEL, 0, 0);
				ShowWindow(GetDlgItem(hWnd, rpMenuResourceID[curActiveWin]), SW_SHOWNORMAL);
				SetFocusChild(GetDlgItem(hWnd, rpMenuResourceID[curActiveWin]));
			}
			else if (curMenuWindow == MENU_PHOTOGRAPH) {
				SendMessage(GetDlgItem(hWnd, ppMenuResourceID[curActiveWin]), LB_SETCURSEL, 0, 0);
				ShowWindow(GetDlgItem(hWnd, ppMenuResourceID[curActiveWin]), SW_SHOWNORMAL);
				SetFocusChild(GetDlgItem(hWnd, ppMenuResourceID[curActiveWin]));
			}

		}
		break;
	case MWM_CHANGE_FROM_WINDOW:
		//menu->updateMenuListsTexts();
		break;
	case MSG_FONTCHANGED:
		//menu->updateActiveWindowFont();
		menu->updateWindowFont();
		menu->updateMenuListsTexts();
		break;
	case MSG_CDRMAIN_CUR_WINDOW_CHANGED:
		windowState_t *curWindowState;
		curWindowState = (windowState_t*)wParam;
		if(curWindowState == NULL) {
			db_error("invalid windowState\n");
			break;
		}
		menu->handleWindowChanged(curWindowState);
		break;
	case MSG_CDR_KEY:
		return menu->keyProc(wParam, lParam);
	default:
		break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

Menu::Menu(CdrMain* pCdrMain)
	: mCdrMain(pCdrMain)
	, mCurActiveIcon(MB_ICON_WIN0)
	, mCurIconCount(RP_ICON_COUNT)
	, showTMXState(0)
{
	HWND hParent;
	CDR_RECT rect;
	ResourceManager* rm;
	int retval;
	PicObj* pic[MENUBAR_MAX_COUNT];

	hParent = pCdrMain->getHwnd();
	rm = ResourceManager::getInstance();

	/* menu rect */
	retval = rm->getResRect(ID_MENU, rect);
	if(retval < 0) {
		db_error("get rect of ID_MENU failed\n");
		return;
	}
	/* menu bar background pic */
	retval = rm->getResBmp(ID_MENUBAR, BMPTYPE_BASE, mBgBmp);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENUBAR failed\n");
		return;
	}
	/* menu bar vertical line pic */
	retval = rm->getResBmp(ID_MENUBAR_VLINE, BMPTYPE_BASE, mVerticalLine);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENUBAR_VLINE failed\n");
		return;
	}
	/* menu bar light icon background pic */
	retval = rm->getResBmp(ID_MENUBAR_LIGHT, BMPTYPE_BASE, mLightBg);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENUBAR_LIGHT failed\n");
		return;
	}
	
	/* menu list background pic */
	retval = rm->getResBmp(ID_MENU_LIST, BMPTYPE_BASE, mMlBgPic);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENU_LIST failed\n");
		return;
	}

	/* menu item light background pic */
	retval = rm->getResBmp(ID_MENU_LIST_LIGHT, BMPTYPE_BASE, mMlLightBg);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENU_LIST_HLINE failed\n");
		return;
	}
	
	/* menu list horizontal line pic */
	retval = rm->getResBmp(ID_MENU_LIST_HLINE, BMPTYPE_BASE, mHorizontalLine);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENU_LIST_HLINE failed\n");
		return;
	}

	/* menu list horizontal line pic */
	retval = rm->getResBmp(ID_MENU_LIST_SELECT, BMPTYPE_BASE, mMlSelectIcon);
	if(retval < 0) {
		db_error("loadb bitmap of ID_MENU_LIST_SELECT failed\n");
		return;
	}

	/* init icon pic */
	for(unsigned int i = 0; i < MENUBAR_MAX_COUNT; i++ ) {
		pic[i] = new PicObj(String8(icon_win_pic[i].name));
		pic[i]->setId(icon_win_pic[i].id);
		mIconPic.add(pic[i]);
	}

	//db_info("rect.x=%d, rect.y=%d, rect.w=%d, rect.h=%d--------------\n", rect.x, rect.y, rect.w, rect.h);
	mHwnd = CreateWindowEx(WINDOW_MENU, "",
			WS_CHILD,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			WINDOWID_MENU,
			rect.x, rect.y, rect.w, rect.h,
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create menu bar window failed\n");
		return;
	}
	rm->setHwnd(WINDOWID_MENU, mHwnd);
	mCurMenuWindow = MENU_RECORDPREVIEW;

	ShowWindow(mHwnd, SW_HIDE);
}

Menu::~Menu()
{
	for(unsigned int i = 0; i < mIconPic.size(); i++) {
		delete mIconPic.itemAt(i);
	}

	if(mBgBmp.bmBits != NULL) {
		UnloadBitmap(&mBgBmp);
	}
	if(mVerticalLine.bmBits != NULL) {
		UnloadBitmap(&mVerticalLine);
	}
	if(mLightBg.bmBits != NULL) {
		UnloadBitmap(&mLightBg);
	}
	if(mMlBgPic.bmBits != NULL) {
		UnloadBitmap(&mMlBgPic);
	}
	if(mMlLightBg.bmBits != NULL) {
		UnloadBitmap(&mMlLightBg);
	}
	if(mHorizontalLine.bmBits != NULL) {
		UnloadBitmap(&mHorizontalLine);
	}
}

int Menu::createMenuBar(HWND hWnd)
{
	int retval, i;
	CDR_RECT iconRect, mbRect, menuRect;
	HWND retWnd;
	ResourceManager* rm;
	PicObj *po;
	int current;
	
	rm = ResourceManager::getInstance();
	rm->getResRect(ID_MENU, menuRect);
	rm->getResRect(ID_MENUBAR, mbRect);

	for (i = 0; i < MENUBAR_MAX_COUNT; i++) {
		po = mIconPic.itemAt(i);
		po->mParent = hWnd;
		current = rm->getResIconCurrent(po->getId());
		if (current >= 0) 
			po->set(current);
		retval = rm->getResRect(po->getId(), iconRect);
		if(retval < 0) {
			db_error("get %s failed\n", po->mName.string());
			return -1;
		}
		retval = rm->getResBmp(po->getId(), BMPTYPE_BASE, *po->getImage());
		if(retval < 0) {
			db_error("loadb bitmap of %s failed\n", po->mName.string());
		}

		retWnd = CreateWindowEx(CTRL_STATIC, "",
				WS_CHILD | SS_BITMAP | SS_CENTERIMAGE | SS_REALSIZEIMAGE,
				WS_EX_TRANSPARENT,
				po->getId(),
				iconRect.x, iconRect.y-menuRect.y, iconRect.w, iconRect.h,
				hWnd, (DWORD)(po->getImage()));
		if(retWnd == HWND_INVALID) {
			db_error("create num %d icon failed\n", i);
			return -1;
		}
	}

	return 0;
}

int Menu::createMenulists(HWND hWnd, enum ResourceID* pResID, int count)
{
	HWND retWnd;
	int contentCount;
	CDR_RECT iconRect, mbRect, menuRect;
	CDR_RECT tmpRect;
	int itemHeight;
	menuListAttr_t menuListAttr;
	ResourceManager* rm = ResourceManager::getInstance();
	
	rm->getResRect(ID_MENU, menuRect);
	rm->getResRect(ID_MENUBAR, mbRect);
	
	itemHeight = rm->getResIntValue(ID_MENU_LIST_ITEM_HEIGHT, INTVAL_ITEMHEIGHT);
	if (itemHeight<0) {
		db_error("err itemheight\n");
		itemHeight = 38;
	}

	for (int i = 0; i < count; i++) {
		getMenuListAttr(menuListAttr, pResID[i]);
		rm->getResRect(icon_win_pic[i].id, iconRect);
		contentCount = getMenuContentCount(pResID[i]);
		tmpRect.x = iconRect.x-1;
		tmpRect.w = iconRect.w+2;
		tmpRect.h = (contentCount+1)*itemHeight;
		tmpRect.y = menuRect.h-mbRect.h-tmpRect.h;
		if (tmpRect.y < 0) {
			tmpRect.h = 216;
			tmpRect.y = 230;
		}
		retWnd = CreateWindowEx(CTRL_CDRMenuList, "",
				WS_CHILD | WS_VSCROLL | LBS_SBNEVER | LBS_USEBITMAP | LBS_SCENTER,
				WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
				pResID[i],
				tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
				hWnd, (DWORD)&menuListAttr);
		if(retWnd == HWND_INVALID) {
			db_error("create num %d menulist failed\n", i);
			return -1;
		}
		createMenuListContents(pResID[i], retWnd);
	}

	return 0;
}

int Menu::createSubWidgets(HWND hWnd)
{
	CDR_RECT menuRect;
	HWND retWnd;
	ResourceManager* rm;

	//reserve
	rm = ResourceManager::getInstance();
	rm->getResRect(ID_MENU, menuRect);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE,
			WS_EX_TRANSPARENT,
			ID_MENUBAR,
			0, 0, menuRect.w, menuRect.h,
			hWnd, 0);
	if(retWnd == HWND_INVALID) {
		db_error("create menu reserve window failed\n");
		return -1;
	}

	createMenuBar(hWnd);
	createMenulists(hWnd, rpMenuResourceID, RP_ICON_COUNT);
	createMenulists(hWnd, ppMenuResourceID, PP_ICON_COUNT);

	CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_TRANSPARENT,
			ID_MENUTMX_QR,
			0, 0, 960, 540,
			hWnd, 0);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE | SS_VCENTER | SS_CENTER,
			WS_EX_TRANSPARENT,
			ID_MENUTMX_SNO,
			40, 220, 500, 90,
			hWnd, 0);
	mLogFont = CreateLogFont("*", "fixed", "GB2312-0",
			FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
			FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 38, 0);
	SetWindowFont(retWnd, mLogFont);

	//SetWindowElementAttr(retWnd, WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, 0xFFFFFFFF));

	return 0;
}

void Menu::showTMXMessage(void)
{
	int ret;
	String8 bmpFile;
	HWND retWnd;

	if ((ret = LoadBitmapFromFile(HDC_SCREEN, &tmxImage, "system/etc/tmximage.png")) != ERR_BMP_OK) {
		db_error("load %s failed, ret is %d\n", "system/etc/tmximage.png", ret);
		return;
	}

	retWnd = GetDlgItem(mHwnd, ID_MENUTMX_QR);
	SendMessage(retWnd, STM_SETIMAGE, (WPARAM)&tmxImage, 0);
	ShowWindow(GetDlgItem(mHwnd, ID_MENUTMX_QR), SW_SHOWNORMAL);

	showTMXState = 1;
}

void Menu::showTMXNextMessage(void)
{
	int ret;
	String8 bmpFile;
	HWND retWnd;
	char *imei;
	char buf[60];

	ShowWindow(GetDlgItem(mHwnd, ID_MENUTMX_QR), SW_HIDE);

	if ((ret = LoadBitmapFromFile(HDC_SCREEN, &tmxImage, "system/etc/tmximage2.png")) != ERR_BMP_OK) {
		db_error("load %s failed, ret is %d\n", "system/etc/tmximage2.png", ret);
		return;
	}

	retWnd = GetDlgItem(mHwnd, ID_MENUTMX_QR);
	SendMessage(retWnd, STM_SETIMAGE, (WPARAM)&tmxImage, 0);
	ShowWindow(GetDlgItem(mHwnd, ID_MENUTMX_QR), SW_SHOWNORMAL);

	imei = getImei();
	sprintf(buf, "IMEI: %s", imei);
	SetWindowText(GetDlgItem(mHwnd, ID_MENUTMX_SNO), buf);
	ShowWindow(GetDlgItem(mHwnd, ID_MENUTMX_SNO), SW_SHOWNORMAL);
	showTMXState = 2;
}

void Menu::hideTMXMessage(void)
{
	ShowWindow(GetDlgItem(mHwnd, ID_MENUTMX_QR), SW_HIDE);
	ShowWindow(GetDlgItem(mHwnd, ID_MENUTMX_SNO), SW_HIDE);
	showTMXState = 0;
}

int Menu::getMenuContentCount(enum ResourceID resID)
{
	ResourceManager* rm;
	int contentCount;

	rm = ResourceManager::getInstance();
	if (resID == ID_MENU_LIST_VIDEO_SETTING)
		contentCount = RP_SETTING_MENU_COUNT;
	else if (resID == ID_MENU_LIST_PHOTO_SETTING)
		contentCount = PP_SETTING_MENU_COUNT;
	else
		contentCount = rm->getResIntValue(resID, INTVAL_SUBMENU_COUNT);

	return contentCount;
}

int Menu::getMenuListAttr(menuListAttr_t &attr, enum ResourceID resID)
{
	ResourceManager* rm = ResourceManager::getInstance();
	int itemHeight = rm->getResIntValue(ID_MENU_LIST_ITEM_HEIGHT, INTVAL_ITEMHEIGHT);
	if (itemHeight<0) {
		db_error("err itemheight\n");
		itemHeight = 38;
	}

	attr.normalBgc = rm->getResColor(ID_MENU_LIST, COLOR_BGC);
	attr.normalFgc = rm->getResColor(ID_MENU_LIST, COLOR_FGC);
	attr.linec = rm->getResColor(ID_MENU_LIST, COLOR_LINEC_ITEM);
	attr.normalStringc = rm->getResColor(ID_MENU_LIST, COLOR_STRINGC_NORMAL);
	attr.normalValuec = rm->getResColor(ID_MENU_LIST, COLOR_VALUEC_NORMAL);
	attr.selectedStringc = rm->getResColor(ID_MENU_LIST, COLOR_STRINGC_SELECTED);
	attr.selectedValuec = rm->getResColor(ID_MENU_LIST, COLOR_VALUEC_SELECTED);
	attr.scrollbarc = rm->getResColor(ID_MENU_LIST, COLOR_SCROLLBARC);
	attr.itemHeight = itemHeight;
	attr.mlBgpic = &mMlBgPic;
	attr.mlLightBgPic = &mMlLightBg;
	attr.mlLinePic = &mHorizontalLine;
	attr.mlSelectIcon = &mMlSelectIcon;
	attr.title = (char *)(rm->getResMenuTitle(resID));
	
	return 0;
}

int Menu::getItemStrings(enum ResourceID resID, MENULISTITEMINFO* mlii)
{
	int i;
	ResourceManager* rm;
	int content0Cmd;
	int contentCount;
	int iSelect;

	rm = ResourceManager::getInstance();

	if (resID == ID_MENU_LIST_VIDEO_SETTING) {		
		for(i = 0; i < RP_SETTING_MENU_COUNT; i++) {
			mlii[i].string = (char*)rm->getResMenuItemString(rpMenuResourceID[RP_ICON_COUNT+i]);
			if(mlii[i].string == NULL) {
				db_error("get the %d label string failed\n", i);
				return -1;
			}
		}
	}
	else if (resID == ID_MENU_LIST_PHOTO_SETTING) {
		for(i = 0; i < PP_SETTING_MENU_COUNT; i++) {
			mlii[i].string = (char*)rm->getResMenuItemString(ppMenuResourceID[PP_ICON_COUNT+i]);
			if(mlii[i].string == NULL) {
				db_error("get the %d label string failed\n", i);
				return -1;
			}
		}
	}
	else {
		
		/* current submenu index */
		iSelect = rm->getResIntValue(resID, INTVAL_SUBMENU_INDEX );
		if(iSelect < 0) {
			db_error("get res submenu index failed\n");
			return -1;
		}

		content0Cmd = rm->getResMenuContent0Cmd(resID);
		contentCount = rm->getResIntValue(resID, INTVAL_SUBMENU_COUNT);
		for(i = 0; i < contentCount; i++) {
			mlii[i].string = (char*)rm->getLabel(content0Cmd + i);
			if(mlii[i].string == NULL) {
				db_error("get the %d label string failed\n", i);
				return -1;
			}
			if(i == iSelect) {
				/* set the current item chioce image */
				mlii[i].flagsEX.imageFlag = IMGFLAG_IMAGE;
				mlii[i].hBmpImage = (DWORD)&mMlSelectIcon;
			}
		}
	}

	return 0;
}

int Menu::createMenuListContents(enum ResourceID resID, HWND hMenuList)
{
	int contentCount;
	MENULISTITEMINFO menuListII[MENU_LIST_MAX_COUNT];
	const char* ptr;
	int iSelect;
	ResourceManager* rm = ResourceManager::getInstance();
	int itemHeight = rm->getResIntValue(ID_MENU_LIST_ITEM_HEIGHT, INTVAL_ITEMHEIGHT);
	if (itemHeight<0) {
		db_error("err itemheight\n");
		itemHeight = 38;
	}

	memset(&menuListII, 0, sizeof(menuListII));

	if(getItemStrings(resID, menuListII) < 0) {
		db_error("get item strings failed\n");
		return -1;
	}
	
	contentCount = getMenuContentCount(resID);		
	SendMessage(hMenuList, LB_SETITEMHEIGHT, 0, itemHeight);	/* set the item height */
	SendMessage(hMenuList, LB_MULTIADDITEM, contentCount, (LPARAM)menuListII);
	SendMessage(hMenuList, MLM_HILIGHTED_SPACE, 1, 1);

	if (resID == ID_MENU_LIST_PHOTO_SETTING || resID == ID_MENU_LIST_VIDEO_SETTING)
		return 0;
		
	iSelect = rm->getResIntValue(resID, INTVAL_SUBMENU_INDEX);
	if (iSelect < 0) {
		db_error("get res submenu index failed\n");
		return -1;
	}
	SendMessage(hMenuList, LB_SETSELECT, 0, iSelect);

	return 0;
}

int updateNewSelected(HWND hMenuList, int oldSel, int newSel, PBITMAP pSelectIcon)
{
	MENULISTITEMINFO mlii;
	ResourceManager* rm;
	int retval;

	db_msg("xxxxxoldSel is %d, newSel is %d\n", oldSel, newSel);

	if(oldSel == newSel)
		return 0;

	rm = ResourceManager::getInstance();
	if(oldSel >= 0) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, oldSel, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed\n");
			return -1;
		}
		mlii.flagsEX.imageFlag = 0;
		mlii.hBmpImage = 0;

		SendMessage(hMenuList, LB_SETITEMDATA, oldSel, (LPARAM)&mlii);
	}

	if(newSel >= 0) {
		if(SendMessage(hMenuList, LB_GETITEMDATA, newSel, (LPARAM)&mlii) != LB_OKAY) {
			db_error("get item info failed\n");
			return -1;
		}
		mlii.flagsEX.imageFlag = IMGFLAG_IMAGE;
		mlii.hBmpImage = (DWORD)pSelectIcon;
		
		SendMessage(hMenuList, LB_SETITEMDATA, newSel, (LPARAM)&mlii);
	}

	return 0;
}

int Menu::getSubMenuData(enum ResourceID resID, subMenuData_t &subMenuData)
{
	int retval;
	int contentCount;
	int content0Cmd;
	const char* ptr;
	ResourceManager* rm;
	CDR_RECT iconRect, screenRect, mbRect, menuRect;
	int itemHeight;

	rm = ResourceManager::getInstance();

	getMenuListAttr(subMenuData.menuListAttr, resID);
	subMenuData.pLogFont = rm->getLogFont();
	subMenuData.id = resID;

	switch(resID) {
	case ID_MENU_LIST_AWMD:
	case ID_MENU_LIST_POR:
	case ID_MENU_LIST_GPS:
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
#endif
	case ID_MENU_LIST_SILENTMODE:
	case ID_MENU_LIST_WIFI:
	case ID_MENU_LIST_TWM:
		{
			bool curStatus;
			curStatus = rm->getResBoolValue(resID);
			subMenuData.selectedIndex = ( (curStatus == true) ? 1 : 0 );
			contentCount = 2;
			break;
		}
	default:
		retval = rm->getResIntValue(resID, INTVAL_SUBMENU_INDEX );
		if(retval < 0) {
			db_error("get res submenu index failed\n");
			return -1;
		}
		subMenuData.selectedIndex = retval;
		contentCount = rm->getResIntValue(resID, INTVAL_SUBMENU_COUNT);
		break;
	}

	content0Cmd = rm->getResMenuContent0Cmd(resID);

	/* submenu contents */
	for(int i = 0; i < contentCount; i++) {
		ptr = rm->getLabel(content0Cmd + i);
		if(ptr == NULL) {
			db_error("get res%d submenu content %d failed", resID, i);
			return -1;
		}
		subMenuData.contents.add(String8(ptr));
	}

	/* get submenu rect */
	itemHeight = rm->getResIntValue(ID_MENU_LIST_ITEM_HEIGHT, INTVAL_ITEMHEIGHT);
	if (itemHeight<0) {
		db_error("err itemheight\n");
		itemHeight = 38;
	}
	rm->getResRect(ID_SCREEN, screenRect);
	rm->getResRect(ID_MENU, menuRect);
	rm->getResRect(ID_MENUBAR, mbRect);
	rm->getResRect(icon_win_pic[mCurActiveIcon].id, iconRect);
	subMenuData.rect.x = iconRect.x - 1;
	subMenuData.rect.w = iconRect.w + 2;
	subMenuData.rect.h = (contentCount+1)*itemHeight;
	if (subMenuData.rect.h > (menuRect.h-mbRect.h))
		subMenuData.rect.h = menuRect.h-mbRect.h;
	subMenuData.rect.y = screenRect.h-mbRect.h-subMenuData.rect.h;

	db_error("------y=%d-------h=%d------------------------\n", subMenuData.rect.y, subMenuData.rect.h);

	subMenuData.pBmpChoice = &mMlSelectIcon;

	return 0;
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
#ifdef USE_NEWUI
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_BACKGROUND, configData.bgPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_CONFIRM, configData.btnSelectPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_CANCEL, configData.btnUnselectPic);
#endif
	configData.rm = rm;
	configData.Cdr = mCdrMain;

	retval = apSettingDialog(mHwnd, &configData);

#ifdef USE_NEWUI
	rm->unloadBitMap(configData.bgPic);
	rm->unloadBitMap(configData.btnSelectPic);
	rm->unloadBitMap(configData.btnUnselectPic);
#endif
	
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

	retval = dateSettingDialog(mHwnd, &configData);
	return retval;
}

int Menu::getMessageBoxData(enum ResourceID resID, MessageBox_t &messageBoxData)
{
	const char* ptr;
	unsigned int i;
	ResourceManager* rm;

	unsigned int indexTable [] = {
		ID_MENU_LIST_FORMAT,
		ID_MENU_LIST_FRESET
	};

	unsigned int resCmd[][2] = {
		{LANG_LABEL_SUBMENU_FORMAT_TITLE, LANG_LABEL_SUBMENU_FORMAT_TEXT},
		{LANG_LABEL_SUBMENU_FRESET_TITLE, LANG_LABEL_SUBMENU_FRESET_TEXT},
	};


	rm = ResourceManager::getInstance();

	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;
	messageBoxData.flag_end_key = 1; /* end the dialog when endkey keyup */

	for(i = 0; i < TABLESIZE(indexTable); i++) {
		if(indexTable[i] == resID) {
			ptr = rm->getLabel(resCmd[i][0]);
			if(ptr == NULL) {
				db_error("get title failed, resIDis %d\n", resID);
				return -1;
			}
			messageBoxData.title = ptr;

			ptr = rm->getLabel(resCmd[i][1]);
			if(ptr == NULL) {
				db_error("get text failed, resID is %d\n", resID);
				return -1;
			}
			messageBoxData.text = ptr;

			ptr = rm->getLabel(LANG_LABEL_SUBMENU_OK);
			if(ptr == NULL) {
				db_error("get ok string failed, resID is %d\n", resID);
				return -1;
			}
			messageBoxData.buttonStr[0] = ptr;

			ptr = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
			if(ptr == NULL) {
				db_error("get ok string failed, resID is %d\n", resID);
				return -1;
			}
			messageBoxData.buttonStr[1] = ptr;
		}
		db_msg("xxxxxxxx\n");
	}

	if(i > TABLESIZE(indexTable)) {
		db_error("invalid resID: %d\n", resID);
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

int Menu::ShowMessageBox(enum ResourceID resID)
{
	int retval;
	MessageBox_t messageBoxData;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	memset(&messageBoxData, 0, sizeof(messageBoxData));
	if(getMessageBoxData(resID, messageBoxData) < 0) {
		db_error("get messagebox data failed\n");
		return -1;
	}

	retval = showMessageBox(mHwnd, &messageBoxData);

	db_msg("retval is %d\n", retval);
	return retval;
}

int Menu::updateMenuTextsWithVQ(int oldSel, int newSel)
{
	int itemCount = 0;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[MENU_LIST_MAX_COUNT];
	ResourceManager* rm = ResourceManager::getInstance();
	char *ptr;
	int iSelect;

	db_msg("updateMenuTextsWithVQ ");
	hMenuList = GetDlgItem(mHwnd, ID_MENU_LIST_VQ);
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get resID %d menu counts failed\n", ID_MENU_LIST_VQ);
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	if(getItemStrings(ID_MENU_LIST_VQ, menuListII) < 0) {
		db_error("get resID %d item strings failed\n", ID_MENU_LIST_VQ);
		return -1;
	}

	menuListII[oldSel].hBmpImage = 0;
	menuListII[oldSel].flagsEX.imageFlag = 0;
	
	for(int count = 0; count < itemCount; count++) {
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
	}
	
	ptr = (char *)(rm->getResMenuTitle(ID_MENU_LIST_VQ));
	SendMessage(hMenuList, LB_SETTITLE, 0, (LPARAM)ptr);
	
	iSelect = rm->getResIntValue(ID_MENU_LIST_VQ, INTVAL_SUBMENU_INDEX);
	if (iSelect < 0) {
		db_error("get resID %d submenu index failed\n", ID_MENU_LIST_VQ);
		return -1;
	}
	SendMessage(hMenuList, LB_SETSELECT, 0, iSelect);

	return 0;
	
}


int Menu::updateMenuTextsWithVTL(int newSel)
{
	int itemCount = 0;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[MENU_LIST_MAX_COUNT];
	ResourceManager* rm = ResourceManager::getInstance();
	char *ptr;
	int iSelect;

	db_msg("updateMenuTextsWithVTL ");
	hMenuList = GetDlgItem(mHwnd, ID_MENU_LIST_VTL);
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get resID %d menu counts failed\n", ID_MENU_LIST_VTL);
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	if(getItemStrings(ID_MENU_LIST_VTL, menuListII) < 0) {
		db_error("get resID %d item strings failed\n", ID_MENU_LIST_VTL);
		return -1;
	}

	for(int i=0; i < itemCount; i++)
	{
		if(i != newSel)
		{
			menuListII[i].hBmpImage = 0;
			menuListII[i].flagsEX.imageFlag = 0;
		}
	}
	
	
	for(int count = 0; count < itemCount; count++) {
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
	}
	
	ptr = (char *)(rm->getResMenuTitle(ID_MENU_LIST_VTL));
	SendMessage(hMenuList, LB_SETTITLE, 0, (LPARAM)ptr);
	
	iSelect = rm->getResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX);
	if (iSelect < 0) {
		db_error("get resID %d submenu index failed\n", ID_MENU_LIST_VTL);
		return -1;
	}
	SendMessage(hMenuList, LB_SETSELECT, 0, iSelect);

	return 0;
	
}



int Menu::updateMenuTextsWithVideoSettings(int newSel, enum ResourceID resID)
{
	int itemCount = 0;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[MENU_LIST_MAX_COUNT];
	ResourceManager* rm = ResourceManager::getInstance();
	char *ptr;
	int iSelect;

	db_msg("updateMenuTextsWithVideoSettings ");
	hMenuList = GetDlgItem(mHwnd, resID);
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get resID %d menu counts failed\n", resID);
		return -1;
	}

	memset(menuListII, 0, sizeof(menuListII));
	for(int count = 0; count < itemCount; count++) {
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
	}

	if(getItemStrings(resID, menuListII) < 0) {
		db_error("get resID %d item strings failed\n", resID);
		return -1;
	}


	for(int count = 0; count < itemCount; count++) {
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
	}
	
	ptr = (char *)(rm->getResMenuTitle(resID));
	SendMessage(hMenuList, LB_SETTITLE, 0, (LPARAM)ptr);
	
	iSelect = rm->getResIntValue(resID, INTVAL_SUBMENU_INDEX);
	if (iSelect < 0) {
		db_error("get resID %d submenu index failed\n", resID);
		return -1;
	}
	SendMessage(hMenuList, LB_SETSELECT, 0, iSelect);

	return 0;
	
}



int Menu::updateMenuTexts(enum ResourceID *pMlResourceID, int mlCount)
{
	int itemCount = 0;
	HWND hMenuList;
	MENULISTITEMINFO menuListII[MENU_LIST_MAX_COUNT];
	ResourceManager* rm = ResourceManager::getInstance();
	char *ptr;

	for (int i=0; i<mlCount; i++) {
		hMenuList = GetDlgItem(mHwnd, pMlResourceID[i]);
		itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
		if(itemCount < 0) {
			db_error("get resID %d menu counts failed\n", pMlResourceID[i]);
			return -1;
		}

		memset(menuListII, 0, sizeof(menuListII));
		for(int count = 0; count < itemCount; count++) {
			if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&menuListII[count]) != LB_OKAY) {
				db_msg("get the %d item data failed\n", count);
				return -1;
			}
		}

		if(getItemStrings(pMlResourceID[i], menuListII) < 0) {
			db_error("get resID %d item strings failed\n", pMlResourceID[i]);
			return -1;
		}
		
		for(int count = 0; count < itemCount; count++) {
			SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&menuListII[count] );
		}
		
		ptr = (char *)(rm->getResMenuTitle(pMlResourceID[i]));
		SendMessage(hMenuList, LB_SETTITLE, 0, (LPARAM)ptr);

		if (pMlResourceID[i] != ID_MENU_LIST_VIDEO_SETTING 
			&& pMlResourceID[i] != ID_MENU_LIST_PHOTO_SETTING) {
			int iSelect = rm->getResIntValue(pMlResourceID[i], INTVAL_SUBMENU_INDEX);
			if (iSelect < 0) {
				db_error("get resID %d submenu index failed\n", pMlResourceID[i]);
				return -1;
			}
			SendMessage(hMenuList, LB_SETSELECT, 0, iSelect);
		}
	}
	return 0;
}

void Menu::updateMenuListsTexts(void)
{	
	updateMenuTexts(rpMenuResourceID, RP_ICON_COUNT);
	updateMenuTexts(ppMenuResourceID, PP_ICON_COUNT);
}

void Menu::doFactoryReset(void)
{
	db_msg("ido factory reset\n");
	ResourceManager *rm = ResourceManager::getInstance();
	rm->resetResource();
	//resetDateTime();
	updateMenuListsTexts();
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

	retval = showTipLabel(mHwnd, &tipLabelData);

	return retval;
}

void Menu::handleWindowChanged(windowState_t *windowState)
{
	ResourceManager* rm;
	int size;
	int current;

	rm = ResourceManager::getInstance();
	switch (windowState->curWindowID) {
	case WINDOWID_RECORDPREVIEW:
		{
			RPWindowState_t state;
			state = windowState->RPWindowState;
			if(state == STATUS_RECORDPREVIEW) {
				current = rm->getResIntValue(ID_MENU_LIST_VQ, INTVAL_SUBMENU_INDEX);
				mCdrMain->notify(EVENT_VIDEO_PHOTO_VALUE, current+1);

				current = rm->getResIntValue(ID_MENU_LIST_VM, INTVAL_SUBMENU_INDEX);
				mCdrMain->notify(EVENT_VIDEO_MODE, current+1);
				
				mCurMenuWindow = MENU_RECORDPREVIEW;
				mCurIconCount = RP_ICON_COUNT;
				mIconPic.itemAt(MB_ICON_WIN1)->set(1);
				mIconPic.itemAt(MB_ICON_WIN1)->update();
			} else if(state == STATUS_PHOTOGRAPH){
				current = rm->getResIntValue(ID_MENU_LIST_PQ, INTVAL_SUBMENU_INDEX);
				mCdrMain->notify(EVENT_VIDEO_PHOTO_VALUE, current+3);
				mCdrMain->notify(EVENT_VIDEO_MODE, 0);
				
				mCurMenuWindow = MENU_PHOTOGRAPH;
				mCurIconCount = PP_ICON_COUNT;
				mIconPic.itemAt(MB_ICON_WIN1)->set(3);
				mIconPic.itemAt(MB_ICON_WIN1)->update();
			}
		}
		break;
	case WINDOWID_PLAYBACKPREVIEW:
		mCdrMain->notify(EVENT_VIDEO_MODE, 0);
		mCdrMain->notify(EVENT_VIDEO_PHOTO_VALUE, 0);
		mCurMenuWindow = MENU_PLAYBACKPREVIEW;
		break;
	default:
		break;
	}
}

int Menu::ShowSubMenu(HWND hMenuList, enum ResourceID resID)
{
	int oldSel, retval;
	subMenuData_t subMenuData;
	StorageManager *sm = StorageManager::getInstance();

	switch (resID) {
	case ID_MENU_LIST_FORMAT:
	case ID_MENU_LIST_FRESET:
		if(resID == ID_MENU_LIST_FORMAT) {
			if (!sm->isInsert()) {
				ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
				break;
			}
		}
		retval = ShowMessageBox(resID);
		if(retval < 0) {
			db_error("ShowMessageBox err, retval is %d\n", retval);
			break;
		}
		if(retval == IDC_BUTTON_OK) {
			if(resID == ID_MENU_LIST_FRESET) {
			#ifdef MESSAGE_FRAMEWORK
				CommuReq req;
				req.cmdType = COMMU_TYPE_FACTORY_RESET;
				req.operType = 1;
				db_msg("fwk_msg send factory reset \n");
				fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
			#else
				doFactoryReset();
				mCdrMain->reboot();
			#endif
			} else if(resID == ID_MENU_LIST_FORMAT) {
				if(sm->isInsert() == false) {
					db_msg("tfcard not mount\n");
					ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
					break;
				}
				ShowFormattingTip();
			}
		}
		break;
	case ID_MENU_LIST_DATE:
		ShowDateDialog();
		break;
	case ID_MENU_LIST_AP:
		ShowAPSettingDialog();
		break;
	case ID_MENU_LIST_FIRMWARE:
		showFirmwareDialog(mHwnd);
		break;
	case ID_MENU_LIST_ABOUT_TMX:
		showTMXMessage();
		break;
	default:
		if(getSubMenuData(resID, subMenuData) < 0) {
			db_error("get submenu data failed\n");
			return -1;
		}
		oldSel = subMenuData.selectedIndex;
		
		ShowWindow(hMenuList, SW_HIDE);
		showSubMenu(mHwnd, &subMenuData);
		ShowWindow(hMenuList, SW_SHOWNORMAL);
		SetFocusChild(hMenuList);
		
		break;
	}

	return 0;
}

int HandleSubMenuChange(enum ResourceID resID, int newSel)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	switch(resID) {
	case ID_MENU_LIST_AWMD:
	case ID_MENU_LIST_POR:
	case ID_MENU_LIST_GPS:
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
#endif
	case ID_MENU_LIST_SILENTMODE:
	case ID_MENU_LIST_WIFI:
	case ID_MENU_LIST_TWM:
		{
			bool newValue = newSel ? true : false;
			#ifdef MESSAGE_FRAMEWORK
			if ((resID == ID_MENU_LIST_GPS) || (resID == ID_MENU_LIST_CELLULAR) || (resID == ID_MENU_LIST_TWM) || (resID == ID_MENU_LIST_AWMD))
			{
				if(resID == ID_MENU_LIST_TWM)
				{
					CommuReq req;
					req.cmdType = COMMU_TYPE_WATERMARK;
					req.operType = newValue;
					db_msg("fwk_msg send TimeWaterMark on \n");
					fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
				}
				else if(resID == ID_MENU_LIST_AWMD)
				{
					CommuReq req;
					req.cmdType = COMMU_TYPE_MOVING;
					req.operType = newValue;
					db_msg("fwk_msg send AWMD on \n");
					fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
				}
			}
			else
			{
				if(rm->setResBoolValue(resID, newValue) < 0) {
					db_error("set %d to %d failed\n", resID, newValue);
					return -1;
				}
			}
			#else
			if(rm->setResBoolValue(resID, newValue) < 0) {
				db_error("set %d to %d failed\n", resID, newValue);
				return -1;
			}
			#endif
			if(resID == ID_MENU_LIST_GPS) {
				if(newValue) {
				#ifdef MESSAGE_FRAMEWORK
					{
						CommuReq req;
						req.cmdType = COMMU_TYPE_GPS;
						req.operType = 1;
						db_msg("fwk_msg send gps on \n");
						fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
					}
				#else
					enableGps();
				#endif
				} else {
				#ifdef MESSAGE_FRAMEWORK
					{
						CommuReq req;
						req.cmdType = COMMU_TYPE_GPS;
						req.operType = 0;
						db_msg("fwk_msg send gps off \n");
						fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
					}
				#else
					disableGps();
				#endif
				}
			}
#ifdef MODEM_ENABLE
			if(resID == ID_MENU_LIST_CELLULAR) {
				if(newValue) {
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
			
			if(resID == ID_MENU_LIST_WIFI) {
				if(newValue) {
					if (isSoftapEnabled()) {
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
					enable_wifi();
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
					disable_wifi();
				#endif
				}
			}		
		}
		break;
	default:
		#ifdef MESSAGE_FRAMEWORK
		if(resID == ID_MENU_LIST_VQ)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_RESOLUTION;
			db_debug("VQ newSel=%d", newSel);
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
		else if(resID == ID_MENU_LIST_VTL)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_RECORD_TIME;
			db_debug("VTL newSel=%d", newSel);
			req.value = (newSel+1)*60;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(resID == ID_MENU_LIST_EXPOSURE)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_EXPOSURE;
			db_debug("Exposure newSel=%d", newSel);
			req.value = newSel-3;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(resID == ID_MENU_LIST_WB)
		{
			ValueReq req;

			memset(&req, 0x0, sizeof(req));
			req.cmdType = VALUE_TYPE_WB;
			db_debug("WhiteBalance newSel=%d", newSel);
			req.value = newSel;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_VALUE_REQ, &req, sizeof(req));
		}
		else if(resID == ID_MENU_LIST_CONTRAST)
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
			if(rm->setResIntValue(resID, INTVAL_SUBMENU_INDEX, newSel) < 0) {
				db_error("set %d to %d failed\n", resID, newSel);	
				return -1;
			}
		}
		#else
		if(rm->setResIntValue(resID, INTVAL_SUBMENU_INDEX, newSel) < 0) {
			db_error("set %d to %d failed\n", resID, newSel);	
			return -1;
		}
		#endif
		break;
	}

	return 0;
}

int Menu::keyProc(int keyCode, int isLongPress)
{
	if (showTMXState == 2) {
		hideTMXMessage();
		return WINDOWID_MENU;
	}

	switch(keyCode) {
	case CDR_KEY_MODE:
		{
			ResourceManager* rm;	
			RECT rect;
			int oldActiveIcon;
			int current;

			if (showTMXState) {
				hideTMXMessage();
				return WINDOWID_MENU;
			}

			oldActiveIcon = mCurActiveIcon;			
			current = mIconPic.itemAt(oldActiveIcon)->get();
			mIconPic.itemAt(oldActiveIcon)->set(current+1);
			mIconPic.itemAt(oldActiveIcon)->update();
			mCurActiveIcon++;
			if (mCurMenuWindow == MENU_RECORDPREVIEW) {
				ShowWindow(GetDlgItem(mHwnd, rpMenuResourceID[oldActiveIcon]), SW_HIDE);
				if (mCurActiveIcon > RP_ICON_COUNT-1) {
					mCurActiveIcon = MB_ICON_WIN0;
				}
			}
			else if (mCurMenuWindow == MENU_PHOTOGRAPH) {
				ShowWindow(GetDlgItem(mHwnd, ppMenuResourceID[oldActiveIcon]), SW_HIDE);
				if (mCurActiveIcon > PP_ICON_COUNT-1) {
					mCurActiveIcon = MB_ICON_WIN0;
				}
			}
			current = mIconPic.itemAt(mCurActiveIcon)->get();
			mIconPic.itemAt(mCurActiveIcon)->set(current-1);
			mIconPic.itemAt(mCurActiveIcon)->update();
			if (mCurActiveIcon == MB_ICON_WIN0) {
				rm = ResourceManager::getInstance();
				rm->syncConfigureToDisk();
				for (unsigned int i=0; i<mCurIconCount; i++)
					ShowWindow(GetDlgItem(mHwnd, icon_win_pic[i].id), SW_HIDE);
				if (mCurMenuWindow == MENU_PHOTOGRAPH 
					|| mCurMenuWindow == MENU_RECORDPREVIEW)
					return WINDOWID_RECORDPREVIEW;
				else
					return WINDOWID_PLAYBACKPREVIEW;
			} 
			if (mCurMenuWindow == MENU_RECORDPREVIEW) {
				SendMessage(GetDlgItem(mHwnd, rpMenuResourceID[mCurActiveIcon]), LB_SETCURSEL, 0, 0);
				ShowWindow(GetDlgItem(mHwnd, rpMenuResourceID[mCurActiveIcon]), SW_SHOWNORMAL);
				SetFocusChild(GetDlgItem(mHwnd, rpMenuResourceID[mCurActiveIcon]));
			}
			else if (mCurMenuWindow == MENU_PHOTOGRAPH) {
				SendMessage(GetDlgItem(mHwnd, ppMenuResourceID[mCurActiveIcon]), LB_SETCURSEL, 0, 0);
				ShowWindow(GetDlgItem(mHwnd, ppMenuResourceID[mCurActiveIcon]), SW_SHOWNORMAL);
				SetFocusChild(GetDlgItem(mHwnd, ppMenuResourceID[mCurActiveIcon]));
			}
		}
		break;
	case CDR_KEY_OK:
		{
			HWND hMenuList;
			int oldSel, hlItem;
			enum ResourceID resID;
			ResourceManager *rm;
			bool isModified;

			if (showTMXState == 1) {
				showTMXNextMessage();
				return WINDOWID_MENU;
			}

			hMenuList = GetFocusChild(mHwnd);
			resID = (enum ResourceID)GetDlgCtrlID(hMenuList);
			hlItem = SendMessage(hMenuList, LB_GETCURSEL, 0, 0);
			if(isLongPress == SHORT_PRESS) {
				if (resID == ID_MENU_LIST_VIDEO_SETTING) {
					ShowSubMenu(hMenuList, rpMenuResourceID[RP_ICON_COUNT+hlItem]);
				}
				else if (resID == ID_MENU_LIST_PHOTO_SETTING) {
					ShowSubMenu(hMenuList, ppMenuResourceID[PP_ICON_COUNT+hlItem]);
				}
				else {
					oldSel = SendMessage(hMenuList, LB_GETSELECT, 0, 0);
					if (oldSel != hlItem) {
						updateNewSelected(hMenuList, oldSel, hlItem, &mMlSelectIcon);
						SendMessage(hMenuList, LB_SETSELECT, 0, hlItem);
						HandleSubMenuChange(resID, hlItem);
					}
				}
			}
			else {
                db_msg("LONG_PRESS:selectedItem is %d\n", hlItem);
				if (resID == ID_MENU_LIST_VIDEO_SETTING) {
					if (rpMenuResourceID[RP_ICON_COUNT+hlItem] == ID_MENU_LIST_WIFI)
						if (check_if_wifi_enabled())
							startWifiWpsPBC();
				}
				else if (resID == ID_MENU_LIST_PHOTO_SETTING) {
					if (ppMenuResourceID[PP_ICON_COUNT+hlItem] == ID_MENU_LIST_WIFI)
						if (check_if_wifi_enabled())
							startWifiWpsPBC();
				}
			}
		}
		break;
	default:
		if (showTMXState) {
			hideTMXMessage();
			return WINDOWID_MENU;
		}
		break;
	}

	return WINDOWID_MENU;
}

void Menu::updateActiveWindowFont()
{
	SetWindowFont(GetActiveWindow(), GetWindowFont(mHwnd));
}

PBITMAP Menu::getBgBmp()
{
	return &mBgBmp;
}

PBITMAP Menu::getLightBgBmp()
{
	return &mLightBg;
}

PBITMAP Menu::getVerticalLineBmp()
{
	return &mVerticalLine;
}

PBITMAP Menu::getMlBgBmp()
{
	return &mMlBgPic;
}

PBITMAP Menu::getHorizontalLineBmp()
{
	return &mHorizontalLine;
}

PBITMAP Menu::getMlLightBmp()
{
	return &mMlLightBg;
}

int Menu::getCurActiveIcon()
{
	return mCurActiveIcon;
}

int Menu::getCurIconCount()
{
	return mCurIconCount;
}

MenuWindow_t Menu::getCurMenuWindow()
{
	return mCurMenuWindow;
}

