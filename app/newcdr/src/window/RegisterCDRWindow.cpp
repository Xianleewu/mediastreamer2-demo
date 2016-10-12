/*************************************************************************
  > File Name: RegisterCDRWindow.cpp
  > Description: 
  > Author: CaoDan
  > Mail: caodan2519@gmail.com 
  > Created Time: 2014年09月07日 星期日 15时58分30秒
 ************************************************************************/

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "windows.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "RegisterCDRWindow.cpp"
#include "debug.h"

extern int StatusBarProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int RecordPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
#ifdef MEDIAPLAYER_READY
extern int PlayBackPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
extern int PlayBackProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
#endif
extern int MenuProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);

typedef int (*Proc) (HWND, int, WPARAM, LPARAM);
int RegisterCDRWindows(void)
{
	int count;

	const char *windowClass[] = {
		WINDOW_STATUSBAR,
		WINDOW_RECORDPREVIEW,
		WINDOW_PLAYBACKPREVIEW,
		WINDOW_PLAYBACK,
		WINDOW_MENU
	};

	WNDCLASS WndClass = {
		NULL,
		0,
#ifdef USE_NEWUI
		WS_CHILD,
#else
		WS_CHILD | WS_VISIBLE,
#endif
		WS_EX_USEPARENTFONT,
		GetSystemCursor(0),
		COLOR_lightgray,
		NULL,
		0
	};

	Proc windowProc[] = {
		StatusBarProc,
		RecordPreviewProc,
#ifdef MEDIAPLAYER_READY
		PlayBackPreviewProc,
		PlayBackProc,
#else
		NULL,
		NULL,
#endif
		MenuProc
	};

	for( count = 0; count < 5; count++) {
		WndClass.spClassName = windowClass[count];
		WndClass.WinProc = windowProc[count];
		WndClass.iBkColor = COLOR_lightgray;

		if(RegisterWindowClass(&WndClass) == FALSE) {
			db_error("register %s failed\n", windowClass[count]);
			return -1;
		}
	}
	return 0;
}

void UnRegisterCDRWindows(void)
{
	UnregisterWindowClass(WINDOW_MENU);
	UnregisterWindowClass(WINDOW_PLAYBACK);
	UnregisterWindowClass(WINDOW_PLAYBACKPREVIEW);
	UnregisterWindowClass(WINDOW_RECORDPREVIEW);
	UnregisterWindowClass(WINDOW_STATUSBAR);
}
