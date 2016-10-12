
#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "cdr_widgets.h"
#undef LOG_TAG
#define LOG_TAG "TipLabel.cpp"
#include "debug.h"

#define IDC_TITLE	100
#define IDC_TEXT	101
#define ONE_SHOT_TIMER	200
#define TIMEOUT_TIMER	201

CTRLDATA ctrlData[2] = {
	{
		CTRL_STATIC,
		WS_VISIBLE | SS_CENTER | SS_VCENTER,
		0, 0, 0, 0,
		IDC_TITLE,
		"",
		0,
		WS_EX_NONE | WS_EX_TRANSPARENT,
		NULL, NULL
	},

	{
		CTRL_STATIC,
		WS_VISIBLE | SS_CENTER,
		0, 0, 0, 0,
		IDC_TEXT,
		"",
		0,
		WS_EX_NONE | WS_EX_TRANSPARENT,
		NULL, NULL
	}
};

static BOOL timerCallback(HWND hDlg, int id, DWORD data)
{
	tipLabelData_t* tipLabelData;

	tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);

	db_msg("timerCallback\n");
	if(id == ONE_SHOT_TIMER) {
		db_msg("one shot timer timerCallback\n");
		if(tipLabelData->callback != NULL)
			(*tipLabelData->callback)(hDlg, tipLabelData->callbackData);
		db_msg("one shot timer timerCallback\n");
	} else if(id == TIMEOUT_TIMER) {
		db_msg("timeout timer timerCallback\n");
		SendNotifyMessage(hDlg, MSG_CLOSE_TIP_LABEL, 0, 0);
	}

	return FALSE;
}

#ifdef USE_NEWUI
int FillBoxBg(HDC hdc, RECT &rect, PBITMAP pBg)
{
	CDR_RECT tmpRect;

	tmpRect.x = 0;
	tmpRect.y = 0;
	tmpRect.w = pBg->bmWidth/2;
	tmpRect.h = pBg->bmHeight/2;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, 1, 1, tmpRect.w, tmpRect.h);
	
	tmpRect.x = RECTW(rect)-tmpRect.w;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, pBg->bmWidth-tmpRect.w, 1, tmpRect.w, tmpRect.h);

	tmpRect.x = tmpRect.w;
	tmpRect.w = RECTW(rect) - 2*tmpRect.w;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, 12, 1, pBg->bmWidth-24, tmpRect.h);

	tmpRect.x = 0;
	tmpRect.y = pBg->bmHeight/2;
	tmpRect.w = pBg->bmWidth/2;
	tmpRect.h = RECTH(rect) - 2*tmpRect.h;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, 1, 12, tmpRect.w, pBg->bmHeight-24);
	
	tmpRect.x = RECTW(rect)-tmpRect.w;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, pBg->bmWidth-tmpRect.w, 12, tmpRect.w, pBg->bmHeight-24);

	tmpRect.x = pBg->bmWidth/2;
	tmpRect.y = pBg->bmHeight/2;
	tmpRect.w = RECTW(rect) - 2*tmpRect.x;
	tmpRect.h = RECTH(rect) - 2*tmpRect.y;			
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, 12, 12, pBg->bmHeight-24, pBg->bmHeight-24);

	tmpRect.x = 0;
	tmpRect.y = RECTH(rect)-pBg->bmHeight/2;
	tmpRect.w = pBg->bmWidth/2;
	tmpRect.h = pBg->bmHeight/2;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, 1, pBg->bmHeight-tmpRect.h, tmpRect.w, tmpRect.h);

	tmpRect.x = RECTW(rect)-tmpRect.w;
	tmpRect.y = RECTH(rect)-tmpRect.h;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, pBg->bmWidth-tmpRect.w, pBg->bmHeight-tmpRect.h, tmpRect.w, tmpRect.h);

	tmpRect.x = tmpRect.w;
	tmpRect.w = RECTW(rect) - 2*tmpRect.w;
	FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h, 
		pBg, 12, pBg->bmHeight-tmpRect.h, pBg->bmWidth-24, tmpRect.h);

	return 0;
}
#endif

