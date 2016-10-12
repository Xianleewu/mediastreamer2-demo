
#include <utils/Thread.h>
#include "resourceManager.h"
#include "cdr_message.h"
#include "Dialog.h"
#undef LOG_TAG
#define LOG_TAG "Dialog.cpp"
#include "debug.h"

using namespace android;

typedef struct {
	enum LANG_STRINGS TitleCmd;
	enum LANG_STRINGS TextCmd;
}labelStringCmd_t;

labelStringCmd_t labelStringCmd[] = {
	{LANG_LABEL_TIPS,				LANG_LABEL_NO_TFCARD},
	{LANG_LABEL_WARNING,			LANG_LABEL_TFCARD_FULL},
	{LANG_LABEL_WARNING,			LANG_LABEL_PLAYBACK_FAIL},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_LOW_POWER_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_10S_SHUTDOWN},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_SHUTDOWN_NOW},
	{LANG_LABEL_SHUTDOWN_TITLE,		LANG_LABEL_30S_NOWORK_SHUTDOWN},
	{LANG_LABEL_TIPS,				LANG_LABEL_FILELIST_EMPTY},
};

int getTipLabelData(tipLabelData_t* tipLabelData)
{
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	retval = rm->getResRect(ID_TIPLABEL, tipLabelData->rect);
	if(retval < 0) {
		db_error("get tiplabel rect failed\n");	
		return -1;
	}
	retval = rm->getResIntValue(ID_TIPLABEL, INTVAL_TITLEHEIGHT);
	if(retval < 0) {
		db_error("get tiplabel rect failed\n");	
		return -1;
	}
	tipLabelData->titleHeight = retval;
	db_msg("titleHeight is %d\n", tipLabelData->titleHeight);

	tipLabelData->bgc_widget =	rm->getResColor(ID_TIPLABEL, COLOR_BGC);
	tipLabelData->fgc_widget	=	rm->getResColor(ID_TIPLABEL, COLOR_FGC);
	tipLabelData->linec_title =	rm->getResColor(ID_TIPLABEL, COLOR_LINEC_TITLE);

	tipLabelData->pLogFont = rm->getLogFont();
#ifdef USE_NEWUI
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_BACKGROUND, tipLabelData->bgPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_LINE, tipLabelData->linePic);
#endif
	return 0;
}

/*
 *  if endLabelKeyUp is true, then end the label while any key up
 *  if endLabelKeyUp is flase, then end the label while any key down
 * */
int ShowTipLabel(HWND hParent, enum labelIndex index, bool endLabelKeyUp, unsigned int timeOutMs)
{
	tipLabelData_t tipLabelData;
	ResourceManager* rm;

	if(index >= TABLESIZE(labelStringCmd)) {
		db_error("invalid index: %d\n", index);
		return -1;
	}

	rm = ResourceManager::getInstance();
	memset(&tipLabelData, 0, sizeof(tipLabelData) );

	if(getTipLabelData(&tipLabelData) < 0) {
		db_error("get tipLabelData failed\n");	
		return -1;
	}

	tipLabelData.title = rm->getLabel(labelStringCmd[index].TitleCmd);
	if(!tipLabelData.title) {
		db_error("get LANG_LABEL_LOW_POWER_TITLE failed\n");
		return -1;
	}

	tipLabelData.text = rm->getLabel(labelStringCmd[index].TextCmd);
		if(!tipLabelData.text) {
			db_error("get LANG_LABEL_LOW_POWER_TEXT failed\n");
			return -1;
	}
	tipLabelData.endLabelKeyUp = endLabelKeyUp;
	tipLabelData.timeoutMs = timeOutMs;
	if(index == LABEL_LOW_POWER_SHUTDOWN || 
			index == LABEL_SHUTDOWN_NOW  || 
			index == LABEL_30S_NOWORK_SHUTDOWN) {
		tipLabelData.timeoutMs = 2 * 1000;
	} else if(index == LABEL_10S_SHUTDOWN) {
		tipLabelData.timeoutMs = 10 * 1000;
	}

#ifdef USE_NEWUI	
	int ret = showTipLabel(hParent, &tipLabelData);

	if(tipLabelData.bgPic.bmBits != NULL) {
		UnloadBitmap(&tipLabelData.bgPic);
	}
	if(tipLabelData.linePic.bmBits != NULL) {
		UnloadBitmap(&tipLabelData.linePic);
	}

	return ret;
#else
	return showTipLabel(hParent, &tipLabelData);
#endif
}

