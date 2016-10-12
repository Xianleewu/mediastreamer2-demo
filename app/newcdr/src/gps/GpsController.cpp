#include <gps/GpsController.h>
#include <cellular/CellularController.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Rtc.h"
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <linux/in.h>
#include <linux/in6.h>

#undef LOG_TAG
#define LOG_TAG "GpsController"
#include "debug.h"

#define WAKE_LOCK_NAME  	"GPS"
#define MAX_THREAD_COUNTS	20

// temporary storage for GPS callbacks
static const char* sNmeaString;
static int sNmeaStringLength;
static const GpsInterface* sGpsInterface = NULL;
static const GpsXtraInterface* sGpsXtraInterface = NULL;
static const AGpsInterface* sAGpsInterface = NULL;
static const GpsNiInterface* sGpsNiInterface = NULL;
static const GpsDebugInterface* sGpsDebugInterface = NULL;
static const AGpsRilInterface* sAGpsRilInterface = NULL;
static const GpsGeofencingInterface* sGpsGeofencingInterface = NULL;
static const GpsMeasurementInterface* sGpsMeasurementInterface = NULL;
static const GpsNavigationMessageInterface* sGpsNavigationMessageInterface = NULL;
static const GnssConfigurationInterface* sGnssConfigurationInterface = NULL;
static hw_module_t* mGpsModule;
static GpsLocation mGpsLocation;
static GpsStatus mGpsStatus;
static GpsSvStatus mGpsSvStatus;
static GpsData mGpsData;
static int mCallbackIndex = -1;
static pthread_t mptContain[MAX_THREAD_COUNTS];
static bool mGpsStart = false;
static bool setedRtcTime = false;
static double valid_latitude = 0.00000000000000;
static double valid_longtitude = 0.00000000000000;
#define GPS_LOCATION_STR_LEN		(32)
#define GPS_LOCATION_NO_SINGLE_STR			("0.00000000000000")

GpsCallbacks sGpsCallbacks = {
    sizeof(GpsCallbacks),
    locationCallback,
    statusCallback,
    svStatusCallback,
    nmeaCallback,
    setCapabilitiesCallback,
    acquireWakelockCallback,
    releaseWakelockCallback,
    createThreadCallback,
    requestUtcTimeCallback,
};

GpsXtraCallbacks sGpsXtraCallbacks = {
    xtraDownloadRequestCallback,
	createThreadCallback,
};

GpsNiCallbacks sGpsNiCallbacks = {
    gpsNiNotifyCallback,
	createThreadCallback,
};

AGpsCallbacks sAGpsCallbacks = {
    agpsStatusCallback,
	createThreadCallback,
};

AGpsRilCallbacks sAGpsRilCallbacks = {
    agpsRequestSetId,
    agpsRequestRefLocation,
    createThreadCallback,
};

GpsMeasurementCallbacks sGpsMeasurementCallbacks = {
    sizeof(GpsMeasurementCallbacks),
    measurementCallback,
};

static void update_valid_location(double latitude, double longtitude)
{
	char LongitudeStr[GPS_LOCATION_STR_LEN+1]={0};
	char LatitudeStr[GPS_LOCATION_STR_LEN+1]={0};

	memset(LongitudeStr, 0, sizeof(LongitudeStr));
	snprintf(LongitudeStr, GPS_LOCATION_STR_LEN, "%.14lf", longtitude);

	memset(LatitudeStr, 0, sizeof(LatitudeStr));
	snprintf(LatitudeStr, GPS_LOCATION_STR_LEN, "%.14lf", latitude);

	db_msg("update_valid_location latitude=%.14lf longtitude=%.14lf latitude_str=%s longtitude_str=%s \n", latitude, longtitude, LatitudeStr, LongitudeStr);
	if((strcasecmp(LongitudeStr, GPS_LOCATION_NO_SINGLE_STR) == 0) && (strcasecmp(LatitudeStr, GPS_LOCATION_NO_SINGLE_STR) == 0))
	{
		db_msg("update_valid_location no sigle, no update \n");
	}
	else
	{
		db_msg("update_valid_location update \n");
		valid_latitude  = latitude;
		valid_longtitude = longtitude;
	}

	return;
}

