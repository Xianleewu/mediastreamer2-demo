/*
 * Copyright (c) 2016 Fuzhou Rockchip Electronics Co.Ltd All rights reserved.
 * (wilen.gao@rock-chips.com)
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <network/hotspot.h>
#include <network/hotspotSetting.h>
#include <network/wifiIf.h>
#include <network/debug.h>

#ifdef MESSAGE_FRAMEWORK
#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"
#endif


#ifdef USE_NEWUI
#include "../../widget/include/ctrlclass.h"
extern int FillBoxBg(HDC hdc, RECT &rect, PBITMAP pBg);
static WNDPROC button_default_callback;

typedef struct tagBUTTONDATA
{
	DWORD status;           /* button flags */
	DWORD data;             /* bitmap or icon of butotn. */
} BUTTONDATA;
typedef BUTTONDATA* PBUTTONDATA;

#define VBORDER 10
#define HBORDER 5
#define BTN_WIDTH_BMP       20 
#define BTN_HEIGHT_BMP      20 
#define BTN_INTER_BMPTEXT   2

#define BUTTON_STATUS(pctrl)   (((PBUTTONDATA)((pctrl)->dwAddData2))->status)

#define BUTTON_GET_POSE(pctrl)  \
	(BUTTON_STATUS(pctrl) & BST_POSE_MASK)

#define BUTTON_SET_POSE(pctrl, status) \
	do { \
		BUTTON_STATUS(pctrl) &= ~BST_POSE_MASK; \
		BUTTON_STATUS(pctrl) |= status; \
	}while(0)

#define BUTTON_TYPE(pctrl)     ((pctrl)->dwStyle & BS_TYPEMASK)

#define BUTTON_IS_PUSHBTN(pctrl) \
	(BUTTON_TYPE(pctrl) == BS_PUSHBUTTON || \
	 BUTTON_TYPE(pctrl) == BS_DEFPUSHBUTTON)

#define BUTTON_IS_AUTO(pctrl) \
	(BUTTON_TYPE(pctrl) == BS_AUTO3STATE || \
	 BUTTON_TYPE(pctrl) == BS_AUTOCHECKBOX || \
	 BUTTON_TYPE(pctrl) == BS_AUTORADIOBUTTON)


/*btnGetRects:
 *      get the client rect(draw body of button), the content rect
 *      (draw contern), and bitmap rect(draw a little icon)
 */
static void btnGetRects (PCONTROL pctrl, RECT* prcClient, RECT* prcContent, RECT* prcBitmap)
{
	GetClientRect ((HWND)pctrl, prcClient);

	if (BUTTON_IS_PUSHBTN(pctrl) || (pctrl->dwStyle & BS_PUSHLIKE))
	{
		SetRect (prcContent, (prcClient->left   + BTN_WIDTH_BORDER),
				(prcClient->top    + BTN_WIDTH_BORDER),
				(prcClient->right  - BTN_WIDTH_BORDER),
				(prcClient->bottom - BTN_WIDTH_BORDER));

		if (BUTTON_GET_POSE(pctrl) == BST_PUSHED)
		{
			prcContent->left ++;
			prcContent->top ++;
			prcContent->right ++;
			prcContent->bottom++;
		}

		SetRectEmpty (prcBitmap);
		return;
	}

	if (pctrl->dwStyle & BS_LEFTTEXT) {
		SetRect (prcContent, prcClient->left + 1,
				prcClient->top + 1,
				prcClient->right - BTN_WIDTH_BMP - BTN_INTER_BMPTEXT,
				prcClient->bottom - 1);
		SetRect (prcBitmap, prcClient->right - BTN_WIDTH_BMP,
				prcClient->top + 1,
				prcClient->right - 1,
				prcClient->bottom - 1);
	}
	else {
		SetRect (prcContent, prcClient->left + BTN_WIDTH_BMP + BTN_INTER_BMPTEXT,
				prcClient->top + 1,
				prcClient->right - 1,
				prcClient->bottom - 1);
		SetRect (prcBitmap, prcClient->left + 1,
				prcClient->top + 1,
				prcClient->left + BTN_WIDTH_BMP,
				prcClient->bottom - 1);
	}

}

