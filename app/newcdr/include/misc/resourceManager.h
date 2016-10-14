
#ifndef __RESOURCEMANAGER_H__
#define __RESOURCEMANAGER_H__


#include <minigui/common.h>
#include <minigui/gdi.h>

#include "cdr.h"
#include "cdrLang.h"

#include <utils/Vector.h>
#include <utils/String8.h>
#include <utils/Mutex.h>
#include "cdr_message.h"

#define FIRMWARE_VERSION	"v2.0 rc1"

enum ResourceID {
	ID_SCREEN	= 0,
	ID_STATUSBAR,
	ID_STATUSBAR_ICON_WINDOWPIC,
	#ifdef USE_NEWUI
	ID_STATUSBAR_ICON_VALUE,
	ID_STATUSBAR_ICON_MODE,
	#else
	ID_STATUSBAR_LABEL1,
	ID_STATUSBAR_LABEL_RESERVE,
	#endif
	ID_STATUSBAR_ICON_AWMD,
	ID_STATUSBAR_ICON_UVC,
	ID_STATUSBAR_ICON_WIFI,
	ID_STATUSBAR_ICON_AP,
	ID_STATUSBAR_ICON_LOCK,
	ID_STATUSBAR_ICON_TFCARD,
	ID_STATUSBAR_ICON_GPS,
#ifdef MODEM_ENABLE
	ID_STATUSBAR_ICON_CELLULAR,
#endif
	ID_STATUSBAR_ICON_AUDIO,
	ID_STATUSBAR_ICON_BAT,	/* end of status bar */
	#ifdef USE_NEWUI
	ID_MENU,
	ID_MENUBAR,
	ID_MENUTMX_QR,
	ID_MENUTMX_SNO,
	ID_MENUBAR_VLINE,
	ID_MENUBAR_LIGHT,
	ID_MENUBAR_ICON_WIN0,
	ID_MENUBAR_ICON_WIN1,
	ID_MENUBAR_ICON_WIN2,
	ID_MENUBAR_ICON_WIN3,
	ID_MENU_LIST,
	ID_MENU_LIST_NUM,
	ID_MENU_LIST_ITEM_HEIGHT,
	ID_MENU_LIST_HLINE,
	ID_MENU_LIST_LIGHT,
	ID_MENU_LIST_SELECT,
	ID_MENU_LIST_MB,
	ID_MENU_LIST_VQ,
	ID_MENU_LIST_PQ,
	ID_MENU_LIST_VTL,
	ID_MENU_LIST_VM,
	ID_MENU_LIST_VIDEO_SETTING,
	ID_MENU_LIST_PHOTO_SETTING,
	#else
	ID_STATUSBAR_LABEL_TIME,	/* end of status bar */
	ID_MENU_LIST,
	ID_MENU_LIST_MB,
	ID_MENU_LIST_VQ,
	ID_MENU_LIST_PQ,
	ID_MENU_LIST_VTL,
	#endif
	ID_MENU_LIST_AWMD,
	ID_MENU_LIST_WB,
	ID_MENU_LIST_CONTRAST,
	ID_MENU_LIST_EXPOSURE,
	ID_MENU_LIST_POR,
	ID_MENU_LIST_GPS,
#ifdef MODEM_ENABLE
	ID_MENU_LIST_CELLULAR,
#endif
	ID_MENU_LIST_SS,
	ID_MENU_LIST_SILENTMODE,
	ID_MENU_LIST_WIFI,
	ID_MENU_LIST_AP,
	ID_MENU_LIST_DATE,
	ID_MENU_LIST_LANG,
	ID_MENU_LIST_TWM,
	ID_MENU_LIST_FORMAT,
	ID_MENU_LIST_FRESET,
	ID_MENU_LIST_FIRMWARE,
	ID_MENU_LIST_ABOUT_TMX,
	ID_MENU_LIST_UNFOLD_PIC,
	ID_MENU_LIST_CHECKBOX_PIC,
	ID_MENU_LIST_UNCHECKBOX_PIC,	/* end of menulist */
	ID_SUBMENU,
	ID_SUBMENU_CHOICE_PIC,
	ID_PLAYBACKPREVIEW_IMAGE,
	ID_PLAYBACKPREVIEW_ICON,
	ID_PLAYBACKPREVIEW_MENU,
	ID_PLAYBACKPREVIEW_MB,
	ID_PLAYBACKPREVIEW_FC,
	ID_PLAYBACK_ICON,
	ID_PLAYBACK_PGB,
	ID_VOCIE,
	ID_CONNECT2PC,
	ID_WARNNING_MB,
	ID_POWEROFF,
	ID_TIPLABEL,
	ID_PREVIEW_WATERMARK_TIME,
	ID_PREVIEW_WATERMARK_GPSINFO,
	ID_RECORD_PREVIEW1,
	ID_RECORD_PREVIEW2
};

