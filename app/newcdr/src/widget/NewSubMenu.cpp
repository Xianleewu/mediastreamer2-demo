/*************************************************************************
  > File Name: src/widget/DlgSubMenu.c
  > Description: 
  > Author: CaoDan
  > Mail: caodan2519@gmail.com 
  > Created Time: 2014年04月23日 星期三 08:48:31
 ************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
#include <minigui/ctrl/listbox.h>

#include <cutils/atomic.h>

#include "cdr_res.h"
#include "misc.h"
#include "cdr_widgets.h"
#include "keyEvent.h"
#include "cdr_message.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SubMenu.cpp"
#include "debug.h"

#define IDC_SUB_TITLE			410
#define IDC_SUB_LIST_BOX		411


static int updateSubMenuTexts(HWND hMenuList, enum ResourceID resID)
{
	int itemCount = 0;
	MENULISTITEMINFO mlii;
	int content0Cmd;
	char *ptr;
	
	ResourceManager* rm = ResourceManager::getInstance();

	// title
	ptr = (char *)(rm->getResMenuTitle(resID));
	SendMessage(hMenuList, LB_SETTITLE, 0, (LPARAM)ptr);

	/* submenu contents */
	itemCount = SendMessage(hMenuList, LB_GETCOUNT, 0, 0);
	if(itemCount < 0) {
		db_error("get resID %d menu counts failed\n", resID);
		return -1;
	}
	content0Cmd = rm->getResMenuContent0Cmd(resID);
	for(int count = 0; count < itemCount; count++) {
		memset(&mlii, 0, sizeof(mlii));
		if( SendMessage(hMenuList, LB_GETITEMDATA, count, (LPARAM)&mlii) != LB_OKAY) {
			db_msg("get the %d item data failed\n", count);
			return -1;
		}
		ptr = (char *)rm->getLabel(content0Cmd + count);
		mlii.string = ptr;
		SendMessage(hMenuList, LB_SETITEMDATA, count, (LPARAM)&mlii);
	}

	return 0;
}