static int btnGetTextFmt (int dwStyle)
{
	UINT dt_fmt;
	if (dwStyle & BS_MULTLINE)
		dt_fmt = DT_WORDBREAK;
	else
		dt_fmt = DT_SINGLELINE;

	if ((dwStyle & BS_TYPEMASK) == BS_PUSHBUTTON
			|| (dwStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON
			|| (dwStyle & BS_PUSHLIKE))
		dt_fmt |= DT_CENTER | DT_VCENTER;
	else {
		if ((dwStyle & BS_ALIGNMASK) == BS_RIGHT)
			dt_fmt = DT_WORDBREAK | DT_RIGHT;
		else if ((dwStyle & BS_ALIGNMASK) == BS_CENTER)
			dt_fmt = DT_WORDBREAK | DT_CENTER;
		else dt_fmt = DT_WORDBREAK | DT_LEFT;

		if ((dwStyle & BS_ALIGNMASK) == BS_TOP)
			dt_fmt |= DT_SINGLELINE | DT_TOP;
		else if ((dwStyle & BS_ALIGNMASK) == BS_BOTTOM)
			dt_fmt |= DT_SINGLELINE | DT_BOTTOM;
		else dt_fmt |= DT_SINGLELINE | DT_VCENTER;
	}

	return dt_fmt;
}

static void paint_content_focus(HDC hdc, PCONTROL pctrl, RECT* prc_cont)
{
	DWORD dt_fmt = 0;
 	BOOL is_get_fmt = FALSE;
	gal_pixel old_pixel;
	gal_pixel text_pixel;
	gal_pixel frame_pixel;
	RECT focus_rc;
	int status, type;

	status = BUTTON_STATUS (pctrl);
	type = BUTTON_TYPE (pctrl);

	/*draw button content*/
	switch (pctrl->dwStyle & BS_CONTENTMASK)
	{
	case BS_BITMAP:
		break;
	case BS_ICON:
		break;
	default:
		if(BUTTON_STATUS(pctrl) & BST_FOCUS) {
			text_pixel = DWORD2PIXEL(HDC_SCREEN, 0xFF292929);
		} else {
			text_pixel = DWORD2PIXEL(HDC_SCREEN, 0xFFDCDCDC);
		}

		old_pixel = SetTextColor(hdc, text_pixel);
		dt_fmt = btnGetTextFmt(pctrl->dwStyle);
		is_get_fmt = TRUE;
		SetBkMode (hdc, BM_TRANSPARENT);
		DrawText (hdc, pctrl->spCaption, -1, prc_cont, dt_fmt);
		SetTextColor(hdc, old_pixel);
	}
}

static void paint_push_btn (HDC hdc, PCONTROL pctrl)
{
	RECT rcClient;
	RECT rcContent;
	RECT rcBitmap;

	btnGetRects (pctrl, &rcClient, &rcContent, &rcBitmap);
	paint_content_focus(hdc, pctrl, &rcContent);
}

static int button_callback(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	PopUpMenuInfo_t *info;
	PCONTROL    pctrl;

	switch(message)	{
	case MSG_PAINT:
		{
			HDC hdc = BeginPaint (hWnd);

			pctrl   = gui_Control(hWnd);
			SelectFont (hdc, GetWindowFont(hWnd));

			if (BUTTON_IS_PUSHBTN(pctrl) || (pctrl->dwStyle & BS_PUSHLIKE)) {
				paint_push_btn(hdc, pctrl);
			}

			EndPaint (hWnd, hdc);
			return 0;
		}
		break;
	default:
		break;
	}

	return (*button_default_callback)(hWnd, message, wParam, lParam);
}
#endif

static void setCtrlText(HWND hDlg, int ID, const char *string)
{
	LOGD("setHotspotText:%s", string);
	SetWindowText(GetDlgItem(hDlg, ID), string);
}

static void enableKeyBoard(HWND hDlg, int ID, bool enable)
{
	int i;
	HWND hwnd;
	LOGD("enableKeyBoard, ID:%d, enable:%d", ID, enable);
	if (ID == 0) {
		for (i=IDC_NUM_0; i<=IDC_NUM_OK; i++) {
			hwnd = GetDlgItem(hDlg, i);
			if (enable) {
				IncludeWindowStyle(hwnd, WS_VISIBLE);
			} else {
				ExcludeWindowStyle(hwnd, WS_VISIBLE);
			}
		}
	} else {
		hwnd = (GetDlgItem(hDlg, ID));
		if (enable) {
			IncludeWindowStyle(hwnd, WS_VISIBLE);
		} else {
			ExcludeWindowStyle(hwnd, WS_VISIBLE);
		}
	}
	UpdateWindow (hDlg, true);
}

static void setNextFocusCtrlId(HWND hDlg, bool KeyBoardOn, int *IDCSelected, bool Right)
{
	LOGD("setNextFocusCtrlId, KeyBoardOn:%d, IDCSelected:%d, Right:%d", KeyBoardOn, *IDCSelected, Right);
	if (!KeyBoardOn) {
		if (*IDCSelected == IDC_SWITCH) *IDCSelected = IDC_PASSWORD;
		else *IDCSelected = IDC_SWITCH;
	} else {
		if (*IDCSelected == IDC_PASSWORD) {
			*IDCSelected = IDC_NUM_0;
			setCtrlText(hDlg, IDC_PASSWORD, "");
		} else {
			if (Right && *IDCSelected <= IDC_NUM_OK) {
				(*IDCSelected)++;
				if (*IDCSelected == IDC_NUM_OK && !IsWindowVisible(GetDlgItem(hDlg, IDC_NUM_OK)))
					*IDCSelected = IDC_NUM_0;
				
				if (*IDCSelected > IDC_NUM_OK)
					*IDCSelected = IDC_NUM_0;
			} else if (!Right && *IDCSelected >= IDC_NUM_0) {
				(*IDCSelected)--;
				if (*IDCSelected < IDC_NUM_0)
					*IDCSelected = IDC_NUM_OK;

				if (*IDCSelected == IDC_NUM_OK && !IsWindowVisible(GetDlgItem(hDlg, IDC_NUM_OK)))
					*IDCSelected = IDC_NUM_9;
			}
			
		}
	}

	SetFocusChild(GetDlgItem(hDlg, *IDCSelected));
	InvalidateRect(hDlg, NULL, FALSE);
}

static void SetBorderColor(HWND hDlg, unsigned int IDC, unsigned int select_flg)
{
	RECT rect;
	apData_t* configData;
	HWND hCtrl;

	configData = (apData_t*)GetWindowAdditionalData(hDlg);
	hCtrl = GetDlgItem(hDlg, IDC);

	HDC hdc = GetDC(hCtrl);

	GetClientRect(hCtrl, &rect);
	if (select_flg == SELECT_BORDER){
		SetPenColor(hdc, configData->borderc_selected );
	}
	else if (select_flg == UNSELECT_BORDER){
		SetPenColor(hdc, configData->borderc_normal );
	}

	Rectangle (hdc, rect.left+5, rect.top+5, rect.right-5, rect.bottom-5);

	EndPaint(hCtrl, hdc);
}

static int apSettingProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	static int IDCSelected;
	static unsigned long long tickCnt;
	apData_t* configData;
	static bool KeyBoardOn = false;
	
	//LOGD("message is %s\n", Message2Str(message));
	
	switch(message) {
	case MSG_INITDIALOG:
	{
		configData = (apData_t*)lParam;
		if(configData == NULL) {
			LOGE("configData is NULL\n");
			EndDialog(hDlg, -1);
		}
		SetWindowAdditionalData(hDlg, (DWORD)configData);

		for (unsigned int i = IDC_SWITCH_TITLE; i <= IDC_PASSWORD; i++){
			SetWindowElementAttr(GetDlgItem (hDlg, i), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, configData->fgc_number) );//0xFF919191
		}

#ifdef USE_NEWUI
		button_default_callback = SetWindowCallbackProc(GetDlgItem(hDlg, IDC_SWITCH), button_callback);
		for (unsigned int i = IDC_NUM_0; i <= IDC_NUM_OK; i++){
			button_default_callback = SetWindowCallbackProc(GetDlgItem(hDlg, i), button_callback);
		}
#else
		for (unsigned int i = IDC_NUM_0; i <= IDC_NUM_OK; i++){
			SetWindowElementAttr(GetDlgItem (hDlg, i), WE_FGC_THREED_BODY, PIXEL2DWORD(HDC_SCREEN, configData->fgc_number) );//0xFF919191
		}
		SetWindowElementAttr(GetDlgItem (hDlg, IDC_SWITCH), WE_FGC_THREED_BODY, PIXEL2DWORD(HDC_SCREEN, configData->fgc_number) );//0xFF919191
#endif
		SetWindowBkColor(hDlg, configData->bgc_widget);
		SetWindowFont(hDlg, configData->pLogFont);

		IDCSelected = IDC_SWITCH;
		KeyBoardOn = false;
		SetFocusChild(GetDlgItem(hDlg, IDCSelected));

#ifndef USE_NEWUI
		if (isSoftapEnabled()) {
			SetWindowBkColor(GetDlgItem(hDlg, IDC_SWITCH), configData->borderc_selected);
		} else {
			SetWindowBkColor(GetDlgItem(hDlg, IDC_SWITCH), configData->borderc_normal);
		}
#endif
	}
	break;
	case MSG_FONTCHANGED:
	{
		PLOGFONT pLogFont;
		pLogFont = GetWindowFont(hDlg);
		for(unsigned int i = IDC_SWITCH; i <= IDC_NUM_OK; i++) {
			SetWindowFont(GetDlgItem(hDlg, i), pLogFont);
		}
	}
	break;
	case MSG_PAINT:
	{
#ifdef USE_NEWUI
		HDC hdc;
		RECT rcClient;
		configData = (apData_t*)GetWindowAdditionalData(hDlg);

		hdc = BeginPaint(hDlg);
		GetClientRect(hDlg, &rcClient);
		FillBoxBg(hdc, rcClient, &configData->bgPic);
		if (IDCSelected == IDC_SWITCH) {	
			FillBoxWithBitmap(hdc, configData->pRect[INDEX_SWITCH].x, configData->pRect[INDEX_SWITCH].y, 
			configData->pRect[INDEX_SWITCH].w, configData->pRect[INDEX_SWITCH].h, &configData->btnSelectPic);
		}
		else {
			FillBoxWithBitmap(hdc, configData->pRect[INDEX_PASSWORD].x, configData->pRect[INDEX_PASSWORD].y, 
			configData->pRect[INDEX_PASSWORD].w, configData->pRect[INDEX_PASSWORD].h, &configData->btnSelectPic);

			if (KeyBoardOn) {
				FillBoxWithBitmap(hdc, configData->pRect[IDCSelected-IDC_START].x, configData->pRect[IDCSelected-IDC_START].y, 
				configData->pRect[IDCSelected-IDC_START].w, configData->pRect[IDCSelected-IDC_START].h, &configData->btnSelectPic);
			}
		}			
		EndPaint(hDlg, hdc);
#else
		HDC hdc = BeginPaint(hDlg);
		if (!KeyBoardOn) {
			int unSelected; 
			if (IDCSelected == IDC_SWITCH)
				unSelected = IDC_PASSWORD;
			else 
				unSelected = IDC_SWITCH;
			SetBorderColor(hDlg, IDCSelected, SELECT_BORDER);
			SetBorderColor(hDlg, unSelected, UNSELECT_BORDER);
		} else {
			for (int i=IDC_NUM_0; i<=IDC_NUM_OK; i++) {
				if (i == IDCSelected)
					SetBorderColor(hDlg, i, SELECT_BORDER);
				else
					SetBorderColor(hDlg, i, UNSELECT_BORDER);
			}
		}
		EndPaint(hDlg, hdc);
#endif
	}
	break;
	case MSG_COMMAND:
	{
		int id = LOWORD(wParam);
		switch (id) {
		case IDC_SWITCH:
		{
			configData = (apData_t*)GetWindowAdditionalData(hDlg);
			if (isSoftapEnabled()) {
				disable_Softap();
				setCtrlText(hDlg, IDC_SWITCH, configData->offStr);
				SetWindowBkColor(GetDlgItem(hDlg, IDC_SWITCH), configData->borderc_normal);
				configData->Cdr->notify(EVENT_AP, 0);
				configData->rm->setResBoolValue(ID_MENU_LIST_AP, 0);
			} else {
				if (check_if_wifi_enabled()) {
					HWND pDlg = configData->Cdr->getWindowHandle(WINDOWID_MENU);
					configData->Cdr->notify(EVENT_WIFI, 0);
					configData->rm->setResBoolValue(ID_MENU_LIST_WIFI, 0);
					SendMessage(pDlg, MSG_CREATE, 0, 0);
				}
				enable_Softap();
				setCtrlText(hDlg, IDC_SWITCH, configData->onStr);
				SetWindowBkColor(GetDlgItem(hDlg, IDC_SWITCH), configData->borderc_selected);
				configData->Cdr->notify(EVENT_AP, 1);
				configData->rm->setResBoolValue(ID_MENU_LIST_AP, 1);
			}
			configData->rm->syncConfigureToDisk();
		}
		break;
		case IDC_NUM_0: case IDC_NUM_1: case IDC_NUM_2: case IDC_NUM_3:
		case IDC_NUM_4: case IDC_NUM_5: case IDC_NUM_6: case IDC_NUM_7:
		case IDC_NUM_8: case IDC_NUM_9:
		{
			char buf[32] = {0}, buffer[32] = {0};
			if (IDCSelected < IDC_NUM_0 || IDCSelected > IDC_NUM_9)
				break;
			sprintf (buf, "%d", IDCSelected - IDC_NUM_0);
			SendMessage (GetDlgItem(hDlg, IDC_PASSWORD), MSG_GETTEXT, 32, (LPARAM)buffer);
			strcat (buffer, buf);
			SendMessage (GetDlgItem(hDlg, IDC_PASSWORD), MSG_SETTEXT, 0, (LPARAM)buffer);
			if (strlen(buffer) >= 8) {
				enableKeyBoard(hDlg, IDC_NUM_OK, true);
			} else {
				enableKeyBoard(hDlg, IDC_NUM_OK, false);
			}
		}
		break;
		case IDC_NUM_OK:
		{
			char buffer[32]={0};
		#ifndef MESSAGE_FRAMEWORK
			char buf[32] = {0};
			char Encryp = 0;
		#endif
			HWND SWhwnd = GetDlgItem(hDlg, IDC_SWITCH);
			IDCSelected = IDC_SWITCH;
			KeyBoardOn = false;
			enableKeyBoard(hDlg, 0, KeyBoardOn);
			SendMessage (GetDlgItem(hDlg, IDC_PASSWORD), MSG_GETTEXT, 32, (LPARAM)buffer);
			SetFocusChild(GetDlgItem(hDlg, IDCSelected));
			IncludeWindowStyle(SWhwnd, WS_DISABLED);
			
		#ifdef MESSAGE_FRAMEWORK
			{
				WRTC_WIFI_PASSWD_REQ req;

				memset(&req, 0x0, sizeof(req));
				strncpy(req.pwd, buffer, WRTC_MAX_VALUE_LEN);
				
				fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_WIFIPASSWD_REQ, &req, sizeof(req));		
			}
		#else
			Encryp = getSoftap_SecType_PW(buf);
			LOGD("Encryp:%d, buf:%s, buffer:%s", Encryp, buf, buffer);
			if (!Encryp && strlen(buffer) >= 8) {
				setSoftap_SecType_PW(WPA2_PSK, buffer);
				restart_Softap();
			} else if (Encryp && strlen(buffer) == 0) {
				setSoftap_SecType_PW(NONE_SECURT, buffer);
				restart_Softap();
			} else if (Encryp && strcmp(buf, buffer) != 0) {
				setSoftap_SecType_PW(WPA2_PSK, buffer);
				restart_Softap();
			}
		#endif
			ExcludeWindowStyle(SWhwnd, WS_DISABLED);
			UpdateWindow (hDlg, true);
		}
		break;
		default: break;
		}
	}
	break;
	case MSG_KEYDOWN:
	{
		if(wParam == CDR_KEY_MODE) {
			tickCnt = GetTickCount();
		} else if (wParam == CDR_KEY_OK) {
			SendMessage (hDlg, MSG_KEYDOWN, SCANCODE_SPACE, 0);
		}
	}
	break;
	case MSG_KEYUP:
	{
		switch(wParam) {
		case CDR_KEY_OK:
			SendMessage (hDlg, MSG_KEYUP, SCANCODE_SPACE, 0);
			if (IDCSelected == IDC_PASSWORD) {
				KeyBoardOn = true;
				enableKeyBoard(hDlg, 0, KeyBoardOn);
				setNextFocusCtrlId(hDlg, KeyBoardOn, &IDCSelected, false);
			}
			//InvalidateRect(hDlg, NULL, FALSE);
		break;
		case CDR_KEY_LEFT:
			setNextFocusCtrlId(hDlg, KeyBoardOn, &IDCSelected, false);
		break;
		case CDR_KEY_RIGHT:
			setNextFocusCtrlId(hDlg, KeyBoardOn, &IDCSelected, true);
		break;
		case CDR_KEY_MODE:
			if (KeyBoardOn) {
				char buf[32]={0};
				KeyBoardOn = false;
				enableKeyBoard(hDlg, 0, KeyBoardOn);
				IDCSelected = IDC_SWITCH;
				setNextFocusCtrlId(hDlg, KeyBoardOn, &IDCSelected, false);
				getSoftap_SecType_PW(buf);
				setCtrlText(hDlg, IDC_PASSWORD, buf);
			} else {
				EndDialog(hDlg, 0);
			}
		break;
		}
	}
	break;
	}
	return DefaultDialogProc (hDlg, message, wParam, lParam);
}