enum BmpType{
	BMPTYPE_BASE = 0,
	BMPTYPE_WINDOWPIC_RECORDPREVIEW,
	BMPTYPE_WINDOWPIC_PHOTOGRAPH,
	BMPTYPE_WINDOWPIC_PLAYBACKPREVIEW,
	BMPTYPE_WINDOWPIC_PLAYBACK,
	BMPTYPE_WINDOWPIC_MENU,
	BMPTYPE_UNSELECTED,
	BMPTYPE_SELECTED,
#ifdef USE_NEWUI
	BMPTYPE_BACKGROUND,
	BMPTYPE_LINE,
	BMPTYPE_CONFIRM,
	BMPTYPE_CANCEL,
#endif
};

enum ColorType{
	COLOR_BGC = 0,
	COLOR_FGC,
	COLOR_FGC_LABEL,
	COLOR_FGC_NUMBER,
	COLOR_BGC_ITEMFOCUS,
	COLOR_BGC_ITEMNORMAL,
	COLOR_MAIN3DBOX,
	COLOR_LINEC_ITEM,
	COLOR_LINEC_TITLE,
	COLOR_STRINGC_NORMAL,
	COLOR_STRINGC_SELECTED,
	COLOR_SCROLLBARC,
	COLOR_VALUEC_NORMAL,
	COLOR_VALUEC_SELECTED,
	COLOR_BORDERC_NORMAL,
	COLOR_BORDERC_SELECTED,
};

enum IntValueType{
	INTVAL_ITEMWIDTH = 0,
	INTVAL_ITEMHEIGHT,
	INTVAL_TITLEHEIGHT,
	INTVAL_HBORDER,
	INTVAL_YEARWIDTH,
	INTVAL_LABELWIDTH,
	INTVAL_NUMBERWIDTH,
	INTVAL_BOXHEIGHT,
	INTVAL_SUBMENU_INDEX,
	INTVAL_SUBMENU_COUNT
};

namespace android{

	class ResourceManager : public cdrLang
	{
	private:
		ResourceManager();
		~ResourceManager();
		ResourceManager& operator=(const ResourceManager& o);

	public:
		virtual int initLangAndFont(void);

		static ResourceManager* getInstance();
		/*
		 * Only read the screen rect and configuration about status bar from the configs
		 * */
		int initStage1(void);
		/*
		 * init others resources
		 * */
		int initStage2(void);

		/* 
		 * get the GAL pixel color from the config_file
		 * color in the config_file is in the A B G R format
		 * */
		gal_pixel getResColor(enum ResourceID resID, enum ColorType type);

		int getResRect(enum ResourceID resID, CDR_RECT &rect);

		int getResBmp(enum ResourceID resID, enum BmpType, BITMAP &bmp);
		/*
		 * get the current subMenu's check box bmp
		 * */
		int getResBmpSubMenuCheckbox(enum ResourceID, bool isHilight, BITMAP &bmp);

		int getResIntValue(enum ResourceID, enum IntValueType type);
		int setResIntValue(enum ResourceID, enum IntValueType type, int value);

		bool getResBoolValue(enum ResourceID resID);
		int setResBoolValue(enum ResourceID resID, bool value);

		void setCurrentIconValue(enum ResourceID, int cur_val);

		const char* getResMenuItemString(enum ResourceID resID);
		const char* getResSubMenuCurString(enum ResourceID resID);
		#ifdef USE_NEWUI
		const char* getResMenuTitle(enum ResourceID resID);
		int getResIconCurrent(enum ResourceID resID);
		int getResMenuContent0Cmd(enum ResourceID resID);
		#else
		const char* getResSubMenuTitle(enum ResourceID resID);
		#endif
		LANGUAGE getLanguage(void);

		PLOGFONT getLogFont(void);

