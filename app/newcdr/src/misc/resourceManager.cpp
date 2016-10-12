
#include "resourceManager.h"

#include <string.h>
#include <minigui/minigui.h>
#include <minigui/window.h>

#include <utils/Mutex.h>
#include <gps/GpsController.h>
#include <network/hotspot.h>
#include <network/wifiIf.h>
#include <network/ConnectivityServer.h>
#ifdef MODEM_ENABLE
#include <cellular/CellularController.h>
#endif

#include "cdr.h"
#include "resource_impl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ResourceManager.cpp"
#include "debug.h"

typedef struct {
	const char* mainkey;
	void* addr;
}configTable2;

typedef struct {
	const char* mainkey;
	const char* subkey;
	void* addr;
}configTable3;

using namespace android;

static ResourceManager *gInstance;
static Mutex gInstanceMutex;

void ResourceManager::setHwnd(unsigned int win_id, HWND hwnd)
{
	mHwnd[win_id] = hwnd;
}

ResourceManager::ResourceManager()
	: configFile("/data/windows.cfg")
	, configMenu("/data/menu.cfg")
	, defaultConfigFile("/res/cfg/windows.cfg")
	, defaultConfigMenu("/res/cfg/menu.cfg")
	, mCfgFileHandle(0)
	, mCfgMenuHandle(0)
	, mFontSize(16)
{
	memset(mHwnd, 0, sizeof(HWND)*WIN_CNT);
	db_msg("ResourceManager Constructor\n");
	if(getCfgMenuHandle() < 0) {
		db_info("restore menu.cfg\n");
		if(copyFile(defaultConfigMenu, configMenu) < 0) {
			db_error("copyFile config file failed\n");	
		}
		if(copyFile(defaultConfigFile, configFile) < 0) {
			db_error("copyFile config file failed\n");	
		}
	} else {
		if (verifyConfig() == false){
			db_info("restore menu.cfg\n");
			if(copyFile(defaultConfigMenu, configMenu) < 0) {
				db_error("copyFile config file failed\n");
			}
		}
		releaseCfgMenuHandle();
	}
}

ResourceManager::~ResourceManager()
{
	db_msg("ResourceManager Destructor\n");
}

