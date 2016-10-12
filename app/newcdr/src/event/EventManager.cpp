#include "EventManager.h"
#include "StorageManager.h"
#ifdef TVDECODER_ENABLE
#include <TVDecoder.h>
#endif
#include <sys/mman.h> 
#include <linux/videodev2.h> 
#include "debug.h"
#include <drv_disp.h> 
#include <math.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EventManager"

#define SHARE_VOL 						1
#define NANOS_PER_MS					1000000
#define FILTER_TIME_CONSTANT_MS 		200.0
#define MAX_FILTER_DELTA_TIME_NANOS 	1000*(NANOS_PER_MS)
#define M_PI 							3.14159265358979323846

void get_oneline(const char *pathname, String8 &val);

EventManager::EventManager(bool tvdOnline, bool hasTVD)
	: mUT(NULL)
	, mIT(NULL)
#ifdef EVENT_TVD
	, mTVDDetectThread(NULL)
#endif
	, mListener(NULL)
	, mBatteryOnline(false)
	, mUSBDisconnect(false)
	, mUSBConnectFirst(true)
	, mUSBDisconnectFromPC(false)
#ifdef EVENT_TVD
	, mTVDOnline(tvdOnline)
	, mNeedDelay(false)
#endif
{
#ifdef EVENT_TVD
	mNeedDelay = mTVDOnline;
#endif
	//init(hasTVD);
}

EventManager::~EventManager()
{
	exit();
}

