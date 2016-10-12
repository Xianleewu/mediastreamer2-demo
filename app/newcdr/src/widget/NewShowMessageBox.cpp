
#include <string.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "include/ctrlclass.h"

#include "cdr_widgets.h"
#include "cdr_message.h"
#include "misc.h"
#include "keyEvent.h"
#include "StorageManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "showMessageBox.cpp"
#include "debug.h"

#define IDC_TITLE		650
#define IDC_TEXT		651
#define TIMER_CONFIRM_CALLBACK	200

extern int FillBoxBg(HDC hdc, RECT &rect, PBITMAP pBg);

typedef struct {
	unsigned int dwStyle;
	unsigned int IDCSelected;
	unsigned char flag_end_key;
	BITMAP boxBgPic;
	BITMAP linePic;
	BITMAP buttonConfirmPic;
	BITMAP buttonCancelPic;
	CDR_RECT rect[MESSAGEBOX_MAX_BUTTONS];
	PLOGFONT pLogFont;	
	const char* title;
	const char* text;
	gal_pixel bgcWidget;
	gal_pixel fgcWidget;
	const char** buttonStr;
	gal_pixel linecTitle;
	gal_pixel linecItem;
	unsigned int buttonNRs;
	void (*confirmCallback)(HWND hDlg, void* data);
	void* confirmData;
	const char* msg4ConfirmCallback;
	int id;
}MBPrivData;

static gal_pixel gp_hilite_bgc;
static gal_pixel gp_hilite_fgc;
static gal_pixel gp_normal_bgc;
static gal_pixel gp_normal_fgc;

#define BUTTON_SELECTED	 0
#define BUTTON_UNSELECTED	 1

static void drawButton(HWND hDlg, HDC IDCButton, unsigned int flag)
{
	HWND hWnd;
	RECT rect;
	MBPrivData* privData;
	DWORD old_bkmode;
	gal_pixel old_color;
	HDC hdc;
	char* text;

	privData = (MBPrivData*)GetWindowAdditionalData(hDlg);
	hWnd = GetDlgItem(hDlg, IDCButton);
	GetClientRect(hWnd, &rect);

	hdc = GetDC(hWnd);

	if(flag == BUTTON_SELECTED) {
		old_color = SetBrushColor(hdc, gp_hilite_bgc);
		old_bkmode = SetBkMode(hdc, BM_TRANSPARENT);
		FillBox(hdc, rect.left, rect.top, RECTW(rect), RECTH(rect));
		SetTextColor(hdc, gp_hilite_fgc);
		text = (char*)GetWindowAdditionalData2(hWnd);
		DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SetBrushColor(hdc, old_color);
		SetBkMode(hdc, old_bkmode);
	} else if(flag == BUTTON_UNSELECTED) {
		old_color = SetBrushColor(hdc, gp_normal_bgc);
		old_bkmode = SetBkMode(hdc, BM_TRANSPARENT);
		FillBox(hdc, rect.left, rect.top, RECTW(rect), RECTH(rect));
		SetTextColor (hdc, gp_normal_fgc);
		text = (char*)GetWindowAdditionalData2(hWnd);
		DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SetBrushColor(hdc, old_color);
		SetBkMode(hdc, old_bkmode);
	}

	ReleaseDC(hdc);
}

static BOOL timerCallback(HWND hDlg, int id, DWORD data)
{
	MBPrivData* privData;
	privData = (MBPrivData *)GetWindowAdditionalData(hDlg);

	if(id == TIMER_CONFIRM_CALLBACK) {
		db_msg("confirmCallback\n");
		(*privData->confirmCallback)(hDlg , privData->confirmData);
		db_msg("confirmCallback\n");
	}

	return FALSE;
}

