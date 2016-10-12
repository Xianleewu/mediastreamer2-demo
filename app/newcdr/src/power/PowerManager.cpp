#include "PowerManager.h"
#undef LOG_TAG
#define LOG_TAG "PowerManager"
#include "debug.h"
#include "windows.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <network/wifiIf.h>
#include <network/hotspot.h>
#include <gps/GpsController.h>
#include <network/hotspot.h>
#ifdef MODEM_ENABLE
#include <cellular/CellularController.h>
#endif
#include "cutils/properties.h"

int wifi_module[PM_SW_NUM] = { PM_ENABLE };
int ap_module[PM_SW_NUM] = { PM_ENABLE };
int gps_module[PM_SW_NUM] = { PM_ENABLE };
int cellular_module[PM_SW_NUM] = { PM_ENABLE };
int usb_module[PM_SW_NUM] = { PM_ENABLE };
int cam_preview[PM_SW_NUM] = { PM_ENABLE };

int get_def_backlight_level();

PowerManager::PowerManager(CdrMain *cm)
	: mSS(NULL),
  	  mDispFd(0),
  	  mOffTime(TIME_INFINITE),
  	  mState(SCREEN_ON),
	  mBatteryLevel(BATTERY_HIGH_LEVEL),
	  mBacklight_level(0)
{
	char value[PROPERTY_VALUE_MAX];
	int backlight_level;

	mDispFd = open(DISP_DEV, O_RDWR);
	if (mDispFd < 0) {
		db_msg("fail to open %s", DISP_DEV);
	}

	property_get("ro.rk.def_brightness", value, "100");
	mBacklight_level = atoi(value);
	db_msg("backlight level = %d\n", mBacklight_level);
	backlight_level = get_def_backlight_level();
	mBacklight_status = backlight_level ? 1 : 0;
	if (mBacklight_status == 1)
		backlightOn();

	mSS = new ScreenSwitchThread(this);
	mSS->startThread();
	mCdrMain = cm;
}

int PowerManager::setBatteryLevel(int level)
{
	if (level != mBatteryLevel) {
		db_msg("need set new battery Level %d", level);
		mBatteryLevel = level;
		return mBatteryLevel;
	}
	
	return BATTERY_CHANGE_BASE;
}

int PowerManager::getBatteryLevel(void)
{
	return mBatteryLevel;
}

PowerManager::~PowerManager()
{	
	mSS.clear();
	
	close(mDispFd);
}

void PowerManager::setOffTime(unsigned int time)
{
	Mutex::Autolock _l(mLock);
	
	mOffTime = time;
	pulse();
}

void PowerManager::pulse()
{
	db_msg("pulse\n");
	mCon.signal();
}

void PowerManager::screenSwitch()
{
	Mutex::Autolock _l(mLock);
	if (mState == SCREEN_ON) {
		screenOff();
	} else {
		screenOn();
	}
	pulse();
}

bool PowerManager::isScreenOn()
{
	Mutex::Autolock _l(mLock);
	if (mState == SCREEN_ON && mBacklight_status == 1)
		return true;
	return false;
}

void PowerManager::powerOff()
{
	mPO = new poweroffThread(this);
	mPO->start();
}

int PowerManager::poweroff()
{
	//mCdrMain->
	db_error("power off");
	android_reboot(ANDROID_RB_POWEROFF, 0, 0);
	db_error("power off!");

	return 0;
}

