#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <ProcessState.h>
#include <IPCThreadState.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include "windows.h"
#include "cdr_message.h"
#include "keyEvent.h"

#ifdef POWERMANAGER_READY
#include "PowerManager.h"
#endif
#ifdef EVENTMANAGER_READY
#include "EventManager.h"
#endif

#ifdef MESSAGE_FRAMEWORK
#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"

CdrMain *pView_cdrMain = NULL;

#endif


/* MainWindow */
#define ID_TIMER_KEY 100

#ifdef MESSAGE_FRAMEWORK
#define CDRMain \
MiniGUIAppMain (int args, const char* argv[], CdrMain*); \
int main_entry (int args, const char* argv[]) \
{ \
    int iRet = 0; \
	fwk_controller_init(); \
	fwk_view_init(); \
    if (InitGUI (args, argv) != 0) { \
        return 1; \
    } \
	CdrMain *cdrMain = new CdrMain(); \
	pView_cdrMain = cdrMain; \
	cdrMain->initPreview(NULL); \
    iRet = MiniGUIAppMain (args, argv, cdrMain); \
    TerminateGUI (iRet); \
    return iRet; \
} \
int MiniGUIAppMain
#else /*MESSAGE_FRAMEWORK*/

#define CDRMain \
MiniGUIAppMain (int args, const char* argv[], CdrMain*); \
int main_entry (int args, const char* argv[]) \
{ \
    int iRet = 0; \
    if (InitGUI (args, argv) != 0) { \
        return 1; \
    } \
	CdrMain *cdrMain = new CdrMain(); \
	cdrMain->initPreview(NULL); \
    iRet = MiniGUIAppMain (args, argv, cdrMain); \
    TerminateGUI (iRet); \
    return iRet; \
} \
int MiniGUIAppMain
#endif/*MESSAGE_FRAMEWORK*/
#endif