		int getResVTLms(void);

		/*
		 * Load Bitmap from config through the giving section name and key name
		 * On success return 0
		 * On faile return -1
		 * */
		int loadBmpFromConfig(const char* pSection, const char* pKey, PBITMAP bmp);

		/*
		 * unload the giving bitmap structure
		 * */
		void unloadBitMap(BITMAP &bmp);

		void setHwnd(unsigned int win_id, HWND hwnd);
		int notifyAll();
		void dump();
		void syncConfigureToDisk(void);
		void resetResource(void);
	private:
		bool verifyConfig();
		int initStatusBarResource(void);
		int initS1MenuResource(void);
		#ifdef USE_NEWUI
		int initMenuBarResource(void);
		#endif
		int initPlayBackPreviewResource(void);
		int initPlayBackResource(void);
		int initMenuListResource(void);
		int initMenuResource(void);
		int initOtherResource(void);

		int getWindPicFileName(enum BmpType type, String8 &file);
		int getCurrentIconFileName(enum ResourceID resID, String8 &file);
		int getMLPICFileName(enum ResourceID, enum BmpType type, String8 &file);

		gal_pixel getStatusBarColor(enum ResourceID resID, enum ColorType type);
		gal_pixel getPlaybackPreviewColor(enum ResourceID resID, enum ColorType type);
		gal_pixel getPlaybackColor(enum ResourceID resID, enum ColorType type);
		gal_pixel getMenuListColor(enum ResourceID resID, enum ColorType type);
		gal_pixel getSubMenuColor(enum ResourceID resID, enum ColorType type);
		gal_pixel getWarningMBColor(enum ResourceID resID, enum ColorType type);
		gal_pixel getConnect2PCColor(enum ResourceID resID, enum ColorType type);

		int getSubMenuCurrentIndex(enum ResourceID resID);
		int getSubMenuCounts(enum ResourceID resID);
		int getSubMenuIntAttr(enum ResourceID, enum IntValueType type);

		int setSubMenuCurrentIndex(enum ResourceID resID, int value);

		void updateLangandFont(int langValue);

		typedef struct {
			unsigned int current;
			Vector < String8 >icon;
		}currentIcon_t;

		typedef struct {
			unsigned int current;
			unsigned int count;
		}menuItem_t;

		/* 
		 * print error msg through the err_code
		 * */
		static char* pEtcError(int err_code);
		static char* pBmpError(int err_code);

		/* 
		 * -------------------------------------------------------------------------------------------
		 * before call the following functions, should use getCfgFileHandle or getCfgMenuHandle first
		 * in the end, should call the releaseCfgFileHandle or releaseCfgMenuHandle to release Handle;
		 * */
		int getCfgFileHandle(void);
		int getCfgMenuHandle(void);
		void releaseCfgFileHandle(void);
		void releaseCfgMenuHandle(void);

		/*
		 *  get the int value from the giving file, mainkey is the section name, subkey is the key value
		 * */
		int getIntValueFromHandle(GHANDLE cfgHandle, const char *mainkey, const char *subkey);


		/*
		 * get the rect from config_file
		 * pWindow the the section name in the config file
		 * On success return ETC_OK
		 * On fail return err code
		 * */
		int getRectFromFileHandle(const char *pWindow, CDR_RECT *rect);

		/*
		 * get the ABGR format color from the config_file
		 * On success return ETC_OK
		 * On fail return err code
		 * */
		int getABGRColorFromFileHandle(const char* pWindow, const char* subkey, gal_pixel &pixel);

		/*
		 * On success return ETC_OK
		 * On fail return err code
		 * */
		int getValueFromFileHandle(const char*mainkey, const char* subkey, char* value, unsigned int len);

		/*
		 * get current icon info from config_file, and fill the giving currentIcon
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
		int fillCurrentIconInfoFromFileHandle(const char* pWindow, currentIcon_t &currentIcon);

		/*
		 * get language from config menu
		 * On success return LANGUAGE
		 * On faile return LANG_ERR
		 * */
		LANGUAGE getLangFromMenuHandle(void);
		/* --------------------------------------------------- */

		/* -------------------- the following function do not need the cfgHandle------------------------------- */
		LANGUAGE getLangFromConfigFile(void);