static int msgBoxProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam )
{
	MBPrivData* privData;
	switch(message) {
	case MSG_FONTCHANGED:
		{
			HWND hChild;	
			hChild = GetNextChild(hDlg, 0);
			while( hChild && (hChild != HWND_INVALID) )
			{
				SetWindowFont(hChild, GetWindowFont(hDlg));
				hChild = GetNextChild(hDlg, hChild);
			}
		}
		break;
	case MSG_INITDIALOG:
		{
			privData = (MBPrivData*)lParam;
			SetWindowAdditionalData(hDlg, (DWORD)privData);
			SetWindowFont(hDlg, privData->pLogFont);

			/******* the hilight is the global attibute *******/
			gp_hilite_bgc = GetWindowElementPixel(hDlg, WE_BGC_HIGHLIGHT_ITEM);
			gp_hilite_fgc = GetWindowElementPixel(hDlg, WE_FGC_HIGHLIGHT_ITEM);
			gp_normal_bgc = privData->bgcWidget;
			gp_normal_fgc = privData->fgcWidget;

			SetWindowBkColor(hDlg, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));

			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TITLE), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TEXT), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
			SetWindowText(GetDlgItem(hDlg, IDC_TITLE), privData->title);
			SetWindowText(GetDlgItem(hDlg, IDC_TEXT), privData->text);

			for(unsigned int i = 0; i < privData->buttonNRs; i++) {
				SetWindowElementAttr( GetDlgItem(hDlg, IDC_BUTTON_START + i), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, gp_normal_fgc));
				SetWindowText(GetDlgItem(hDlg, IDC_BUTTON_START+i), privData->buttonStr[i]);
				//SetWindowAdditionalData2( GetDlgItem(hDlg, IDC_BUTTON_START + i), (DWORD)privData->buttonStr[i]);
			}
		}
		break;
	case MSG_KEYUP:
		{
			privData = (MBPrivData *)GetWindowAdditionalData(hDlg);
			switch(wParam) {
			case CDR_KEY_OK:
				if(privData->flag_end_key == 1) {
					db_msg("hDlg is %x\n", hDlg);
					if( (privData->IDCSelected == IDC_BUTTON_START) && privData->confirmCallback) {
						if(privData->msg4ConfirmCallback) {
							SetWindowText(GetDlgItem(hDlg, IDC_TEXT), privData->msg4ConfirmCallback);
							SetTimerEx(hDlg, TIMER_CONFIRM_CALLBACK, 5, timerCallback);
							SetNullFocus(hDlg);
						} else {
							db_msg("confirmCallback\n");
							(*privData->confirmCallback)(hDlg , privData->confirmData);
							db_msg("confirmCallback\n");
						}
					} else {
						EndDialog(hDlg, privData->IDCSelected);
					}
				}
				break;
			case CDR_KEY_MODE:
				if(privData->flag_end_key == 1)
					EndDialog(hDlg, IDC_BUTTON_ELSE);
				break;
			case CDR_KEY_LEFT:
				privData->IDCSelected--;
				if(privData->IDCSelected < IDC_BUTTON_START) {
					privData->IDCSelected = IDC_BUTTON_START + privData->buttonNRs - 1;
				}
				InvalidateRect(hDlg, NULL, TRUE);	
				break;
			case CDR_KEY_RIGHT:
				privData->IDCSelected++;
				if(privData->IDCSelected > (IDC_BUTTON_START + privData->buttonNRs - 1) ) {
					privData->IDCSelected = IDC_BUTTON_START;
				}
				InvalidateRect(hDlg, NULL, TRUE);	
				break;
			default:
				break;
			}
		}
		break;
	case MSG_KEYDOWN:
		{
			RECT rect;
			GetClientRect(hDlg, &rect);
			privData = (MBPrivData *)GetWindowAdditionalData(hDlg);
			switch(wParam) {
			case CDR_KEY_OK:
				if(privData->flag_end_key == 0) {
					db_msg("hDlg is %x\n", hDlg);
					if( (privData->IDCSelected == IDC_BUTTON_START) && privData->confirmCallback ) {
						if(privData->msg4ConfirmCallback) {
							SetWindowText(GetDlgItem(hDlg, IDC_TEXT), privData->msg4ConfirmCallback);
							SetTimerEx(hDlg, TIMER_CONFIRM_CALLBACK, 5, timerCallback);
							SetNullFocus(hDlg);
						} else {
							db_msg("confirmCallback\n");
							(*privData->confirmCallback)(hDlg , privData->confirmData);
							db_msg("confirmCallback\n");
						}
					} else {
						EndDialog(hDlg, privData->IDCSelected);	
					}
				}
				break;
			case CDR_KEY_MODE:
				if(privData->flag_end_key == 0) {
					EndDialog(hDlg, 0);	
				}
				break;

			default:
				break;
			}
		}
		break;
	case MSG_PAINT:
		{
			RECT rect, rect1;
			HDC hdc = BeginPaint(hDlg);
			privData = (MBPrivData *)GetWindowAdditionalData(hDlg);

			GetClientRect(GetDlgItem(hDlg, IDC_TITLE), &rect);
			GetClientRect(hDlg, &rect1);

			if (privData->boxBgPic.bmBits)
				FillBitmapPartInBox(hdc, rect1.left, rect1.top, rect1.right, rect1.bottom,
					&privData->boxBgPic, privData->boxBgPic.bmWidth/3, privData->boxBgPic.bmHeight/3,
					privData->boxBgPic.bmWidth/3, privData->boxBgPic.bmHeight/3);
				//FillBoxBg(hdc, rect1, &privData->boxBgPic);
			
			/**** draw the line below the title *******/
			if (privData->linePic.bmBits)
				FillBitmapPartInBox(hdc, 6, RECTH(rect), RECTW(rect)-12, 2, 
					&privData->linePic, 4, 1, 4, 2);

			/**** draw the line above the button *******/
			//SetPenColor(hdc, privData->linecItem );
			//LineEx(hdc, 4, RECTH(rect1) * 3 / 4 - 2, RECTW(rect1) - 4, RECTH(rect1) * 3 / 4 - 2);

			for(unsigned int i = 0; i < privData->buttonNRs; i++) {
				if(IDC_BUTTON_START + i == privData->IDCSelected) {
					if (privData->buttonConfirmPic.bmBits)
						FillBoxWithBitmap(hdc, privData->rect[i].x, privData->rect[i].y,
						privData->rect[i].w, privData->rect[i].h, &privData->buttonConfirmPic);			
				} else {
					if (privData->buttonCancelPic.bmBits)
						FillBoxWithBitmap(hdc, privData->rect[i].x, privData->rect[i].y,
						privData->rect[i].w, privData->rect[i].h, &privData->buttonCancelPic);
				}
			}
			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_CLOSE_TIP_LABEL:
		EndDialog(hDlg, 0);
		break;
	default:
		break;
	}

	return DefaultDialogProc (hDlg, message, wParam, lParam);
}


