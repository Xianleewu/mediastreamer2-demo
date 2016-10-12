
#include "include/dateSetting.h"
#undef LOG_TAG
#define LOG_TAG "dateSetting.cpp"
#include "debug.h"

#ifdef MESSAGE_FRAMEWORK
#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"
#endif



void getSystemDate(date_t* date)
{
	struct tm * ptm;
	getDateTime(&ptm);

	date->year = ptm->tm_year + 1900;
	date->month = ptm->tm_mon + 1;
	date->day = ptm->tm_mday;
	date->hour = ptm->tm_hour;
	date->minute = ptm->tm_min;
	date->second = ptm->tm_sec;
}

static void getCurrentSetDate(HWND hDlg, date_t* date)
{
	char timeStr[5] = {0};

	GetWindowText(GetDlgItem(hDlg, IDC_YEAR), timeStr, sizeof(timeStr));
	date->year = atoi(timeStr);
	bzero(timeStr, sizeof(timeStr));

	GetWindowText(GetDlgItem(hDlg, IDC_MONTH), timeStr, sizeof(timeStr));
	date->month = atoi(timeStr);

	GetWindowText(GetDlgItem(hDlg, IDC_DAY), timeStr, sizeof(timeStr));
	date->day = atoi(timeStr);

	GetWindowText(GetDlgItem(hDlg, IDC_HOUR), timeStr, sizeof(timeStr));
	date->hour = atoi(timeStr);

	GetWindowText(GetDlgItem(hDlg, IDC_MINUTE), timeStr, sizeof(timeStr));
	date->minute = atoi(timeStr);

	GetWindowText(GetDlgItem(hDlg, IDC_SECOND), timeStr, sizeof(timeStr));
	date->second = atoi(timeStr);

	db_msg("current set date is %04d-%02d-%02d--%02d-%02d-%02d\n", date->year, date->month, date->day,
			date->hour, date->minute, date->second);
}

void setSystemDate(date_t* date)
{
	struct tm tm;

	tm.tm_year = date->year - 1900;
	tm.tm_mon = date->month - 1;
	tm.tm_mday = date->day;
	tm.tm_hour = date->hour;
	tm.tm_min = date->minute;
	tm.tm_sec = date->second;
	tm.tm_wday = 0;
	tm.tm_yday = 0;
	tm.tm_isdst = 0;

	setDateTime(&tm);
}

static void SetBorderColor(HWND hDlg, unsigned int IDC, unsigned int select_flg)
{
	RECT rect;
	dateSettingData_t* configData;
	HWND hCtrl;

	configData = (dateSettingData_t*)GetWindowAdditionalData(hDlg);
	hCtrl = GetDlgItem(hDlg, IDC);

	HDC hdc = GetDC(hCtrl);

	GetClientRect(hCtrl, &rect);
	if (select_flg == SELECT_BORDER){
		SetPenColor(hdc, configData->borderc_selected );
	}
	else if (select_flg == UNSELECT_BORDER){
		SetPenColor(hdc, configData->borderc_normal );
	}

	Rectangle (hdc, rect.left, rect.top, rect.right-1, rect.bottom-1);

	EndPaint(hCtrl, hdc);
}

static int getMonthDays(int year, int month)
{
	int days;
	switch (month){
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		days = 31;
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		days = 30;
		break;
	case 2:
		if ((year % 400 == 0) || ((year % 4 == 0 && year % 100 != 0)))
			days = 29;
		else 
			days = 28;
		break;
	default:
		days = -1;
		break;
	}

	return days;
}

static void add(HWND hDlg, unsigned int IDCSelected, int addEnd)
{
	int value;
	char timeStr[5] = {0};

	GetWindowText(GetDlgItem(hDlg, IDCSelected), timeStr, sizeof(timeStr));
	value = atoi(timeStr);

	value += addEnd;
	switch(IDCSelected) {
	case IDC_YEAR:
		if(value < 2016)
			value = 2016;
		if(value >= 2036)
			value = 2016;
		break;
	case IDC_MONTH:
		if(value > 12)
			value = 1;
		else if(value < 1)
			value = 12;
		break;
	case IDC_DAY:
		{
			int year, month, days;	
			GetWindowText(GetDlgItem(hDlg, IDC_YEAR), timeStr, sizeof(timeStr));
			year = atoi(timeStr);
			GetWindowText(GetDlgItem(hDlg, IDC_MONTH), timeStr, sizeof(timeStr));
			month = atoi(timeStr);
			days = getMonthDays(year, month);
			db_msg("year-%d-month-%d have %d days\n", year, month, days);
			if(value > days)
				value = 1;
			else if(value < 1)
				value = days;
		}
		break;
	case IDC_HOUR:
		if(value > 23)
			value = 0;
		else if(value < 0)
			value = 23;
		break;
	case IDC_MINUTE:
	case IDC_SECOND:
		if(value > 59)
			value = 0;
		else if(value < 0)
			value = 59;
		break;
	}

	if(IDCSelected == IDC_YEAR)
		sprintf(timeStr, "%04d", value);
	else
		sprintf(timeStr, "%02d", value);

	SetWindowText(GetDlgItem(hDlg, IDCSelected), timeStr);
}

