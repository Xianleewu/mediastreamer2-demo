#ifndef _POWER_MANAGER_H
#define _POWER_MANAGER_H

#include <utils/Thread.h>
#include <utils/Log.h>
#include <utils/Mutex.h>

#include <cutils/android_reboot.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DISP_DEV "/dev/graphics/fb0"
#include "windows.h"
enum
{
	SCREEN_ON = 0,
	SCREEN_OFF
};

#define FBIOBLANK		0x4611
#define FB_BLANK_UNBLANK	0
#define FB_BLANK_POWERDOWN	4

#define DISP_CMD_LCD_ON		0x140
#define DISP_CMD_LCD_OFF	0x141



using namespace android;
#ifdef USE_NEWUI
#define BATTERY_HIGH_LEVEL 4
#else
#define BATTERY_HIGH_LEVEL 5
#endif
#define BATTERY_LOW_LEVEL 1
#define BATTERY_CHANGE_BASE 0

#define CPU_FREQ_LOW	"416000"
#define CPU_FREQ_MID	"728000"
#define CPU_FREQ_HIGH	"900000"
#define CPU_FREQ_UHIGH	"1040000"


#define SCALING_MAX_FREQ  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define SCALING_GOVERNOR  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define SCALING_SETSPEED  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed" 

#define CPUINFO_CUR_FREQ  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"  

#define POWER_STATE_MEM	"mem"
#define POWER_STATE_ON	"off"
#define POWER_STATE	"/sys/power/autosleep"

#define USB_MODE_NODE   "/sys/kernel/debug/intel_otg/mode"
#define USB_DEVICE      "peripheral"
#define USB_HOST        "host"
#define USB_NONE        "none"

#define BACKLIGHT_BRIGHTNESS	"/sys/class/leds/lcd-backlight/brightness"

enum scaling_mode
{
	PERFORMANCE = 0,
	USERSPACE,
	POWERSAVE,
	ONDEMAND,
	CONSERVATIVE,
};

enum pm_sw_cause {
	PM_SW_THERMAL = 0,
	PM_SW_SLEEP,
	PM_SW_USER,
	PM_SW_NUM,
};

enum pm_sw_mode {
	PM_ENABLE = 0,
	PM_DISABLE,
	PM_USB_DEVICE,
	PM_USB_HOST,
	PM_USB_NONE,
};

#endif
