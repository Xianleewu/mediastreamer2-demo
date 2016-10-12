/*************************************************************************
    > File Name: cdr.h
    > Description: 
    > Author: CaoDan
    > Mail: caodan2519@gmail.com 
    > Created Time: 2014年05月09日 星期五 12:37:41
 ************************************************************************/

#ifndef __CDR_H__
#define __CDR_H__

#include <pthread.h>
#include <semaphore.h>

#include <minigui/window.h>

#include "misc.h"

#define TIME_INFINITE 0xFFFFFF
#define WIN_CNT 6

/* Window ID*/
enum {
	WINDOWID_RECORDPREVIEW = 0,
	WINDOWID_MENU,
	WINDOWID_PLAYBACKPREVIEW,
	WINDOWID_PLAYBACK,
	WINDOWID_STATUSBAR,
	WINDOWID_MAIN
};

#define CDR_FILE_LENGHT			100


enum {
	MSG_RM_MSG_BASE = MSG_USER,
	MSG_RM_LANG_CHANGED,
	MSG_RM_VIDEO_QUALITY,
#ifdef USE_NEWUI
	MSG_RM_VIDEO_MODE,
#endif
	MSG_RM_PIC_QUALITY,
	MSG_RM_VIDEO_TIME_LENGTH,
	MSG_RM_WHITEBALANCE,
	MSG_RM_CONTRAST,
	MSG_RM_EXPOSURE,
	MSG_RM_SCREENSWITCH,
	MSG_CDRMAIN_CUR_WINDOW_CHANGED,
	MSG_AUDIO_SILENT,
	MSG_AWMD,
	MSG_WATER_MARK,
	MSG_REFRESH,
	MSG_BATTERY_CHANGED,
	MSG_BATTERY_CHARGING,

	MSG_CONNECT2PC,
	MSG_DISCONNECT_FROM_PC,
	MSG_CLOSE_CONNECT2PC_DIALOG,
	MSG_SHOW_CDR_DIALOG,
	MSG_CLOSE_CDR_DIALOG,
	MSG_SHOW_TIP_LABEL,
	MSG_CLOSE_TIP_LABEL,

	MSG_PWSET_OK,
	MSG_PWSET_NUM,

	/* set the progress bar time */
	PGBM_SETTIME_RANGE,
	PGBM_SETCURTIME,

	MLM_HILIGHTED_SPACE,	/* 设置MenuList被选中的高亮的item到window边框的距离 */
	MLM_NEW_SELECTED,		/**** 选中了新的item *******/

	/* ************* Status Bar message ******************* */
	STBM_START_RECORD_TIMER,
	STBM_STOP_RECORD_TIMER,
	STBM_RESUME_RECORD_TIMER,
	STBM_GET_RECORD_TIME,
	STBM_SETLABEL1_TEXT,
	STBM_CLEAR_LABEL1_TEXT,
	STBM_MOUNT_TFCARD,
	STBM_ENABLE_DISABLE_GPS,
#ifdef MODEM_ENABLE
	STBM_ENABLE_DISABLE_CELLULAR,
#endif
	STBM_ENABLE_DISABLE_WIFI,
	STBM_ENABLE_DISABLE_AP,
	STBM_VOICE,
	STBM_LOCK,
	STBM_UVC,
	#ifdef USE_NEWUI
	STBM_VIDEO_PHOTO_VALUE,
	STBM_VIDEO_MODE,
	STBM_AWMD,
	STBM_PHOTO_GRAPH,
	RPM_START_RECORD_TIMER,
	RPM_STOP_RECORD_TIMER,
	#endif

	/**************** PlayBack Preview  Message ******************/
	MSG_PLB_COMPLETE,
	MSG_UPDATE_LIST,
	MSG_PREPARE_PLAYBACK,

	/**************** MainWindow Message *******************/
	/* old_window ---> new_window, wParam is the old window ID */
	MWM_CHANGE_FROM_WINDOW,
	/* wParam: old window, lParam: new window */
	MWM_CHANGE_WINDOW,
	MSG_WINDOW_CHANGED,
};


typedef enum {
	LANG_CN = 0,
	LANG_TW = 1,
	LANG_EN = 2,
	LANG_JPN = 3,
	LANG_KR = 4,
	LANG_RS = 5,
	LANG_ERR
}LANGUAGE;



#endif