int ResourceManager::initLangAndFont(void)
{

	lang = getLangFromConfigFile();
	if(lang == LANG_ERR) {
		db_error("get language failed\n");
		return -1;
	}

	if (mLogFont != NULL)
		DestroyLogFont(mLogFont);

	if(lang == LANG_TW) {
		mLogFont = CreateLogFont ("*", "ming", "BIG5",
				FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
				FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);	
	}
	else if(lang == LANG_EN || lang == LANG_CN){
		mLogFont = CreateLogFont("*", "fixed", "GB2312-0", 
				FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
				FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
	}
	else if (lang == LANG_RS) {
		mLogFont = CreateLogFont("*", "fixed", "RUSSAIN", 
				FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
				FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
	}
	else if(lang == LANG_JPN){ 
		mLogFont = CreateLogFont("*", "shift", FONT_CHARSET_JISX0208_1, 
				FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
				FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
	}
	else if(lang == LANG_KR){
		mLogFont = CreateLogFont("*", "euckr", FONT_CHARSET_EUCKR, 
				FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
				FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
	}

	initLabel(); /* init lang labels */
	SendMessage(mHwnd[WINDOWID_MAIN], MSG_RM_LANG_CHANGED, (WPARAM)&lang, 0);
	SendMessage(mHwnd[WINDOWID_STATUSBAR], MSG_RM_LANG_CHANGED, (WPARAM)&lang, 0);
	return 0;
}

ResourceManager* ResourceManager::getInstance()
{
	//	db_msg("getInstance\n");
	Mutex::Autolock _l(gInstanceMutex);
	if(gInstance != NULL) {
		return gInstance;
	}

	gInstance = new ResourceManager();
	return gInstance;
}

int ResourceManager::initStage1(void)
{
	int err_code;
	int retval = 0;

	db_msg("initStage1\n");

	if(getCfgFileHandle() < 0) {
		db_error("get file handle failed\n");
		return -1;
	}
	if(getCfgMenuHandle() < 0) {
		db_error("get menu handle failed\n");
		retval = -1;
		goto out;
	}
	
	err_code = getRectFromFileHandle("rect_screen", &rScreenRect);
	if(err_code != ETC_OK) {
		db_error("get screen rect failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}

	#ifndef USE_NEWUI
	if ((rScreenRect.w >= 800) && (rScreenRect.h >= 600)) {
		mFontSize = 24;
	}
	#endif

	err_code = getRectFromFileHandle("rect_record_preview1", &rRecordPreviewRect1);
	if(err_code != ETC_OK) {
		db_error("get record preview1 rect failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}

	err_code = getRectFromFileHandle("rect_record_preview2", &rRecordPreviewRect2);
	if(err_code != ETC_OK) {
		db_error("get record preview2 rect failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}

	if(initStatusBarResource() < 0) {
		db_error("init status bar resource failed\n");	
		retval = -1;
		goto out;
	}

	if(initS1MenuResource() < 0) {
		db_error("init stage 1 menu resoutce failed\n");	
		retval = -1;
		goto out;
	}

	return retval;

out:
	releaseCfgFileHandle();
	releaseCfgMenuHandle();
	return retval;
}

int ResourceManager::initS1MenuResource(void)
{
	configTable2 configTableMenuItem[] = {
#ifdef USE_NEWUI
		{CFG_MENU_VM,	(void*)&menuDataVM},
#endif
	};

	configTable2 configTableSwitch[] = {
		{CFG_MENU_TWM,		(void*)&menuDataTWMEnable},
		{CFG_MENU_AWMD,     (void*)&menuDataAWMDEnable},
	};

	int value;
	for(unsigned int i = 0; i < TABLESIZE(configTableMenuItem); i++) {
		value = getIntValueFromHandle(mCfgMenuHandle, configTableMenuItem[i].mainkey, "current");	
		if(value < 0) {
			db_error("get current %s failed\n", configTableMenuItem[i].mainkey);
			return -1;
		}
		((menuItem_t*)(configTableMenuItem[i].addr))->current = value;

		value = getIntValueFromHandle(mCfgMenuHandle, configTableMenuItem[i].mainkey, "count");	
		if(value < 0) {
			db_error("get count %s failed\n", configTableMenuItem[i].mainkey);
			return -1;
		}
		((menuItem_t*)(configTableMenuItem[i].addr))->count = value;
				db_msg("%s current is %d, count is %d\n", configTableMenuItem[i].mainkey, ((menuItem_t*)(configTableMenuItem[i].addr))->current, value);	
	}

	for(unsigned int i = 0; i < TABLESIZE(configTableSwitch); i++) {
		value = getIntValueFromHandle(mCfgMenuHandle, "switch", configTableSwitch[i].mainkey);
		if(value < 0) {
			db_error("get switch %s failed\n", configTableSwitch[i].mainkey);
			return -1;
		}
		if(value == 0) {
			*((bool*)configTableSwitch[i].addr) = false;
		} else {
			*((bool*)configTableSwitch[i].addr) = true;
		}
		db_msg("switch %s is %d\n", configTableSwitch[i].mainkey, value);
	}
	db_msg("menuDataVM.current=%d menuDataVM.count=%d\n", menuDataVM.current, menuDataVM.count);
	db_msg("menuDataTWMEnable=%d\n", menuDataTWMEnable);
	
	return 0;
}

int ResourceManager::notifyAll()
{
	dump();
	SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_REFRESH, 0, 0);
#ifdef USE_NEWUI
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VIDEO_PHOTO_VALUE, rStatusBarData.valueIcon.current, 0);
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VIDEO_MODE, rStatusBarData.modeIcon.current, 0);
#endif
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VOICE, rStatusBarData.audioIcon.current, 0);
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_GPS, rStatusBarData.gpsIcon.current, 0);
#ifdef MODEM_ENABLE
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_CELLULAR, rStatusBarData.cellularIcon.current, 0);
#endif
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_WIFI, rStatusBarData.wifiIcon.current, 0);
	SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_AP, rStatusBarData.apIcon.current, 0);
	SendMessage(mHwnd[WINDOWID_MAIN], MSG_RM_SCREENSWITCH, menuDataSS.current, 0);
	return 0;
}
int ResourceManager::initStage2(void)
{
	int retval = 0;

	db_msg("initStage2\n");

	initLangAndFont();
#ifdef MODEM_ENABLE
	creatRilSocket();
#endif

	if(getCfgFileHandle() < 0) {
		db_error("get file handle failed\n");
		return -1;
	}
	if(getCfgMenuHandle() < 0) {
		db_error("get menu handle failed\n");
		retval = -1;
		goto out;
	}

	if(initPlayBackPreviewResource() < 0) {
		db_error("init PlayBackPreview resource failed\n");
		retval = -1;
		goto out;
	}

	if(initPlayBackResource() < 0) {
		db_error("init PlayBack resource failed\n");
		retval = -1;
		goto out;
	}
#ifdef USE_NEWUI
	if(initMenuBarResource() < 0) {
		db_error("init Menu Bar resource failed\n");
		retval = -1;
		goto out;
	}
#endif
	if(initMenuListResource() < 0) {
		db_error("init Menu List resource failed\n");
		retval = -1;
		goto out;
	}
	/* ----------------------- */

	if(initMenuResource() < 0) {
		db_error("init menu resource failed\n");
		retval = -1;
		goto out;
	}

	if(initOtherResource() < 0) {
		db_error("init other resource failed\n");
		retval = -1;
		goto out;	
	}
#ifdef USE_NEWUI
	rStatusBarData.valueIcon.current = menuDataVQ.current + 1;
	rStatusBarData.modeIcon.current = menuDataVM.current + 1;
#endif
	start_connectivity_server();

	if(menuDataVolumeEnable == true)
		rStatusBarData.audioIcon.current = 1;
	else
		rStatusBarData.audioIcon.current = 0;

	if(menuDataAWMDEnable == true)
		rStatusBarData.awmdIcon.current = 1;
	else
		rStatusBarData.awmdIcon.current = 0;

	if(menuDataGPSEnable == true) {
		rStatusBarData.gpsIcon.current = 1;
		enableGps();
	} else {
		rStatusBarData.gpsIcon.current = 0;
	}

	if(menuDataWIFIEnable == true) {
		rStatusBarData.wifiIcon.current = 1;
		enable_wifi();
	} else {
		rStatusBarData.wifiIcon.current = 0;
	}

	if(menuDataAPEnable == true) {
		rStatusBarData.apIcon.current = 1;
		enable_Softap();
	} else {
		rStatusBarData.apIcon.current = 0;
	}

#ifdef MODEM_ENABLE
	if(menuDataCELLULAREnable == true){
		rStatusBarData.cellularIcon.current = 1;
		enableCellular();
	}else{
		rStatusBarData.cellularIcon.current = 0;
	}
#endif

	notifyAll();

out:
	releaseCfgFileHandle();
	releaseCfgMenuHandle();
	return retval;
}

int ResourceManager::initStatusBarResource(void)
{
	int err_code;
	int retval = 0;

	/*++++ Rect ++++*/
	configTable2 configTableRect[] = {
		{CFG_FILE_STB,			(void*)&rStatusBarData.STBRect },
		{CFG_FILE_STB_WNDPIC,	(void*)&rStatusBarData.windowPicRect },
#ifdef USE_NEWUI
		{CFG_FILE_MENU,			(void*)&rMenuBarData.MenuRect},
		{CFG_FILE_STB_VALUE,	(void*)&rStatusBarData.valueRect },
		{CFG_FILE_STB_MODE,		(void*)&rStatusBarData.modeRect },
#else
		{CFG_FILE_STB_LABEL1,	(void*)&rStatusBarData.label1Rect },
		{CFG_FILE_STB_RESERVELABLE,	(void*)&rStatusBarData.reserveLabelRect },
		{CFG_FILE_STB_TIME,			(void*)&rStatusBarData.timeRect },
#endif
		{CFG_FILE_STB_WIFI,			(void*)&rStatusBarData.wifiRect },
		{CFG_FILE_STB_AP,			(void*)&rStatusBarData.apRect },
		{CFG_FILE_STB_UVC,			(void*)&rStatusBarData.uvcRect },
		{CFG_FILE_STB_AWMD,			(void*)&rStatusBarData.awmdRect },
		{CFG_FILE_STB_LOCK,			(void*)&rStatusBarData.lockRect },
		{CFG_FILE_STB_TFCARD,		(void*)&rStatusBarData.tfCardRect },
		{CFG_FILE_STB_GPS,			(void*)&rStatusBarData.gpsRect },
#ifdef MODEM_ENABLE
		{CFG_FILE_STB_CELLULAR,                 (void*)&rStatusBarData.cellularRect },
#endif
		{CFG_FILE_STB_AUDIO,		(void*)&rStatusBarData.audioRect },
		{CFG_FILE_STB_BAT,			(void*)&rStatusBarData.batteryRect },
	};

	/*++++ get color ++++*/
	configTable3 configTableColor[] = {
		{CFG_FILE_STB, FGC_WIDGET, (void*)&rStatusBarData.fgc},
		{CFG_FILE_STB, BGC_WIDGET, (void*)&rStatusBarData.bgc},
	};

	/*++++ get the status_bar window picture name ++++*/
	configTable3 configTableValue[] = {
#ifdef USE_NEWUI	
		{CFG_FILE_STB,		   "status_bar_bg", 	(void*)&rStatusBarData.stbBackgroundPic},
		{CFG_FILE_OTHER_PIC,	"record_dot",		(void*)&rRecordDotPic},
#endif
		{CFG_FILE_STB_WNDPIC,  "record_preview",	(void*)&rStatusBarData.recordPreviewPic },
		{CFG_FILE_STB_WNDPIC,  "screen_shot",		(void*)&rStatusBarData.screenShotPic },
		{CFG_FILE_STB_WNDPIC,  "playback_preview",	(void*)&rStatusBarData.playBackPreviewPic },
		{CFG_FILE_STB_WNDPIC,  "playback",			(void*)&rStatusBarData.playBackPic },
#ifndef USE_NEWUI
		{CFG_FILE_STB_WNDPIC,  "menu",				(void*)&rStatusBarData.menuPic },
#endif
	};

	/*++++ get the current icon info ++++*/
	configTable2 configTableIcon[] = {
#ifdef USE_NEWUI
		{CFG_FILE_STB_VALUE,		(void*)&rStatusBarData.valueIcon},
		{CFG_FILE_STB_MODE, 		(void*)&rStatusBarData.modeIcon},
#endif
		{CFG_FILE_STB_WIFI,			(void*)&rStatusBarData.wifiIcon},
		{CFG_FILE_STB_AP,			(void*)&rStatusBarData.apIcon},
		{CFG_FILE_STB_UVC,			(void*)&rStatusBarData.uvcIcon},
		{CFG_FILE_STB_AWMD,			(void*)&rStatusBarData.awmdIcon},
		{CFG_FILE_STB_LOCK,			(void*)&rStatusBarData.lockIcon},
		{CFG_FILE_STB_TFCARD,		(void*)&rStatusBarData.tfCardIcon },
		{CFG_FILE_STB_GPS,			(void*)&rStatusBarData.gpsIcon },
#ifdef MODEM_ENABLE
		{CFG_FILE_STB_CELLULAR,                 (void*)&rStatusBarData.cellularIcon },
#endif
		{CFG_FILE_STB_AUDIO,		(void*)&rStatusBarData.audioIcon },
		{CFG_FILE_STB_BAT,			(void*)&rStatusBarData.batteryIcon },
	};

	/*---- Rect ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableRect); i++) {
		err_code = getRectFromFileHandle(configTableRect[i].mainkey, (CDR_RECT*)configTableRect[i].addr );
		if(err_code != ETC_OK) {
			db_error("get %s rect failed: %s\n", configTableRect[i].mainkey,  pEtcError(err_code));
			retval = -1;
			goto out;
		}
	}

	/*---- get color ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableColor); i++) {
		err_code = getABGRColorFromFileHandle(configTableColor[i].mainkey, configTableColor[i].subkey, *(gal_pixel*)configTableColor[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s %s failed: %s\n", configTableColor[i].mainkey, configTableColor[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		//		db_msg("%s %s color is 0x%lx\n", configTableColor[i].mainkey, configTableColor[i].subkey, 
		//				Pixel2DWORD(HDC_SCREEN, *(gal_pixel*)configTableColor[i].addr) );
	}

	char buf[50];
	/*---- get the status_bar window picture name ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableValue); i++) {
		err_code = getValueFromFileHandle(configTableValue[i].mainkey, configTableValue[i].subkey, buf, sizeof(buf));
		if(err_code != ETC_OK) {
			db_error("get %s %s pic failed: %s\n", configTableValue[i].mainkey, configTableValue[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		*(String8*)configTableValue[i].addr = buf;
		db_msg("get %s %s pic is %s\n", configTableValue[i].mainkey, configTableValue[i].subkey, (*(String8*)configTableValue[i].addr).string() );
	}


	/*---- get the current icon info ----*/
	unsigned int current;
	for(unsigned int i = 0; i < TABLESIZE(configTableIcon); i++) {
		err_code = fillCurrentIconInfoFromFileHandle(configTableIcon[i].mainkey, *(currentIcon_t*)configTableIcon[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s icon failed: %s\n", configTableIcon[i].mainkey, pEtcError(err_code) );
			retval = -1;
			goto out;
		}
		current = (*(currentIcon_t*)configTableIcon[i].addr).current;
		//		db_msg("%s current icon is %s\n", configTableIcon[i].mainkey, (*(currentIcon_t*)configTableIcon[i].addr).icon.itemAt(current).string() );
	}

	/************* end of init status bar resource ******************/

out:
	return retval;
}

void ResourceManager::setCurrentIconValue(enum ResourceID resID, int cur_val)
{
	switch(resID) {
	case ID_STATUSBAR_ICON_BAT:
		rStatusBarData.batteryIcon.current = (cur_val - 1);
		break;
	case ID_STATUSBAR_ICON_TFCARD:
		rStatusBarData.tfCardIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_GPS:
		rStatusBarData.gpsIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_WIFI:
		rStatusBarData.wifiIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_AP:
		rStatusBarData.apIcon.current = cur_val;
		break;
#ifdef MODEM_ENABLE
	case ID_STATUSBAR_ICON_CELLULAR:
		rStatusBarData.cellularIcon.current = cur_val;
		break;
#endif
	case ID_STATUSBAR_ICON_AUDIO: 
		rStatusBarData.audioIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_LOCK: 
		rStatusBarData.lockIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_AWMD:
		rStatusBarData.awmdIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_UVC:
		rStatusBarData.uvcIcon.current = cur_val;
		break;
#ifdef USE_NEWUI
	case ID_STATUSBAR_ICON_VALUE:
		rStatusBarData.valueIcon.current = cur_val;
		break;
	case ID_STATUSBAR_ICON_MODE:
		rStatusBarData.modeIcon.current = cur_val;
		break;
	case ID_MENUBAR_ICON_WIN0:
		rMenuBarData.win0Icon.current = cur_val;
		break;
	case ID_MENUBAR_ICON_WIN1:
		rMenuBarData.win1Icon.current = cur_val;
		break;
	case ID_MENUBAR_ICON_WIN2:
		rMenuBarData.win2Icon.current = cur_val;
		break;
	case ID_MENUBAR_ICON_WIN3:
		rMenuBarData.win3Icon.current = cur_val;
		break;
#endif
	default:
		db_error("invalide resID: %d\n", resID);
		break;
	}
}

int ResourceManager::initPlayBackPreviewResource(void)
{
	int err_code;
	int retval = 0;

	/*++++ Rect ++++*/
	configTable2 configTableRect[] = {
		/*  session name in the configFile			addr to store the value */
		{CFG_FILE_PLBPREVIEW_IMAGE,			(void*)&rPlayBackPreviewData.imageRect},
		{CFG_FILE_PLBPREVIEW_ICON,			(void*)&rPlayBackPreviewData.iconRect},
		{CFG_FILE_PLBPREVIEW_MENU,			(void*)&rPlayBackPreviewData.popupMenuRect},
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX,	(void*)&rPlayBackPreviewData.messageBoxRect},
	};

	/*++++ item_width and item_height ++++*/
	configTable3 configTableIntValue[] = {
		{CFG_FILE_PLBPREVIEW_MENU,			"item_width",  (void*)&rPlayBackPreviewData.popupMenuItemWidth  },
		{CFG_FILE_PLBPREVIEW_MENU,			"item_height", (void*)&rPlayBackPreviewData.popupMenuItemHeight },
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX,	"item_width",  (void*)&rPlayBackPreviewData.messageBoxItemWidth },
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX,	"item_height", (void*)&rPlayBackPreviewData.messageBoxItemHeight}
	};

	/*++++ get color ++++*/
	configTable3 configTableColor[] = {
		{CFG_FILE_PLBPREVIEW_MENU, BGC_WIDGET,			(void*)&rPlayBackPreviewData.popupMenuBgcWidget },
		{CFG_FILE_PLBPREVIEW_MENU, BGC_ITEM_NORMAL,		(void*)&rPlayBackPreviewData.popupMenuBgcItemNormal },
		{CFG_FILE_PLBPREVIEW_MENU, BGC_ITEM_FOCUS,		(void*)&rPlayBackPreviewData.popUpMenuBgcItemFocus },
		{CFG_FILE_PLBPREVIEW_MENU, MAINC_THREED_BOX,	(void*)&rPlayBackPreviewData.popUpMenuMain3dBox },

		{CFG_FILE_PLBPREVIEW_MESSAGEBOX, BGC_WIDGET,			(void*)&rPlayBackPreviewData.messageBoxBgcWidget },
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX, FGC_WIDGET,			(void*)&rPlayBackPreviewData.messageBoxFgcWidget },
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX, BGC_ITEM_NORMAL,		(void*)&rPlayBackPreviewData.messageBoxBgcItemNormal },
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX, BGC_ITEM_FOCUS,		(void*)&rPlayBackPreviewData.messageBoxBgcItemFous },
		{CFG_FILE_PLBPREVIEW_MESSAGEBOX, MAINC_THREED_BOX,		(void*)&rPlayBackPreviewData.messageBoxMain3dBox },
	};


	/*---- Rect ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableRect); i++) {
		err_code = getRectFromFileHandle(configTableRect[i].mainkey, (CDR_RECT*)configTableRect[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s rect failed: %s\n", configTableRect[i].mainkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
	}

	/* currentIcon_t */
	err_code = fillCurrentIconInfoFromFileHandle(CFG_FILE_PLBPREVIEW_ICON, rPlayBackPreviewData.icon);
	if(err_code != ETC_OK) {
		db_error("get playback preview current icon info failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}


	/*---- item_width and item_height ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableIntValue); i++) {
		err_code = getIntValueFromHandle(mCfgFileHandle, configTableIntValue[i].mainkey, configTableIntValue[i].subkey);
		if(err_code < 0) {
			db_error("get %s %s failed\n", configTableIntValue[i].mainkey, configTableIntValue[i].subkey);
			retval = -1;
			goto out;
		}
		*(unsigned int*)configTableIntValue[i].addr = err_code;
	}


	/*---- get color ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableColor); i++) {
		err_code = getABGRColorFromFileHandle(configTableColor[i].mainkey, configTableColor[i].subkey, *(gal_pixel*)configTableColor[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s %s failed: %s\n", configTableColor[i].mainkey, configTableColor[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		//		db_msg("%s %s color is 0x%x\n", configTableColor[i].mainkey, configTableColor[i].subkey, 
		//				Pixel2DWORD(HDC_SCREEN, *(gal_pixel*)configTableColor[i].addr) );
	}

	/**** end of init the resource for PlayBackPreview window *****/

out:
	return retval;
}

int ResourceManager::initPlayBackResource(void)
{
	int err_code;
	int retval = 0;

	/* init the resource PlayBack window */
	err_code = getRectFromFileHandle(CFG_FILE_PLB_ICON, &rPlayBackData.iconRect);
	if(err_code != ETC_OK) {
		db_error("get playback icon rect failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}
	err_code = fillCurrentIconInfoFromFileHandle(CFG_FILE_PLB_ICON, rPlayBackData.icon);
	if(err_code != ETC_OK) {
		db_error("get playback current icon info failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}

	err_code = getRectFromFileHandle(CFG_FILE_PLB_PGB, &rPlayBackData.PGBRect);
	if(err_code != ETC_OK) {
		db_error("get playback ProgressBar rect failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}
	err_code = getABGRColorFromFileHandle(CFG_FILE_PLB_PGB, BGC_WIDGET, rPlayBackData.PGBBgcWidget);
	if(err_code != ETC_OK) {
		db_error("get playback bgc_widget failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}
	err_code = getABGRColorFromFileHandle(CFG_FILE_PLB_PGB, FGC_WIDGET, rPlayBackData.PGBFgcWidget);
	if(err_code != ETC_OK) {
		db_error("get playback fgc_widget failed: %s\n", pEtcError(err_code));
		retval = -1;
		goto out;
	}

out:
	return retval;
}

#ifdef USE_NEWUI
int ResourceManager::initMenuBarResource(void)
{
	int err_code;
	int retval = 0;

	/*++++ Rect ++++*/
	configTable2 configTableRect[] = {
		{CFG_FILE_MB,			(void*)&rMenuBarData.MenuBarRect},
		{CFG_FILE_MB_ICON_WIN0,			(void*)&rMenuBarData.win0Rect},
		{CFG_FILE_MB_ICON_WIN1,			(void*)&rMenuBarData.win1Rect},
		{CFG_FILE_MB_ICON_WIN2,			(void*)&rMenuBarData.win2Rect},
		{CFG_FILE_MB_ICON_WIN3,			(void*)&rMenuBarData.win3Rect},
	};
	
	/*++++ get the menu_bar picture name ++++*/
	configTable3 configTablePic[] = {
		{CFG_FILE_MENU,    "menu_list_bg",		(void*)&rMenuBarData.mlBgPic},
		{CFG_FILE_MENU,	   "menu_list_hline",		(void*)&rMenuBarData.hlinePic},
		{CFG_FILE_MENU,	   "menu_list_light",		(void*)&rMenuBarData.mlLightPic},
		{CFG_FILE_MENU,    "menu_list_select",		(void*)&rMenuBarData.mlSelectIcon},
		{CFG_FILE_MB,  	   "menu_bar_bg",		(void*)&rMenuBarData.backgroundPic},
		{CFG_FILE_MB,	   "menu_bar_vline",		(void*)&rMenuBarData.vlinePic},
		{CFG_FILE_MB,	   "menu_bar_light",		(void*)&rMenuBarData.lightPic},
	};

	/*++++ get the current icon info ++++*/
	configTable2 configTableIcon[] = {
		{CFG_FILE_MB_ICON_WIN0,			(void*)&rMenuBarData.win0Icon},
		{CFG_FILE_MB_ICON_WIN1,			(void*)&rMenuBarData.win1Icon},
		{CFG_FILE_MB_ICON_WIN2,			(void*)&rMenuBarData.win2Icon},
		{CFG_FILE_MB_ICON_WIN3,			(void*)&rMenuBarData.win3Icon},
	};

	/*---- Rect ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableRect); i++) {
		err_code = getRectFromFileHandle(configTableRect[i].mainkey, (CDR_RECT*)configTableRect[i].addr );
		if(err_code != ETC_OK) {
			db_error("get %s rect failed: %s\n", configTableRect[i].mainkey,  pEtcError(err_code));
			retval = -1;
			goto out;
		}
	}

	/*---- get the menulist picture name ----*/
	char buf[50];
	for(unsigned int i = 0; i < TABLESIZE(configTablePic); i++) {
		err_code = getValueFromFileHandle(configTablePic[i].mainkey, configTablePic[i].subkey, buf, sizeof(buf));
		if(err_code != ETC_OK) {
			db_error("get %s %s pic failed: %s\n", configTablePic[i].mainkey, configTablePic[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		*(String8*)configTablePic[i].addr = buf;
	}
		
	/*---- get the current icon info ----*/
	unsigned int current;
	for(unsigned int i = 0; i < TABLESIZE(configTableIcon); i++) {
		err_code = fillCurrentIconInfoFromFileHandle(configTableIcon[i].mainkey, *(currentIcon_t*)configTableIcon[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s icon failed: %s\n", configTableIcon[i].mainkey, pEtcError(err_code) );
			retval = -1;
			goto out;
		}
		current = (*(currentIcon_t*)configTableIcon[i].addr).current;
		//		db_msg("%s current icon is %s\n", configTableIcon[i].mainkey, (*(currentIcon_t*)configTableIcon[i].addr).icon.itemAt(current).string() );
	}

	/************* end of init menu bar resource ******************/
out:
	return retval;
}
#endif

int ResourceManager::initMenuListResource(void)
{
	int err_code;
	int retval = 0;

	/*++++ get the menulist picture name ++++*/
	configTable3 configTableValue[] = {
	#ifdef USE_NEWUI
		{CFG_FILE_ML, "box_bg", 					(void*)&rMenuList.boxBgPic},
		{CFG_FILE_ML, "box_line",					(void*)&rMenuList.boxLinePic },
		{CFG_FILE_ML, "button_confirm",				(void*)&rMenuList.btnConfirmPic },
		{CFG_FILE_ML, "button_cancel", 				(void*)&rMenuList.btnCancelPic },
	#else
		{CFG_FILE_ML, "choice", 					(void*)&rMenuList.choicePic },
		{CFG_FILE_ML, "checkbox_checked_normal",	(void*)&rMenuList.checkedNormalPic },
		{CFG_FILE_ML, "checkbox_checked_press", 	(void*)&rMenuList.checkedPressPic },
		{CFG_FILE_ML, "checkbox_unchecked_normal",	(void*)&rMenuList.uncheckedNormalPic },
		{CFG_FILE_ML, "checkbox_unchecked_press",	(void*)&rMenuList.uncheckedPressPic },
		{CFG_FILE_ML, "unfold_normal",	(void*)&rMenuList.unfoldNormalPic },
		{CFG_FILE_ML, "unfold_press",	(void*)&rMenuList.unfoldPressPic },
	#endif
	};

	/*++++ get color ++++*/
	configTable3 configTableColor[] = {
		{CFG_FILE_ML, FGC_WIDGET,		(void*)&rMenuList.fgcWidget },
		{CFG_FILE_ML, BGC_WIDGET,		(void*)&rMenuList.bgcWidget },
		{CFG_FILE_ML, LINEC_ITEM,		(void*)&rMenuList.linecItem },
		{CFG_FILE_ML, STRINGC_NORMAL,	(void*)&rMenuList.stringcNormal },
		{CFG_FILE_ML, STRINGC_SELECTED, (void*)&rMenuList.stringcSelected },
		{CFG_FILE_ML, VALUEC_NORMAL,	(void*)&rMenuList.valuecNormal },
		{CFG_FILE_ML, VALUEC_SELECTED,	(void*)&rMenuList.valuecSelected },
		{CFG_FILE_ML, SCROLLBARC,		(void*)&rMenuList.scrollbarc },
		{CFG_FILE_SUBMENU, LINEC_TITLE,		(void*)&rMenuList.subMenuLinecTitle },
		{CFG_FILE_ML_MB, FGC_WIDGET,	(void*)&rMenuList.messageBoxFgcWidget },
		{CFG_FILE_ML_MB, BGC_WIDGET,	(void*)&rMenuList.messageBoxBgcWidget },
		{CFG_FILE_ML_MB, LINEC_TITLE,	(void*)&rMenuList.messageBoxLinecTitle },
		{CFG_FILE_ML_MB, LINEC_ITEM,	(void*)&rMenuList.messageBoxLinecItem },
		{CFG_FILE_ML_DATE, BGC_WIDGET,	(void*)&rMenuList.dateBgcWidget },
		{CFG_FILE_ML_DATE, "fgc_label",	(void*)&rMenuList.dateFgc_label },
		{CFG_FILE_ML_DATE, "fgc_number",	(void*)&rMenuList.dateFgc_number },
		{CFG_FILE_ML_DATE, LINEC_TITLE,	(void*)&rMenuList.dateLinecTitle },
		{CFG_FILE_ML_DATE, "borderc_selected",	(void*)&rMenuList.dateBordercSelected },
		{CFG_FILE_ML_DATE, "borderc_normal",	(void*)&rMenuList.dateBordercNormal },
		{CFG_FILE_ML_AP, BGC_WIDGET,	(void*)&rMenuList.apBgcWidget },
		{CFG_FILE_ML_AP, "fgc_label",	(void*)&rMenuList.apFgc_label },
		{CFG_FILE_ML_AP, "fgc_number",	(void*)&rMenuList.apFgc_number },
		{CFG_FILE_ML_AP, LINEC_TITLE,	(void*)&rMenuList.apLinecTitle },
		{CFG_FILE_ML_AP, "borderc_selected",	(void*)&rMenuList.apBordercSelected },
		{CFG_FILE_ML_AP, "borderc_normal",	(void*)&rMenuList.apBordercNormal },
	};

	/*++++ Rect ++++*/
	configTable2 configTableRect[] = {
#ifndef USE_NEWUI
		{CFG_FILE_ML,		(void*)&rMenuList.menuListRect},
		{CFG_FILE_SUBMENU,	(void*)&rMenuList.subMenuRect},
#endif
		{CFG_FILE_ML_MB,        (void*)&rMenuList.messageBoxRect},
		{CFG_FILE_ML_DATE,      (void*)&rMenuList.dateRect},
		{CFG_FILE_ML_AP,	(void*)&rMenuList.apRect},
	};

	/*++++ get the current icon info ++++*/
	configTable2 configTableIcon[] = {
		{CFG_FILE_ML_VQ,	(void*)&rMenuList.videoQualityIcon},
		{CFG_FILE_ML_PQ,	(void*)&rMenuList.photoQualityIcon},
		{CFG_FILE_ML_VTL,	(void*)&rMenuList.timeLengthIcon},
		{CFG_FILE_ML_AWMD,	(void*)&rMenuList.moveDetectIcon},
		{CFG_FILE_ML_WB,	(void*)&rMenuList.whiteBalanceIcon},
		{CFG_FILE_ML_CONTRAST,	(void*)&rMenuList.contrastIcon},
		{CFG_FILE_ML_EXPOSURE,	(void*)&rMenuList.exposureIcon},
		{CFG_FILE_ML_POR,		(void*)&rMenuList.PORIcon},
		{CFG_FILE_ML_GPS,		(void*)&rMenuList.GPSIcon},
#ifdef MODEM_ENABLE
		{CFG_FILE_ML_CELLULAR,          (void*)&rMenuList.CELLULARIcon},
#endif
		{CFG_FILE_ML_SS,		(void*)&rMenuList.screenSwitchIcon},
		{CFG_FILE_ML_VOLUME,	(void*)&rMenuList.volumeIcon},
		{CFG_FILE_ML_WIFI,		(void*)&rMenuList.wifiIcon},
		{CFG_FILE_ML_AP,		(void*)&rMenuList.apIcon},
		{CFG_FILE_ML_DATE,		(void*)&rMenuList.dateIcon},
		{CFG_FILE_ML_LANG,		(void*)&rMenuList.languageIcon},
		{CFG_FILE_ML_TWM,		(void*)&rMenuList.TWMIcon},
		{CFG_FILE_ML_FORMAT,	(void*)&rMenuList.formatIcon},
		{CFG_FILE_ML_FACTORYRESET,	(void*)&rMenuList.factoryResetIcon},
		{CFG_FILE_ML_FIRMWARE,		(void*)&rMenuList.firmwareIcon}
	};

	configTable3 configTableIntValue[] = {
		{CFG_FILE_ML_DATE,		"title_height",	(void*)&rMenuList.dateTileRectH  },
		{CFG_FILE_ML_DATE,		"hBorder",		(void*)&rMenuList.dateHBorder  },
		{CFG_FILE_ML_DATE,		"yearW",		(void*)&rMenuList.dateYearW  },
		{CFG_FILE_ML_DATE,		"dateLabelW",	(void*)&rMenuList.dateLabelW  },
		{CFG_FILE_ML_DATE,		"numberW",		(void*)&rMenuList.dateNumberW  },
		{CFG_FILE_ML_DATE,		"boxH",			(void*)&rMenuList.dateBoxH  },
		#ifdef USE_NEWUI
		{CFG_FILE_ML,		"menulist_item_height", 		(void*)&rMenuList.itemHeight  },
		#endif
	};

	/*---- get the menulist picture name ----*/
	char buf[50];
	for(unsigned int i = 0; i < TABLESIZE(configTableValue); i++) {
		err_code = getValueFromFileHandle(configTableValue[i].mainkey, configTableValue[i].subkey, buf, sizeof(buf));
		if(err_code != ETC_OK) {
			db_error("get %s %s pic failed: %s\n", configTableValue[i].mainkey, configTableValue[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		*(String8*)configTableValue[i].addr = buf;
		//		db_error("get %s %s pic is %s\n", configTableValue[i].mainkey, configTableValue[i].subkey, (*(String8*)configTableValue[i].addr).string() );
	}

	/*---- get color ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableColor); i++) {
		err_code = getABGRColorFromFileHandle(configTableColor[i].mainkey, configTableColor[i].subkey, *(gal_pixel*)configTableColor[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s %s failed: %s\n", configTableColor[i].mainkey, configTableColor[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		//		db_error("get %s %s color is 0x%x\n", configTableColor[i].mainkey, configTableColor[i].subkey, 
		//				Pixel2DWORD(HDC_SCREEN, *(gal_pixel*)configTableColor[i].addr) );
	}
	
	/*---- Rect ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableRect); i++) {
		err_code = getRectFromFileHandle(configTableRect[i].mainkey, (CDR_RECT*)configTableRect[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s rect failed: %s\n", configTableRect[i].mainkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
	}

#ifndef USE_NEWUI
	/*---- get the current icon info ----*/
	unsigned int current;
	for(unsigned int i = 0; i < TABLESIZE(configTableIcon); i++) {
		err_code = fillCurrentIconInfoFromFileHandle(configTableIcon[i].mainkey, *(currentIcon_t*)configTableIcon[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s icon failed: %s\n", configTableIcon[i].mainkey, pEtcError(err_code) );
			retval = -1;
			goto out;
		}
		current = (*(currentIcon_t*)configTableIcon[i].addr).current;
		//		db_msg("%s current icon is %s\n", configTableIcon[i].mainkey, (*(currentIcon_t*)configTableIcon[i].addr).icon.itemAt(current).string() );
	}
#endif

	/*  init inval */
	for(unsigned int i = 0; i < TABLESIZE(configTableIntValue); i++) {
		err_code = getIntValueFromHandle(mCfgFileHandle, configTableIntValue[i].mainkey, configTableIntValue[i].subkey);
		if(err_code < 0) {
			db_error("get %s %s failed\n", configTableIntValue[i].mainkey, configTableIntValue[i].subkey);
			retval = -1;
			goto out;
		}
		*(unsigned int*)configTableIntValue[i].addr = err_code;
	}
out:
	return retval;
}

void ResourceManager::dump()
{

	db_msg("menuDataLang.count %d  menuDataLang.current %d\n", menuDataLang.count, menuDataLang.current);
	db_msg("menuDataVQ.count %d	menuDataVQ.current %d\n",menuDataVQ.count, menuDataVQ.current);
	#ifdef USE_NEWUI
	db_msg("menuDataVM.count %d	menuDataVM.current %d\n",menuDataVM.count, menuDataVM.current);
	#endif
	db_msg("menuDataPQ.count %d menuDataPQ.current %d\n",menuDataPQ.count,menuDataPQ.current);
	db_msg("menuDataVTL.count %d menuDataVTL.current %d\n",menuDataVTL.count,menuDataVTL.current);
	db_msg("menuDataWB.count %d menuDataWB.current %d\n",menuDataWB.count,menuDataWB.current);
	db_msg("menuDataContrast.count %d menuDataContrast.current %d\n",menuDataContrast.count,menuDataContrast.current);
	db_msg("menuDataExposure.count %d menuDataExposure.current %d\n",menuDataExposure.count,menuDataExposure.current);
	db_msg("menuDataSS.count %d menuDataSS.current %d\n",menuDataSS.count,menuDataSS.current);
	db_msg("menuDataPOREnable %d\n",menuDataPOREnable);
	db_msg("menuDataGPSEnable %d\n",menuDataGPSEnable);
#ifdef MODEM_ENABLE
	db_msg("menuDataCELLULAREnable %d\n",menuDataCELLULAREnable);
#endif
	db_msg("menuDataWIFIEnable %d\n",menuDataWIFIEnable);
	db_msg("menuDataAPEnable %d\n",menuDataAPEnable);
	db_msg("menuDataVolumeEnable %d\n",menuDataVolumeEnable);
	db_msg("menuDataTWMEnable %d\n",menuDataTWMEnable);
	db_msg("menuDataAWMDEnable %d\n",menuDataAWMDEnable);
}

int ResourceManager::initMenuResource(void)
{

	configTable2 configTableMenuItem[] = {
		{CFG_MENU_LANG, (void*)&menuDataLang},
		{CFG_MENU_VQ,	(void*)&menuDataVQ},
		{CFG_MENU_PQ,	(void*)&menuDataPQ},
		{CFG_MENU_VTL,	(void*)&menuDataVTL},
		{CFG_MENU_WB,	(void*)&menuDataWB},
		{CFG_MENU_CONTRAST, (void*)&menuDataContrast},
		{CFG_MENU_EXPOSURE, (void*)&menuDataExposure},
		{CFG_MENU_SS,		(void*)&menuDataSS},
	};

	configTable2 configTableSwitch[] = {
		{CFG_MENU_POR, (void*)&menuDataPOREnable},
		{CFG_MENU_GPS, (void*)&menuDataGPSEnable},
#ifdef MODEM_ENABLE
		{CFG_MENU_CELLULAR, (void*)&menuDataCELLULAREnable},
#endif
		{CFG_MENU_WIFI, (void*)&menuDataWIFIEnable},
		{CFG_MENU_AP, (void*)&menuDataAPEnable},
		{CFG_MENU_VOLUME,	(void*)&menuDataVolumeEnable},
	};

	int value;
	for(unsigned int i = 0; i < TABLESIZE(configTableMenuItem); i++) {
		value = getIntValueFromHandle(mCfgMenuHandle, configTableMenuItem[i].mainkey, "current");	
		if(value < 0) {
			db_error("get current %s failed\n", configTableMenuItem[i].mainkey);
			return -1;
		}
		((menuItem_t*)(configTableMenuItem[i].addr))->current = value;

		value = getIntValueFromHandle(mCfgMenuHandle, configTableMenuItem[i].mainkey, "count");	
		if(value < 0) {
			db_error("get count %s failed\n", configTableMenuItem[i].mainkey);
			return -1;
		}
		((menuItem_t*)(configTableMenuItem[i].addr))->count = value;
				db_msg("%s current is %d, count is %d\n", configTableMenuItem[i].mainkey, ((menuItem_t*)(configTableMenuItem[i].addr))->current, value);	
	}

	for(unsigned int i = 0; i < TABLESIZE(configTableSwitch); i++) {
		value = getIntValueFromHandle(mCfgMenuHandle, "switch", configTableSwitch[i].mainkey);
		if(value < 0) {
			db_error("get switch %s failed\n", configTableSwitch[i].mainkey);
			return -1;
		}
		if(value == 0) {
			*((bool*)configTableSwitch[i].addr) = false;
		} else {
			*((bool*)configTableSwitch[i].addr) = true;
		}
		db_msg("switch %s is %d\n", configTableSwitch[i].mainkey, value);
	}

	return 0;
}

int ResourceManager::initOtherResource(void)
{
	int err_code;
	int retval = 0;
	configTable2 configTableRect[] = {
		{CFG_FILE_WARNING_MB,		(void*)&rWarningMB.rect},
		{CFG_FILE_CONNECT2PC,		(void*)&rConnectToPC.rect},
		{CFG_FILE_TIPLABEL,			(void*)&rTipLabel.rect}
	};

	configTable3 configTableColor[] = {
		{CFG_FILE_WARNING_MB, FGC_WIDGET,	(void*)&rWarningMB.fgcWidget },
		{CFG_FILE_WARNING_MB, BGC_WIDGET,	(void*)&rWarningMB.bgcWidget },
		{CFG_FILE_WARNING_MB, LINEC_TITLE,	(void*)&rWarningMB.linecTitle },
		{CFG_FILE_WARNING_MB, LINEC_ITEM,	(void*)&rWarningMB.linecItem },
		{CFG_FILE_CONNECT2PC, BGC_WIDGET,	(void*)&rConnectToPC.bgcWidget },
		{CFG_FILE_CONNECT2PC, BGC_ITEM_FOCUS,	(void*)&rConnectToPC.bgcItemFocus},
		{CFG_FILE_CONNECT2PC, BGC_ITEM_NORMAL,	(void*)&rConnectToPC.bgcItemNormal},
		{CFG_FILE_CONNECT2PC, MAINC_THREED_BOX,	(void*)&rConnectToPC.mainc_3dbox},
		{CFG_FILE_TIPLABEL,	  FGC_WIDGET,	(void*)&rTipLabel.fgcWidget},
		{CFG_FILE_TIPLABEL,	  BGC_WIDGET,	(void*)&rTipLabel.bgcWidget},
		{CFG_FILE_TIPLABEL,	  LINEC_TITLE,	(void*)&rTipLabel.linecTitle},
	};

	configTable3 configTableIntValue[] = {
		{CFG_FILE_CONNECT2PC, "item_width",		(void*)&rConnectToPC.itemWidth  },
		{CFG_FILE_CONNECT2PC, "item_height",	(void*)&rConnectToPC.itemHeight  },
		{CFG_FILE_TIPLABEL,	  "title_height",	(void*)&rTipLabel.titleHeight }
	};

	configTable3 configTableValue[] = {
		{CFG_FILE_OTHER_PIC, "poweroff",		(void*)&rPoweroffPic },
	};

	/*---- Rect ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableRect); i++) {
		err_code = getRectFromFileHandle(configTableRect[i].mainkey, (CDR_RECT*)configTableRect[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s rect failed: %s\n", configTableRect[i].mainkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
	}

	/*---- get color ----*/
	for(unsigned int i = 0; i < TABLESIZE(configTableColor); i++) {
		err_code = getABGRColorFromFileHandle(configTableColor[i].mainkey, configTableColor[i].subkey, *(gal_pixel*)configTableColor[i].addr);
		if(err_code != ETC_OK) {
			db_error("get %s %s failed: %s\n", configTableColor[i].mainkey, configTableColor[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
	}

	/*  init inval */
	for(unsigned int i = 0; i < TABLESIZE(configTableIntValue); i++) {
		err_code = getIntValueFromHandle(mCfgFileHandle, configTableIntValue[i].mainkey, configTableIntValue[i].subkey);
		if(err_code < 0) {
			db_error("get %s %s failed\n", configTableIntValue[i].mainkey, configTableIntValue[i].subkey);
			retval = -1;
			goto out;
		}
		*(unsigned int*)configTableIntValue[i].addr = err_code;
	}

	/*---- get the menulist picture name ----*/
	char buf[50];
	for(unsigned int i = 0; i < TABLESIZE(configTableValue); i++) {
		err_code = getValueFromFileHandle(configTableValue[i].mainkey, configTableValue[i].subkey, buf, sizeof(buf));
		if(err_code != ETC_OK) {
			db_error("get %s %s pic failed: %s\n", configTableValue[i].mainkey, configTableValue[i].subkey, pEtcError(err_code));
			retval = -1;
			goto out;
		}
		*(String8*)configTableValue[i].addr = buf;
		//db_error("get %s %s pic is %s\n", configTableValue[i].mainkey, configTableValue[i].subkey, (*(String8*)configTableValue[i].addr).string() );
	}

out:
	return retval;
}

gal_pixel ResourceManager::getStatusBarColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_STATUSBAR) {
		switch(type) {
		case COLOR_FGC:
			color = rStatusBarData.fgc;
			break;
		case COLOR_BGC:
			color = rStatusBarData.bgc;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}

gal_pixel ResourceManager::getPlaybackPreviewColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_PLAYBACKPREVIEW_MENU) {
		switch(type) {
		case COLOR_BGC:
			color = rPlayBackPreviewData.popupMenuBgcWidget;
			break;
		case COLOR_BGC_ITEMFOCUS:
			color = rPlayBackPreviewData.popUpMenuBgcItemFocus;
			break;
		case COLOR_BGC_ITEMNORMAL:
			color = rPlayBackPreviewData.popupMenuBgcItemNormal;
			break;
		case COLOR_MAIN3DBOX:
			color = rPlayBackPreviewData.popUpMenuMain3dBox;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else if(resID == ID_PLAYBACKPREVIEW_MB) {
		switch(type) {
		case COLOR_BGC:
			color = rPlayBackPreviewData.messageBoxBgcWidget;
			break;
		case COLOR_FGC:
			color = rPlayBackPreviewData.messageBoxFgcWidget;
			break;
		case COLOR_BGC_ITEMFOCUS:
			color = rPlayBackPreviewData.messageBoxBgcItemFous;
			break;
		case COLOR_BGC_ITEMNORMAL:
			color = rPlayBackPreviewData.messageBoxBgcItemNormal;
			break;
		case COLOR_MAIN3DBOX:
			color = rPlayBackPreviewData.messageBoxMain3dBox;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}

gal_pixel ResourceManager::getPlaybackColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_PLAYBACK_PGB) {
		switch(type) {
		case COLOR_BGC:
			color = rPlayBackData.PGBBgcWidget;
			break;
		case COLOR_FGC:
			color = rPlayBackData.PGBFgcWidget;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}


gal_pixel ResourceManager::getMenuListColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_MENU_LIST) {
		switch(type) {
		case COLOR_BGC:
			color = rMenuList.bgcWidget;
			break;
		case COLOR_FGC:
			color = rMenuList.fgcWidget;
			break;
		case COLOR_LINEC_ITEM:
			color = rMenuList.linecItem;
			break;
		case COLOR_STRINGC_NORMAL:
			color = rMenuList.stringcNormal;
			break;
		case COLOR_STRINGC_SELECTED:
			color = rMenuList.stringcSelected;
			break;
		case COLOR_VALUEC_NORMAL:
			color = rMenuList.valuecNormal;
			break;
		case COLOR_VALUEC_SELECTED:
			color = rMenuList.valuecSelected;
			break;
		case COLOR_SCROLLBARC:
			color = rMenuList.scrollbarc;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else if(resID == ID_MENU_LIST_MB) {
		switch(type) {
		case COLOR_BGC:
			color = rMenuList.messageBoxBgcWidget;
			break;
		case COLOR_FGC:
			color = rMenuList.messageBoxFgcWidget;
			break;
		case COLOR_LINEC_TITLE:
			color = rMenuList.messageBoxLinecTitle;
			break;
		case COLOR_LINEC_ITEM:
			color = rMenuList.messageBoxLinecItem;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else if(resID == ID_MENU_LIST_DATE) {
		switch(type) {
		case COLOR_BGC:
			color = rMenuList.dateBgcWidget;
			break;
		case COLOR_FGC_LABEL:
			color = rMenuList.dateFgc_label;
			break;
		case COLOR_FGC_NUMBER:
			color = rMenuList.dateFgc_number;
			break;
		case COLOR_LINEC_TITLE:
			color = rMenuList.dateLinecTitle;
			break;
		case COLOR_BORDERC_NORMAL:
			color = rMenuList.dateBordercNormal;
			break;
		case COLOR_BORDERC_SELECTED:
			color = rMenuList.dateBordercSelected;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}

	} else if(resID == ID_MENU_LIST_AP) {
		switch(type) {
		case COLOR_BGC:
			color = rMenuList.apBgcWidget;
			break;
		case COLOR_FGC_LABEL:
			color = rMenuList.apFgc_label;
			break;
		case COLOR_FGC_NUMBER:
			color = rMenuList.apFgc_number;
			break;
		case COLOR_LINEC_TITLE:
			color = rMenuList.apLinecTitle;
			break;
		case COLOR_BORDERC_NORMAL:
			color = rMenuList.apBordercNormal;
			break;
		case COLOR_BORDERC_SELECTED:
			color = rMenuList.apBordercSelected;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}

gal_pixel ResourceManager::getSubMenuColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_SUBMENU) {
		switch(type) {
		case COLOR_BGC:
			color = rMenuList.subMenuBgcWidget;
			break;
		case COLOR_FGC:
			color = rMenuList.subMenuFgcWidget;
			break;
		case COLOR_LINEC_TITLE:
			color = rMenuList.subMenuLinecTitle;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}

gal_pixel ResourceManager::getWarningMBColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_WARNNING_MB) {
		switch(type) {
		case COLOR_BGC:
			color = rWarningMB.bgcWidget;
			break;
		case COLOR_FGC:
			color = rWarningMB.fgcWidget;
			break;
		case COLOR_LINEC_TITLE:
			color = rWarningMB.linecTitle;
			break;
		case COLOR_LINEC_ITEM:
			color = rWarningMB.linecItem;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}

gal_pixel ResourceManager::getConnect2PCColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;
	if(resID == ID_CONNECT2PC) {
		switch(type) {
		case COLOR_BGC:
			color = rConnectToPC.bgcWidget;
			break;
		case COLOR_BGC_ITEMFOCUS:
			color = rConnectToPC.bgcItemFocus;
			break;
		case COLOR_BGC_ITEMNORMAL:
			color = rConnectToPC.bgcItemNormal;
			break;
		case COLOR_MAIN3DBOX:
			color = rConnectToPC.mainc_3dbox;
			break;
		default:
			db_error("invalid ColorType: %d\n", type);
			break;
		}
	} else {
		db_error("invalid resID: %d\n", resID);
	}

	return color;
}

gal_pixel ResourceManager::getResColor(enum ResourceID resID, enum ColorType type)
{
	gal_pixel color = 0;

	switch(resID) {
	case ID_STATUSBAR:
		color = getStatusBarColor(resID, type);
		break;
	case ID_PLAYBACKPREVIEW_MENU:
	case ID_PLAYBACKPREVIEW_MB:
		color = getPlaybackPreviewColor(resID, type);
		break;
	case ID_PLAYBACK_PGB:
		color = getPlaybackColor(resID, type);
		break;
	case ID_MENU_LIST:
	case ID_MENU_LIST_MB:
	case ID_MENU_LIST_DATE:
	case ID_MENU_LIST_AP:
		color = getMenuListColor(resID, type);
		break;
	case ID_SUBMENU:
		color = getSubMenuColor(resID, type);
		break;
	case ID_WARNNING_MB:
		color = getWarningMBColor(resID, type);
		break;
	case ID_CONNECT2PC:
		color = getConnect2PCColor(resID, type);
		break;
	case ID_TIPLABEL:
		if(type == COLOR_BGC)
			color = rTipLabel.bgcWidget;
		else if(type == COLOR_FGC)
			color = rTipLabel.fgcWidget;
		else if(type == COLOR_LINEC_TITLE)
			color = rTipLabel.linecTitle;
		break;
	default:
		db_error("invalid resIndex for the color\n");
		break;
	}

	return color;
}

int ResourceManager::getResRect(enum ResourceID resID, CDR_RECT &rect)
{
	switch(resID) {
	case ID_SCREEN:
	case ID_POWEROFF:
		rect = rScreenRect;
		break;
	case ID_RECORD_PREVIEW1:
		rect = rRecordPreviewRect1;
		break;
	case ID_RECORD_PREVIEW2:
		rect = rRecordPreviewRect2;
		break;
	case ID_STATUSBAR:
		rect = rStatusBarData.STBRect;
		break;
	case ID_STATUSBAR_ICON_WINDOWPIC:
		rect = rStatusBarData.windowPicRect;
		break;
#ifdef USE_NEWUI
	case ID_STATUSBAR_ICON_VALUE:
		rect = rStatusBarData.valueRect;
		break;
	case ID_STATUSBAR_ICON_MODE:
		rect = rStatusBarData.modeRect;
		break;
#else
	case ID_STATUSBAR_LABEL1:
		rect = rStatusBarData.label1Rect;
		break;
	case ID_STATUSBAR_LABEL_RESERVE:
		rect = rStatusBarData.reserveLabelRect;
		break;
#endif
	case ID_STATUSBAR_ICON_WIFI:
		rect = rStatusBarData.wifiRect;
		break;
	case ID_STATUSBAR_ICON_AP:
		rect = rStatusBarData.apRect;
		break;
	case ID_STATUSBAR_ICON_AWMD:
		rect = rStatusBarData.awmdRect;
		break;
	case ID_STATUSBAR_ICON_LOCK:
#ifdef USE_NEWUI
#ifdef MODEM_ENABLE
		rect = rStatusBarData.lockRect;
#else
		rect = rStatusBarData.lockRect;
		rect.w = rect.w * 2; 
#endif
#endif
		break;
	case ID_STATUSBAR_ICON_UVC:
		rect = rStatusBarData.uvcRect;
		break;
	case ID_STATUSBAR_ICON_TFCARD:
		rect = rStatusBarData.tfCardRect;
		break;
	case ID_STATUSBAR_ICON_GPS:
		rect = rStatusBarData.gpsRect;
		break;
#ifdef MODEM_ENABLE
	case ID_STATUSBAR_ICON_CELLULAR:
		rect = rStatusBarData.cellularRect;
		break;
#endif
	case ID_STATUSBAR_ICON_AUDIO:
		rect = rStatusBarData.audioRect;
		break;
	case ID_STATUSBAR_ICON_BAT:
		rect = rStatusBarData.batteryRect;
		break;
#ifndef USE_NEWUI
	case ID_STATUSBAR_LABEL_TIME:
		rect = rStatusBarData.timeRect;
		break;
#endif
	case ID_PLAYBACKPREVIEW_IMAGE:
		rect = rPlayBackPreviewData.imageRect;
		break;
	case ID_PLAYBACKPREVIEW_ICON:
		rect = rPlayBackPreviewData.iconRect;
		break;
	case ID_PLAYBACKPREVIEW_MENU:
		rect = rPlayBackPreviewData.popupMenuRect;
		break;
	case ID_PLAYBACKPREVIEW_MB:
		rect = rPlayBackPreviewData.messageBoxRect;
		break;
	case ID_PLAYBACK_ICON:
		rect = rPlayBackData.iconRect;
		break;
	case ID_PLAYBACK_PGB:
		rect = rPlayBackData.PGBRect;
		break;
#ifdef USE_NEWUI
	case ID_MENU:
		rect = rMenuBarData.MenuRect;
		break;
	case ID_MENUBAR:
		rect  = rMenuBarData.MenuBarRect;
		break;
	case ID_MENUBAR_ICON_WIN0:
		rect  = rMenuBarData.win0Rect;
		break;
	case ID_MENUBAR_ICON_WIN1:
		rect  = rMenuBarData.win1Rect;
		break;
	case ID_MENUBAR_ICON_WIN2:
		rect  = rMenuBarData.win2Rect;
		break;
	case ID_MENUBAR_ICON_WIN3:
		rect  = rMenuBarData.win3Rect;
		break;
#endif
	case ID_MENU_LIST:
		rect  = rMenuList.menuListRect;
		break;
	case ID_SUBMENU:
		rect = rMenuList.subMenuRect;
		break;
	case ID_MENU_LIST_MB:
		rect = rMenuList.messageBoxRect;
		break;
	case ID_WARNNING_MB:
		rect = rWarningMB.rect;
		break;
	case ID_MENU_LIST_DATE:
		rect = rMenuList.dateRect;
		break;
	case ID_MENU_LIST_AP:
		rect = rMenuList.apRect;
		break;
	case ID_CONNECT2PC:
		rect = rConnectToPC.rect;
		break;
	case ID_TIPLABEL:
		rect = rTipLabel.rect;
		break;
	default:
		db_error("invalid resID index: %d\n", resID);
		return -1;
	}

	return 0;
}

int ResourceManager::getResBmp(enum ResourceID resID, enum BmpType type, BITMAP &bmp)
{
	int err_code;
	String8 file;
	if(resID == ID_STATUSBAR_ICON_WINDOWPIC) {
		if(getWindPicFileName(type, file) < 0 ) {
			db_error("get window pic bmp failed\n");	
			return -1;
		}
#ifdef USE_NEWUI
	} else if(resID == ID_STATUSBAR) {
		file = rStatusBarData.stbBackgroundPic;
	} else if(resID >= ID_STATUSBAR_ICON_VALUE && resID <= ID_STATUSBAR_ICON_BAT) {
		if(getCurrentIconFileName(resID, file) < 0) {
			db_error("get current statusbar icon pic bmp failed\n");	
			return -1;
		}
	} else if(resID == ID_MENU_LIST) {
		file = rMenuBarData.mlBgPic;
	} else if(resID == ID_MENU_LIST_HLINE) {
		file = rMenuBarData.hlinePic;
	} else if(resID == ID_MENU_LIST_LIGHT) {
		file = rMenuBarData.mlLightPic;
	} else if(resID == ID_MENU_LIST_SELECT) {
		file = rMenuBarData.mlSelectIcon;
	} else if(resID == ID_MENUBAR) {
		file = rMenuBarData.backgroundPic;
	} else if(resID == ID_MENUBAR_VLINE) {
		file = rMenuBarData.vlinePic;
	} else if(resID == ID_MENUBAR_LIGHT) {
		file = rMenuBarData.lightPic;
	} else if(resID >= ID_MENUBAR_ICON_WIN0 && resID <= ID_MENUBAR_ICON_WIN3) {
		if(getCurrentIconFileName(resID, file) < 0) {
			db_error("get current menubar icon pic bmp failed\n");	
			return -1;
		}
	} else if(resID == ID_MENU_LIST_MB) {
		if (type == BMPTYPE_BACKGROUND)
			file = rMenuList.boxBgPic;
		else if (type == BMPTYPE_LINE)
			file = rMenuList.boxLinePic;
		else if (type == BMPTYPE_CONFIRM)
			file = rMenuList.btnConfirmPic;
		else if (type == BMPTYPE_CANCEL)
			file = rMenuList.btnCancelPic;
	} else if(resID == ID_RECORD_PREVIEW1) {
		file = rRecordDotPic;
#else
	} else if(resID >= ID_STATUSBAR_ICON_AWMD && resID <= ID_STATUSBAR_ICON_BAT) {
		if(getCurrentIconFileName(resID, file) < 0) {
			db_error("get current statusbar icon pic bmp failed\n");	
			return -1;
		}
	} else if(resID >= ID_MENU_LIST_VQ && resID <= ID_MENU_LIST_UNCHECKBOX_PIC) {
		err_code = getMLPICFileName(resID, type, file);	
		if(err_code < 0) {
			db_error("get menu_list pic bmp failed\n");	
			return -1;
		}
	} else if(resID == ID_SUBMENU_CHOICE_PIC){
		file = rMenuList.choicePic;
#endif

	} else if(resID == ID_PLAYBACKPREVIEW_ICON || resID == ID_PLAYBACK_ICON) {
		if(getCurrentIconFileName(resID, file) < 0) {
			db_error("get current icon pic bmp failed\n");	
			return -1;
		}
	} else if(resID == ID_POWEROFF) {
		file = rPoweroffPic;
	} else {
		db_error("invalid resID: %d\n", resID);
		return -1;
	}

	db_msg("resID: %d, bmp file is %s\n", resID, file.string());	

	err_code = LoadBitmapFromFile(HDC_SCREEN, &bmp, file.string());	
	if(err_code != ERR_BMP_OK) {
		db_error("load %s bitmap failed\n", file.string());
	}
	db_msg("LoadBitmapFromFile %s finished\n", file.string());

	return 0;
}

int ResourceManager::getResBmpSubMenuCheckbox(enum ResourceID resID, bool isHilight, BITMAP &bmp)
{

	bool isChecked = false;
	switch(resID) {
	case ID_MENU_LIST_POR:
		if(menuDataPOREnable == true)
			isChecked = true;
		break;
	case ID_MENU_LIST_GPS:
		if(menuDataGPSEnable == true)
			isChecked = true;
		break;
	case ID_MENU_LIST_WIFI:
		if(menuDataWIFIEnable == true)
			isChecked = true;
		break;
	case ID_MENU_LIST_AP:
		if(menuDataAPEnable == true)
			isChecked = true;
		break;
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
		if(menuDataCELLULAREnable == true)
			isChecked = true;
		break;
#endif
	case ID_MENU_LIST_SILENTMODE:
		if(menuDataVolumeEnable == true)
			isChecked = true;
		break;
	case ID_MENU_LIST_TWM:
		if(menuDataTWMEnable == true)
			isChecked = true;
		break;
	case ID_MENU_LIST_AWMD:
		if(menuDataAWMDEnable == true)
			isChecked = true;
		break;
	default:
		db_error("invalid resID: %d\n", resID);
		return -1;
	}

	if(isChecked == true) {
		if(isHilight == true)
			return getResBmp(ID_MENU_LIST_CHECKBOX_PIC, BMPTYPE_SELECTED, bmp);
		else
			return getResBmp(ID_MENU_LIST_CHECKBOX_PIC, BMPTYPE_UNSELECTED, bmp);
	} else {
		if(isHilight == true)
			return getResBmp(ID_MENU_LIST_UNCHECKBOX_PIC, BMPTYPE_SELECTED, bmp);
		else	
			return getResBmp(ID_MENU_LIST_UNCHECKBOX_PIC, BMPTYPE_UNSELECTED, bmp);
	}

}
#ifdef USE_NEWUI
int ResourceManager::getResIconCurrent(enum ResourceID resID)
{
	int current = -1;
	switch(resID) {
	case ID_MENUBAR_ICON_WIN0:
		current = rMenuBarData.win0Icon.current;
		break;
	case ID_MENUBAR_ICON_WIN1:
		current = rMenuBarData.win1Icon.current;
		break;
	case ID_MENUBAR_ICON_WIN2:
		current = rMenuBarData.win2Icon.current;
		break;
	case ID_MENUBAR_ICON_WIN3:
		current = rMenuBarData.win3Icon.current;
		break;
	default:
		db_error("invalid resID: %d\n", resID);
		break;
	}
	return current;
}
#endif
int ResourceManager::getResIntValue(enum ResourceID resID, enum IntValueType type)
{
	unsigned int intValue = -1;

	if(resID >= ID_MENU_LIST_VQ && resID <= ID_MENU_LIST_FIRMWARE) {
		if(type == INTVAL_SUBMENU_INDEX) {
			return getSubMenuCurrentIndex(resID);
		} else if(type == INTVAL_SUBMENU_COUNT) {
			return getSubMenuCounts(resID);
		} else {
			return getSubMenuIntAttr(resID, type);
		}
	} else {
		switch(resID) {
		#ifdef USE_NEWUI
		case ID_MENU_LIST_ITEM_HEIGHT:
				intValue = rMenuList.itemHeight;
			break;
		#endif
		case ID_PLAYBACKPREVIEW_MENU:
			if(type == INTVAL_ITEMWIDTH)
				intValue = rPlayBackPreviewData.popupMenuItemWidth;
			else if(type == INTVAL_ITEMHEIGHT) 
				intValue = rPlayBackPreviewData.popupMenuItemHeight;
			else
				db_error("invalid IntValueType: %d\n", type);
			break;
		case ID_PLAYBACKPREVIEW_MB:
			if(type == INTVAL_ITEMWIDTH)
				intValue = rPlayBackPreviewData.messageBoxItemWidth;
			else if(type == INTVAL_ITEMHEIGHT)
				intValue = rPlayBackPreviewData.messageBoxItemHeight;
			else
				db_error("invalid IntValueType: %d\n", type);
			break;
		case ID_CONNECT2PC:
			if(type == INTVAL_ITEMWIDTH)
				intValue = rConnectToPC.itemWidth;
			else if(type == INTVAL_ITEMHEIGHT)
				intValue = rConnectToPC.itemHeight;
			else
				db_error("invalid IntValueType: %d\n", type);
			break;
		case ID_TIPLABEL:
			if(type == INTVAL_TITLEHEIGHT)
				intValue = rTipLabel.titleHeight;
			else
				db_error("invalid IntValueType: %d\n", type);
			break;
		default:
			db_error("invalid resID: %d\n", resID);
			break;
		}
	}

	return intValue;
}

int ResourceManager::setResIntValue(enum ResourceID resID, enum IntValueType type, int value)
{
	int ret = -1;

	if(resID >= ID_MENU_LIST_VQ && resID <= ID_MENU_LIST_FIRMWARE) {
		if(type == INTVAL_SUBMENU_INDEX) {
			ret = setSubMenuCurrentIndex(resID, value);
		} else {
			db_error("invalide IntValueType: %d\n", type);
		}
	} else {
		db_error("invalid resID %d\n", resID);
	}
	syncConfigureToDisk();

	return ret;
}

bool ResourceManager::getResBoolValue(enum ResourceID resID)
{
	db_msg("getResBoolValue: resID: %d\n", resID);
	bool value = false;
	switch(resID) {
	case ID_MENU_LIST_POR:
		value = menuDataPOREnable;
		db_info("POR: value is %d\n", value);
		break;
	case ID_MENU_LIST_GPS:
		value = menuDataGPSEnable;
		db_info("GPS: value is %d\n", value);
		break;
	case ID_MENU_LIST_WIFI:
		value = menuDataWIFIEnable;
		db_info("WIFI: value is %d\n", value);
		break;
	case ID_MENU_LIST_AP:
		value = menuDataAPEnable;
		db_info("AP: value is %d\n", value);
		break;
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
		value = menuDataCELLULAREnable;
		db_info("CELLULAR: value is %d\n", value);
		break;
#endif
	case ID_MENU_LIST_SILENTMODE:
		value = menuDataVolumeEnable;
		break;
	case ID_MENU_LIST_TWM:
		value = menuDataTWMEnable;
		break;
	case ID_MENU_LIST_AWMD:
		value = menuDataAWMDEnable;
		db_info("AWMD: value is %d\n", value);
		break;
	default:
		db_error("invalid resID %d\n", resID);
	}

	return value;
}

int ResourceManager::setResBoolValue(enum ResourceID resID, bool value)
{
	db_msg("setResBoolValue: resID: %d, value: %d\n", resID, value);
	switch(resID) {
	case ID_MENU_LIST_POR:
		menuDataPOREnable = value;
		break;
	case ID_MENU_LIST_GPS:
		menuDataGPSEnable = value;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_GPS, (WPARAM)value, 0);
#endif
		break;
	case ID_MENU_LIST_WIFI:
		menuDataWIFIEnable = value;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_WIFI, (WPARAM)value, 0);
#endif
		break;
	case ID_MENU_LIST_AP:
		menuDataAPEnable = value;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_AP, (WPARAM)value, 0);
#endif
		break;
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
		menuDataCELLULAREnable = value;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_ENABLE_DISABLE_CELLULAR, (WPARAM)value, 0);
#endif
		break;
#endif
	case ID_STATUSBAR_ICON_AUDIO:
	case ID_MENU_LIST_SILENTMODE:
		menuDataVolumeEnable = value;
		if(value == true)
			rStatusBarData.audioIcon.current = 1;
		else
			rStatusBarData.audioIcon.current = 0;

		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_AUDIO_SILENT, (WPARAM)value, 0);
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VOICE, (WPARAM)value, 0);
		break;
	case ID_MENU_LIST_TWM:
		menuDataTWMEnable = value;
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_WATER_MARK, (WPARAM)value, 0);
		break;
	case ID_MENU_LIST_AWMD:
		menuDataAWMDEnable = value;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_AWMD, (WPARAM)value, 0);
#endif
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_AWMD, (WPARAM)value, 0);
		break;
	default:
		db_error("invalide resID: %d\n", resID);
		return -1;
	}
	syncConfigureToDisk();

	return 0;
}

const char* ResourceManager::getResMenuItemString(enum ResourceID resID)
{
	const char* ptr = NULL;
	switch(resID) {
	case ID_MENU_LIST_VQ:
		ptr = getLabel(LANG_LABEL_MENU_VQ);
		break;
	case ID_MENU_LIST_PQ:
		ptr = getLabel(LANG_LABEL_MENU_PQ);
		break;
	case ID_MENU_LIST_VTL:
		ptr = getLabel(LANG_LABEL_MENU_VTL);
		break;
	case ID_MENU_LIST_AWMD:
		ptr = getLabel(LANG_LABEL_MENU_AWMD);
		break;
	case ID_MENU_LIST_WB:
		ptr = getLabel(LANG_LABEL_MENU_WB);
		break;
	case ID_MENU_LIST_CONTRAST:
		ptr = getLabel(LANG_LABEL_MENU_CONTRAST);
		break;
	case ID_MENU_LIST_EXPOSURE:
		ptr = getLabel(LANG_LABEL_MENU_EXPOSURE);
		break;
	case ID_MENU_LIST_POR:
		ptr = getLabel(LANG_LABEL_MENU_POR);
		break;
	case ID_MENU_LIST_GPS:
		ptr = getLabel(LANG_LABEL_MENU_GPS);
		break;
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
		ptr = getLabel(LANG_LABEL_MENU_CELLULAR);
		break;
#endif
	case ID_MENU_LIST_SS:
		ptr = getLabel(LANG_LABEL_MENU_SS);
		break;
	case ID_MENU_LIST_SILENTMODE:
		ptr = getLabel(LANG_LABEL_MENU_SILENTMODE);
		break;
	case ID_MENU_LIST_DATE:
		ptr = getLabel(LANG_LABEL_MENU_DATE);
		break;
	case ID_MENU_LIST_WIFI:
		ptr = getLabel(LANG_LABEL_MENU_WIFI);
		break;
	case ID_MENU_LIST_AP:
		ptr = getLabel(LANG_LABEL_MENU_AP);
		break;
	case ID_MENU_LIST_LANG:
		ptr = getLabel(LANG_LABEL_MENU_LANG);
		break;
	case ID_MENU_LIST_TWM:
		ptr = getLabel(LANG_LABEL_MENU_TWM);
		break;
	case ID_MENU_LIST_FORMAT:
		ptr = getLabel(LANG_LABEL_MENU_FORMAT);
		break;
	case ID_MENU_LIST_FRESET:
		ptr = getLabel(LANG_LABEL_MENU_FRESET);
		break;
	case ID_MENU_LIST_FIRMWARE:
		ptr = getLabel(LANG_LABEL_MENU_FIRMWARE);
		break;
		default:
		db_error("invalid resIndex %d\n", resID);
		return NULL;
	}

	return ptr;
}
#ifdef USE_NEWUI
int ResourceManager::getResMenuContent0Cmd(enum ResourceID resID)
{
	int Content0Cmd;
	switch(resID) {
	case ID_MENU_LIST_TWM:
		Content0Cmd = LANG_LABEL_SUBMENU_TWM_CONTENT0;
		break;
	case ID_MENU_LIST_SILENTMODE:
		Content0Cmd = LANG_LABEL_SUBMENU_SILENTMODE_CONTENT0;
		break;
	case ID_MENU_LIST_WIFI:
		Content0Cmd = LANG_LABEL_SUBMENU_WIFI_CONTENT0;
		break;
	case ID_MENU_LIST_GPS:
		Content0Cmd = LANG_LABEL_SUBMENU_GPS_CONTENT0;
		break;
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
		Content0Cmd = LANG_LABEL_SUBMENU_CELLULAR_CONTENT0;
		break;
#endif
	case ID_MENU_LIST_AWMD:
		Content0Cmd = LANG_LABEL_SUBMENU_AWMD_CONTENT0;
		break;
	case ID_MENU_LIST_POR:
		Content0Cmd = LANG_LABEL_SUBMENU_POR_CONTENT0;
		break;
	case ID_MENU_LIST_VQ:
		Content0Cmd = LANG_LABEL_SUBMENU_VQ_CONTENT0;
		break;
	case ID_MENU_LIST_PQ:
		Content0Cmd = LANG_LABEL_SUBMENU_PQ_CONTENT0;
		break;
	case ID_MENU_LIST_VTL:
		Content0Cmd = LANG_LABEL_SUBMENU_VTL_CONTENT0;
		break;
	case ID_MENU_LIST_VM:
		Content0Cmd = LANG_LABEL_SUBMENU_VM_CONTENT0;
		break;
	case ID_MENU_LIST_WB:
		Content0Cmd = LANG_LABEL_SUBMENU_WB_CONTENT0;
		break;
	case ID_MENU_LIST_CONTRAST:
		Content0Cmd = LANG_LABEL_SUBMENU_CONTRAST_CONTENT0;
		break;
	case ID_MENU_LIST_EXPOSURE:
		Content0Cmd = LANG_LABEL_SUBMENU_EXPOSURE_CONTENT0;
		break;
	case ID_MENU_LIST_SS:
		Content0Cmd = LANG_LABEL_SUBMENU_SS_CONTENT0;
		break;
	case ID_MENU_LIST_LANG:
		Content0Cmd = LANG_LABEL_SUBMENU_LANG_CONTENT0;
		break;
	default:
		db_error("invalid resIndex %d\n", resID);
		return -1;
	}

	return Content0Cmd;
}
#endif

#define CHECK_CURRENT_VALID(menuItem) do{ \
	if(menuItem.current >= menuItem.count) { \
		db_error("invalid current value\n"); break; } \
}while(0)
const char* ResourceManager::getResSubMenuCurString(enum ResourceID resID)
{
	const char* ptr = NULL;
	unsigned int current;
	switch(resID) {
	case ID_MENU_LIST_LANG:
		CHECK_CURRENT_VALID(menuDataLang);
		current = menuDataLang.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_LANG_CONTENT0 + current);
		break;
	case ID_MENU_LIST_VQ:
		CHECK_CURRENT_VALID(menuDataVQ);
		current = menuDataVQ.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_VQ_CONTENT0 + current);
		break;
#ifdef USE_NEWUI
	case ID_MENU_LIST_VM:
		CHECK_CURRENT_VALID(menuDataVM);
		current = menuDataVM.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_VM_CONTENT0 + current);
		break;
#endif
	case ID_MENU_LIST_PQ:
		CHECK_CURRENT_VALID(menuDataPQ);
		current = menuDataPQ.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_PQ_CONTENT0 + current);
		break;
	case ID_MENU_LIST_VTL:
		CHECK_CURRENT_VALID(menuDataVTL);
		current = menuDataVTL.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_VTL_CONTENT0 + current);
		break;
	case ID_MENU_LIST_WB:
		CHECK_CURRENT_VALID(menuDataWB);
		current = menuDataWB.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_WB_CONTENT0 + current);
		break;
	case ID_MENU_LIST_CONTRAST:
		CHECK_CURRENT_VALID(menuDataContrast);
		current = menuDataContrast.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_CONTRAST_CONTENT0 + current);
		break;
	case ID_MENU_LIST_EXPOSURE:
		CHECK_CURRENT_VALID(menuDataExposure);
		current = menuDataExposure.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_EXPOSURE_CONTENT0 + current);
		break;
	case ID_MENU_LIST_SS:
		CHECK_CURRENT_VALID(menuDataSS);
		current = menuDataSS.current;
		ptr = getLabel(LANG_LABEL_SUBMENU_SS_CONTENT0 + current);
		break;
	case ID_MENU_LIST_AP:
		ptr = "carSoftAP";
		break;
	case ID_MENU_LIST_FIRMWARE:
		ptr = FIRMWARE_VERSION;
		break;
	default:
		db_error("invalid resIndex %d\n", resID);
		return NULL;
	}

	return ptr;
}

#ifdef USE_NEWUI
const char* ResourceManager::getResMenuTitle(enum ResourceID resID)
#else
const char* ResourceManager::getResSubMenuTitle(enum ResourceID resID)
#endif
{
	const char* ptr = NULL;
	switch(resID) {
	case ID_MENU_LIST_LANG:
		ptr = getLabel(LANG_LABEL_SUBMENU_LANG_TITLE);
		break;
	case ID_MENU_LIST_VQ:
		ptr = getLabel(LANG_LABEL_SUBMENU_VQ_TITLE);
		break;
	case ID_MENU_LIST_PQ:
		ptr = getLabel(LANG_LABEL_SUBMENU_PQ_TITLE);
		break;
	case ID_MENU_LIST_VTL:
		ptr = getLabel(LANG_LABEL_SUBMENU_VTL_TITLE);
		break;
	case ID_MENU_LIST_WB:
		ptr = getLabel(LANG_LABEL_SUBMENU_WB_TITLE);
		break;
	case ID_MENU_LIST_CONTRAST:
		ptr = getLabel(LANG_LABEL_SUBMENU_CONTRAST_TITLE);
		break;
	case ID_MENU_LIST_EXPOSURE:
		ptr = getLabel(LANG_LABEL_SUBMENU_EXPOSURE_TITLE);
		break;
	case ID_MENU_LIST_SS:
		ptr = getLabel(LANG_LABEL_SUBMENU_SS_TITLE);
		break;
#ifdef USE_NEWUI
	case ID_MENU_LIST_VM:
		ptr = getLabel(LANG_LABEL_SUBMENU_VM_TITLE);
		break;
	case ID_MENU_LIST_VIDEO_SETTING:
	case ID_MENU_LIST_PHOTO_SETTING:
		ptr = getLabel(LANG_LABEL_MENU);
		break;
	case ID_MENU_LIST_AWMD:
		ptr = getLabel(LANG_LABEL_SUBMENU_AWMD_TITLE);
		break;
	case ID_MENU_LIST_POR:
		ptr = getLabel(LANG_LABEL_SUBMENU_POR_TITLE);
		break;
	case ID_MENU_LIST_GPS:
		ptr = getLabel(LANG_LABEL_SUBMENU_GPS_TITLE);
		break;
#ifdef MODEM_ENABLE
	case ID_MENU_LIST_CELLULAR:
		ptr = getLabel(LANG_LABEL_SUBMENU_CELLULAR_TITLE);
		break;
#endif
	case ID_MENU_LIST_SILENTMODE:
		ptr = getLabel(LANG_LABEL_SUBMENU_SILENTMODE_TITLE);
		break;
	case ID_MENU_LIST_WIFI:
		ptr = getLabel(LANG_LABEL_SUBMENU_WIFI_TITLE);
		break;
	case ID_MENU_LIST_TWM:
		ptr = getLabel(LANG_LABEL_SUBMENU_TWM_TITLE);
		break;
#endif
	default:
		db_error("invalid resIndex %d\n", resID);
		return NULL;
	}

	return ptr;
}

LANGUAGE ResourceManager::getLanguage(void)
{

	return lang;
}

PLOGFONT ResourceManager::getLogFont(void)
{

	return mLogFont;
}

int ResourceManager::getResVTLms(void)
{
	int value;
	value = menuDataVTL.current;
	if(value == 0)
		return 60 * 1000;
	else if(value == 1)
		return 120 * 1000;
	else
		return -1;
}



int ResourceManager::getWindPicFileName(enum BmpType type, String8 &file)
{

	switch(type) {
	case BMPTYPE_WINDOWPIC_RECORDPREVIEW:
		file = rStatusBarData.recordPreviewPic;
		break;
	case BMPTYPE_WINDOWPIC_PHOTOGRAPH:
		file = rStatusBarData.screenShotPic;
		break;
	case BMPTYPE_WINDOWPIC_PLAYBACKPREVIEW:
		file = rStatusBarData.playBackPreviewPic;
		break;
	case BMPTYPE_WINDOWPIC_PLAYBACK:
		file = rStatusBarData.playBackPic;
		break;
	case BMPTYPE_WINDOWPIC_MENU:
		file = rStatusBarData.menuPic;
		break;
	default:
		db_error("invalid BmpType %d\n", type);
		return -1;
	}

	return 0;
}

#define CHECK_CURRENT_ICON_VALID(curIcon) do{ \
	if(curIcon.current >= curIcon.icon.size()) { \
		db_error("invalid current value\n"); break; } \
}while(0)
int ResourceManager::getCurrentIconFileName(enum ResourceID resID, String8 &file)
{

	int current;
	switch(resID) {
	case ID_STATUSBAR_ICON_AWMD:
		CHECK_CURRENT_ICON_VALID(rStatusBarData.awmdIcon);
		current = rStatusBarData.awmdIcon.current;
		file = rStatusBarData.awmdIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_LOCK:
		CHECK_CURRENT_ICON_VALID(rStatusBarData.lockIcon);
		current = rStatusBarData.lockIcon.current;
		file = rStatusBarData.lockIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_UVC:
		CHECK_CURRENT_ICON_VALID(rStatusBarData.uvcIcon);
		current = rStatusBarData.uvcIcon.current;
		file = rStatusBarData.uvcIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_TFCARD:	
		CHECK_CURRENT_ICON_VALID(rStatusBarData.tfCardIcon);
		current = rStatusBarData.tfCardIcon.current;
		file = rStatusBarData.tfCardIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_GPS:	
		CHECK_CURRENT_ICON_VALID(rStatusBarData.gpsIcon);
		current = rStatusBarData.gpsIcon.current;
		file = rStatusBarData.gpsIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_WIFI:	
		CHECK_CURRENT_ICON_VALID(rStatusBarData.wifiIcon);
		current = rStatusBarData.wifiIcon.current;
		file = rStatusBarData.wifiIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_AP:	
		CHECK_CURRENT_ICON_VALID(rStatusBarData.apIcon);
		current = rStatusBarData.apIcon.current;
		file = rStatusBarData.apIcon.icon.itemAt(current);
		break;
#ifdef MODEM_ENABLE
	case ID_STATUSBAR_ICON_CELLULAR:
		CHECK_CURRENT_ICON_VALID(rStatusBarData.cellularIcon);
		current = rStatusBarData.cellularIcon.current;
		file = rStatusBarData.cellularIcon.icon.itemAt(current);
		break;
#endif
	case ID_STATUSBAR_ICON_AUDIO:	
		CHECK_CURRENT_ICON_VALID(rStatusBarData.audioIcon);
		current = rStatusBarData.audioIcon.current;
		file = rStatusBarData.audioIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_BAT :	
		CHECK_CURRENT_ICON_VALID(rStatusBarData.batteryIcon);
		current = rStatusBarData.batteryIcon.current;
		file = rStatusBarData.batteryIcon.icon.itemAt(current);
		break;
#ifdef USE_NEWUI
	case ID_STATUSBAR_ICON_VALUE:
		CHECK_CURRENT_ICON_VALID(rStatusBarData.valueIcon);
		current = rStatusBarData.valueIcon.current;
		file = rStatusBarData.valueIcon.icon.itemAt(current);
		break;
	case ID_STATUSBAR_ICON_MODE:
		CHECK_CURRENT_ICON_VALID(rStatusBarData.modeIcon);
		current = rStatusBarData.modeIcon.current;
		file = rStatusBarData.modeIcon.icon.itemAt(current);
		break;
	case ID_MENUBAR_ICON_WIN0:
		CHECK_CURRENT_ICON_VALID(rMenuBarData.win0Icon);
		current = rMenuBarData.win0Icon.current;
		file = rMenuBarData.win0Icon.icon.itemAt(current);
		break;
	case ID_MENUBAR_ICON_WIN1:
		CHECK_CURRENT_ICON_VALID(rMenuBarData.win1Icon);
		current = rMenuBarData.win1Icon.current;
		file = rMenuBarData.win1Icon.icon.itemAt(current);
		break;
	case ID_MENUBAR_ICON_WIN2:
		CHECK_CURRENT_ICON_VALID(rMenuBarData.win2Icon);
		current = rMenuBarData.win2Icon.current;
		file = rMenuBarData.win2Icon.icon.itemAt(current);
		break;
	case ID_MENUBAR_ICON_WIN3:
		CHECK_CURRENT_ICON_VALID(rMenuBarData.win3Icon);
		current = rMenuBarData.win3Icon.current;
		file = rMenuBarData.win3Icon.icon.itemAt(current);
		break;
#endif
	case ID_PLAYBACKPREVIEW_ICON:
		CHECK_CURRENT_ICON_VALID(rPlayBackPreviewData.icon);
		current = rPlayBackPreviewData.icon.current;
		file = rPlayBackPreviewData.icon.icon.itemAt(current);
		break;
	case ID_PLAYBACK_ICON:
		CHECK_CURRENT_ICON_VALID(rPlayBackData.icon);
		current = rPlayBackData.icon.current;
		file = rPlayBackData.icon.icon.itemAt(current);
		break;
	default:
		db_error("invalid resID %d\n", resID);
		return -1;
	}

	return 0;
}

#ifndef USE_NEWUI
int ResourceManager::getMLPICFileName(enum ResourceID resID, enum BmpType type, String8 &file)
{
	if(resID >= ID_MENU_LIST_VQ && resID <= ID_MENU_LIST_UNCHECKBOX_PIC) {
		switch(resID) {
		case ID_MENU_LIST_VQ:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.videoQualityIcon.icon.itemAt(1);
			else
				file = rMenuList.videoQualityIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_PQ:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.photoQualityIcon.icon.itemAt(1);
			else
				file = rMenuList.photoQualityIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_VTL:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.timeLengthIcon.icon.itemAt(1);
			else
				file = rMenuList.timeLengthIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_AWMD:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.moveDetectIcon.icon.itemAt(1);
			else
				file = rMenuList.moveDetectIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_WB:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.whiteBalanceIcon.icon.itemAt(1);
			else
				file = rMenuList.whiteBalanceIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_CONTRAST:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.contrastIcon.icon.itemAt(1);
			else
				file = rMenuList.contrastIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_EXPOSURE:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.exposureIcon.icon.itemAt(1);
			else
				file = rMenuList.exposureIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_POR:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.PORIcon.icon.itemAt(1);
			else
				file = rMenuList.PORIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_GPS:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.GPSIcon.icon.itemAt(1);
			else
				file = rMenuList.GPSIcon.icon.itemAt(0);
			break;
#ifdef MODEM_ENABLE
		case ID_MENU_LIST_CELLULAR:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.CELLULARIcon.icon.itemAt(1);
			else
				file = rMenuList.CELLULARIcon.icon.itemAt(0);
			break;
#endif
		case ID_MENU_LIST_SS:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.screenSwitchIcon.icon.itemAt(1);
			else
				file = rMenuList.screenSwitchIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_SILENTMODE:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.volumeIcon.icon.itemAt(1);
			else
				file = rMenuList.volumeIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_DATE:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.dateIcon.icon.itemAt(1);
			else
				file = rMenuList.dateIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_WIFI:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.wifiIcon.icon.itemAt(1);
			else
				file = rMenuList.wifiIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_AP:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.apIcon.icon.itemAt(1);
			else
				file = rMenuList.apIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_LANG:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.languageIcon.icon.itemAt(1);
			else
				file = rMenuList.languageIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_TWM:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.TWMIcon.icon.itemAt(1);
			else
				file = rMenuList.TWMIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_FORMAT:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.formatIcon.icon.itemAt(1);
			else
				file = rMenuList.formatIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_FRESET:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.factoryResetIcon.icon.itemAt(1);
			else
				file = rMenuList.factoryResetIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_FIRMWARE:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.firmwareIcon.icon.itemAt(1);
			else
				file = rMenuList.firmwareIcon.icon.itemAt(0);
			break;
		case ID_MENU_LIST_UNFOLD_PIC:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.unfoldPressPic;
			else
				file = rMenuList.unfoldNormalPic;
			break;
		case ID_MENU_LIST_CHECKBOX_PIC:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.checkedPressPic;
			else
				file = rMenuList.checkedNormalPic;
			break;
		case ID_MENU_LIST_UNCHECKBOX_PIC:
			if(type == BMPTYPE_SELECTED)
				file = rMenuList.uncheckedPressPic;
			else
				file = rMenuList.uncheckedNormalPic;
			break;
		default:
			break;
		}
	} else {
		db_error("invalid resID %d\n", resID);
		return -1;
	}

	return 0;
}
#endif

int ResourceManager::getSubMenuCurrentIndex(enum ResourceID resID)
{
	int index;

	switch(resID) {
	case ID_MENU_LIST_VQ:
		index = menuDataVQ.current;
		break;
#ifdef USE_NEWUI
	case ID_MENU_LIST_VM:
		index = menuDataVM.current;
		break;
#endif
	case ID_MENU_LIST_PQ:
		index = menuDataPQ.current;
		break;
	case ID_MENU_LIST_VTL:
		index = menuDataVTL.current;
		break;
	case ID_MENU_LIST_WB:
		index = menuDataWB.current;
		break;
	case ID_MENU_LIST_CONTRAST:
		index = menuDataContrast.current;
		break;
	case ID_MENU_LIST_EXPOSURE:
		index = menuDataExposure.current;
		break;
	case ID_MENU_LIST_SS:
		index = menuDataSS.current;
		break;
	case ID_MENU_LIST_LANG:
		index = menuDataLang.current;
		break;
	default:
		db_error("invalid resID: %d\n", resID);
		index = -1;
		break;
	}

	return index;
}

int ResourceManager::getSubMenuCounts(enum ResourceID resID)
{

	int counts;

	switch(resID) {
	case ID_MENU_LIST_VQ:
		counts = menuDataVQ.count;
		break;
#ifdef USE_NEWUI
	case ID_MENU_LIST_VM:
		counts = menuDataVM.count;
		break;
#endif
	case ID_MENU_LIST_PQ:
		counts = menuDataPQ.count;
		break;
	case ID_MENU_LIST_VTL:
		counts = menuDataVTL.count;
		break;
	case ID_MENU_LIST_WB:
		counts = menuDataWB.count;
		break;
	case ID_MENU_LIST_CONTRAST:
		counts = menuDataContrast.count;
		break;
	case ID_MENU_LIST_EXPOSURE:
		counts = menuDataExposure.count;
		break;
	case ID_MENU_LIST_SS:
		counts = menuDataSS.count;
		break;
	case ID_MENU_LIST_LANG:
		counts = menuDataLang.count;
		break;
	default:
		db_error("invalid resID: %d\n", resID);
		counts = -1;
		break;
	}

	return counts;
}

int ResourceManager::getSubMenuIntAttr(enum ResourceID resID, enum IntValueType type)
{
	int intValue = -1;
	if(resID == ID_MENU_LIST_DATE) {
		switch(type) {
		case INTVAL_TITLEHEIGHT:
			intValue = rMenuList.dateTileRectH;
			break;
		case INTVAL_HBORDER:
			intValue = rMenuList.dateHBorder;
			break;
		case INTVAL_YEARWIDTH:
			intValue = rMenuList.dateYearW;
			break;
		case INTVAL_LABELWIDTH:
			intValue = rMenuList.dateLabelW;
			break;
		case INTVAL_NUMBERWIDTH:
			intValue = rMenuList.dateNumberW;
			break;
		case INTVAL_BOXHEIGHT:
			intValue = rMenuList.dateBoxH;
			break;	
		default:
			db_error("invalid IntValueType: %d\n", type);
		}
	} else {
		db_error("invalid ResourceID: %d\n", resID);	
	}

	return intValue;
}

int ResourceManager::setSubMenuCurrentIndex(enum ResourceID resID, int newSel)
{

	int count;
	switch(resID) {
	case ID_MENU_LIST_LANG:
		count = menuDataLang.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataLang.current = newSel;
		updateLangandFont(newSel);
		break;
	case ID_MENU_LIST_VQ:
		count = menuDataVQ.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataVQ.current = newSel;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VIDEO_PHOTO_VALUE, (WPARAM)(newSel+1), 0);
#endif
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_VIDEO_QUALITY, (WPARAM)newSel, 0);
		break;
#ifdef USE_NEWUI
	case ID_MENU_LIST_VM:
		count = menuDataVM.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataVM.current = newSel;
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VIDEO_MODE, (WPARAM)(newSel+1), 0);
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_VIDEO_MODE, (WPARAM)newSel, 0);
		break;
#endif
	case ID_MENU_LIST_PQ:
		count = menuDataPQ.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataPQ.current = newSel;
#ifdef USE_NEWUI
		SendMessage(mHwnd[WINDOWID_STATUSBAR], STBM_VIDEO_PHOTO_VALUE, (WPARAM)(newSel+3), 0);
#endif
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_PIC_QUALITY, (WPARAM)newSel, 0);
		break;
	case ID_MENU_LIST_VTL:
		count = menuDataVTL.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataVTL.current = newSel;
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_VIDEO_TIME_LENGTH, (WPARAM)newSel, 0);
		break;
	case ID_MENU_LIST_WB:
		count = menuDataWB.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataWB.current = newSel;
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_WHITEBALANCE, (WPARAM)newSel, 0);
		break;
	case ID_MENU_LIST_CONTRAST:
		count = menuDataContrast.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataContrast.current = newSel;
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_CONTRAST, (WPARAM)newSel, 0);
		break;
	case ID_MENU_LIST_EXPOSURE:
		count = menuDataExposure.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataExposure.current = newSel;
		SendMessage(mHwnd[WINDOWID_RECORDPREVIEW], MSG_RM_EXPOSURE, (WPARAM)newSel, 0);
		break;
	case ID_MENU_LIST_SS:
		count = menuDataSS.count;
		if(newSel >= count) {
			db_error("invalid value: %d, count is %d\n", newSel, count);
			return -1;
		}
		menuDataSS.current = newSel;
		SendMessage(mHwnd[WINDOWID_MAIN], MSG_RM_SCREENSWITCH, (WPARAM)newSel, 0);
		break;
	default:
		db_error("invalid resID: %d\n", resID);
		return -1;
	}

	return 0;
}