double getValidLatitude(void)
{
	return valid_latitude;
}

double getValidLongtitude(void)
{
	return valid_longtitude;
}

static int startGps()
{
	if (sGpsInterface) {
        if (sGpsInterface->start() == 0) {
            return 0;
        } else {
        	db_error("%s::start GPS fail.", __func__);
            return -1;
        }
    }
    else {
		db_error("%s::sGpsInterface is null.", __func__);
		return -1;
    }
}

static int stopGps()
{
	if (sGpsInterface) {
		if (sGpsInterface->stop() == 0) {
			return 0;
		} else {
			db_error("%s::stop GPS fail.", __func__);
			return -1;
		}
	}
	else {
		db_error("%s::sGpsInterface is null.", __func__);
		return -1;
	}

}

//Closes gps interface
static void cleanUpGps()
{
	if (sGpsInterface)
        sGpsInterface->cleanup();
}

static void deleteAidingData(GpsAidingData flags)
{
	if (sGpsInterface)
        sGpsInterface->delete_aiding_data(flags);
}
void gpsUpdateNetworkState(int connected, int type, int roaming, const char* extra_info)
{
	db_error("gpsUpdateNetworkState %d,%d,%d",connected,type,sAGpsRilInterface);
	if(sAGpsRilInterface)
		sAGpsRilInterface->update_network_state(connected, type, roaming, NULL);
}

static int setPositionMode(GpsPositionMode mode, GpsPositionRecurrence recurrence, 
			uint32_t minInterval, uint32_t preferredAccuracy, uint32_t preferredTime)
{
	if (sGpsInterface) {
        if (sGpsInterface->set_position_mode(mode, recurrence, minInterval, preferredAccuracy,
                preferredTime) == 0) {
            return 0;
        } else {
            return -1;
        }
    }
    else
        return -1;
}

static bool initGpsController()
{
	int err;
	err = hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&mGpsModule);
    if (err == 0) {
        hw_device_t* device;
        err = mGpsModule->methods->open(mGpsModule, GPS_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            gps_device_t* gps_device = (gps_device_t *)device;
            sGpsInterface = gps_device->get_gps_interface(gps_device);
        }
    }

    if (sGpsInterface) {
        sGpsXtraInterface =
            (const GpsXtraInterface*)sGpsInterface->get_extension(GPS_XTRA_INTERFACE);
        sAGpsInterface =
            (const AGpsInterface*)sGpsInterface->get_extension(AGPS_INTERFACE);
        sGpsNiInterface =
            (const GpsNiInterface*)sGpsInterface->get_extension(GPS_NI_INTERFACE);
        sGpsDebugInterface =
            (const GpsDebugInterface*)sGpsInterface->get_extension(GPS_DEBUG_INTERFACE);
        sAGpsRilInterface =
            (const AGpsRilInterface*)sGpsInterface->get_extension(AGPS_RIL_INTERFACE);
        sGpsGeofencingInterface =
            (const GpsGeofencingInterface*)sGpsInterface->get_extension(GPS_GEOFENCING_INTERFACE);
        sGpsMeasurementInterface =
            (const GpsMeasurementInterface*)sGpsInterface->get_extension(GPS_MEASUREMENT_INTERFACE);
        sGpsNavigationMessageInterface =
            (const GpsNavigationMessageInterface*)sGpsInterface->get_extension(
                    GPS_NAVIGATION_MESSAGE_INTERFACE);
        sGnssConfigurationInterface =
            (const GnssConfigurationInterface*)sGpsInterface->get_extension(
                    GNSS_CONFIGURATION_INTERFACE);
    }else {
		db_error("%s::sGpsInterface is null.", __func__);
		return false;
	}

	// fail if the main interface fails to initialize
	if (!sGpsInterface || sGpsInterface->init(&sGpsCallbacks) != 0) {
		db_error("%s::init sGpsCallbacks fail.", __func__);
		return false;
	}

	// if XTRA initialization fails we will disable it by sGpsXtraInterface to NULL,
	// but continue to allow the rest of the GPS interface to work.
	if (sGpsXtraInterface && sGpsXtraInterface->init(&sGpsXtraCallbacks) != 0)
		sGpsXtraInterface = NULL;
	if (sGpsNiInterface)
		sGpsNiInterface->init(&sGpsNiCallbacks);
	if (sAGpsRilInterface)
		sAGpsRilInterface->init(&sAGpsRilCallbacks);
	 if (sAGpsInterface)
        sAGpsInterface->init(&sAGpsCallbacks);

	return true;
}

