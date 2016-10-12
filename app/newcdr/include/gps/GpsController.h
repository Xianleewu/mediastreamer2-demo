#ifndef _RK_GPS_CONTROLLER_H
#define _RK_GPS_CONTROLLER_H

#include "hardware/hardware.h"
#include "hardware/gps.h"
#include "hardware_legacy/power.h"
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <cutils/log.h>
#include <stdint.h>

void locationCallback(GpsLocation *location);
void statusCallback(GpsStatus *status);
void svStatusCallback(GpsSvStatus *svStatus);
void nmeaCallback(GpsUtcTime timestamp, const char *nmea, int length);
void setCapabilitiesCallback(uint32_t capabilities);
void acquireWakelockCallback();
void releaseWakelockCallback();
void requestUtcTimeCallback();
pthread_t createThreadCallback(const char *name, void* (*start)(void *), void *arg);
void xtraDownloadRequestCallback();

void gpsNiNotifyCallback(GpsNiNotification *notification);
void agpsStatusCallback(AGpsStatus *agpsStatus);
void agpsRequestSetId(uint32_t flags);
void agpsRequestRefLocation(uint32_t flags);

void configurationUpdate(const char *data, int length);
void measurementCallback(GpsData* data);
bool isMeasurementSupported();
bool startMeasurementCollection();
bool stopMeasurementCollection();
bool gpsIsSupportsXtra();

//////////////////////////////////
//								    //
//								    //
//		gps public interface		    //
//								    //
//								    //
//////////////////////////////////
bool enableGps();
bool disableGps();
bool gpsIsSupported();
GpsStatusValue getGpsStatus();
double getLatitude();
double getLongitude();
double getAltitude();
float getSpeed();
int readGpsNmea(char *str, int bufferSize);
GpsUtcTime getUtcTime();
int getSvsNum();
int getSvsId(int index);
float getSvsElevation(int index);
float getSvsAzimuth(int index) ;
float getSignal2NoiseRatio(int index);
int getGpsInfo(char *info, int bufferSize);
double getValidLatitude(void);
double getValidLongtitude(void);

void gpsUpdateNetworkState(int connected, int type, int roaming, const char* extra_info);


#endif //_RK_GPS_CONTROLLER_H
