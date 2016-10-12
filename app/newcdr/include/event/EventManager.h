#ifndef _EVENT_MANAGER_H
#define _EVENT_MANAGER_H

#include <hardware_legacy/uevent.h>
#include <hardware/sensors.h>

#include <utils/String8.h>

#include <utils/Thread.h>
#include <utils/Log.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "ConfigData.h"

extern int detectPlugIn(int fd);

using namespace android;

enum event_msg {
	EVENT_UVC_PLUG_IN	= 1,
	EVENT_UVC_PLUG_OUT,
	EVENT_CONNECT2PC,
	EVENT_DISCONNECFROMPC,
	EVENT_IMPACT,
	EVENT_BATTERY,
#ifdef USE_NEWUI
	EVENT_VIDEO_PHOTO_VALUE,
	EVENT_VIDEO_MODE,
#endif
	EVENT_GPS,
#ifdef MODEM_ENABLE
	EVENT_CELLULAR,
#endif
	EVENT_WIFI,
	EVENT_AP,
	EVENT_DELAY_SHUTDOWN,
	EVENT_CANCEL_SHUTDOWN,
	EVENT_MOUNT,
	EVENT_HDMI_PLUGIN,
	EVENT_HDMI_PLUGOUT,
	EVENT_TVD_PLUG_IN,
	EVENT_TVD_PLUG_OUT,
	EVENT_TVOUT_PLUG_IN,
	EVENT_TVOUT_PLUG_OUT,
};

//#define EVENT_TVD	1
//#define EVENT_HDMI	1
#define EVENT_UVC	1

#define THRESHOLD_VALUE 40

#define HDMI_NODE "hdmi"
#define HDMI_ONLINE_FILE	"sys/devices/virtual/switch/hdmi/state"


#define UVC_NODE	"video"
#define UVC_ADD		"add@/devices/soc0/e0000000.noc/ef010000.l2_noc/e2000000.ahb_per/e2100000.usb"
#define UVC_REMOVE	"remove@/devices/soc0/e0000000.noc/ef010000.l2_noc/e2000000.ahb_per/e2100000.usb"

#define BATTERY_NODE		"BATTERY"
#define USB_NODE		"USB"
#define POWER_BATTERY		"power_supply/BATTERY"
#define UEVENT_CHANGE		"change"
#define USB_ONLINE_FILE		"/sys/class/power_supply/USB/online"
#define AC_ONLINE_FILE		"/sys/class/power_supply/AC/online"
#define BATTERY_CAPACITY	"/sys/class/power_supply/BATTERY/capacity"

#define TVD_FILE_ONLINE			"/sys/devices/tvd/tvd"

#define CD_TFCARD_PATH "/dev/block/vold/179:0"
#define CD_TFCARD_LUN_PATH "/sys/class/android_usb/android0/f_mass_storage/lun/file"

class EventManager;

class EventListener
{
public:
		EventListener(){};
		virtual ~EventListener(){};
		virtual int notify(int message, int val) = 0;
};

class ImpactThread : public Thread
{
public:
	ImpactThread(EventManager *em);
	~ImpactThread();
	int init(void);
	int exit(void);
	void startThread()
	{
		run("ImpactThread", PRIORITY_NORMAL);
	}
	void stopThread()
	{
		requestExitAndWait();
	}
	bool threadLoop()
	{
		impactLoop();
		return true;
	}
private:
	int sensorCount;
	struct sensors_poll_device_t* device;
	struct sensor_t const *sensorList;
	long mLastFilteredTimestampNanos;
	float mLastFilteredX, mLastFilteredY, mLastFilteredZ;
	
	EventManager *mEM;
	int impactInit(void);
	int impactLoop(void);
};

class EventManager
{
public:
	EventManager(bool tvdOnline, bool hasTVD);
	~EventManager();
	int init(bool hasTVD);
	int exit(void);
	void connect2PC();
	void disconnectFromPC();
	int impactEvent();
	int checkUSB(char *udata);
	bool isBatteryOnline();
	int getTiltAngle(); 
	void setTiltAngle(int tiltAngle);
	void getCoordinate(float *x, float *y, float *z);
	void setCoordinate(float x, float y, float z);
	
#ifdef EVENT_HDMI
	int checkHDMI(const char *str);
#endif
#ifdef EVENT_TVD
	void detectTVD();
	void startDetectTVD();
#endif
#ifdef EVENT_HDMI
	void hdmiAudioOn(bool open);
#endif
#ifdef EVENT_TVD
	void detectTVout();
	int startDetectTVout();
#endif
	class UeventThread : public Thread
	{
	public:
		UeventThread(EventManager *em)
			: Thread(false)
			, mEM(em)
		{
		}
		void startThread()
		{
			run("UeventThread", PRIORITY_NORMAL);
		}
		void stopThread()
		{
			requestExitAndWait();
		}
		bool threadLoop()
		{
			mEM->ueventLoop();
			return true;
		}
	private:
		EventManager *mEM;
	};
#ifdef EVENT_TVD
	class TVDDetectThread : public Thread
	{
	public:
		TVDDetectThread(EventManager *em)
			: Thread(false),
			  mEM(em)
		{}
		void startThread()
		{run("TVDDetectThread", PRIORITY_NORMAL);}
		void stopThread()
		{requestExitAndWait();}
		bool threadLoop()
		{
			mEM->detectTVD();
			return true;
		}
	private:
		EventManager *mEM;
		bool mNeedStop;
		int mFD;
	};
	
	class TVoutDetectThread : public Thread
	{
	public:
		TVoutDetectThread(EventManager *em)
			: Thread(false),
			  mEM(em)
		{}
		void startThread()
		{run("TVoutDetectThread", PRIORITY_NORMAL);}
		void stopThread()
		{requestExitAndWait();}
		bool threadLoop()
		{
			mEM->detectTVout();
			return true;
		}
	private:
		EventManager *mEM;
	};
#endif

	void setEventListener(EventListener *pListener);
private:
	sp<UeventThread> mUT;
	sp<ImpactThread> mIT;
#ifdef EVENT_TVD
	sp<TVDDetectThread>mTVDDetectThread;
	sp<TVoutDetectThread> mTVoutDetectThread;
#endif
	EventListener *mListener;
	int ueventLoop(void);
	int checkBattery(int &level);
	int checkACPlug();
	bool mBatteryOnline;
	bool mUSBDisconnect;
	bool mUSBConnectFirst;
	bool mUSBDisconnectFromPC;
	int mTiltAngle;
	float mXCoordinate;
	float mYCoordinate;
	float mZCoordinate;
#ifdef EVENT_TVD
	bool mTVDOnline;
	bool mNeedDelay;
#endif
};

#endif