	private:
		Mutex mLock;
		HWND mHwnd[WIN_CNT];
		const char* configFile;
		const char* configMenu;
		const char* defaultConfigFile;
		const char* defaultConfigMenu;
		GHANDLE mCfgFileHandle;
		GHANDLE mCfgMenuHandle;

		int mScreenOffTime;
		int mFontSize;
		
		/* ------------- resource datas ------------ */
		CDR_RECT rScreenRect;

		CDR_RECT rRecordPreviewRect1;
		CDR_RECT rRecordPreviewRect2;

		/* ---- status bar ------ */
		struct {
			CDR_RECT STBRect;
			gal_pixel bgc;
			gal_pixel fgc;

			CDR_RECT windowPicRect;
			String8 recordPreviewPic;
			String8 screenShotPic;
			String8 playBackPreviewPic;
			String8 playBackPic;
			String8 menuPic;

			#ifdef USE_NEWUI
			String8 stbBackgroundPic;
			CDR_RECT valueRect;
			currentIcon_t valueIcon;
			
			CDR_RECT modeRect;
			currentIcon_t modeIcon;
			#else
			CDR_RECT label1Rect;
			CDR_RECT reserveLabelRect;			
			#endif
			CDR_RECT wifiRect;
			currentIcon_t wifiIcon;

			CDR_RECT uvcRect;
			currentIcon_t uvcIcon;

			CDR_RECT awmdRect;
			currentIcon_t awmdIcon;
			
			CDR_RECT lockRect;
			currentIcon_t lockIcon;

			CDR_RECT tfCardRect;
			currentIcon_t tfCardIcon;

			CDR_RECT audioRect;
			currentIcon_t audioIcon;

			CDR_RECT batteryRect;
			currentIcon_t batteryIcon;

			CDR_RECT gpsRect;
			currentIcon_t gpsIcon;

			CDR_RECT apRect;
			currentIcon_t apIcon;
#ifdef MODEM_ENABLE
			CDR_RECT cellularRect;
			currentIcon_t cellularIcon;
#endif
			CDR_RECT timeRect;
		}rStatusBarData;

#ifdef USE_NEWUI
		/* ---- menu bar ------ */
		struct {
			CDR_RECT MenuRect;
			CDR_RECT MenuBarRect;
			
			CDR_RECT win0Rect;
			currentIcon_t win0Icon;

			CDR_RECT win1Rect;
			currentIcon_t win1Icon;

			CDR_RECT win2Rect;
			currentIcon_t win2Icon;
			
			CDR_RECT win3Rect;
			currentIcon_t win3Icon;

			String8 mlBgPic;
			String8 hlinePic;
			String8 mlLightPic;
			String8 mlSelectIcon;

			String8 backgroundPic;
			String8 vlinePic;
			String8 lightPic;
		}rMenuBarData;
#endif
		/* PlaybackPreview */
		struct {
			CDR_RECT imageRect;		/* display the image thumnail */
			CDR_RECT iconRect;		/* display the play icon */
			currentIcon_t icon;

			CDR_RECT popupMenuRect;
			unsigned int popupMenuItemWidth;
			unsigned int popupMenuItemHeight;
			gal_pixel popupMenuBgcWidget;
			gal_pixel popupMenuBgcItemNormal;
			gal_pixel popUpMenuBgcItemFocus;
			gal_pixel popUpMenuMain3dBox;

			CDR_RECT messageBoxRect;
			unsigned int messageBoxItemWidth;
			unsigned int messageBoxItemHeight;
			gal_pixel messageBoxBgcWidget;
			gal_pixel messageBoxFgcWidget;
			gal_pixel messageBoxBgcItemNormal;
			gal_pixel messageBoxBgcItemFous;
			gal_pixel messageBoxMain3dBox;
		}rPlayBackPreviewData;

		/* PlayBack */
		struct {
			CDR_RECT iconRect;
			currentIcon_t icon;

			CDR_RECT PGBRect;
			gal_pixel PGBBgcWidget;
			gal_pixel PGBFgcWidget;
		}rPlayBackData;

		/* Menu */
		struct {
			CDR_RECT menuListRect;
			CDR_RECT subMenuRect;
			CDR_RECT messageBoxRect;
			CDR_RECT dateRect;
			CDR_RECT apRect;

			unsigned int dateTileRectH;
			unsigned int dateHBorder;
			unsigned int dateYearW;;
			unsigned int dateLabelW;;
			unsigned int dateNumberW;;
			unsigned int dateBoxH;;
			
