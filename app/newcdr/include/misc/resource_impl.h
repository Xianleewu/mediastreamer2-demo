/*************************************************************************
  > File Name: resource_impl.h
  > Description: 
  > Author: CaoDan
  > Mail: caodan2519@gmail.com 
  > Created Time: 2014年09月07日 星期日 19时55分14秒
 ************************************************************************/

#ifndef __RESOURCE_IMPL_H__
#define __RESOURCE_IMPL_H__

/* config options in the 480X272.cfg */
#ifdef USE_NEWUI
#define CFG_FILE_MENU			"menu"
#define CFG_FILE_MB				"menu_bar"
#define CFG_FILE_MB_ICON_WIN0	"menu_bar_win0_icon"
#define CFG_FILE_MB_ICON_WIN1	"menu_bar_win1_icon"
#define CFG_FILE_MB_ICON_WIN2	"menu_bar_win2_icon"
#define CFG_FILE_MB_ICON_WIN3	"menu_bar_win3_icon"
#endif
#define CFG_FILE_ML				"menu_list"			/* 在配置文件中的key */
#define CFG_FILE_SUBMENU		"sub_menu"			/* 子菜单 */
#define CFG_FILE_ML_MB			"menu_list_message_box"		/* 对话框  */
#define CFG_FILE_STB			"status_bar"		/*状态栏在配置文件的的Key*/
#define CFG_FILE_STB_WNDPIC		"status_bar_window_pic"
#define CFG_FILE_STB_LABEL1		"status_bar_label1"
#define CFG_FILE_STB_RESERVELABLE	"status_bar_reserve_label"
#ifdef USE_NEWUI
#define CFG_FILE_STB_VALUE		"status_bar_value"
#define CFG_FILE_STB_MODE		"status_bar_mode"
#endif
#define CFG_FILE_STB_WIFI			"status_bar_wifi"
#define CFG_FILE_STB_AP				"status_bar_ap"
#define CFG_FILE_STB_UVC			"status_bar_uvc"
#define CFG_FILE_STB_AWMD			"status_bar_awmd"
#define CFG_FILE_STB_LOCK			"status_bar_lock"
#define CFG_FILE_STB_TFCARD			"status_bar_tfcard"
#define CFG_FILE_STB_GPS			"status_bar_gps"
#ifdef MODEM_ENABLE
#define CFG_FILE_STB_CELLULAR			"status_bar_cellular"
#endif
#define CFG_FILE_STB_AUDIO			"status_bar_audio"
#define CFG_FILE_STB_BAT			"status_bar_battery"
#define CFG_FILE_STB_TIME			"status_bar_time"

#define CFG_FILE_ML_VQ			"menu_list_video_quality"
#define CFG_FILE_ML_PQ			"menu_list_photo_quality"
#define CFG_FILE_ML_VTL			"menu_list_time_length"
#define CFG_FILE_ML_AWMD		"menu_list_awmd"
#define CFG_FILE_ML_WB			"menu_list_white_balance"
#define CFG_FILE_ML_CONTRAST		"menu_list_contrast"
#define CFG_FILE_ML_EXPOSURE		"menu_list_exposure"
#define CFG_FILE_ML_POR			"menu_list_power_on_record"
#define CFG_FILE_ML_GPS			"menu_list_gps"
#ifdef MODEM_ENABLE
#define CFG_FILE_ML_CELLULAR		"menu_list_cellular"
#endif
#define CFG_FILE_ML_SS			"menu_list_screen_switch"
#define CFG_FILE_ML_VOLUME		"menu_list_volume"
#define CFG_FILE_ML_WIFI			"menu_list_wifi"
#define CFG_FILE_ML_AP				"menu_list_ap"
#define CFG_FILE_ML_DATE			"menu_list_date"
#define CFG_FILE_ML_LANG			"menu_list_language"
#define CFG_FILE_ML_TWM				"menu_list_time_water_mask"
#define CFG_FILE_ML_FORMAT			"menu_list_format"
#define CFG_FILE_ML_FACTORYRESET	"menu_list_factory_reset"
#define CFG_FILE_ML_FIRMWARE		"menu_list_firmware"

#define CFG_FILE_PLBPREVIEW				"playback_preview"
#define CFG_FILE_PLBPREVIEW_IMAGE		"playback_preview_image"
#define CFG_FILE_PLBPREVIEW_ICON		"playback_preview_icon"
#define CFG_FILE_PLBPREVIEW_MENU		"playback_preview_menu"
#define CFG_FILE_PLBPREVIEW_MESSAGEBOX	"playback_preview_messagebox"

#define CFG_FILE_PLB_ICON				"playback_icon"
#define CFG_FILE_PLB_PGB				"playback_progress_bar"
#define CFG_FILE_CONNECT2PC				"connect2PC"
#define CFG_FILE_WARNING_MB				"warning_messagebox"
#define CFG_FILE_OTHER_PIC				"other_picture"
#define CFG_FILE_TIPLABEL				"tip_label"

/************** define the color attribute subkey *********************/
#define FGC_WIDGET				"fgc_widget"
#define BGC_WIDGET				"bgc_widget"
#define BGC_ITEM_FOCUS			"bgc_item_focus"
#define BGC_ITEM_NORMAL			"bgc_item_normal"
#define MAINC_THREED_BOX		"mainc_3dbox"
#define LINEC_ITEM				"linec_item"
#define LINEC_TITLE				"linec_title"
#define STRINGC_NORMAL			"stringc_normal"
#define STRINGC_SELECTED		"stringc_selected"
#define VALUEC_NORMAL			"valuec_normal"
#define VALUEC_SELECTED			"valuec_selected"
#define SCROLLBARC				"scrollbarc"
#define DATE_NUM				"date_num"
#define DATE_WORD				"date_word"

/* --------------------------- configs for Menu --------------------------- */

/* config options in the menu.cfg */
#define CFG_MENU_SWITCH		"switch"
#define CFG_MENU_LANG		"language"
#define CFG_MENU_VQ			"video_quality"
#ifdef USE_NEWUI
#define CFG_MENU_VM			"video_mode"
#endif
#define CFG_MENU_PQ			"photo_quality"
#define CFG_MENU_VTL		"video_time_length"
#define CFG_MENU_WB			"white_balance"
#define CFG_MENU_CONTRAST	"contrast"
#define CFG_MENU_EXPOSURE	"exposure"
#define CFG_MENU_POR		"power_on_record"
#define CFG_MENU_GPS		"gps_switch"
#ifdef MODEM_ENABLE
#define CFG_MENU_CELLULAR      "cellular_switch"
#endif
#define CFG_MENU_WIFI		"wifi_switch"
#define CFG_MENU_AP		"ap_switch"
#define CFG_MENU_SS			"screen_switch"
#define CFG_MENU_VOLUME		"volume"
#define CFG_MENU_TWM		"time_water_mask"
#define CFG_MENU_AWMD		"awmd"
#define CFG_VERIFICATION	"verification"


#endif