int get_def_backlight_level()
{
	int fd = 0;
	int ret;
	char buf[10];

	fd =  open(BACKLIGHT_BRIGHTNESS, O_RDWR);
	if(fd < 0) {
		db_error("open BACKLIGHT_BRIGHTNESS fail\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	ret = read(fd, buf, sizeof(buf) - 1);
	if(ret > 0)
		db_msg("get brightness success!\n");
	else {
		db_error(" read Error (%s),ret=%d,fd=%d \n",strerror(errno),ret,fd);
	}
    close(fd);
    return atoi(buf);
}

int set_backlight_level(int level)
{
	int fd = 0;
	int ret;
	char buf[10];

	fd =  open(BACKLIGHT_BRIGHTNESS, O_RDWR);
	if(fd < 0) {
		db_error("open BACKLIGHT_BRIGHTNESS fail\n");
		return -1;
    }
	sprintf(buf, "%d", level);
	ret = write(fd, buf, strlen(buf));
	if(ret > 0)
		db_msg("set brightness success!\n");
	else {
		db_error(" write Error (%s),ret=%d,fd=%d \n",strerror(errno),ret,fd);
	}
    close(fd);
    return (ret > 0) ? 0 : -1;
}

int PowerManager::backlightOff()
{
	int level;
	int time;

	level = mBacklight_level;
	time = 0;
	if (level > 20) {
		time = 500 / (level / 20);
	}
	for (int i = 0; i < 20; i++) {
		level -= 20;
		if (level <= 0) {
			set_backlight_level(0);
			mBacklight_status = 0;
			return 0;
		}
		set_backlight_level(level);
		usleep(time * 1000);
	}
	set_backlight_level(0);
	mBacklight_status = 0;
	return 0;
}

int PowerManager::backlightOn()
{
	if (mBacklight_level != 0) {
		set_backlight_level(mBacklight_level);
	}
	mBacklight_status = 1;
	return 0;
}

void PowerManager::runReboot()
{
	adjustCpuFreq(CPU_FREQ_MID);
	android_reboot(ANDROID_RB_RESTART, 0, 0);
}

void PowerManager::reboot()
{
	mCdrMain->readyToShutdown();
	runReboot();
}

int PowerManager::screenOff()
{
	db_msg("screenOff");
	int retval;
	int ret = 0;

	backlightOff();
	retval = ioctl(mDispFd, FBIOBLANK, FB_BLANK_POWERDOWN);
	if (retval < 0) {
		db_error("fail to set screen on\n");
		ret = -1;
	}
#ifdef MODEM_ENABLE
	setScreenState(false);
#endif
	mState = SCREEN_OFF;
	return ret;
}

int PowerManager::screenOn()
{
	db_msg("screenOn");
	int retval;
	int ret = 0;

	retval = ioctl(mDispFd, FBIOBLANK, FB_BLANK_UNBLANK);
	if (retval < 0) {
		db_error("fail to set screen on\n");
		ret = -1;
	}
	backlightOn();
#ifdef MODEM_ENABLE
	setScreenState(true);
#endif
	mState = SCREEN_ON;
	return ret;
}

int PowerManager::readyToOffScreen()
{
	db_msg("readyToOffScreen");
	mLock.lock();
	status_t result;
	unsigned int time = 0;
	db_msg("waitRelative");
	time = (mState == SCREEN_ON) ? mOffTime : TIME_INFINITE;
	result  = mCon.waitRelative(mLock, seconds(time));
	if (result != NO_ERROR) { //timeout
		db_msg("timeout");
		if (mState == SCREEN_ON) {
			if (!mCdrMain->isPlaying()) {
#ifdef POWERMANAGER_ENABLE_DEEPSLEEP
                                pmSleep();
#else
                                screenOff();
                                if (!mCdrMain->isRecording()) {
                                        systemSuspend();
                                }
#endif
                        } else {
				db_msg("isPlaying, not to close screen");
			}
		}
    } else {
		db_msg("signaled\n");
	}
	mLock.unlock();
	return 0;
}

int set_scaling_governor(int mode)
{
	int fd = 0;
	int ret = -1;
    char scale_mode[20];
	
	fd =  open(SCALING_GOVERNOR, O_RDWR);
    if(fd < 0) {
		printf("open SCALING_GOVERNOR fail\n");  
		return -1;  
    }
	memset(scale_mode,0,sizeof(scale_mode));
	if(mode == PERFORMANCE)
	{
		strncpy(scale_mode,"performance",strlen("performance"));
	}
	else if(mode == USERSPACE) {
	    strncpy(scale_mode,"userspace",strlen("userspace"));
//		printf("scale_mode=%s\n",scale_mode);
	}
	else {
		strncpy(scale_mode,"userspace",strlen("userspace"));
	}
	ret = write(fd, scale_mode, strlen(scale_mode)+1);
//	ret = write(fd, "userspace", strlen("userspace"));

	if(ret > 0)
		printf("write SCALING_GOVERNOR success!\n");
	else {
		printf("write SCALING_GOVERNOR fail\n");
		printf("Error (%s),ret=%d,fd=%d",strerror(errno),ret,fd);
	}
    close(fd);  
    return (ret > 0) ? 0 : -1;  
}

int set_scaling_speed(const char *freq)
{
	int fd = 0;
	int ret = -1;
//	char freq[10]={"640000"};
	fd =  open(SCALING_SETSPEED, O_RDWR);
	if(fd < 0) {
		db_error("open SCALING_SETSPEED fail\n");  
		return -1;  
    }
	ret = write(fd, freq, strlen(freq));
	if(ret > 0)
		db_msg("write cpu_freq_value success!\n");
	else {
		db_error(" write Error (%s),ret=%d,fd=%d \n",strerror(errno),ret,fd);
	}	
    close(fd);  
    return (ret > 0) ? 0 : -1;  
}

unsigned int get_scaling_speed( )
{
	int fd = 0;
	int ret = -1;
	char cpu_freq_value[10] ="1007000";;
	fd =  open(CPUINFO_CUR_FREQ, O_RDONLY);
	if(fd < 0) {
		printf("open CPUINFO_CUR_FREQ fail\n");  
		return -1;  
    }
	ret = read(fd, cpu_freq_value, strlen(cpu_freq_value));
	if(ret > 0)
		printf("read cpu_freq_value success!\n");
	else {
		printf(" read Error (%s),ret=%d,fd=%d\n",strerror(errno),ret,fd);
    }	
	db_msg("cpu_freq_value =%s\n",cpu_freq_value);	
    close(fd);  
    return (ret > 0) ? 0 : -1;  
}

int _adjustCpuFreq(char *cpu_freq)
{
	//get_scaling_speed();
	
	set_scaling_governor(USERSPACE);
	db_msg("set new freq %s", cpu_freq);
	set_scaling_speed(cpu_freq);
	get_scaling_speed();
    return 0;
}

void PowerManager::adjustCpuFreq(char *cpu_freq)
{
	_adjustCpuFreq(cpu_freq);
}

int set_power_state(char *state)
{
	int fd = 0;
	int ret = -1;

	fd =  open(POWER_STATE, O_RDWR);
	if(fd < 0) {
		db_error("open POWER_STATE fail\n");
		return -1;
    }
	ret = write(fd, state, strlen(state));
	if (ret > 0)
		db_msg("write power_state success!\n");
	else {
		db_error(" write Error (%s),ret=%d,fd=%d \n", strerror(errno), ret, fd);
	}
    close(fd);
    return (ret > 0) ? 0 : -1;
}

static int set_usb_mode(char *mode)
{
	int fd = 0;
	int ret = -1;

	fd =  open(USB_MODE_NODE, O_RDWR);
	if(fd < 0) {
		db_error("open USB_MODE_NODE fail\n");
		return -1;
	}
	ret = write(fd, mode, strlen(mode));
	if (ret > 0)
		db_msg("write USB_MODE_NODE success!\n");
	else {
		db_error(" write Error (%s),ret=%d,fd=%d \n", strerror(errno), ret, fd);
	}
	close(fd);
	return (ret > 0) ? 0 : -1;
}

static int get_usb_mode()
{
	int fd = 0;
	int ret;
	char buf[20];

	fd =  open(USB_MODE_NODE, O_RDWR);
	if(fd < 0) {
		db_error("open USB_MODE_NODE fail\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	ret = read(fd, buf, sizeof(buf) - 1);
	if(ret > 0)
		db_msg("get USB_MODE_NODE success!\n");
	else {
		db_error(" read Error (%s),ret=%d,fd=%d \n",strerror(errno),ret,fd);
	}
	close(fd);
	if (strstr(buf, USB_DEVICE)) {
		return PM_USB_DEVICE;
	}
	if (strstr(buf, USB_HOST)) {
		return PM_USB_HOST;
	}

	return PM_USB_NONE;
}

static bool pm_get_user_cfg(enum ResourceID resID)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	return rm->getResBoolValue(resID);
}

static int pm_get_user_val(enum ResourceID resID)
{
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	return rm->getResIntValue(resID, INTVAL_SUBMENU_INDEX);
}

static void pm_update_gps_user_cfg()
{
	bool curStatus;

	curStatus = pm_get_user_cfg(ID_MENU_LIST_GPS);
	gps_module[PM_SW_USER] = ( (curStatus == true) ? PM_ENABLE : PM_DISABLE );
}

static void pm_update_wifi_user_cfg()
{
	bool curStatus;

	curStatus = pm_get_user_cfg(ID_MENU_LIST_WIFI);
	wifi_module[PM_SW_USER] = ( (curStatus == true) ? PM_ENABLE : PM_DISABLE );
}

static void pm_update_ap_user_cfg()
{
	bool curStatus;

	curStatus = pm_get_user_cfg(ID_MENU_LIST_AP);
	ap_module[PM_SW_USER] = ( (curStatus == true) ? PM_ENABLE : PM_DISABLE );
}

static void pm_update_cellular_user_cfg()
{
	bool curStatus;

	curStatus = pm_get_user_cfg(ID_MENU_LIST_CELLULAR);
	cellular_module[PM_SW_USER] = ( (curStatus == true) ? PM_ENABLE : PM_DISABLE );
}

static void pm_update_usb_user_cfg()
{
	usb_module[PM_SW_USER] = get_usb_mode();
}

static int pm_get_gps_mode()
{
	int i;

	pm_update_gps_user_cfg();
	for (i = 0; i < PM_SW_NUM; i++) {
		if (gps_module[i] == PM_DISABLE) {
			return PM_DISABLE;
		}
	}
	return PM_ENABLE;
}

static int pm_get_wifi_mode()
{
	int i;

	pm_update_wifi_user_cfg();
	for (i = 0; i < PM_SW_NUM; i++) {
		if (wifi_module[i] == PM_DISABLE)
			return PM_DISABLE;
	}
	return PM_ENABLE;
}

static int pm_get_ap_mode()
{
	int i;

	pm_update_ap_user_cfg();
	for (i = 0; i < PM_SW_NUM; i++) {
		if (ap_module[i] == PM_DISABLE)
			return PM_DISABLE;
	}
	return PM_ENABLE;
}

static int pm_get_cellular_mode()
{
	int i;

	pm_update_cellular_user_cfg();
	for (i = 0; i < PM_SW_NUM; i++) {
		if (cellular_module[i] == PM_DISABLE)
			return PM_DISABLE;
	}
	return PM_ENABLE;
}

int PowerManager::pm_get_cam_preview_mode()
{
	int i;
	bool ret;

	ret = mCdrMain->isPreviewing();
	cam_preview[PM_SW_USER] = ((ret == true) ? PM_ENABLE : PM_DISABLE);
	for (i = 0; i < PM_SW_NUM; i++) {
		if (cam_preview[i] == PM_DISABLE)
			return PM_DISABLE;
	}
	return PM_ENABLE;
}

/* ---------------- gps switch ---------------- */
static void pm_gps_enable_by_resume()
{
	gps_module[PM_SW_SLEEP] = PM_ENABLE;
	if (pm_get_gps_mode() == PM_ENABLE)
		enableGps();
}

static void pm_gps_disable_by_suspend()
{
	gps_module[PM_SW_SLEEP] = PM_DISABLE;
	disableGps();
}

static void pm_gps_enable_by_thermal()
{
	gps_module[PM_SW_THERMAL] = PM_ENABLE;
	if (pm_get_gps_mode() == PM_ENABLE)
		enableGps();
}

static void pm_gps_disable_by_thermal()
{
	gps_module[PM_SW_THERMAL] = PM_DISABLE;
	disableGps();
}

/* ---------------- wifi switch ---------------- */
static void pm_wifi_enable_by_resume()
{
	wifi_module[PM_SW_SLEEP] = PM_ENABLE;
	if (pm_get_wifi_mode() == PM_ENABLE)
		enable_wifi();
}

static void pm_wifi_disable_by_suspend()
{
	wifi_module[PM_SW_SLEEP] = PM_DISABLE;
	disable_wifi();
}

static void pm_wifi_enable_by_thermal()
{
	wifi_module[PM_SW_THERMAL] = PM_ENABLE;
	if (pm_get_wifi_mode() == PM_ENABLE)
		enable_wifi();
}

static void pm_wifi_disable_by_thermal()
{
	wifi_module[PM_SW_THERMAL] = PM_DISABLE;
	disable_wifi();
}

/* ---------------- ap switch ---------------- */
static void pm_ap_enable_by_resume()
{
	ap_module[PM_SW_SLEEP] = PM_ENABLE;
	if (pm_get_ap_mode() == PM_ENABLE)
		enable_Softap();
}

static void pm_ap_disable_by_suspend()
{
	ap_module[PM_SW_SLEEP] = PM_DISABLE;
	disable_Softap();
}

static void pm_ap_enable_by_thermal()
{
	ap_module[PM_SW_THERMAL] = PM_ENABLE;
	if (pm_get_ap_mode() == PM_ENABLE)
		enable_Softap();
}

static void pm_ap_disable_by_thermal()
{
	ap_module[PM_SW_THERMAL] = PM_DISABLE;
	disable_Softap();
}

/* ---------------- USB switch ---------------- */
static void pm_usb_enable_by_resume()
{
	usb_module[PM_SW_SLEEP] = PM_ENABLE;
	if (usb_module[PM_SW_USER] != PM_USB_DEVICE)
		set_usb_mode(USB_HOST);
	if (usb_module[PM_SW_USER] == PM_USB_DEVICE)
		set_usb_mode(USB_DEVICE);
}

static void pm_usb_disable_by_suspend()
{
	usb_module[PM_SW_SLEEP] = PM_USB_NONE;
	usb_module[PM_SW_USER] = get_usb_mode();
	set_usb_mode(USB_NONE);
}

/* ---------------- preview switch ---------------- */
void PowerManager::pm_startpreview_by_resume()
{
	int val;

	cam_preview[PM_SW_SLEEP] = PM_ENABLE;
	if (pm_get_cam_preview_mode() == PM_ENABLE) {
		mCdrMain->startPreview();
	}
}

void PowerManager::pm_stoppreview_by_suspend()
{
	cam_preview[PM_SW_SLEEP] = PM_DISABLE;
	mCdrMain->stopPreview();
}

int PowerManager::systemSuspend(void)
{
	return set_power_state(POWER_STATE_MEM);
}

int PowerManager::systemResume(void)
{
	return set_power_state(POWER_STATE_ON);
}

int PowerManager::pmSleep(void)
{
	int screen_state;

	screen_state = mState;
	screenOff();
	if (screen_state == SCREEN_OFF)
		return 0;
	if (!mCdrMain->isRecording()) {
		pm_usb_disable_by_suspend();
		pm_gps_disable_by_suspend();
		pm_ap_disable_by_suspend();
		pm_wifi_disable_by_suspend();
		pm_stoppreview_by_suspend();
		systemSuspend();
	}
	return 0;
}

int PowerManager::pmWakeUp(void)
{
	if (mState == SCREEN_ON && mBacklight_status == 0) {
		screenOn();
		return 0;
	}
	if (!mCdrMain->isRecording()) {
		pm_usb_enable_by_resume();
		pm_startpreview_by_resume();
	}

	screenOn();

	if (!mCdrMain->isRecording()) {
		systemResume();
		pm_gps_enable_by_resume();
		pm_wifi_enable_by_resume();
		pm_ap_enable_by_resume();
	}
	return 0;
}

