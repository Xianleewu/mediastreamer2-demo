
#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <minigui/common.h>
#include "cdr_widgets.h"

typedef enum {
	CDR_DIALOG_SHUTDOWN,
}cdrDialogType_t;

typedef enum {
	IMAGEWIDGET_POWEROFF = 0,
}imageWidgetType_t;

#define DIALOG_OK			(IDC_BUTTON_START)
#define DIALOG_CANCEL		(IDC_BUTTON_START + 1)

enum labelIndex {
	LABEL_NO_TFCARD = 0,
	LABEL_TFCARD_FULL,
	LABEL_PLAYBACK_FAIL,
	LABEL_LOW_POWER_SHUTDOWN,
	LABEL_10S_SHUTDOWN,
	LABEL_SHUTDOWN_NOW,
	LABEL_30S_NOWORK_SHUTDOWN,
	LABEL_FILELIST_EMPTY
};

extern int CdrDialog(HWND hParent, cdrDialogType_t type);

extern int showImageWidget(HWND hParent, imageWidgetType_t type, unsigned char* data);

extern int ShowTipLabel(HWND hParent, enum labelIndex index, bool endLabelKeyUp = true, unsigned int timeoutMs = TIME_INFINITE);
extern int CloseTipLabel(void);
extern int getTipLabelData(tipLabelData_t* tipLabelData);
#ifdef USE_NEWUI
extern int showFirmwareDialog(HWND hParent);
#endif
#endif