#ifdef USE_NEWUI
int showFirmwareDialog(HWND hParent)
{
	MessageBox_t messageBoxData;
	ResourceManager* rm;

	memset(&messageBoxData, 0, sizeof(messageBoxData));
	rm = ResourceManager::getInstance();

	messageBoxData.flag_end_key = 1;	/* End Dialog when key up */
	messageBoxData.dwStyle = MB_HAVE_TITLE | MB_HAVE_TEXT;

	messageBoxData.title = rm->getResMenuItemString(ID_MENU_LIST_FIRMWARE);
	if(messageBoxData.title == NULL) {
		db_msg("get firmware title failed\n");
		return -1;
	}
	messageBoxData.text = rm->getResSubMenuCurString(ID_MENU_LIST_FIRMWARE);
	if(messageBoxData.text == NULL) {
		db_msg("get firmware text failed\n");
		return -1;
	}

	messageBoxData.buttonStr[0] = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(messageBoxData.buttonStr[0] == NULL) {
		db_msg("get button ok text failed\n");
		return -1;
	}
	
	messageBoxData.pLogFont = rm->getLogFont();
	rm->getResRect(ID_WARNNING_MB, messageBoxData.rect);
	messageBoxData.bgcWidget	= rm->getResColor(ID_WARNNING_MB, COLOR_BGC);
	messageBoxData.fgcWidget	= rm->getResColor(ID_WARNNING_MB, COLOR_FGC);
	messageBoxData.linecTitle	= rm->getResColor(ID_WARNNING_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem	= rm->getResColor(ID_WARNNING_MB, COLOR_LINEC_ITEM);

	return showMessageBox(hParent, &messageBoxData);
}
#endif

static int shutdownDialog(HWND hParent)
{
	MessageBox_t messageBoxData;
	ResourceManager* rm;

	memset(&messageBoxData, 0, sizeof(messageBoxData));
	rm = ResourceManager::getInstance();

	messageBoxData.flag_end_key = 1;	/* End Dialog when key up */
	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;

	messageBoxData.title = rm->getLabel(LANG_LABEL_SHUTDOWN_TITLE);
	if(messageBoxData.title == NULL) {
		db_msg("get shutdown title failed\n");
		return -1;
	}
	messageBoxData.text = rm->getLabel(LANG_LABEL_SHUTDOWN_TEXT);
	if(messageBoxData.text == NULL) {
		db_msg("get shutdown text failed\n");
		return -1;
	}

	messageBoxData.buttonStr[0] = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(messageBoxData.buttonStr[0] == NULL) {
		db_msg("get button ok text failed\n");
		return -1;
	}
	messageBoxData.buttonStr[1] = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
	if(messageBoxData.buttonStr[1] == NULL) {
		db_msg("get button ok text failed\n");
		return -1;
	}
	
	messageBoxData.pLogFont = rm->getLogFont();
	rm->getResRect(ID_WARNNING_MB, messageBoxData.rect);
	messageBoxData.bgcWidget	= rm->getResColor(ID_WARNNING_MB, COLOR_BGC);
	messageBoxData.fgcWidget	= rm->getResColor(ID_WARNNING_MB, COLOR_FGC);
	messageBoxData.linecTitle	= rm->getResColor(ID_WARNNING_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem	= rm->getResColor(ID_WARNNING_MB, COLOR_LINEC_ITEM);

	return showMessageBox(hParent, &messageBoxData);
}

int CdrDialog(HWND hParent, cdrDialogType_t type)
{
	switch(type) {
	case CDR_DIALOG_SHUTDOWN:
		return shutdownDialog(hParent);
		break;
	default:
		break;
	}

	return 0;
}

int CloseTipLabel(void)
{
	db_msg("close TipLabel\n");
	BroadcastMessage(MSG_CLOSE_TIP_LABEL, 0, 0);
	return 0;
}



static int createImageWidget(enum ResourceID resID, HWND hParent, BITMAP* bmp)
{
	int retval;
	CDR_RECT rect;
	HWND retWnd;
	ResourceManager *rm;
	
	rm = ResourceManager::getInstance();

	retval = rm->getResRect(resID, rect);
	if(retval < 0) {
		db_error("get rect failed, resID is %d\n", resID);
		return -1;
	}
	db_msg("%d %d %d %d\n", rect.x, rect.y, rect.w, rect.h);
/*
	retval = rm->getResBmp(resID, BMPTYPE_BASE, *bmp);
	if(retval < 0) {
		db_error("loadb bitmap failed, resID is %d\n", resID);
		return -1;	
	}
*/	
	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE ,
			WS_EX_NONE,
			resID,
			rect.x, rect.y, rect.w, rect.h,
			hParent, (DWORD)bmp);
	if(retWnd == HWND_INVALID) {
		db_error("create widget failed\n");
		return -1;
	}

	return 0;
}

static BITMAP poweroffBmp;
static int showPowerOffWidget(HWND hParent)
{
	int ret;
	HWND retWnd;
	ret = createImageWidget(ID_POWEROFF, hParent, &poweroffBmp);
	if(ret < 0) {
		db_error("create imageWidget failed\n");
		return ret;
	}
	retWnd = GetDlgItem(hParent, ID_POWEROFF);
	SendMessage(GetDlgItem(hParent, ID_POWEROFF), MSG_PAINT, 0 ,0);
	//ShowWindow(GetDlgItem(hParent, ID_POWEROFF), SW_SHOWNORMAL);
	//UpdateWindow(GetDlgItem(hParent, ID_POWEROFF), true);

	return 0;
}


void bmpInfo(unsigned char *mem, int *w, int *h, int *bpp)
{
	int width, height;

	width = *(mem+0x12)+*(mem+0x13)*0x100+*(mem+0x14)*0x10000+*(mem+0x15)*0x1000000;
	height = *(mem+0x16)+*(mem+0x17)*0x100+*(mem+0x18)*0x10000+*(mem+0x19)*0x1000000;
	height = (height) >0 ? (height) : -(height);
	*bpp = *(mem+0x1c)+*(mem+0x1f)*0x100;
	if (0x20 == *bpp)
	{
		*bpp = 32;
	}
	else if (0x18 == *bpp)
	{
		*bpp = 24;
	}
	else if (0xf == *bpp)
	{
		*bpp = 16;
	}
	*w = width;
	*h = height;
}

int getBmpSize(unsigned char *data)
{
	int w, h, bpp;
	bmpInfo(data, &w, &h, &bpp);
	db_msg("w:%d h:%d bpp:%d\n", w, h, bpp);
	return (w*h*bpp/4);
}

int showImageWidget(HWND hParent, imageWidgetType_t type, unsigned char *data)
{
	switch(type) {
	case IMAGEWIDGET_POWEROFF:
		LoadBitmapFromMem(HDC_SCREEN, &poweroffBmp, data, 512*1024, "jpg");
		return showPowerOffWidget(hParent);
		break;
	}
		
	return 0;
}

