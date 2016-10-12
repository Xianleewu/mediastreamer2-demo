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

static int SubMenuProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	subMenuData_t* subMenuData;
	HDC hdc;

//	db_msg("message is %s\n", Message2Str(message));

	switch(message) {
	case MSG_INITDIALOG:
		{
			int count;
			HWND hMenuList;
			MENULISTITEMINFO mlii;

			subMenuData = (subMenuData_t*)lParam;
			if(subMenuData == NULL) {
				db_error("invalid subMenuData\n");
				EndDialog(hDlg, -3);
			}

			SetWindowAdditionalData(hDlg, (DWORD)lParam);
			hMenuList = GetDlgItem(hDlg, IDC_SUB_LIST_BOX);

			SetWindowFont(hDlg, subMenuData->pLogFont);
			SetWindowBkColor(hDlg, subMenuData->menuListAttr.normalBgc);
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_SUB_TITLE), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, subMenuData->menuListAttr.normalFgc));

			/* set the MENULIST item Height */
			SendMessage(hMenuList, LB_SETITEMHEIGHT, 0, 42);	/* set the item height */
			SendMessage(hMenuList, MLM_HILIGHTED_SPACE, 8, 8);
			SetFocusChild(hMenuList);

			/* set submenu title */
			SetWindowText(GetDlgItem(hDlg, IDC_SUB_TITLE), subMenuData->title.string());

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
					mlii.flagsEX.imageFlag = 0;
					mlii.flagsEX.valueCount = 1;			
					mlii.flagsEX.valueFlag[0] = VMFLAG_IMAGE;
					mlii.hValue[0] = (DWORD)&subMenuData->BmpChoice;
				}
				SendMessage(hMenuList, LB_ADDSTRING, 0, (LPARAM)&mlii);
			}

			/* set the current selected item */
			db_msg("current is %d\n", subMenuData->selectedIndex);
			SendMessage(hMenuList, LB_SETCURSEL, subMenuData->selectedIndex, 0);
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;

			pLogFont = GetWindowFont(hDlg);
			if(pLogFont == NULL)
				break;
			db_msg("type: %s, family: %s, charset: %s\n", pLogFont->type, pLogFont->family, pLogFont->charset);
			SetWindowFont(GetDlgItem(hDlg, IDC_SUB_TITLE), GetWindowFont(hDlg));
			SetWindowFont(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), GetWindowFont(hDlg));
		}
		break;
	case MSG_KEYUP:
		{
			int index;
			if(wParam == CDR_KEY_OK) {
				db_msg("press key ok\n");
				index = SendMessage(GetDlgItem(hDlg, IDC_SUB_LIST_BOX), LB_GETCURSEL, 0, 0);
				EndDialog(hDlg, index);
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
				EndDialog(hDlg, -2);
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
	case MSG_PAINT:
		{
			RECT rect;
			hdc = BeginPaint(hDlg);

			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			GetClientRect(GetDlgItem(hDlg, IDC_SUB_TITLE), &rect);
			SetPenColor(hdc, subMenuData->lincTitle);
			LineEx(hdc, 0, RECTH(rect) + 2, RECTW(rect), RECTH(rect) + 2);

			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_ERASEBKGND:
		break;
	case MSG_CLOSE:
		{
			subMenuData = (subMenuData_t*)GetWindowAdditionalData(hDlg);
			if(subMenuData->BmpChoice.bmBits) {
				UnloadBitmap(&subMenuData->BmpChoice);
			}

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
	CTRLDATA CtrlSubMenu[2];

	memset(&DlgSubMenu, 0, sizeof(DlgSubMenu));
	memset(CtrlSubMenu, 0, sizeof(CtrlSubMenu));

	rect = subMenuData->rect;

	CtrlSubMenu[0].class_name = CTRL_STATIC;
	CtrlSubMenu[0].dwStyle = WS_CHILD | WS_VISIBLE | SS_CENTER | SS_VCENTER;
	CtrlSubMenu[0].dwExStyle = WS_EX_TRANSPARENT;
	CtrlSubMenu[0].caption = "";
	CtrlSubMenu[0].x = 0;
	CtrlSubMenu[0].y = 0;
	CtrlSubMenu[0].w = rect.w;
	CtrlSubMenu[0].h = 40;
	CtrlSubMenu[0].id = IDC_SUB_TITLE;

	CtrlSubMenu[1].class_name = CTRL_CDRMenuList;
	CtrlSubMenu[1].dwStyle = WS_CHILD| WS_VISIBLE | WS_VSCROLL | LBS_USEBITMAP | LBS_HAVE_VALUE;
	CtrlSubMenu[1].dwExStyle = WS_EX_TRANSPARENT;
	CtrlSubMenu[1].caption = "";
	CtrlSubMenu[1].x = 0;
	CtrlSubMenu[1].y = 40;
	CtrlSubMenu[1].w = rect.w;
	CtrlSubMenu[1].h = rect.h - 40;
	CtrlSubMenu[1].id = IDC_SUB_LIST_BOX;
	CtrlSubMenu[1].dwAddData = (DWORD)&subMenuData->menuListAttr;

	DlgSubMenu.dwStyle = WS_VISIBLE | WS_CHILD;
	DlgSubMenu.dwExStyle = WS_EX_TRANSPARENT;
	DlgSubMenu.x = rect.x;
	DlgSubMenu.y = rect.y;
	DlgSubMenu.w = rect.w;
	DlgSubMenu.h = rect.h;
	DlgSubMenu.caption = "";
	DlgSubMenu.controlnr = 2;
	DlgSubMenu.controls = (PCTRLDATA)CtrlSubMenu;

	return DialogBoxIndirectParam(&DlgSubMenu, hWnd, SubMenuProc, (DWORD)subMenuData);
}