bool gpsIsSupported()
{
    if (sGpsInterface != NULL) {
        return true;
    } else {
        return false;
    }
}

void locationCallback(GpsLocation *location)
{
	struct tm *local;
	time_t t = location->timestamp/1000;	
	local=localtime(&t);
	if(!setedRtcTime) {
		setDateTime(local);
		setedRtcTime = true;
		db_debug("  datetime->tm_year=%d",local->tm_year+1900);
		db_debug("  datetime->tm_mon=%d",local->tm_mon);
		db_debug("  datetime->tm_mday=%d",local->tm_mday);
		db_debug("  datetime->tm_hour=%d",local->tm_hour);
		db_debug("  datetime->tm_min=%d",local->tm_min);
		db_debug("  datetime->tm_sec=%d",local->tm_sec);
    }	
	db_debug("GpsController::locationCallback latitude=%lf longitude=%lf speed=%f \n", 
		location->latitude, location->longitude, location->speed);
	memcpy(&mGpsLocation, location, sizeof(mGpsLocation));
	update_valid_location(location->latitude, location->longitude);
}

pthread_t createThreadCallback(const char *name, void* (*start)(void *), void *arg)
{
	db_msg("createThreadCallback name=%s\n", name);
	mCallbackIndex++;
	if(mCallbackIndex >= MAX_THREAD_COUNTS - 1) {
		mCallbackIndex = 0;
	}

	pthread_create(&mptContain[mCallbackIndex], NULL, start, arg);
	return mptContain[mCallbackIndex];
}

/***************************************
****************************************
GPS status unknown  
#define GPS_STATUS_NONE				0

GPS has begun navigating. 
#define GPS_STATUS_SESSION_BEGIN		1

GPS has stopped navigating.   
#define GPS_STATUS_SESSION_END		2  
GPS has powered on but is not navigating.   
#define GPS_STATUS_ENGINE_ON			3

GPS is powered off. AgpsCallbacks  
#define GPS_STATUS_ENGINE_OFF			4  

****************************************
****************************************/
void statusCallback(GpsStatus *status)
{
	//db_msg("GpsController::statusCallback >>>>>>>>>>>>>>>>GpsStatusValue=%d\n", status->status);
	memcpy(&mGpsStatus, status, sizeof(mGpsStatus));
}

void svStatusCallback(GpsSvStatus *svStatus)
{
	//db_msg("GpsController::svStatusCallback num_svs=%d\n", svStatus->num_svs);
	memcpy(&mGpsSvStatus, svStatus, sizeof(mGpsSvStatus));
}

void nmeaCallback(GpsUtcTime timestamp, const char *nmea, int length)
{
	//db_msg("nmeaCallback nmea:%s\n", nmea);
	sNmeaString = nmea;
    sNmeaStringLength = length;
}

void setCapabilitiesCallback(uint32_t capabilities)
{
	db_msg("setCapabilitiesCallback capabilities=%du\n", capabilities);
}

void acquireWakelockCallback()
{
	db_msg("acquireWakelockCallback\n");
	acquire_wake_lock(PARTIAL_WAKE_LOCK, WAKE_LOCK_NAME);
}