			#ifdef USE_NEWUI
			unsigned int itemHeight;
			String8 boxBgPic;
			String8 boxLinePic;
			String8 btnConfirmPic;
			String8 btnCancelPic;
			#else
			String8 choicePic;
			String8 checkedNormalPic;
			String8 checkedPressPic;
			String8 uncheckedNormalPic;
			String8 uncheckedPressPic;
			String8 unfoldNormalPic;
			String8 unfoldPressPic;
			#endif
			
			gal_pixel bgcWidget;
			gal_pixel fgcWidget;
			gal_pixel linecItem;
			gal_pixel stringcNormal;
			gal_pixel stringcSelected;
			gal_pixel valuecNormal;
			gal_pixel valuecSelected;
			gal_pixel scrollbarc;

			gal_pixel subMenuBgcWidget;
			gal_pixel subMenuFgcWidget;
			gal_pixel subMenuLinecTitle;

			gal_pixel messageBoxBgcWidget;
			gal_pixel messageBoxFgcWidget;
			gal_pixel messageBoxLinecTitle;
			gal_pixel messageBoxLinecItem;

			gal_pixel dateBgcWidget;
			gal_pixel dateFgc_label;
			gal_pixel dateFgc_number;
			gal_pixel dateLinecTitle;
			gal_pixel dateBordercSelected;
			gal_pixel dateBordercNormal;

			gal_pixel apBgcWidget;
			gal_pixel apFgc_label;
			gal_pixel apFgc_number;
			gal_pixel apLinecTitle;
			gal_pixel apBordercSelected;
			gal_pixel apBordercNormal;

			currentIcon_t videoQualityIcon;
			currentIcon_t photoQualityIcon;
			currentIcon_t timeLengthIcon;
			currentIcon_t moveDetectIcon;
			currentIcon_t whiteBalanceIcon;
			currentIcon_t contrastIcon;
			currentIcon_t exposureIcon;
			currentIcon_t PORIcon;
			currentIcon_t GPSIcon;
#ifdef MODEM_ENABLE
			currentIcon_t CELLULARIcon;
#endif
			currentIcon_t screenSwitchIcon;
			currentIcon_t volumeIcon;
			currentIcon_t wifiIcon;
			currentIcon_t apIcon;
			currentIcon_t dateIcon;
			currentIcon_t languageIcon;
			currentIcon_t TWMIcon;
			currentIcon_t formatIcon;
			currentIcon_t factoryResetIcon;
			currentIcon_t firmwareIcon;
			currentIcon_t aboutTMXIcon;
		}rMenuList;

		struct {
			CDR_RECT rect;
			gal_pixel fgcWidget;
			gal_pixel bgcWidget;
			gal_pixel linecTitle;
			gal_pixel linecItem;
		}rWarningMB;

		struct {
			CDR_RECT rect;	
			unsigned int itemWidth;
			unsigned int itemHeight;

			gal_pixel bgcWidget;
			gal_pixel bgcItemFocus;
			gal_pixel bgcItemNormal;
			gal_pixel mainc_3dbox;
		}rConnectToPC;

		struct {
			CDR_RECT rect;	
			unsigned int titleHeight;
			gal_pixel fgcWidget;
			gal_pixel bgcWidget;
			gal_pixel linecTitle;
		}rTipLabel;

		String8 rPoweroffPic;
#ifdef USE_NEWUI
		String8 rRecordDotPic;
#endif
	public:
		menuItem_t menuDataLang;
		menuItem_t menuDataVQ;
#ifdef USE_NEWUI
		menuItem_t menuDataVM;
#endif
		menuItem_t menuDataPQ;
		menuItem_t menuDataVTL;
		menuItem_t menuDataWB;
		menuItem_t menuDataContrast;
		menuItem_t menuDataExposure;
		menuItem_t menuDataSS;
		bool menuDataPOREnable;
		bool menuDataGPSEnable;
#ifdef MODEM_ENABLE
		bool menuDataCELLULAREnable;
#endif
		bool menuDataWIFIEnable;
		bool menuDataAPEnable;
		bool menuDataVolumeEnable;
		bool menuDataTWMEnable;
		bool menuDataAWMDEnable;
		/* ---------- end of resource datas ------------ */
	};

}

#endif