static int dateSettingProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)  
{
	static unsigned int IDCSelected = IDC_YEAR;
	static unsigned long long tickCnt;
	dateSettingData_t* configData;
	db_msg("message is %s\n", Message2Str(message));
	switch(message) {
	case MSG_INITDIALOG:
		{
			configData = (dateSettingData_t*)lParam;
			if(configData == NULL) {
				db_error("configData is NULL\n");	
				EndDialog(hDlg, -1);
			}
			SetWindowAdditionalData(hDlg, (DWORD)configData);

			for (unsigned int i = 0; i < 7; i++){
				SetWindowElementAttr(GetDlgItem (hDlg, IDC_LABEL_START + i ), WE_FGC_WINDOW , PIXEL2DWORD(HDC_SCREEN, configData->fgc_label) ); //0xFFDCDCDC
			}
#ifdef USE_NEWUI
			SetWindowElementAttr(GetDlgItem (hDlg, IDC_LABEL_START + 13 ), WE_FGC_WINDOW , PIXEL2DWORD(HDC_SCREEN, configData->fgc_label) ); //0xFFDCDCDC
#endif
			for (unsigned int i = 0; i < 6; i++){
				SetWindowElementAttr(GetDlgItem (hDlg, IDC_START + i), WE_FGC_WINDOW, PIXEL2DWORD(HDC_SCREEN, configData->fgc_number) );//0xFF919191
			}

			SetWindowBkColor(hDlg, configData->bgc_widget);
			SetWindowFont(hDlg, configData->pLogFont);
		}
		break;
	case MSG_FONTCHANGED:
		{
			PLOGFONT pLogFont;
			pLogFont = GetWindowFont(hDlg);
			for(unsigned int i = 0; i < 7; i++) {
				SetWindowFont(GetDlgItem(hDlg, IDC_LABEL_START + i), pLogFont);
			}
#ifdef USE_NEWUI
			SetWindowFont(GetDlgItem(hDlg, IDC_LABEL_START + 13), pLogFont);
#endif

		}
		break;
	case MSG_PAINT:
		{
			RECT rect;
			HDC hdc = BeginPaint(hDlg);

			configData = (dateSettingData_t*)GetWindowAdditionalData(hDlg);
			GetClientRect(GetDlgItem(hDlg, IDC_LABEL_TITLE), &rect);
			SetPenColor(hdc, configData->linec_title);

			LineEx(hdc, 0, RECTH(rect), RECTW(rect), RECTH(rect));
			SetBorderColor(hDlg, IDCSelected, SELECT_BORDER);
			for(unsigned int i = 0; i < 6; i++) {
				if(IDC_START + i != IDCSelected)	
					SetBorderColor(hDlg, IDC_START + i, UNSELECT_BORDER);
			}
			EndPaint(hDlg, hdc);
		}
		break;
	case MSG_KEYDOWN:
		{
			if(wParam == CDR_KEY_MODE) {
				tickCnt = GetTickCount();
			}
		}
		break;
	case MSG_KEYUP:
		{
			switch(wParam) {
			case CDR_KEY_OK:
				IDCSelected++;
				if(IDCSelected > IDC_END)
					IDCSelected = IDC_START;
				db_msg("IDCSelected is %d\n", IDCSelected);
				InvalidateRect(hDlg, NULL, FALSE);
				break;
			case CDR_KEY_LEFT:
				add(hDlg, IDCSelected, -1);
				break;
			case CDR_KEY_RIGHT:
				add(hDlg, IDCSelected, 1);
				break;
			case CDR_KEY_MODE:
				if((GetTickCount() - tickCnt ) < 50) 
				{
					date_t date;
					getCurrentSetDate(hDlg, &date);
				#ifdef MESSAGE_FRAMEWORK
					{
						WRTC_DATE_TIME dt;
						
						memset(&dt, 0x0, sizeof(dt));
						dt.year = date.year;
						dt.month = date.month;
						dt.day = date.day;
						dt.hour = date.hour;
						dt.minute = date.minute;
						dt.second = date.second;
						
						fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_SET_DATE_TIME_REQ, &dt, sizeof(dt));
					}
				#else
					setSystemDate(&date);
				#endif
				}
				EndDialog(hDlg, 0);
				break;
			}
		}
		break;
	}
	return DefaultDialogProc (hDlg, message, wParam, lParam);
}