void ResourceManager::updateLangandFont(int langValue)
{
	LANGUAGE newLang;

	if(langValue == LANG_TW)
		newLang = LANG_TW;
	else if(langValue == LANG_CN)
		newLang = LANG_CN;
	else if(langValue == LANG_EN)
		newLang = LANG_EN;
	else if(langValue == LANG_JPN)
		newLang = LANG_JPN;
	else if(langValue == LANG_KR)
		newLang = LANG_KR;
	else if(langValue == LANG_RS)
		newLang = LANG_RS;
	else return;
	if(newLang == lang)
		return;
	
	if( !(newLang == LANG_CN && lang == LANG_EN) && !(newLang == LANG_EN && lang == LANG_CN)) {
		db_msg("detroy logfont\n");
		DestroyLogFont(mLogFont);
		mLogFont = NULL;
		if(newLang == LANG_TW) {
			mLogFont = CreateLogFont ("*", "ming", "BIG5",
					FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
					FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0); 
		}else if(newLang == LANG_CN || newLang == LANG_EN){
			mLogFont = CreateLogFont("*", "fixed", "GB2312-0", 
					FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
					FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
		}else if(newLang == LANG_RS){
			mLogFont = CreateLogFont("*", "fixed", "RUSSAIN", 
					FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
					FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
		}else if(newLang == LANG_JPN){
			mLogFont = CreateLogFont("*", "shift", FONT_CHARSET_JISX0208_1, 
					FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
					FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
		}else if(newLang == LANG_KR){
			mLogFont = CreateLogFont("*", "euckr", FONT_CHARSET_EUCKR, 
					FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
					FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, mFontSize, 0);
		}
	}

	lang = newLang;
	db_msg("new lang is %d\n", lang);

	initLabel(); /* init lang labels */
	SendMessage(mHwnd[WINDOWID_MAIN], MSG_RM_LANG_CHANGED, (WPARAM)&lang, 0);
}

int ResourceManager::loadBmpFromConfig(const char* pSection, const char* pKey, PBITMAP bmp)
{

	int retval;
	char buf[CDR_FILE_LENGHT] = {0};

	if((retval = GetValueFromEtcFile(configFile, pSection, pKey, buf, sizeof(buf))) != ETC_OK) {
		db_error("get %s->%s failed: %s\n", pSection, pKey, pEtcError(retval));
		return -1;
	}

	if( (retval = LoadBitmapFromFile(HDC_SCREEN, bmp, buf)) != ERR_BMP_OK) {
		db_error("load %s failed: %s\n", buf, pBmpError(retval));
		return -1;
	}

	return 0;
}

void ResourceManager::unloadBitMap(BITMAP &bmp)
{
	if(bmp.bmBits != NULL) {
		UnloadBitmap(&bmp);
	}
}

static char err_buf[100];
char* ResourceManager::pEtcError(int err_code)
{
	bzero(err_buf, sizeof(err_buf));
	switch(err_code) 
	{
	case ETC_FILENOTFOUND:
		sprintf(err_buf, "The etc file not found");
		break;
	case ETC_SECTIONNOTFOUND:
		sprintf(err_buf, "The section not found");
		break;
	case ETC_KEYNOTFOUND:
		sprintf(err_buf, "The key not found");
		break;
	case ETC_TMPFILEFAILED:
		sprintf(err_buf, "Create tmpfile failed");
		break;
	case ETC_FILEIOFAILED:
		sprintf(err_buf, "IO operation failed to etc file");
		break;
	case ETC_INTCONV:
		sprintf(err_buf, "Convert the value string to an integer failed");
		break;
	case ETC_INVALIDOBJ:
		sprintf(err_buf, "Invalid object to etc file");
		break;
	case ETC_READONLYOBJ:
		sprintf(err_buf, "Read only to etc file");
		break;
	default:
		sprintf(err_buf, "Unkown error code %d", err_code);
		break;
	}

	return err_buf;
}

char* ResourceManager::pBmpError(int err_code)
{
	bzero(err_buf, sizeof(err_buf));
	switch(err_code)
	{
	case ERR_BMP_IMAGE_TYPE:
		sprintf(err_buf, "Not a valid bitmap");
		break;
	case ERR_BMP_UNKNOWN_TYPE:
		sprintf(err_buf, "Not recongnized bitmap type");
		break;
	case ERR_BMP_CANT_READ:
		sprintf(err_buf, "Read error");
		break;
	case ERR_BMP_CANT_SAVE:
		sprintf(err_buf, "Save error");
		break;
	case ERR_BMP_NOT_SUPPORTED:
		sprintf(err_buf, "Not supported bitmap type");
		break;
	case ERR_BMP_MEM:
		sprintf(err_buf, "Memory allocation error");
		break;
	case ERR_BMP_LOAD:
		sprintf(err_buf, "Loading error");
		break;
	case ERR_BMP_FILEIO:
		sprintf(err_buf, "I/O failed");
		break;
	case ERR_BMP_OTHER:
		sprintf(err_buf, "Other error");
		break;
	case ERR_BMP_ERROR_SOURCE:
		sprintf(err_buf, "A error data source");
		break;
	default:
		sprintf(err_buf, "Unkown error code %d", err_code);
		break;
	}

	return err_buf;
}



int ResourceManager::getCfgFileHandle(void)
{
	if (mCfgFileHandle != 0) {
		return 0;
	}
	mCfgFileHandle = LoadEtcFile(configFile);
	if(mCfgFileHandle == 0) {
		db_error("getCfgFileHandle failed\n");
		return -1;
	}
	return 0;
}

int ResourceManager::getCfgMenuHandle(void)
{
	if (mCfgMenuHandle != 0) {
		return 0;
	}
	mCfgMenuHandle = LoadEtcFile(configMenu);
	if(mCfgMenuHandle == 0) {
		db_error("getCfgMenuHandle failed\n");
		return -1;
	}

	return 0;
}

void ResourceManager::releaseCfgFileHandle(void)
{
	if(mCfgFileHandle == 0) {
		return;
	}
	UnloadEtcFile(mCfgFileHandle);
	mCfgFileHandle = 0;
}

void ResourceManager::releaseCfgMenuHandle(void)
{
	if(mCfgMenuHandle == 0) {
		return;
	}
	UnloadEtcFile(mCfgMenuHandle);
	mCfgMenuHandle = 0;
}

/*
 * On success return the int value
 * On fail return -1
 * */
int ResourceManager::getIntValueFromHandle(GHANDLE cfgHandle, const char* mainkey, const char* subkey)
{
	int retval = 0;
	int err_code;
	if(mainkey == NULL || subkey == NULL) {
		db_error("NULL pointer\n");	
		return -1;
	}

	if(cfgHandle == 0) {
		db_error("cfgHandle not initialized\n");
		return -1;
	}

	if((err_code = GetIntValueFromEtc(cfgHandle, mainkey, subkey, &retval)) != ETC_OK) {
		db_error("get %s->%s failed:%s\n", mainkey, subkey, pEtcError(err_code));
		return -1;
	}

	return retval;	
}

int ResourceManager::getValueFromFileHandle(const char*mainkey, const char* subkey, char* value, unsigned int len)
{
	int err_code;
	if(mainkey == NULL || subkey == NULL) {
		db_error("NULL pointer\n");	
		return ETC_INVALIDOBJ;
	}

	if(mCfgFileHandle == 0) {
		db_error("cfgHandle not initialized\n");
		return ETC_INVALIDOBJ;
	}

	if((err_code = GetValueFromEtc(mCfgFileHandle, mainkey, subkey, value, len)) != ETC_OK) {
		db_error("get %s->%s failed:%s\n", mainkey, subkey, pEtcError(err_code));
		return err_code;
	}

	return ETC_OK;
}

/* get the rect from configFile
 * pWindow the the section name in the config file
 * */
int ResourceManager::getRectFromFileHandle(const char *pWindow, CDR_RECT *rect)
{
	int err_code;

	if(mCfgFileHandle == 0) {
		db_error("mCfgFileHandle not initialized\n");
		return ETC_INVALIDOBJ;
	}

	if((err_code = GetIntValueFromEtc(mCfgFileHandle, pWindow, "x", &rect->x)) != ETC_OK) {
		return err_code;
	}

	if((err_code = GetIntValueFromEtc(mCfgFileHandle, pWindow, "y", &rect->y)) != ETC_OK) {
		return err_code;
	}

	if((err_code = GetIntValueFromEtc(mCfgFileHandle, pWindow, "w", &rect->w)) != ETC_OK) {
		return err_code;
	}

	if((err_code = GetIntValueFromEtc(mCfgFileHandle, pWindow, "h", &rect->h)) != ETC_OK) {
		return err_code;
	}

	return ETC_OK;
}

/*
 * get the ABGR format color from the configFile
 * On success return ETC_OK
 * On fail return err code
 * */
int ResourceManager::getABGRColorFromFileHandle(const char* pWindow, const char* subkey, gal_pixel &pixel)
{
	char buf[20] = {0};
	int err_code;
	unsigned long int hex;
	unsigned char r, g, b, a;

	if(pWindow == NULL || subkey == NULL)
		return ETC_INVALIDOBJ;

	if(mCfgFileHandle == 0) {
		db_error("mCfgFileHandle not initialized\n");
		return ETC_INVALIDOBJ;
	}

	if((err_code = GetValueFromEtc(mCfgFileHandle, pWindow, subkey, buf, sizeof(buf))) != ETC_OK) {
		return err_code;
	}

	hex = strtoul(buf, NULL, 16);

	a = (hex >> 24) & 0xff;
	b = (hex >> 16) & 0xff;
	g = (hex >> 8) & 0xff;
	r = hex & 0xff;

	pixel = RGBA2Pixel(HDC_SCREEN, r, g, b, a);

	return ETC_OK;
}

/*
 * get current icon info from configFile, and fill the giving currentIcon
 * example : if current is 1, then the icon1 is used
 * [status_bar_battery]
 *	current=1
 *	icon0=/res/topbar/baterry1.png
 *	icon1=/res/topbar/baterry2.png
 *	icon2=/res/topbar/baterry3.png
 *	icon3=/res/topbar/baterry4.png
 * On success return ETC_OK
 * On fail return err code
 * */
int ResourceManager::fillCurrentIconInfoFromFileHandle(const char* pWindow, currentIcon_t &currentIcon)
{
	int current;
	char buf[10] = {0};
	char bufIcon[50] = {0};

	if(pWindow == NULL) {
		db_msg("invalid pWindow\n");
		return ETC_INVALIDOBJ;
	}

	current = getIntValueFromHandle(mCfgFileHandle, pWindow, "current");
	if(current < 0) {
		db_error("get current value from %s failed\n", pWindow);
		return ETC_KEYNOTFOUND;
	}
	currentIcon.current = current;
	//	db_msg("current is %d\n", currentIcon.current);

	current = 0;
	do
	{
		int err_code;
		sprintf(buf, "%s%d", "icon", current);	
		//		db_msg("buf is %s\n", buf);
		err_code = GetValueFromEtc(mCfgFileHandle, pWindow, buf, bufIcon, sizeof(bufIcon));
		if(err_code != ETC_OK) {
			if(current == 0) {
				return err_code;
			} else {
				break;
			}
		}
		currentIcon.icon.add(String8(bufIcon));
		current++;
	}while(1);

	//	db_msg("%s has %u icons\n", pWindow, currentIcon.icon.size());
	for(unsigned int i = 0;  i< currentIcon.icon.size(); i++) {
		//		db_msg("icon%d is %s\n", i, currentIcon.icon.itemAt(i).string());	
	}

	if(currentIcon.current >= currentIcon.icon.size()) {
		db_error("invalide current value, current is %d icon count is %u\n", currentIcon.current, currentIcon.icon.size());	
		return ETC_INVALIDOBJ;
	}

	return ETC_OK;
}


/*
 * get language from config menu
 * On success return LANGUAGE
 * On faile return LANG_ERR
 * */
LANGUAGE ResourceManager::getLangFromMenuHandle(void)
{
	int retval;

	retval = getIntValueFromHandle(mCfgMenuHandle, "language", "current");
	if(retval == -1) {
		db_error("get current language failed\n");
		return LANG_ERR;
	}

	if(retval == LANG_CN)
		return LANG_CN;
	else if(retval == LANG_TW)
		return LANG_TW;
	else if(retval == LANG_EN)
		return LANG_EN;
	else if(retval == LANG_JPN)
		return LANG_JPN;
	else if(retval == LANG_KR)
		return LANG_KR;
	else if(retval == LANG_RS)
		return LANG_RS;

	return LANG_ERR;
}

LANGUAGE ResourceManager::getLangFromConfigFile(void)
{
	int retval;
	if(GetIntValueFromEtcFile(configMenu, "language", "current", &retval) != ETC_OK) {
		db_error("get language failed\n");
		return LANG_ERR;
	}

	if(retval == LANG_CN)
		return LANG_CN;
	else if(retval == LANG_TW)
		return LANG_TW;
	else if(retval == LANG_EN)
		return LANG_EN;
	else if(retval == LANG_JPN)
		return LANG_JPN;
	else if(retval == LANG_KR)
		return LANG_KR;
	else if(retval == LANG_RS)
		return LANG_RS;

	return LANG_ERR;
}

void ResourceManager::syncConfigureToDisk(void)
{
	int fd;
	char buf[3] = {0};
	configTable2 configTableSwitch[] = {
		{CFG_MENU_POR, (void*)&menuDataPOREnable},
		{CFG_MENU_GPS, (void*)&menuDataGPSEnable},
#ifdef MODEM_ENABLE
		{CFG_MENU_CELLULAR, (void*)&menuDataCELLULAREnable},
#endif
		{CFG_MENU_WIFI, (void*)&menuDataWIFIEnable},
		{CFG_MENU_AP, (void*)&menuDataAPEnable},
		{CFG_MENU_VOLUME,	(void*)&menuDataVolumeEnable},
		{CFG_MENU_TWM,		(void*)&menuDataTWMEnable},
		{CFG_MENU_AWMD,         (void*)&menuDataAWMDEnable}
	};

	configTable2 configTableMenuItem[] = {
		{CFG_MENU_LANG, (void*)&menuDataLang},
		{CFG_MENU_VQ,	(void*)&menuDataVQ},
#ifdef USE_NEWUI
		{CFG_MENU_VM,	(void*)&menuDataVM},
#endif
		{CFG_MENU_PQ,	(void*)&menuDataPQ},
		{CFG_MENU_VTL,	(void*)&menuDataVTL},
		{CFG_MENU_WB,	(void*)&menuDataWB},
		{CFG_MENU_CONTRAST, (void*)&menuDataContrast},
		{CFG_MENU_EXPOSURE, (void*)&menuDataExposure},
		{CFG_MENU_SS,		(void*)&menuDataSS}
	};

	if(getCfgMenuHandle() < 0) {
		db_error("get config menu handle failed\n");	
		return;
	}

	db_info("syncConfigureToDisk\n");
	for(unsigned int i = 0; i < TABLESIZE(configTableSwitch); i++) {
		if(*(bool*)configTableSwitch[i].addr == true)
			buf[0] = '1';
		else
			buf[0] = '0';
		db_msg("buf is %s\n", buf);
		SetValueToEtc(mCfgMenuHandle, "switch", configTableSwitch[i].mainkey, buf);
	}
	db_info("syncConfigureToDisk\n");

	for(unsigned int i = 0; i < TABLESIZE(configTableMenuItem); i++) {
		sprintf(buf, "%d", ((menuItem_t*)(configTableMenuItem[i].addr))->current % 10);
		db_msg("buf is %s\n", buf);
		SetValueToEtc(mCfgMenuHandle, configTableMenuItem[i].mainkey, "current", buf);

		sprintf(buf, "%d", ((menuItem_t*)(configTableMenuItem[i].addr))->count % 10);
		db_msg("buf is %s\n", buf);
		SetValueToEtc(mCfgMenuHandle, configTableMenuItem[i].mainkey, "count", buf);
	}
	SetValueToEtc(mCfgMenuHandle, CFG_VERIFICATION, "current", (char*)"1");
	db_info("syncConfigureToDisk\n");
	SaveEtcToFile(mCfgMenuHandle, configMenu);

	releaseCfgMenuHandle();

    if ((fd = open(configMenu, O_RDWR)) >= 0) {
        fsync(fd);
        close(fd);
    }

	db_info("syncConfigureToDisk finished\n");
}

bool ResourceManager::verifyConfig()
{
	int retval;
	int err_code;

	if((err_code = GetIntValueFromEtc(mCfgMenuHandle, CFG_VERIFICATION, "current", &retval)) != ETC_OK) {
		db_error("get %s->%s failed:%s\n", CFG_VERIFICATION, "current", pEtcError(err_code));
		db_msg("verify %s %s\n", CFG_VERIFICATION, (retval==1)?"correct":"incorrect");
		return false;
	}

	if (retval != 1) {
		db_msg("verify %s %s\n", CFG_VERIFICATION, (retval==1)?"correct":"incorrect");
		return false;
	}
	db_msg("verify %s %s\n", CFG_VERIFICATION, (retval==1)?"correct":"incorrect");
	return true;
}

void ResourceManager::resetResource(void)
{
	releaseCfgFileHandle();
	if(copyFile(defaultConfigFile, configFile) < 0) {
		db_error("reset config file failed\n");	
		return;
	}
	releaseCfgMenuHandle();
	if(copyFile(defaultConfigMenu, configMenu) < 0) {
		db_error("reset config menu failed\n");	
		return;
	}

    // reboot directly instead of reinit app
    /*
	if(initStage1() < 0) {
		db_error("initStage1 failed\n");	
		return;
	}

	if(initStage2() < 0) {
		db_error("initStage2 failed\n");
		return;
	}*/
}
