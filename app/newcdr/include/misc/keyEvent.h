#ifndef _KEY_EVENT_H
#define _KEY_EVENT_H

#include <minigui/window.h>

#define MSG_CDR_KEY (MSG_USER + 500)

#define CDR_KEY_LEFT	SCANCODE_KEY_LEFT
#define CDR_KEY_RIGHT	SCANCODE_KEY_RIGHT
#define CDR_KEY_MODE	SCANCODE_KEY_MODE
#define CDR_KEY_OK		SCANCODE_KEY_OK
#define CDR_KEY_POWER	SCANCODE_KEY_POWER

#define LONG_PRESS_TIME 50 /* unit:10ms */
#define LOWPOWER_SHUTDOWN_TIME (10) /* unit: s */
#define NOWORK_SHUTDOWN_TIME (30)
#define NOWORK_SHUTDOWN_TIME2 (25)
#define KEY_CNT 5

/* KEY MACRO */
enum {
	SHORT_PRESS = 0,
	LONG_PRESS
};

#endif