void releaseWakelockCallback()
{
	db_msg("releaseWakelockCallback\n");
	release_wake_lock(WAKE_LOCK_NAME);
}

void requestUtcTimeCallback()
{
	db_msg("requestUtcTimeCallback\n");
}

void xtraDownloadRequestCallback()
{
	db_msg("xtraDownloadRequestCallback\n");
}

void gpsNiNotifyCallback(GpsNiNotification *notification)
{
	db_msg("gpsNiNotifyCallback\n");
}
void agps_data_conn_open(char *apn, int apnIpType)
{
	db_error("agps_data_conn_open");
	if (!sAGpsInterface) {
		db_error("no AGPS interface in agps_data_conn_open");
		return;
	}
 
	int interface_size = sAGpsInterface->size;
	if (interface_size == sizeof(AGpsInterface_v2)) {
		db_error("sAGpsInterface->data_conn_open_with_apn_ip_type(apn)");
		sAGpsInterface->data_conn_open_with_apn_ip_type(apn, apnIpType);
	} else if (interface_size == sizeof(AGpsInterface_v1)) {
		db_error("sAGpsInterface->data_conn_open(apn)");
		sAGpsInterface->data_conn_open(apn);
	} else {
		db_error("Invalid size of AGpsInterface found: %d.", interface_size);
	}
 
}

void reportAGpsStatus(int type, int status, char* ipaddr) 
{
 	db_error("reportAGpsStatus %d",status);
	switch (status) {
		case GPS_REQUEST_AGPS_DATA_CONN:		
			agps_data_conn_open("3gnet",1);
			break;
		case GPS_RELEASE_AGPS_DATA_CONN:
		   	if(sAGpsInterface)
				sAGpsInterface->data_conn_closed();
			break;
		case GPS_AGPS_DATA_CONNECTED:
			break;
		case GPS_AGPS_DATA_CONN_DONE:
			break;
		case GPS_AGPS_DATA_CONN_FAILED:
			break;
		default:
		   	break;
	}
}

void agpsStatusCallback(AGpsStatus *agps_status)
{
	db_error("agpsStatusCallback");
	reportAGpsStatus(agps_status->type,	agps_status->status, NULL);	
}

void agpsCellRequestSetId()
{
	db_msg("agpsRequestSetId %s\n",getImsi());
  	if(sAGpsRilInterface)
    	sAGpsRilInterface->set_set_id(AGPS_SETID_TYPE_IMSI, getImsi());


}

void agpsRequestSetId(uint32_t flags)
{
	db_msg("agpsRequestSetId %s\n",getImsi());
  	if(sAGpsRilInterface)
    	sAGpsRilInterface->set_set_id(AGPS_SETID_TYPE_IMSI, getImsi());

	agps_data_conn_open("3gnet",1);
	stopGps();
	startGps();
}

void agpsRequestRefLocation(uint32_t flags)
{
	db_msg("agpsRequestRefLocation\n");
	AGpsRefLocation location;
	cell_info cell = getCellInfo();
    if (!sAGpsRilInterface) {
        db_error("no AGPS RIL interface in agps_set_reference_location_cellid");
        return;
    }
    switch(flags) {
        case AGPS_RIL_REQUEST_REFLOC_CELLID:
            location.type = AGPS_REF_LOCATION_TYPE_GSM_CELLID;
			
            location.u.cellID.mcc = cell.mcc;
            location.u.cellID.mnc = cell.mnc;
            location.u.cellID.lac = cell.lac;
            location.u.cellID.cid = cell.cell;
			db_error("mcc:%d,mnc:%d,lac%d,cid:%d",location.u.cellID.mcc,location.u.cellID.mnc,location.u.cellID.lac,location.u.cellID.cid);
			sAGpsRilInterface->set_ref_location(&location, sizeof(location));
            break;
        default:
            ALOGE("Neither a GSM nor a UMTS cellid (%s:%d).",__FUNCTION__,__LINE__);
            return;
            break;
    }
}

