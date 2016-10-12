#ifndef _CDRCAMERA_H
#define _CDRCAMERA_H

#include <signal.h>
#include <HerbCamera.h>
#ifdef DISPLAY_READY
#include "CdrDisplay.h"
#endif

#include "camera.h"
#include "misc.h"
using namespace android;

class CdrCamera  
#if defined(HAVE_takePicture) || defined(MOTION_DETECTION_ENABLE) || defined(DISPLAY_READY)
    :
#endif
#if HAVE_takePicture
    public HerbCamera::PictureCallback,
#endif
#ifdef MOTION_DETECTION_ENABLE
    public HerbCamera::AWMoveDetectionListener,
#endif
#ifdef DISPLAY_READY
	public HerbCamera::PreviewCallback
#endif
{
public:
	CdrCamera(int cam_type, CDR_RECT &rect, int node);
	~CdrCamera();
	void requestSurface(void);
	int initCamera(unsigned int width, unsigned int height);
	void setPreviewRect(CDR_RECT &rect, bool bBottom);
	void startPreview(void);
	HerbCamera* getCamera(void);
	void stopPreview(void);
	void release();
#ifdef DISPLAY_READY
	CdrDisplay* getDisp(void);
	void setPreviewCallback(void(*callback)(camera_preview_frame_buffer_t * data, void* caller, int id));
	void onPreviewFrame(camera_preview_frame_buffer_t *data, HerbCamera* pCamera);
	void releasePreviewFrame(int index);
#endif
	void setJpegCallback(void(*callback)(void* data, int size, void* caller, int id));
	void onPictureTaken(void *data, int size, HerbCamera* pCamera);
	void takePicture(void);
	void setImageQuality(ImageQuality_t quality);
#ifdef MOTION_DETECTION_ENABLE
	void setAwmdCallBack(void (*callback)(int value, void *caller));
	void onAWMoveDetection(int value, HerbCamera *pCamera);
#endif
	void setWhiteBalance(const char *mode);
	void setContrast(int mode);
	void setExposure(int mode);
#ifdef MOTION_DETECTION_ENABLE
	void setAWMD(bool val);
#endif
#ifdef WATERMARK_ENABLE
    void setWaterMark(bool val);
	void updatePreviewWaterMarkInfo(char *info);
#endif
	bool isPreviewing();
#ifdef MOTION_DETECTION_ENABLE
	void setAwmdTimerCallBack(void (*callback)(sigval_t val));
	void feedAwmdTimer(int seconds);
	int queryAwmdExpirationTime(time_t* sec, long* nsec);
#endif
	void setCaller(void* caller);
	int getInputSource();
	int getTVinSystem();
#ifdef WATERMARK_ENABLE
	bool needWaterMark();
#endif
private:
#ifdef MOTION_DETECTION_ENABLE
	void startAWMD();
	void stopAWMD();
#endif
#ifdef WATERMARK_ENABLE
	void enableWaterMark();
	void disableWaterMark();
#endif
	HerbCamera		*mHC;
#ifdef DISPLAY_READY
	CdrDisplay		*mCD;
	void (*mPreviewCallback)(camera_preview_frame_buffer_t * data, void* caller, int id);
#endif
	int mId;
	int mHlay;
	CDR_RECT mRect;
	void (*mJpegCallback)(void* data, int size, void* caller, int id);
#ifdef MOTION_DETECTION_ENABLE
	void (*mAwmdCallback)(int value, void *caller);
	void (*mAwmdTimerCallback)(sigval_t val);
	timer_t mAwmdTimerID;
	bool mNeedAWMD;
	bool mAWMDing;
#endif
#ifdef WATERMARK_ENABLE
	bool mNeedWaterMark;
	bool mWaterMarking;
#endif
	Mutex mLock;
	bool mPreviewing;
	void *mCaller;
};
#endif