/*********** 1 Static + 2 Button *************/
int showMessageBox (HWND hParentWnd, MessageBox_t *info)
{
	CDR_RECT rect;
	unsigned int dwStyle;
	int	 retval;

	DLGTEMPLATE MsgBoxDlg;
	CTRLDATA *CtrlData = NULL;
	unsigned int controlNrs;

	MBPrivData* privData;
	ResourceManager* rm = ResourceManager::getInstance();

	dwStyle = info->dwStyle & MB_TYPEMASK;
	rect = info->rect;

	controlNrs = 3;
	if(dwStyle & MB_OKCANCEL){
		controlNrs = 4;
		db_msg("button dwStyle is MB_OKCANCEL\n");
	}

	CtrlData = (PCTRLDATA)malloc(sizeof(CTRLDATA) * controlNrs);
	memset(CtrlData, 0, sizeof(CTRLDATA) * controlNrs);
	memset(&MsgBoxDlg, 0, sizeof(MsgBoxDlg));
	MsgBoxDlg.dwStyle = WS_NONE;
	MsgBoxDlg.dwExStyle = WS_EX_TRANSPARENT | WS_EX_AUTOSECONDARYDC;

	CtrlData[0].class_name = CTRL_STATIC;
	CtrlData[0].dwStyle = SS_CENTER | SS_VCENTER;
	CtrlData[0].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
	CtrlData[0].id = IDC_TITLE;
	CtrlData[0].caption = "";
	if(dwStyle & MB_HAVE_TITLE) {
		CtrlData[0].dwStyle |= WS_VISIBLE;
		/* Title occupy 1/4 the height*/
		db_msg("messageBox have title\n");
		CtrlData[0].x = 0;
		CtrlData[0].y = 0;
		CtrlData[0].w = rect.w;
		CtrlData[0].h = rect.h / 4;
	} else {
		CtrlData[0].x = 0;
		CtrlData[0].y = 0;
		CtrlData[0].w = rect.w;
		CtrlData[0].h = rect.h / 8;
		db_msg("messageBox do not have title\n");
		db_msg("CtrlData[0].h is %d\n", CtrlData[0].h);
	}

	CtrlData[1].class_name = CTRL_STATIC;
	CtrlData[1].dwStyle = SS_LEFT | SS_CENTER | SS_VCENTER;
	CtrlData[1].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
	CtrlData[1].id = IDC_TEXT;
	CtrlData[1].caption = "";
	if(dwStyle & MB_HAVE_TEXT) {
		CtrlData[1].dwStyle |= WS_VISIBLE;
		/******* text *********/
		CtrlData[1].x = rect.w / 16;
		CtrlData[1].y = CtrlData[0].h + CtrlData[0].y + 20;
		CtrlData[1].w = rect.w - CtrlData[1].x;
		CtrlData[1].h = rect.h / 4;
		db_msg("messageBox have text\n");
	} else {
		/******* text *********/
		CtrlData[1].x = rect.w / 16;
		CtrlData[1].y = CtrlData[0].h + CtrlData[0].y + 20;
		CtrlData[1].w = rect.w - CtrlData[1].x;
		CtrlData[1].h = rect.h / 8;
		db_msg("messageBox do not have text\n");
		db_msg("CtrlData[1].h is %d\n", CtrlData[1].h);
	}
	/* -------------------------------------- */

	db_msg("CtrlData[2] is Button OK\n");
	CtrlData[2].class_name = CTRL_STATIC;
	CtrlData[2].dwStyle = SS_CENTER | SS_VCENTER | WS_VISIBLE;
	CtrlData[2].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
	CtrlData[2].id = IDC_BUTTON_START;
	CtrlData[2].caption = "";

	if(dwStyle & MB_OKCANCEL) {
		db_msg("CtrlData[3] is Button Cancel\n");
		CtrlData[3].class_name = CTRL_STATIC;
		CtrlData[3].dwStyle = SS_CENTER | SS_VCENTER | WS_VISIBLE;
		CtrlData[3].dwExStyle = WS_EX_USEPARENTFONT | WS_EX_TRANSPARENT;
		CtrlData[3].id = IDC_BUTTON_START + 1;
		CtrlData[3].caption = "";
	}

	if( (dwStyle & MB_HAVE_TEXT) || (dwStyle & MB_HAVE_TITLE) ) {
		if(dwStyle & MB_OKCANCEL) {
			/********* button ok **********/
			db_msg("MB_OKCANCEL rect\n");
			CtrlData[2].x = rect.w / 32;
			CtrlData[2].y = rect.h * 2 / 3;
			CtrlData[2].w = rect.w * 7 / 16;
			CtrlData[2].h = rect.h / 4 - 3;

			/********* button CANCEL **********/
			CtrlData[3].x = rect.w / 2 + CtrlData[2].x;
			CtrlData[3].y = rect.h * 2 / 3;
			CtrlData[3].w = rect.w * 7 / 16;
			CtrlData[3].h = rect.h / 4 - 3;
		} else {
			/********* button ok **********/
			db_msg("MB_OK rect\n");
			CtrlData[2].x = rect.w * 9 / 32;
			CtrlData[2].y = rect.h * 2 / 3;
			CtrlData[2].w = rect.w * 7 / 16;
			CtrlData[2].h = rect.h / 4 - 3;
		}
	} else {
		if(dwStyle & MB_OKCANCEL) {
			db_msg("MB_OKCANCEL rect\n");
			CtrlData[2].x = 1;
			CtrlData[2].y = CtrlData[1].y + CtrlData[1].h + 2;
			CtrlData[2].w = rect.w / 2 - 2;
			CtrlData[2].h = rect.h - (CtrlData[2].y + 8);
			db_msg("button ok: y is %d, h is %d\n", CtrlData[2].y, CtrlData[2].h);

			/********* button CANCEL **********/
			CtrlData[3].x = rect.w / 2 + 1;
			CtrlData[3].y = CtrlData[1].y + CtrlData[1].h + 2;
			CtrlData[3].w = rect.w / 2 - 2;
			CtrlData[3].h = rect.h - (CtrlData[3].y + 8);
		} else {
			db_msg("MB_OK rect\n");
			CtrlData[2].x = rect.w / 4;
			CtrlData[2].y = CtrlData[1].y + CtrlData[1].h + 5;
			CtrlData[2].w = rect.w / 2;
			CtrlData[2].h = rect.h - CtrlData[2].y - 5;
		}
	}

	MsgBoxDlg.x = rect.x;
	MsgBoxDlg.y = rect.y;
	MsgBoxDlg.w = rect.w;
	MsgBoxDlg.h = rect.h;
	MsgBoxDlg.caption = "";
	MsgBoxDlg.controlnr = controlNrs;
	MsgBoxDlg.controls = CtrlData;
	
	dwStyle = info->dwStyle & MB_TYPEMASK;
	privData = (MBPrivData*)malloc(sizeof(MBPrivData));
	privData->dwStyle = dwStyle;
	
	privData->rect[0].x = CtrlData[2].x;
	privData->rect[0].y = CtrlData[2].y;
	privData->rect[0].w = CtrlData[2].w;
	privData->rect[0].h = CtrlData[2].h;
	
	if(privData->dwStyle & MB_OKCANCEL){
		privData->IDCSelected = IDC_BUTTON_START + 1;
		privData->buttonNRs = 2;
		privData->rect[1].x = CtrlData[3].x;
		privData->rect[1].y = CtrlData[3].y;
		privData->rect[1].w = CtrlData[3].w;
		privData->rect[1].h = CtrlData[3].h;
	} else {
		privData->IDCSelected = IDC_BUTTON_START;
		privData->buttonNRs = 1;
	}
	db_msg("buttonNRs is %d\n", privData->buttonNRs);
	
	privData->flag_end_key = info->flag_end_key;
	privData->linecTitle = info->linecTitle;
	privData->linecItem = info->linecItem;
	privData->bgcWidget = info->bgcWidget;
	privData->fgcWidget = info->fgcWidget;
	privData->confirmCallback = info->confirmCallback;
	privData->confirmData = info->confirmData;
	privData->msg4ConfirmCallback = info->msg4ConfirmCallback;
	privData->pLogFont = info->pLogFont;
	privData->title = info->title;
	privData->text = info->text;
	privData->buttonStr = info->buttonStr;
	
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_BACKGROUND, privData->boxBgPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_CONFIRM, privData->buttonConfirmPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_CANCEL, privData->buttonCancelPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_LINE, privData->linePic);
	
	retval = DialogBoxIndirectParam(&MsgBoxDlg, hParentWnd, msgBoxProc, (LPARAM)privData);
	db_msg("retval is %d\n", retval);
	
	if (privData->boxBgPic.bmBits)
		UnloadBitmap(&privData->boxBgPic);
	if (privData->buttonConfirmPic.bmBits)
		UnloadBitmap(&privData->buttonConfirmPic);
	if (privData->buttonCancelPic.bmBits)
		UnloadBitmap(&privData->buttonCancelPic);
	if (privData->linePic.bmBits)
		UnloadBitmap(&privData->linePic);

	if(privData)
		free(privData);

	free(CtrlData);
	return retval;
}