int dateSettingDialog(HWND hParent, dateSettingData_t* configData)
{
#ifdef USE_NEWUI
	CTRLDATA ctrlData[14];
	LoadBitmap (HDC_SCREEN, &date_bk_bmp, "/system/res/others/date_bk.png"); 
#else
	CTRLDATA ctrlData[13];
#endif
	DLGTEMPLATE dlgDateSetting;
	unsigned int intval;
	memset(ctrlData, 0, sizeof(ctrlData));
	
	intval = configData->rect.h - configData->titleRectH - configData->boxH;
	intval = configData->titleRectH + intval / 3;
	db_msg("intval is %d\n", intval);
#ifdef USE_NEWUI
	for(unsigned int i = 1; i < 13; i++) {
#else
	for(unsigned int i = 0; i < 13; i++) {
#endif
		ctrlData[i].class_name = CTRL_STATIC;
		ctrlData[i].dwStyle = SS_CENTER | SS_VCENTER | WS_VISIBLE;
		ctrlData[i].caption = "";
		ctrlData[i].dwExStyle = WS_EX_TRANSPARENT;
		ctrlData[i].werdr_name = NULL;
		ctrlData[i].we_attrs = NULL;
		ctrlData[i].id	= IDC_LABEL_START + i;

		ctrlData[i].y = intval;
		ctrlData[i].h = configData->boxH;
	}
	ctrlData[INDEX_LABEL_TITLE].caption = configData->title;
	ctrlData[INDEX_LABEL_YEAR].caption = configData->year;
	ctrlData[INDEX_LABEL_MONTH].caption = configData->month;
	ctrlData[INDEX_LABEL_DAY].caption = configData->day;
	ctrlData[INDEX_LABEL_HOUR].caption = configData->hour;
	ctrlData[INDEX_LABEL_MINUTE].caption = configData->minute;
	ctrlData[INDEX_LABEL_SECOND].caption = configData->second;

	ctrlData[INDEX_LABEL_TITLE].x = 0;
	ctrlData[INDEX_LABEL_TITLE].y = 0;
	ctrlData[INDEX_LABEL_TITLE].w = configData->rect.w;
	ctrlData[INDEX_LABEL_TITLE].h = configData->titleRectH;

	ctrlData[INDEX_YEAR].x = configData->hBorder;
	ctrlData[INDEX_YEAR].w = configData->yearW;
	ctrlData[INDEX_LABEL_YEAR].x = ctrlData[INDEX_YEAR].x +  ctrlData[INDEX_YEAR].w + 2;
	ctrlData[INDEX_LABEL_YEAR].w = configData->dateLabelW;

	ctrlData[INDEX_MONTH].x = ctrlData[INDEX_LABEL_YEAR].x +  ctrlData[INDEX_LABEL_YEAR].w + 2;
	ctrlData[INDEX_MONTH].w = configData->numberW;
	ctrlData[INDEX_LABEL_MONTH].x = ctrlData[INDEX_MONTH].x +  ctrlData[INDEX_MONTH].w + 2;
	ctrlData[INDEX_LABEL_MONTH].w = configData->dateLabelW;

	ctrlData[INDEX_DAY].x = ctrlData[INDEX_LABEL_MONTH].x +  ctrlData[INDEX_LABEL_MONTH].w + 2;
	ctrlData[INDEX_DAY].w = configData->numberW;
	ctrlData[INDEX_LABEL_DAY].x = ctrlData[INDEX_DAY].x +  ctrlData[INDEX_DAY].w + 2;
	ctrlData[INDEX_LABEL_DAY].w = configData->dateLabelW;

	ctrlData[INDEX_HOUR].x = ctrlData[INDEX_LABEL_DAY].x +  ctrlData[INDEX_LABEL_DAY].w + 2;
	ctrlData[INDEX_HOUR].w = configData->numberW;
	ctrlData[INDEX_LABEL_HOUR].x = ctrlData[INDEX_HOUR].x +  ctrlData[INDEX_HOUR].w + 2;
	ctrlData[INDEX_LABEL_HOUR].w = configData->dateLabelW;

	ctrlData[INDEX_MINUTE].x = ctrlData[INDEX_LABEL_HOUR].x +  ctrlData[INDEX_LABEL_HOUR].w + 2;
	ctrlData[INDEX_MINUTE].w = configData->numberW;
	ctrlData[INDEX_LABEL_MINUTE].x = ctrlData[INDEX_MINUTE].x +  ctrlData[INDEX_MINUTE].w + 2;
	ctrlData[INDEX_LABEL_MINUTE].w = configData->dateLabelW;

	ctrlData[INDEX_SECOND].x = ctrlData[INDEX_LABEL_MINUTE].x +  ctrlData[INDEX_LABEL_MINUTE].w + 2;
	ctrlData[INDEX_SECOND].w = configData->numberW;
	ctrlData[INDEX_LABEL_SECOND].x = ctrlData[INDEX_SECOND].x +  ctrlData[INDEX_SECOND].w + 2;
	ctrlData[INDEX_LABEL_SECOND].w = configData->dateLabelW;

#ifdef USE_NEWUI
	ctrlData[0].class_name = CTRL_STATIC;
	ctrlData[0].dwStyle =   SS_CENTER | SS_VCENTER | WS_VISIBLE | SS_BITMAP;
	ctrlData[0].dwExStyle = WS_EX_TRANSPARENT;
	ctrlData[0].werdr_name = NULL;
	ctrlData[0].we_attrs = NULL;
	ctrlData[0].id	= IDC_LABEL_START ;
	ctrlData[0].dwAddData=(DWORD)&date_bk_bmp;

	CTRLDATA cdtest;
	ctrlData[13].class_name = CTRL_STATIC;
	//ctrlData[13].dwStyle =  SS_CENTER | SS_VCENTER | WS_VISIBLE | SS_BITMAP;
	ctrlData[13].dwStyle =  SS_CENTER | SS_VCENTER | WS_VISIBLE;
	ctrlData[13].caption = configData->title;
	ctrlData[13].dwExStyle = WS_EX_TRANSPARENT;
	ctrlData[13].werdr_name = NULL;
	ctrlData[13].we_attrs = NULL;
	ctrlData[13].id	= IDC_LABEL_START +13;
	ctrlData[13].x=configData->rect.w/4;
	ctrlData[13].y=0;
	ctrlData[13].w = configData->rect.w/2;
	ctrlData[13].h = configData->titleRectH;
	//ctrlData[13].dwAddData=(DWORD)&date_bk_bmp;

#endif
	date_t date;
	char dateString[6][5];
	getSystemDate(&date);
	sprintf(dateString[0],"%04u", date.year);
	sprintf(dateString[1],"%02u", date.month);
	sprintf(dateString[2],"%02u", date.day);
	sprintf(dateString[3],"%02u", date.hour);
	sprintf(dateString[4],"%02u", date.minute);
	sprintf(dateString[5],"%02u", date.second);	
	ctrlData[INDEX_YEAR].caption = dateString[0];
	ctrlData[INDEX_MONTH].caption = dateString[1];
	ctrlData[INDEX_DAY].caption = dateString[2];
	ctrlData[INDEX_HOUR].caption = dateString[3];
	ctrlData[INDEX_MINUTE].caption = dateString[4];
	ctrlData[INDEX_SECOND].caption = dateString[5];

	memset(&dlgDateSetting, 0, sizeof(dlgDateSetting));
	dlgDateSetting.dwStyle = WS_VISIBLE;
	dlgDateSetting.dwExStyle = WS_EX_NONE;
	dlgDateSetting.x = configData->rect.x;
	dlgDateSetting.y = configData->rect.y;
	dlgDateSetting.w = configData->rect.w;
	dlgDateSetting.h = configData->rect.h;
	dlgDateSetting.caption = "";
	dlgDateSetting.controlnr = TABLESIZE(ctrlData);
	dlgDateSetting.controls = ctrlData;

	return DialogBoxIndirectParam (&dlgDateSetting, hParent, dateSettingProc, (LPARAM)configData);
}