static int DialogProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case MSG_INITDIALOG:
		{
			tipLabelData_t* tipLabelData;

			tipLabelData = (tipLabelData_t*)lParam;
			if(!tipLabelData) {
				db_error("invalid tipLabelData\n");
				return -1;
			}
			SetWindowAdditionalData(hDlg, (DWORD)tipLabelData);

			if(tipLabelData->pLogFont) {
				SetWindowFont(hDlg, tipLabelData->pLogFont);
			}

			SetWindowBkColor(hDlg, tipLabelData->bgc_widget);
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TITLE), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, tipLabelData->fgc_widget) );
			SetWindowElementAttr(GetDlgItem(hDlg, IDC_TEXT), WE_FGC_WINDOW,  PIXEL2DWORD(HDC_SCREEN, tipLabelData->fgc_widget) );

			if(tipLabelData->callback != NULL) {
				db_msg("start timer\n");
				SetTimerEx(hDlg, ONE_SHOT_TIMER, 5, timerCallback);
			}
			if(tipLabelData->timeoutMs != TIME_INFINITE) {
				db_msg("set timeOut %d ms\n", tipLabelData->timeoutMs);
				SetTimerEx(hDlg, TIMEOUT_TIMER, tipLabelData->timeoutMs / 10, timerCallback);
			}
		}
		break;
	case MSG_PAINT:
		{
			RECT rect;
			CDR_RECT tmpRect;
			HDC hdc; 
			tipLabelData_t* tipLabelData;

			hdc = BeginPaint(hDlg);

			tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
			#ifdef USE_NEWUI
			if (tipLabelData->bgPic.bmBits) {
				GetClientRect(hDlg, &rect);
				FillBoxBg(hdc, rect, &tipLabelData->bgPic);
			}
			
			if (tipLabelData->linePic.bmBits) {
				GetClientRect(GetDlgItem(hDlg, IDC_TITLE), &rect);
				#if 1
				FillBitmapPartInBox(hdc, 6, RECTH(rect), RECTW(rect)-12, 2, 
					&tipLabelData->linePic, 4, 1, 4, 2);
				#else
				tmpRect.x = 0;
				tmpRect.y = RECTH(rect);
				tmpRect.w = tipLabelData->linePic.bmWidth/2;
				tmpRect.h = tipLabelData->linePic.bmHeight-1;
				FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
					&tipLabelData->linePic, 0, 1, tmpRect.w, tmpRect.h);

				tmpRect.x = RECTW(rect) - tmpRect.w;
				FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
					&tipLabelData->linePic, tipLabelData->linePic.bmWidth-tmpRect.w, 1, tmpRect.w, tmpRect.h);

				tmpRect.x = tmpRect.w;
				tmpRect.w = RECTW(rect)-2*tmpRect.w;
				FillBitmapPartInBox(hdc, tmpRect.x, tmpRect.y, tmpRect.w, tmpRect.h,
					&tipLabelData->linePic, 4, 1, 3, tmpRect.h);
				#endif
			}
			#else
			GetClientRect(GetDlgItem(hDlg, IDC_TITLE), &rect);
			SetPenColor(hdc, tipLabelData->linec_title );
			LineEx(hdc, 0, RECTH(rect) + 2, RECTW(rect), RECTH(rect) + 2);
			#endif

			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;		
			pLogFont = GetWindowFont(hDlg);
			if(pLogFont) {
				SetWindowFont(GetDlgItem(hDlg, IDC_TITLE), pLogFont);
				SetWindowFont(GetDlgItem(hDlg, IDC_TEXT), pLogFont);
			}
		}
		break;
	case MSG_KEYUP:
		{
			tipLabelData_t* tipLabelData;

			db_msg("key up, wParam is %d\n", wParam);
			tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
			if(tipLabelData->disableKeyEvent == true) {
				db_msg("disable key event\n");
				break;
			}
			if(tipLabelData->endLabelKeyUp == true)
				EndDialog(hDlg, 0);
		}
		break;
	case MSG_KEYDOWN:
		{
			tipLabelData_t* tipLabelData;

			db_msg("key down, wParam is %d\n", wParam);
			tipLabelData = (tipLabelData_t*)GetWindowAdditionalData(hDlg);
			if(tipLabelData->disableKeyEvent == true) {
				db_msg("disable key event\n");
				break;
			}
			if(tipLabelData->endLabelKeyUp == false)
				EndDialog(hDlg, 0);
		}
		break;
	case MSG_CLOSE_TIP_LABEL:
		db_info("MSG_CLOSE_LOWPOWER_DIALOG\n");
		if(IsTimerInstalled(hDlg, ONE_SHOT_TIMER) == TRUE) {
			KillTimer(hDlg, ONE_SHOT_TIMER);
		}
		if(IsTimerInstalled(hDlg, TIMEOUT_TIMER) == TRUE) {
			KillTimer(hDlg, TIMEOUT_TIMER);
		}
		EndDialog(hDlg, 0);
		break;
	}
	return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int showTipLabel(HWND hParent, tipLabelData_t* info)
{
	DLGTEMPLATE dlg;
	CDR_RECT rect;

	if(info == NULL) {
		db_error("invalid info\n");
		return -1;
	}
	memset(&dlg, 0, sizeof(dlg));

	rect = info->rect;

	ctrlData[0].x = 0;
	ctrlData[0].y = 0;
	ctrlData[0].w = rect.w;
	ctrlData[0].h = info->titleHeight;
	ctrlData[0].caption = info->title;

	ctrlData[1].x = 10;
	ctrlData[1].y = ctrlData[0].h + 30;
	ctrlData[1].w = rect.w - 20;
	ctrlData[1].h = rect.h - 10 - ctrlData[1].y;
	ctrlData[1].caption = info->text;
	
	dlg.dwStyle = WS_VISIBLE;
#ifdef USE_NEWUI
	dlg.dwExStyle = WS_EX_TRANSPARENT | WS_EX_AUTOSECONDARYDC;
#else
	dlg.dwExStyle = WS_EX_NONE;
#endif
	dlg.x = rect.x;
	dlg.y = rect.y;
	dlg.w = rect.w;
	dlg.h =	rect.h;
	dlg.caption = "";
	dlg.hIcon = 0;
	dlg.hMenu = 0;
	dlg.controlnr = 2;
	dlg.controls = ctrlData;
	dlg.dwAddData = 0;

	return DialogBoxIndirectParam(&dlg, hParent, DialogProc, (LPARAM)info);
}