void configurationUpdate(const char *data, int length)
{
	if (!sGnssConfigurationInterface) {
        db_error("no GPS configuration interface in configuraiton_update\n");
        return;
    }
    ALOGD("GPS configuration: %s\n", data);
    sGnssConfigurationInterface->configuration_update(
            data, length);
}

void measurementCallback(GpsData* data)
{
	if (data == NULL) {
		db_error("Invalid data provided to gps_measurement_callback\n");
		return;
	}

	if (data->size == sizeof(GpsData)) {
		memcpy(&mGpsData, data, sizeof(mGpsData));
    } else {
        db_error("Invalid GpsData size found in gps_measurement_callback, size=%d\n", data->size);
    }

}

bool isMeasurementSupported()
{
	if (sGpsMeasurementInterface != NULL) {
		return true;
	}
	return false;

}

bool startMeasurementCollection()
{
	if (sGpsMeasurementInterface == NULL) {
        db_error("Measurement interface is not available.\n");
        return false;
    }

    int result = sGpsMeasurementInterface->init(&sGpsMeasurementCallbacks);
    if (result != GPS_GEOFENCE_OPERATION_SUCCESS) {
        db_error("An error has been found on GpsMeasurementInterface::init, status=%d\n", result);
        return false;
    }

    return true;
}

bool stopMeasurementCollection()
{
	if (sGpsMeasurementInterface == NULL) {
        db_error("Measurement interface not available\n");
        return false;
    }

    sGpsMeasurementInterface->close();
    return true;

}

bool gpsIsSupportsXtra()
{
	if (sGpsXtraInterface != NULL) {
        return true;
    } else {
        return false;
    }
}

void opengps()
{
	if(mGpsStart) {
		db_msg("gps has been enable...\n");
		return ;
	}

	db_msg("opengps gps start ...\n");

	if(!gpsIsSupported()) {
		if(!initGpsController()) {
			db_error("enableGps::initGpsController fail.\n");
			return ;
		}
	}
}

bool enableGps()
{
	if(mGpsStart) {
		db_msg("gps has been enable...\n");
		return true;
	}

	db_msg("enable gps start ...\n");

	if(!gpsIsSupported()) {
		if(!initGpsController()) {
			db_error("enableGps::initGpsController fail.\n");
			return false;
		}
	}

	if(setPositionMode(GPS_POSITION_MODE_STANDALONE, GPS_POSITION_RECURRENCE_PERIODIC, 1500, 0, 0)) {
		db_error("enableGps::setPositionMode fail.\n");
		return false;
	}

	if(startGps()) {
		db_error("enableGps::start Softap fail.\n");
		return false;
	}

	mGpsStart = true;
	return true;
}

bool disableGps()
{
	db_msg("disable gps start ...\n");
	if(!mGpsStart || !gpsIsSupported()) {
		db_msg("disableGps::gps has been disable...\n");
		return true;
	}

	if(stopGps()) {
		db_error("disableGps::start Softap fail.\n");
		return false;
	}

	mGpsStart = false;
	return true;
}

GpsStatusValue getGpsStatus()
{
	if(!gpsIsSupported()) {
		db_error("getGpsStatus::gps is can't support.\n");
		return GPS_STATUS_NONE;
	}

	return mGpsStatus.status;
}

double getLatitude()
{
	if(!gpsIsSupported()) {
		db_error("getLatitude::gps can't support.\n");
		return 0;
	}

	return mGpsLocation.latitude;
}

double getLongitude()
{
	if(!gpsIsSupported()) {
		db_error("getLongitude::gps can't support.\n");
		return 0;
	}
	
	return mGpsLocation.longitude;
}

double getAltitude()
{
	if(!gpsIsSupported()) {
		db_error("getAltitude::gps can't support.\n");
		return 0;
	}
	
	return mGpsLocation.altitude;
}