static int SubMenuProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	subMenuData_t* subMenuData;
	HDC hdc;
	HWND hMenuList;
	hMenuList = GetDlgItem(hDlg, IDC_SUB_LIST_BOX);

	switch(message) {
	case MSG_INITDIALOG:
		{
			int count;
			MENULISTITEMINFO mlii;

			subMenuData = (subMenuData_t*)lParam;
			if(subMenuData == NULL) {
				db_error("invalid subMenuData\n");
				EndDialog(hDlg, -3);
			}

			SetWindowAdditionalData(hDlg, (DWORD)lParam);

			SetWindowFont(hDlg, subMenuData->pLogFont);
			SetWindowBkColor(hDlg, subMenuData->menuListAttr.normalBgc);
			//SetWindowBkColor(hDlg, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_SUB_TITLE), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, subMenuData->menuListAttr.normalFgc));

			/* set the MENULIST item Height */
			SendMessage(hMenuList, LB_SETITEMHEIGHT, 0, subMenuData->menuListAttr.itemHeight);	/* set the item height */
			SendMessage(hMenuList, MLM_HILIGHTED_SPACE, 1, 1);
			SetFocusChild(hMenuList);

			/* add item to sub menu list */
			count = subMenuData->contents.size();
			db_msg("count is %d\n", count);
			if(subMenuData->selectedIndex >= count || subMenuData->selectedIndex < 0) {
				db_msg("invalid menu current value\n");
				subMenuData->selectedIndex = 0;
			}

			for(int i = 0; i < count; i++) {
				memset(&mlii, 0, sizeof(mlii));
				mlii.string = (char*)subMenuData->contents.itemAt(i).string();
				if(i == subMenuData->selectedIndex) {
					/* set the current item chioce image */
					mlii.flagsEX.imageFlag = IMGFLAG_IMAGE;
					mlii.hBmpImage = (DWORD)subMenuData->pBmpChoice;
				}
				SendMessage(hMenuList, LB_ADDSTRING, 0, (LPARAM)&mlii);
			}

			/* set the current selected item */
			SendMessage(hMenuList, LB_SETCURSEL, 0, 0);
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;
			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			pLogFont = GetWindowFont(hDlg);
			if(pLogFont == NULL)
				break;
			db_msg("type: %s, family: %s, charset: %s\n", pLogFont->type, pLogFont->family, pLogFont->charset);
			SetWindowFont(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), GetWindowFont(hDlg));
			updateSubMenuTexts(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), (enum ResourceID)subMenuData->id);
		}
		break;
	case MSG_KEYUP:
		{
			int index;
			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			if(wParam == CDR_KEY_OK) {
				db_msg("press key ok\n");
				index = SendMessage(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), LB_GETCURSEL, 0, 0);
				if (index != subMenuData->selectedIndex) {
					updateNewSelected(hMenuList, subMenuData->selectedIndex, index, subMenuData->pBmpChoice);
					subMenuData->selectedIndex = index;
					HandleSubMenuChange((enum ResourceID)subMenuData->id, index);
					if ((enum ResourceID)subMenuData->id == ID_MENU_LIST_LANG)
						SendMessage(hDlg, MSG_FONTCHANGED, 0, 0);
				}
			} else if (wParam == CDR_KEY_POWER) {
#if 0
				int retval;
				native_sock_msg_t msg;	

				memset(&msg, 0, sizeof(native_sock_msg_t));
				msg.id = hDlg;	
				msg.messageid = MWM_CHANGE_SS;

				retval = send_sockmsg2server(SOCK_SERVER_NAME, &msg);
				if(retval == sizeof(native_sock_msg_t)) {
					db_msg("send message successd, retval is %d\n", retval);	
				} else {
					db_msg("send message failed, retval is %d\n", retval);	
				}
#endif
			} else if (wParam == CDR_KEY_MODE) {
				EndDialog(hDlg, subMenuData->selectedIndex);
			}
		}
		break;
	case MSG_KEYDOWN:
		{
#if 0
			int retval;
			native_sock_msg_t msg;	

			memset(&msg, 0, sizeof(native_sock_msg_t));
			msg.id = hDlg;	
			msg.messageid = MWM_WAKEUP_SS;

			retval = send_sockmsg2server(SOCK_SERVER_NAME, &msg);
			if(retval == sizeof(native_sock_msg_t)) {
				db_msg("send message successd, retval is %d\n", retval);	
			} else {
				db_msg("send message failed, retval is %d\n", retval);	
			}
#endif
		}
		break;
	case MSG_ERASEBKGND:
		break;
	case MSG_CLOSE:
		{
			EndDialog(hDlg, IDCANCEL);
		}
		break;
	default:
		break;
	}

	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

/* ********** hWnd 是Menu List的句柄 ************** 
 * 返回值是用户选中的item在ListBox中的序号
 * */
int showSubMenu(HWND hWnd, subMenuData_t* subMenuData)
{
	CDR_RECT rect;
	DLGTEMPLATE DlgSubMenu;
	CTRLDATA CtrlSubMenu;

	memset(&DlgSubMenu, 0, sizeof(DlgSubMenu));
	memset(&CtrlSubMenu, 0, sizeof(CtrlSubMenu));

	CtrlSubMenu.class_name = CTRL_CDRMenuList;
	CtrlSubMenu.dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_SBNEVER | LBS_USEBITMAP | LBS_SCENTER;
	CtrlSubMenu.dwExStyle = WS_EX_TRANSPARENT;
	CtrlSubMenu.caption = "";
	CtrlSubMenu.x = 0;
	CtrlSubMenu.y = 0;
	CtrlSubMenu.w = subMenuData->rect.w;
	CtrlSubMenu.h = subMenuData->rect.h;
	CtrlSubMenu.id = IDC_SUB_LIST_BOX;
	CtrlSubMenu.dwAddData = (DWORD)&subMenuData->menuListAttr;

	DlgSubMenu.dwStyle = WS_VISIBLE | WS_CHILD;
	DlgSubMenu.dwExStyle = WS_EX_TRANSPARENT | WS_EX_AUTOSECONDARYDC;
	DlgSubMenu.x = subMenuData->rect.x;
	DlgSubMenu.y = subMenuData->rect.y;
	DlgSubMenu.w = subMenuData->rect.w;
	DlgSubMenu.h = subMenuData->rect.h;
	DlgSubMenu.caption = "";
	DlgSubMenu.controlnr = 1;
	DlgSubMenu.controls = (PCTRLDATA)&CtrlSubMenu;

	return DialogBoxIndirectParam(&DlgSubMenu, hWnd, SubMenuProc, (DWORD)subMenuData);
}