static status_t getSystem(int fd, int *system)
{
	struct v4l2_format format;

	memset(&format, 0, sizeof(format));

	format.type = V4L2_BUF_TYPE_PRIVATE;
	if (ioctl (fd, VIDIOC_G_FMT, &format) < 0)
	{
		db_error("<F:%s, L:%d>VIDIOC_G_FMT error!", __FUNCTION__, __LINE__);
		return UNKNOWN_ERROR;
	}

	if((format.fmt.raw_data[16] & 1) == 0)
	{
		ALOGV("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
		return WOULD_BLOCK;
	}
#ifdef EVENT_TVD
	if((format.fmt.raw_data[16] & (1 << 4)) != 0)
	{
		*system = TVD_PAL;
	}
	else
	{
		*system = TVD_NTSC;
	}
#endif
	//ALOGV("format.fmt.raw_data[16] =0x%x",format.fmt.raw_data[16]);

	return NO_ERROR;
}

#ifdef EVENT_TVD
static int detectPlugInOut(void)
{
	int i;
	char linebuf[16];
	int val;
	i = 50;

    FILE *fp = fopen(TVD_FILE_ONLINE, "r");
	if (fp == NULL) {
		db_error("fail fopen %s\n", TVD_FILE_ONLINE);
		return -1;
	}
	while(i--) {
		if(fgets(linebuf, sizeof(linebuf), fp) == NULL) {
			db_error("fail fgets %s %s\n", TVD_FILE_ONLINE, strerror(errno));
			fclose(fp);
			return -1;
		}
		ALOGV("linebuf:%s", linebuf);
		val = atoi(linebuf);
		if (val == 0) {
			fclose(fp);
			return 0;
		} else if (val == 1) {
			fclose(fp);
		    return 1;
		}
		fseek(fp, 0, SEEK_SET);
	}
    fclose(fp);
	return -1;
}

int detectPlugIn(int fd)
{
	struct v4l2_format format;

	int system;
	status_t ret;
//RESET_PARAMS:
	ALOGV("TVD interface=%d, system=%d, format=%d, channel=%d", mInterface, mSystem, mFormat, mChannel);
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_PRIVATE;
	format.fmt.raw_data[0] = TVD_CVBS;
	format.fmt.raw_data[1] = TVD_PAL;
	if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
	{
		db_error("<F:%s, L:%d>VIDIOC_S_FMT error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}
	usleep(100000);

	format.fmt.raw_data[2] = TVD_PL_YUV420;

//TVD_CHANNEL_ONLY_1:
	format.fmt.raw_data[8]  = 1;	        //row
	format.fmt.raw_data[9]  = 1;	        //column
	format.fmt.raw_data[10] = 1;		//channel_index0
	format.fmt.raw_data[11] = 0;		//channel_index1
	format.fmt.raw_data[12] = 0;		//channel_index2
	format.fmt.raw_data[13] = 0;		//channel_index3
	if (ioctl(fd, VIDIOC_S_FMT, &format) < 0)
	{
		db_error("<F:%s, L:%d>VIDIOC_S_FMT error(%s)!", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}
	for (int i = 0; i<100; i++)
	{
		ret = getSystem(fd, &system);
		if (ret == NO_ERROR)
		{
			//if (mSystem != system)
			//{
			//	mSystem = system;
			//	goto RESET_PARAMS;
			//}
			return 0;
		} else {
			usleep(10 *1000);
		}
	}
	//db_debug("<F:%s, L:%d>No signal detected", __FUNCTION__, __LINE__);
	return -1;
}
	
void EventManager::startDetectTVD()
{
	db_msg("----startDetectTVD\n");
	if (mTVDDetectThread == NULL) {
		mTVDDetectThread = new TVDDetectThread(this);
		mTVDDetectThread->startThread();
	}
}

void EventManager::detectTVD()
{
	if (mTVDOnline == false) {
		int fd = open ("/dev/video1", O_RDWR|O_NONBLOCK, 0);
		if (fd < 0) {
			//db_msg("fail to open /dev/video1 :%s\n", strerror(errno));
            if (detectPlugInOut() == 1) {
                if (mListener && 0 == mListener->notify(EVENT_TVD_PLUG_IN, 0)) {
                    db_msg("tvd:plug in\n");
                    mTVDOnline = true;
                }
            }
		} else {
    		if (detectPlugIn(fd) == 0) {
    			db_msg("tvd:plug in\n");
    			mTVDOnline = true;
    			close(fd);
				if (mListener)
    				mListener->notify(EVENT_TVD_PLUG_IN, 0);
    		} else {
    			close(fd);
    		}
		}
		mNeedDelay = false;
	} else {
		if (mNeedDelay) {
			usleep(500*1000);	//wait for tvin device to be ready
			mNeedDelay = false;
		}
		if (detectPlugInOut() == 0) {
			if (mListener && 0 == mListener->notify(EVENT_TVD_PLUG_OUT, 0)) {
				mTVDOnline = false;
				db_msg("tvd:plug out\n");
			}
		}
	}
	usleep(200*1000);
}

int EventManager::startDetectTVout()
{
	db_msg("----startDetectTVD\n");
	if (mTVoutDetectThread == NULL) {
		mTVoutDetectThread = new TVoutDetectThread(this);
		mTVoutDetectThread->startThread();
	}
	return 0;
}
void EventManager::detectTVout()
{
	unsigned int arg[4];
	unsigned int dac_status;
	int disp_fd = -1;
	static int current_state = 0;
	arg[0] = 0;
	arg[1] = 0; //DAC 0
	disp_fd = open("/dev/disp", O_RDWR);
	if(disp_fd <= 0){
		db_msg("open /dev/disp error\n");
		return;
	}		
    dac_status = ioctl(disp_fd, DISP_CMD_TV_GET_DAC_STATUS, arg);
	close(disp_fd);
	if(dac_status == 1)
	{
		if(mListener && !current_state)
		{
			db_msg("TVout EVENT_TVOUT_PLUG_IN\n");
			mListener->notify(EVENT_TVOUT_PLUG_IN, 0);
			current_state = 1;
		}
	}else
	{
		if(mListener && current_state)
		{
			db_msg("TVout EVENT_TVOUT_PLUG_OUT\n");
			mListener->notify(EVENT_TVOUT_PLUG_OUT, 0);
			current_state = 0;
		}
	}
	usleep(200*1000);
}
#endif
	
int EventManager::init(bool hasTVD)
{
	String8 val, val_usb;
	bool ac_online, usb_online;
	uevent_init();
	if (mUT == NULL) {
		mUT = new UeventThread(this);
		mUT->startThread();
	}
	
#ifdef IMPLEMENT_IMPACT
	if (mIT == NULL) {
		mIT = new ImpactThread(this);
		mIT->init();
		mTiltAngle = -90;
		mXCoordinate = 0;
		mYCoordinate = 0;
		mZCoordinate = 0;
	}
#endif
	get_oneline(AC_ONLINE_FILE, val);
	get_oneline(USB_ONLINE_FILE, val_usb);
	ac_online = (atoi(val) == 1) ? true : false;
	usb_online = (atoi(val_usb) == 1) ? true : false;
	if (usb_online) {
		mUSBDisconnect = false;
	} else {
		mUSBDisconnect = true;
	}
	if (!ac_online && !usb_online)
		mBatteryOnline = true;
	else
		mBatteryOnline = false;

#ifdef EVENT_TVD
	if (hasTVD) {
		startDetectTVD();
	}
	startDetectTVout();
#endif
	return 0;
}


int EventManager::exit(void)
{
	if (mIT != NULL) {
		mIT->exit();
		mIT.clear();
		mIT = NULL;
	}
	if (mUT != NULL) {
		mUT->stopThread();
		mUT.clear();
		mUT = NULL;
	}
#ifdef EVENT_TVD
	if (mTVDDetectThread != NULL) {
		mTVDDetectThread->stopThread();
		mTVDDetectThread.clear();
		mTVDDetectThread = NULL;
	}
#endif
	
	return 0;
}

#ifdef EVENT_HDMI
void EventManager::hdmiAudioOn(bool open)
{
#if 1
	if (open) { //hdmi audio AUDIO_DEVICE_OUT_AUX_DIGITAL = 0x400
		cfgDataSetString("audio.routing", "1024");
	} else {	//normal audio AUDIO_DEVICE_OUT_SPEAKER = 0x2
		cfgDataSetString("audio.routing", "2");
	}
#endif
}
#endif

void EventManager::setEventListener(EventListener *pListener)
{
	mListener = pListener;
	int level;
	checkBattery(level);
	mListener->notify(EVENT_BATTERY, level);
}

bool EventManager::isBatteryOnline()
{
	return mBatteryOnline;
}

int EventManager::getTiltAngle()
{
	return mTiltAngle;
}

void EventManager::getCoordinate(float *x, float *y, float *z)
{
	*x = mXCoordinate;
	*y = mYCoordinate;
	*z = mZCoordinate;
}

void EventManager::setTiltAngle(int tiltAngle)
{
	mTiltAngle = tiltAngle;
}

void EventManager::setCoordinate(float x, float y, float z)
{
	mXCoordinate = x;
	mYCoordinate = y;
	mZCoordinate = z;
}

void get_oneline(const char *pathname, String8 &val)
{
	FILE * fp;
	char linebuf[1024];

	if(access(pathname, F_OK) != 0) {
		db_msg("%s not exsist\n", pathname);	
	}

	if(access(pathname, R_OK) != 0) {
		db_msg("%s can not read\n", pathname);	
	}

	fp = fopen(pathname, "r");

	if (fp == NULL) {
		return;
	}

	if(fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		fclose(fp);
		val = linebuf;
		return ;
	}

	fclose(fp);
}

#ifdef EVENT_UVC
bool isUVCStr(const char *str, int *index)
{
	char buf[128]={""};
	const char *num;
	int idx;
	getBaseName(str, buf);
	db_msg("isUVCstr :%s\n", buf);
	if (strncmp(buf, UVC_NODE, strlen(UVC_NODE))) {
		return false;
	}
	num = buf+strlen(UVC_NODE);
	db_msg("isUVCstr num :%s\n", num);
	idx = atoi(num);
	*index = idx;
	if (idx < 0 || idx >= 64) {
		return false;
	}
	return true;
}

int checkUVC(char *udata)
{
	int ret = 0;
	const char *tmp_start;
	tmp_start = udata;
	int index = -1;
	char uvcDev[128] = {""};
	int i = 20;
	
	if (isUVCStr(udata, &index)) {
		if (!strncmp(tmp_start, UVC_ADD, strlen(UVC_ADD))) {
			db_msg("\nUVC Plug In\n");
			sprintf(uvcDev, "/dev/video%d", index);
			int uvcfd = -1;
			db_msg("try open %s", uvcDev);
		    while(i-- > 0){
				uvcfd = open(uvcDev, O_RDONLY);
				if(uvcfd < 0) {
					db_msg("Open %s failed! strr: %s",uvcDev, strerror(errno));
				} else {
					db_msg("%s is ok! fd = %d", uvcDev, uvcfd);
					close(uvcfd);
					break;
				}
				usleep(20*1000);
			}	
			
			if(i == 0)
				db_msg("%s stat timeout!", uvcDev);
				
			ret = EVENT_UVC_PLUG_IN;
			//mListener->notify(EVENT_UVC_PLUG_IN);	
		} else if (!strncmp(tmp_start, UVC_REMOVE, strlen(UVC_REMOVE))) {
			db_msg("\nUVC Plug Out\n");
			ret = EVENT_UVC_PLUG_OUT;
			//mListener->notify(EVENT_UVC_PLUG_OUT);
		} else {
			db_msg("unkown message for UVC\n");	
			db_msg("tmp_start is %s\n", tmp_start);
		}
	}
	return ret;
}
#endif

static inline int capacity2Level(int capacity)
{
	return ((capacity - 1) / 20 + 1);
}

int EventManager::checkACPlug()
{
	int ret = 0;
	String8 val_new("");
	bool ac_online, usb_online, battery_online;
	
	get_oneline(AC_ONLINE_FILE, val_new);
	ac_online = (atoi(val_new) == 1) ? true : false;
	get_oneline(USB_ONLINE_FILE, val_new);
	usb_online = (atoi(val_new) == 1) ? true : false;
	
	if (!ac_online && !usb_online)
		battery_online = true;
	else
		battery_online = false;

	db_msg("mBatteryOnline:%d, battery_online:%d\n", mBatteryOnline, battery_online);
	if (mBatteryOnline==false && battery_online==true) {
		mBatteryOnline = battery_online;
		if (mUSBDisconnectFromPC == true) {
			mUSBDisconnectFromPC = false;
			return ret;
		}
		db_debug("checkACPlug EVENT_DELAY_SHUTDOWN\n");
		return EVENT_DELAY_SHUTDOWN;
	} else if(mBatteryOnline==true && battery_online==false) {
		mBatteryOnline = battery_online;
		db_debug("checkACPlug EVENT_CANCEL_SHUTDOWN\n");
		return EVENT_CANCEL_SHUTDOWN;
	}
	
	return ret;
}

static bool isBatteryEvent(char *udata)
{
	const char *tmp_start;
	const char *tmp_end;
	tmp_start = udata;
	tmp_end = udata + strlen(udata) - strlen(POWER_BATTERY);
	
	if (strcmp(tmp_end, POWER_BATTERY)) { //battery
		return false;
	}
	if (strncmp(tmp_start, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return false;
	}
	return true;
}

int EventManager::checkBattery(int &level)
{
	int ret = 0;
	String8 val("");
	int battery_capacity = 0;
	
	get_oneline(BATTERY_CAPACITY, val);
	battery_capacity = atoi(val.string());
	level = capacity2Level(battery_capacity);
	ret = EVENT_BATTERY;
	
	return ret;
}

int EventManager::checkUSB(char *udata)
{
	const char *tmp_start;
	const char *tmp_end;
	tmp_start = udata;
	tmp_end = udata + strlen(udata) - strlen(POWER_BATTERY);
	String8 val_usb;

	if (strcmp(tmp_end, POWER_BATTERY)) { //usb
		return -1;
	}

	if (strncmp(tmp_start, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return -1;
	}
	bool usb_online;
	get_oneline(USB_ONLINE_FILE, val_usb);
	usb_online = (atoi(val_usb) == 1) ? true : false;
	if (usb_online && (mUSBDisconnect == true || mUSBConnectFirst)) {
		db_debug("checkUSB EVENT_CONNECT2PC\n");
		mUSBDisconnect = false;
		mUSBConnectFirst = false;
		return EVENT_CONNECT2PC;
	}
	if (!usb_online && mUSBDisconnect == false) {
		mUSBDisconnect = true;
		mUSBDisconnectFromPC = true;
		db_debug("checkUSB EVENT_DISCONNECFROMPC\n");
		return EVENT_DISCONNECFROMPC;
	}
	return -1;
}

static bool getValue(const char *str)
{
	String8 val;
	get_oneline(str, val);
	return (atoi(val)==1)?true:false;
}

#ifdef EVENT_HDMI
int EventManager::checkHDMI(const char *str)
{
	char buf[32];
	getBaseName(str, buf);
	bool val;
	if (strncmp(buf, HDMI_NODE, strlen(HDMI_NODE))) {
		return 0;
	}
	if (strncmp(str, UEVENT_CHANGE, strlen(UEVENT_CHANGE))) { //change
		return 0;
	}
	val = getValue(HDMI_ONLINE_FILE);
	if (val) {
		return EVENT_HDMI_PLUGIN;
	}
	return EVENT_HDMI_PLUGOUT;
}
#endif

int EventManager::ueventLoop(void)
{
	char udata[4096] = {0};

	uevent_next_event(udata, sizeof(udata) - 2);
	
	db_msg("uevent_loop %s\n", udata);
	if(mListener) {
		int ret = 0;
		int val = 4;
#ifdef EVENT_UVC
		ret = checkUVC(udata);
		if (ret > 0) {
			mListener->notify(ret, 0);
			return ret;
		}
#endif
		ret = checkUSB(udata);
		if (ret > 0) {
			mListener->notify(ret, 0);
		}

		if(isBatteryEvent(udata)) {
			ret = checkACPlug();
			if (ret > 0) {
				mListener->notify(ret, val);
				return ret;
			}
			ret = checkBattery(val);
			if (ret > 0) {
				mListener->notify(ret, val);
				return ret;
			}
		}

#ifdef EVENT_HDIM
		ret = checkHDMI(udata);
		if (ret > 0) {
			mListener->notify(ret, 0);
			return ret;
		}
#endif
		db_msg("The event is not handled\n");	
	}

	return 0;
}
void EventManager::connect2PC()
{
	cfgDataSetString("sys.usb.config", "mass_storage");
#ifdef SHARE_VOL
	StorageManager *sm = StorageManager::getInstance();
	sm->shareVol();
#else
	int fd = open(CD_TFCARD_LUN_PATH, O_RDWR);
	write(fd, CD_TFCARD_PATH, strlen(CD_TFCARD_PATH));
	close(fd);
#endif
}

void EventManager::disconnectFromPC()
{
	cfgDataSetString("sys.usb.config", "none");
    StorageManager *sm = StorageManager::getInstance();
	sm->unShareVol();
}

char const* getSensorName(int type) {
    switch(type) {
        case SENSOR_TYPE_ACCELEROMETER:
            return "Acc";
        case SENSOR_TYPE_MAGNETIC_FIELD:
            return "Mag";
        case SENSOR_TYPE_ORIENTATION:
            return "Ori";
        case SENSOR_TYPE_GYROSCOPE:
            return "Gyr";
        case SENSOR_TYPE_LIGHT:
            return "Lux";
        case SENSOR_TYPE_PRESSURE:
            return "Bar";
        case SENSOR_TYPE_TEMPERATURE:
            return "Tmp";
        case SENSOR_TYPE_PROXIMITY:
            return "Prx";
        case SENSOR_TYPE_GRAVITY:
            return "Grv";
        case SENSOR_TYPE_LINEAR_ACCELERATION:
            return "Lac";
        case SENSOR_TYPE_ROTATION_VECTOR:
            return "Rot";
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
            return "Hum";
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
            return "Tam";
    }
    return "ukn";
}

ImpactThread::ImpactThread(EventManager *em)
	: sensorCount(0),
	  device(NULL),
	  sensorList(NULL),
	  mEM(em)
{
	ALOGV("ImpactMonitor Constructot\n");
}

ImpactThread::~ImpactThread()
{

}

int ImpactThread::init(void)
{
	if(impactInit() < 0) {
		db_msg("init sensors failed\n");
		exit();
		return -1;
	}
	if (sensorCount > 0) {
		startThread();
	}
	
	return 0;
}

int ImpactThread::impactInit(void)
{
	int err;
	struct sensors_module_t* module;

	err = hw_get_module(SENSORS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
	if (err != 0) {
		db_error("hw_get_module() failed (%s)\n", strerror(-err));
		return -1;
	}

	err = sensors_open(&module->common, &device);
	if (err != 0) {
		db_error("sensors_open() failed (%s)\n", strerror(-err));
		return -1;
	}

	sensorCount = module->get_sensors_list(module, &sensorList);
	db_debug("%d sensors found:\n", sensorCount);
	if (sensorCount == 0) {
		return -1;
	}
	for (int i=0 ; i < sensorCount ; i++) {
		db_msg("%s\n"
				"\tvendor: %s\n"
				"\tversion: %d\n"
				"\thandle: %d\n"
				"\ttype: %d\n"
				"\tmaxRange: %f\n"
				"\tresolution: %f\n"
				"\tpower: %f mA\n",
				sensorList[i].name,
				sensorList[i].vendor,
				sensorList[i].version,
				sensorList[i].handle,
				sensorList[i].type,
				sensorList[i].maxRange,
				sensorList[i].resolution,
				sensorList[i].power);
	}

	for (int i = 0 ; i < sensorCount ; i++) {
		err = device->activate(device, sensorList[i].handle, 0);
		if (err != 0) {
			db_error("deactivate() for '%s'failed (%s)\n",
					sensorList[i].name, strerror(-err));
			return -1;
		}
	}

	for (int i = 0 ; i < sensorCount ; i++) {
		err = device->activate(device, sensorList[i].handle, 1);
		if (err != 0) {
			db_error("activate() for '%s'failed (%s)\n",
					sensorList[i].name, strerror(-err));
			/* return -1; */
		}
		device->setDelay(device, sensorList[i].handle, ms2ns(10));
	}

	db_debug("init sensors success\n");
	return 0;
}

int ImpactThread::exit(void)
{
	int err;
	for (int i = 0 ; i < sensorCount ; i++) {
		err = device->activate(device, sensorList[i].handle, 0);
		if (err != 0) {
			db_error("deactivate() for '%s'failed (%s)\n", sensorList[i].name, strerror(-err));
		}
	}

	ALOGV("------------------------------------------\n");
	err = sensors_close(device);
	ALOGV("------------------------------------------\n");
	if (err != 0) {
		db_error("sensors_close() failed (%s)\n", strerror(-err));
	}
	if (sensorCount > 0) {
		stopThread();
	}

	return 0;
}

int ImpactThread::impactLoop(void)
{
	static const size_t numEvents = 16;
	sensors_event_t buffer[numEvents];
	int n;
	n = device->poll(device, buffer, numEvents);
	if (n < 0) {
		db_error("poll() failed\n");
		return -1;
	}

	for (int i=0 ; i<n ; i++) {
		sensors_event_t& data = buffer[i];

		if (data.version != sizeof(sensors_event_t)) {
			db_error("incorrect event version (version=%d, expected=%u", data.version, sizeof(sensors_event_t));
			return -1;
		}

		switch(data.type) {
		case SENSOR_TYPE_ACCELEROMETER:
		case SENSOR_TYPE_MAGNETIC_FIELD:
		case SENSOR_TYPE_ORIENTATION:
		case SENSOR_TYPE_GYROSCOPE:
		case SENSOR_TYPE_GRAVITY:
		case SENSOR_TYPE_LINEAR_ACCELERATION:
		case SENSOR_TYPE_ROTATION_VECTOR:
			if (data.data[0] > THRESHOLD_VALUE ||
					data.data[1] > THRESHOLD_VALUE ||
					data.data[2] > THRESHOLD_VALUE) 
			{
				/* db_msg("impact occur!"); */
				mEM->impactEvent();
			}
			break;

		case SENSOR_TYPE_LIGHT:
		case SENSOR_TYPE_PRESSURE:
		case SENSOR_TYPE_TEMPERATURE:
		case SENSOR_TYPE_PROXIMITY:
		case SENSOR_TYPE_RELATIVE_HUMIDITY:
		case SENSOR_TYPE_AMBIENT_TEMPERATURE:
			ALOGV("sensor=%s, time=%lld, value=%f\n", getSensorName(data.type), data.timestamp, data.data[0]);
			break;

		default:
			db_msg("sensor=%d, time=%lld, value=<%f,%f,%f, ...>\n",
					data.type,
					data.timestamp,
					data.data[0],
					data.data[1],
					data.data[2]);
			break;
		}

		// Calculate the tilt angle.
        // This is the angle between the up vector and the x-y plane (the plane of
        // the screen) in a range of [-90, 90] degrees.
        //   -90 degrees: screen horizontal and facing the ground (overhead)
        //     0 degrees: screen vertical
        //    90 degrees: screen horizontal and facing the sky (on table)
		if(data.type == SENSOR_TYPE_ACCELEROMETER) {
#if	0
			db_msg("sensor=%d, time=%lld, value=<%f,%f,%f, ...>\n",
					data.type,
					data.timestamp,
					data.data[0],
					data.data[1],
					data.data[2]);
#endif
			float x = data.data[0];
		    float y = data.data[1];
		    float z = data.data[2];
			long now = data.timestamp;
			long then = mLastFilteredTimestampNanos;
		    float timeDeltaMS = (now - then) * 0.000001f;
			float RADIANS_TO_DEGREES = (float) (180 / M_PI);
			float alpha = timeDeltaMS / (FILTER_TIME_CONSTANT_MS + timeDeltaMS);
            x = alpha * (x - mLastFilteredX) + mLastFilteredX;
            y = alpha * (y - mLastFilteredY) + mLastFilteredY;
            z = alpha * (z - mLastFilteredZ) + mLastFilteredZ;

			mLastFilteredTimestampNanos = now;
            mLastFilteredX = x;
            mLastFilteredY = y;
            mLastFilteredZ = z;
			
			float magnitude = (float)sqrt(x * x + y * y + z * z);
			int tiltAngle = (int) round(asin(z / magnitude) * RADIANS_TO_DEGREES);
			//db_msg("tiltAngle=%d", tiltAngle);
			mEM->setTiltAngle(tiltAngle);
			mEM->setCoordinate(data.data[0], data.data[1], data.data[2]);
					
		}
	}

	return 0;
}

int EventManager::impactEvent()
{
	if (mListener) {
		mListener->notify(EVENT_IMPACT, 0);
	}
	return 0;
}