int apSettingDialog(HWND hParent, apData_t* configData)
{
	CTRLDATA ctrlData[INDEX_NUM_TOTAL];
	DLGTEMPLATE dlgApSetting;
	unsigned int intval, gap, mwide;
	unsigned int i = 0, n;
	char ssid[32] = {0}, pw[32] = {0};
	const char *num[11];
	memset(ctrlData, 0, sizeof(ctrlData));

	intval = (configData->rect.h)/5;
	gap = intval/5;
	mwide = (configData->rect.w)/5;
	LOGD("intval %d,gap %d, mwide %d\n", intval, gap, mwide);

	ctrlData[INDEX_SWITCH].class_name = CTRL_BUTTON;
	ctrlData[INDEX_SWITCH].dwStyle = WS_VISIBLE | BS_PUSHBUTTON;
	ctrlData[INDEX_SWITCH].caption = isSoftapEnabled()? configData->onStr:configData->offStr;
#ifdef USE_NEWUI
	ctrlData[INDEX_SWITCH].dwExStyle = WS_EX_TRANSPARENT;
#endif
	ctrlData[INDEX_SWITCH].werdr_name = NULL;
	ctrlData[INDEX_SWITCH].we_attrs = NULL;
	ctrlData[INDEX_SWITCH].id  = IDC_SWITCH;

	for(i = INDEX_SWITCH_TITLE; i <= INDEX_PASSWORD; i++) {
		ctrlData[i].class_name = CTRL_STATIC;
		ctrlData[i].dwStyle = SS_CENTER | SS_VCENTER | WS_VISIBLE;
		ctrlData[i].caption = "";
		ctrlData[i].dwExStyle = WS_EX_TRANSPARENT;
		ctrlData[i].werdr_name = NULL;
		ctrlData[i].we_attrs = NULL;
		ctrlData[i].id  = i + IDC_START;
	}

	num[0] = "0";num[1] = "1";num[2] = "2";num[3] = "3";num[4] = "4";
	num[5] = "5";num[6] = "6";num[7] = "7";num[8] = "8";num[9] = "9";
	num[10] = "OK";
	for(i = INDEX_NUM_0; i <= INDEX_NUM_OK; i++) {
		ctrlData[i].class_name = CTRL_BUTTON;
		ctrlData[i].dwStyle = BS_PUSHBUTTON;
		ctrlData[i].caption = num[i-INDEX_NUM_0];
		ctrlData[i].dwExStyle = WS_EX_TRANSPARENT;
		ctrlData[i].werdr_name = NULL;
		ctrlData[i].we_attrs = NULL;
		ctrlData[i].id  = i + IDC_START;
	}
	ctrlData[INDEX_NUM_0].dwStyle = BS_PUSHBUTTON | WS_GROUP;

	ctrlData[INDEX_SWITCH_TITLE].caption = configData->mSwitch;

	ctrlData[INDEX_SSID_TITLE].caption = configData->ssid;
	getSoftapSSID(ssid);
	ctrlData[INDEX_SSID].caption = ssid;

	ctrlData[INDEX_PASSWORD_TITLE].caption = configData->password;
	getSoftap_SecType_PW(pw);
	ctrlData[INDEX_PASSWORD].caption = pw;

	ctrlData[INDEX_SWITCH_TITLE].x = 0;
	ctrlData[INDEX_SWITCH_TITLE].y = gap;
	ctrlData[INDEX_SWITCH_TITLE].w = mwide;
	ctrlData[INDEX_SWITCH_TITLE].h = intval;

	ctrlData[INDEX_SWITCH].x = mwide + gap;
	ctrlData[INDEX_SWITCH].y = gap;
	ctrlData[INDEX_SWITCH].w = mwide * 3;
	ctrlData[INDEX_SWITCH].h = intval;

	ctrlData[INDEX_SSID_TITLE].x = 0;
	ctrlData[INDEX_SSID_TITLE].y = intval + 2*gap;
	ctrlData[INDEX_SSID_TITLE].w = mwide;
	ctrlData[INDEX_SSID_TITLE].h = intval;

	ctrlData[INDEX_SSID].x = mwide + gap;
	ctrlData[INDEX_SSID].y = intval + 2*gap;
	ctrlData[INDEX_SSID].w = mwide * 3;
	ctrlData[INDEX_SSID].h = intval;

	ctrlData[INDEX_PASSWORD_TITLE].x = 0;
	ctrlData[INDEX_PASSWORD_TITLE].y = 2*intval + 3*gap;
	ctrlData[INDEX_PASSWORD_TITLE].w = mwide;
	ctrlData[INDEX_PASSWORD_TITLE].h = intval;

	ctrlData[INDEX_PASSWORD].x = mwide + gap;
	ctrlData[INDEX_PASSWORD].y = 2*intval + 3*gap;
	ctrlData[INDEX_PASSWORD].w = mwide * 3;
	ctrlData[INDEX_PASSWORD].h = intval;

	n = (configData->rect.w)/23;
	for (i = INDEX_NUM_0; i <= INDEX_NUM_OK; i++) {
		ctrlData[i].x = (2*(i-INDEX_NUM_0) + 1) * n;
		ctrlData[i].y = 3*intval + 5*gap;
		ctrlData[i].w = n;
		ctrlData[i].h = (configData->rect.h)/8;
	}

	memset(&dlgApSetting, 0, sizeof(dlgApSetting));
	dlgApSetting.dwStyle = WS_VISIBLE;
#ifdef USE_NEWUI
	dlgApSetting.dwExStyle = WS_EX_TRANSPARENT | WS_EX_AUTOSECONDARYDC;
#else
	dlgApSetting.dwExStyle = WS_EX_NONE;
#endif
	dlgApSetting.x = configData->rect.x;
	dlgApSetting.y = configData->rect.y;
	dlgApSetting.w = configData->rect.w;
	dlgApSetting.h = configData->rect.h;
	dlgApSetting.caption = "";
	dlgApSetting.controlnr = TABLESIZE(ctrlData);
	dlgApSetting.controls = ctrlData;

#ifdef USE_NEWUI
	CDR_RECT cdr_rect[INDEX_NUM_TOTAL];
	for (i=0; i<INDEX_NUM_TOTAL; i++) {
		cdr_rect[i].x = ctrlData[i].x;
		cdr_rect[i].y = ctrlData[i].y;
		cdr_rect[i].w = ctrlData[i].w;
		cdr_rect[i].h = ctrlData[i].h;
	}
	configData->pRect = &cdr_rect[0];
#endif

	return DialogBoxIndirectParam (&dlgApSetting, hParent, apSettingProc, (LPARAM)configData);
}

int apSetSsid(char* buffer)
{
	if(NULL == buffer)
	{
		return -1;
	}
	
	LOGD("apSetSsid:%s ", buffer);
	if (strlen(buffer) > 0) {
		if(setSoftapSSID(buffer) != 0)
			return -1;
		restart_Softap();
	} else {
		return -1;
	}
	
	return 0;
}

int apSetPasswd(char* buffer)
{
	char buf[32] = {0};
	char Encryp = 0;

	if(NULL == buffer)
	{
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	Encryp = getSoftap_SecType_PW(buf);
	LOGD("apSetPasswd Encryp:%d ", Encryp);
	if (!Encryp && strlen(buffer) >= 8) {
		setSoftap_SecType_PW(WPA2_PSK, buffer);
		restart_Softap();
	} else if (Encryp && strlen(buffer) == 0) {
		setSoftap_SecType_PW(NONE_SECURT, buffer);
		restart_Softap();
	} else if (Encryp && strcmp(buf, buffer) != 0) {
		setSoftap_SecType_PW(WPA2_PSK, buffer);
		restart_Softap();
	}

	return 0;
}