float getSpeed()
{
	if(!gpsIsSupported()) {
		db_error("getSpeed::gps can't support.\n");
		return 0;
	}
	
	return mGpsLocation.speed * 3.6;
}

int readGpsNmea(char *str, int bufferSize)
{
	if(str == NULL) {
		db_error("readGpsNmea::str point is null.\n");
		return 0;
	}
	int length = sNmeaStringLength;
	if (length > bufferSize)
		length = bufferSize;
	snprintf(str, length, "%s", sNmeaString);
	
	return length;
}

GpsUtcTime getUtcTime()
{
	if(!gpsIsSupported()) {
		db_error("getSpeed::gps can't support.\n");
		return 0;
	}
	
	return mGpsLocation.timestamp;
}

int getSvsNum()
{
	if(!gpsIsSupported()) {
		db_error("getSvsNum::gps can't support.\n");
		return 0;
	}

	return mGpsSvStatus.num_svs;
}

//the index is 0 -- num_svs-1
int getSvsId(int index)
{
	if(!gpsIsSupported()) {
		db_error("getSvsId::gps can't support.\n");
		return 0;
	}

	if(index >= getSvsNum()) {
		db_error("getSvsId::the index is invalid.\n");
		return 0;
	}

	return mGpsSvStatus.sv_list[index].prn;
}

//the index is 0 -- num_svs-1
float getSvsElevation(int index) 
{
	if(!gpsIsSupported()) {
		db_error("getSvsElevation::gps can't support.\n");
		return 0;
	}

	if(index >= getSvsNum()) {
		db_error("getSvsElevation::the index is invalid.\n");
		return 0;
	}

	return mGpsSvStatus.sv_list[index].elevation;
}

//the index is 0 -- num_svs-1
float getSvsAzimuth(int index) 
{
	if(!gpsIsSupported()) {
		db_error("getSvsAzimuth::gps can't support.\n");
		return 0;
	}

	if(index >= getSvsNum()) {
		db_error("getSvsAzimuth::the index is invalid.\n");
		return 0;
	}

	return mGpsSvStatus.sv_list[index].azimuth;
}

//the index is 0 -- num_svs-1
float getSignal2NoiseRatio(int index)
{
	if(!gpsIsSupported()) {
		db_error("getSignal2NoiseRatio::gps can't support.\n");
		return 0;
	}

	if(index >= getSvsNum()) {
		db_error("getSignal2NoiseRatio::the index is invalid.\n");
		return 0;
	}

	return mGpsSvStatus.sv_list[index].snr;
}

int getGpsInfo(char *info, int bufferSize)
{
	char Latitudedstr[9];
	char Longitudedstr[10];
	char CarSpeedstr[11];
	if(info == NULL) {
		db_error("getGpsInfo::the parameter info point is null.\n");
		return -1;
	}

	if(bufferSize < 30) {
		db_error("getGpsInfo::the buffer size is too small.\n");
		return -1;
	}

	if(!mGpsStart) {
		//db_error("getGpsInfo::gps status has not start.\n");
		return -1;
	}

	double latitude = getLatitude();
	double longitude = getLongitude();
	float carSpeed = getSpeed();

	if(0 == (int)latitude && 0 == (int)longitude && 0 == (int)carSpeed) {
		//db_error("getGpsInfo::GPS positioning is in progress...\n");
		return -1;
	}

	if (latitude < 0) {
		snprintf(Latitudedstr, 8, "S%.2f", -latitude);
    } else {
    	snprintf(Latitudedstr, 8, "N%.2f", latitude);
    }

	if (longitude < 0) {
		snprintf(Longitudedstr, 9, "W%.2f", -longitude);
    } else {
		snprintf(Longitudedstr, 9, "E%.2f", longitude);
    }

	snprintf(CarSpeedstr, 10, "%.2fkm/h", carSpeed);

	snprintf(info, bufferSize, "%s %s  %s", Latitudedstr, Longitudedstr, CarSpeedstr);
	//db_msg("getGpsInfo::info:%s\n", info);
	return 0;
}
