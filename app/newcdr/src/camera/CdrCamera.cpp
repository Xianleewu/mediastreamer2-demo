#include <linux/videodev2.h>
#include "CdrCamera.h"
#include "posixTimer.h"
#include <properties.h>
#undef LOG_TAG
#define LOG_TAG "CdrCamera.cpp"
#include "debug.h"

CdrCamera::CdrCamera(int cam_type, CDR_RECT &rect, int node)
	: mHC(NULL),
#ifdef DISPLAY_READY
	mCD(NULL),
	mPreviewCallback(NULL),
#endif
	mId(cam_type),
	mRect(rect),
	mJpegCallback(NULL)
#ifdef MOTION_DETECTION_ENABLE
	,mAwmdCallback(NULL),
	mAwmdTimerCallback(NULL),
	mAwmdTimerID(0)
#endif
{
	db_error("open camera%d\n",  node);
	mHC = HerbCamera::open(node);
	if(mHC == NULL) {
		db_error("fail to open HerbCamera");
		return;
	}
#ifdef DISPLAY_READY
	mCD = new CdrDisplay(cam_type);
	if(mCD == NULL) {
		db_error("fail to new CedarDisplay");
		return;
	}
	mCD->setRect(mRect);
	mCD->setZorder(cam_type);
#endif

#ifdef MOTION_DETECTION_ENABLE
	mNeedAWMD = false;
	mAWMDing = false;
#endif
#ifdef WATERMARK_ENABLE
	mNeedWaterMark = true;
	mWaterMarking = false;
#endif
	mPreviewing = false;
}

CdrCamera::~CdrCamera()
{
	db_msg("CdrCamera Destructor\n");
#ifdef DISPLAY_READY
	delete mCD;
	mCD = NULL;
#endif
	release();
	delete mHC;
	mHC = NULL;
}

HerbCamera* CdrCamera::getCamera(void)
{
	return mHC;
}

void CdrCamera::setPreviewRect(CDR_RECT &rect, bool bBottom)
{
        mRect = rect;
	mCD->setRect(mRect);
	mCD->setZorder(bBottom ? 0 : 1); 
}

void CdrCamera::startPreview(void)
{
	Mutex::Autolock _l(mLock);

	db_msg("startPreview %d\n", mId);
	if(mPreviewing == true) {
		db_msg("preview is started\n");
		return;
	}

	mCD->open();
	mHC->setPreviewCallback(this);
	mHC->startPreview();
	mPreviewing = true;

#ifdef MOTION_DETECTION_ENABLE
	if (mNeedAWMD) {
		startAWMD();
	}
#endif
#ifdef WATERMARK_ENABLE
	if (mNeedWaterMark) {
		enableWaterMark();
	}
#endif
}

void CdrCamera::stopPreview(void)
{
	Mutex::Autolock _l(mLock);

	db_msg("stopPreview %d\n", mId);
	if(mPreviewing == false) {
		db_msg("preview is already stoped\n");
		return;
	}
#ifdef DISPLAY_READY	
	mCD->close();
#endif
	mHC->stopPreview();
	mPreviewing = false;
	db_msg("stopPreview\n");

#ifdef MOTION_DETECTION_ENABLE
	stopAWMD();
#endif
#ifdef WATERMARK_ENABLE
       disableWaterMark();
#endif
}

#ifdef WATERMARK_ENABLE
void CdrCamera::enableWaterMark()
{
	/*if (mId != CAM_CSI) {
		return ;
	}
	
	if (mHC && (mWaterMarking == false)) {
		mHC->enableWaterMark();
		mHC->setWaterMarkMultiple("32,64,0");
		mWaterMarking = true;
	}*/
	if (mHC && !(mWaterMarking)) {
		HerbCamera::Parameters params;
		mHC->getParameters(&params);
		params.set("timewater", "true");
		mHC->setParameters(&params);	
		mWaterMarking = true;
	}
}

void CdrCamera::disableWaterMark()
{
	/*if (mId != CAM_CSI) {
		return ;
	}
	if (mHC && (mWaterMarking)) {
		mHC->disableWaterMark();
		mWaterMarking = false;
	}*/

	if (mHC && mWaterMarking) {
		HerbCamera::Parameters params;
		mHC->getParameters(&params);
		params.set("timewater", "false");
		mHC->setParameters(&params);
		mWaterMarking = false;
	}
	
}

void CdrCamera::updatePreviewWaterMarkInfo(char *info)
{
	Mutex::Autolock _l(mLock);
	if (mHC && mWaterMarking) {
		HerbCamera::Parameters params;
		mHC->getParameters(&params);
		params.set("uvc-watermark", info);
		mHC->setParameters(&params);
	}
}

#endif

void CdrCamera::setWhiteBalance(const char *mode)
{
	db_error("setWhiteBalance %s", mode);
	HerbCamera::Parameters params;

	mHC->getParameters(&params);
	//params.dump();

	params.setWhiteBalance(mode);
	mHC->setParameters(&params);
}

void CdrCamera::setContrast(int mode)
{
	db_error("setContrast");
	HerbCamera::Parameters params;

	mHC->getParameters(&params);
	//params.dump();

	params.setContrastValue(mode);
	mHC->setParameters(&params);
}

void CdrCamera::setExposure(int mode)
{
	db_error("setExposure");
	HerbCamera::Parameters params;

	mHC->getParameters(&params);
	//params.dump();

	params.setExposureCompensation(mode);
	mHC->setParameters(&params);
}

#ifdef DISPLAY_READY
CdrDisplay* CdrCamera::getDisp(void)
{
	return mCD;
}

void CdrCamera::onPreviewFrame(camera_preview_frame_buffer_t * data, HerbCamera* pCamera)
{
//	db_msg("onPreviewFrame camId %d index %d\n", mId, data->index);
	if(mPreviewCallback) {
		(*mPreviewCallback)(data, mCaller, mId);
	}
	else {
		releasePreviewFrame(data->index);
	}
//	db_msg("onPreviewFrame camId %d index %d done\n", mId, data->index);
}

void CdrCamera::setPreviewCallback(void(*callback)(camera_preview_frame_buffer_t * data, void* caller, int id))
{
	Mutex::Autolock _l(mLock);

	mPreviewCallback = callback;

	if(mPreviewing) {
		mHC->lock();
		mHC->setPreviewCallback(this);
	}
}

void CdrCamera::releasePreviewFrame(int index) {
//	db_msg("releasePreviewFrame camId %d index %d\n", mId, index);
	if(mHC) {
		mHC->releasePreviewFrame(index);
	}
//	db_msg("releasePreviewFrame camId %d index %d done\n", mId, index);
}
#endif

void CdrCamera::onPictureTaken(void *data, int size, HerbCamera* pCamera)
{
	if(mJpegCallback) {
		(*mJpegCallback)(data, size, mCaller, mId);
	}
}

void CdrCamera::setJpegCallback(void(*callback)(void* data, int size, void* caller, int id))
{
	mJpegCallback = callback;
}

#ifdef HAVE_takePicture
void CdrCamera::takePicture(void)
{
	if(mHC) {
		mHC->takePicture(NULL, NULL, NULL, this);
	}
}
#endif

void CdrCamera::setImageQuality(ImageQuality_t quality)
{
	HerbCamera::Parameters params;
	unsigned int w = 0;
	unsigned int h = 0;
	if(mHC == NULL) {
		db_error("mHC is NULL\n");
		return;
	}
	mHC->getParameters(&params);
	//params.dump();

	switch(quality) {
	case ImageQualityQQVGA:
		{
			w = 160;
			h = 120;
		}
		break;
	case ImageQualityQVGA:
		{
			w = 320;
			h = 240;
		}
		break;
	case ImageQualityVGA:
		{
			w = 640;
			h = 480;
		}
		break;
	case ImageQuality720P:
		{
			w = 1280;
			h = 720;
		}
		break;
	case ImageQuality1080P:
		{
			w = 1920;
			h = 1080;
		}
		break;
	case ImageQuality5M:
		{
			w = 2560;
			h = 1920;
		}
		break;
	case ImageQuality8M:
		{
			w = 3264;
			h = 2448;
		}
		break;
	default:
		{
			db_msg("not support image_quality:%d\n", quality);
			return ;
		}
		break;
	}
	params.setPictureSize(w, h);
	mHC->setParameters(&params);
}

int CdrCamera::initCamera(unsigned int width, unsigned int height)
{
	HerbCamera::Parameters params;
	v4l2_queryctrl ctrl;

	mHC->getParameters(&params);
	//get max supported preview size of usb camera
	if(CAM_UVC == mId) {
		Vector<Size> sizes;
		int max_width = 0;
		int max_height = 0;
		params.getSupportedPreviewSizes(sizes);
		for(int i = 0; i < sizes.size(); i++) {
			if (sizes[i].width > max_width) {
				max_width = sizes[i].width;
				max_height = sizes[i].height;
			}
		}
		width = max_width;
		height = max_height;
	}
	//params.dump();
	db_msg("init Camera setpreview size %d x %d \n", width, height);
	params.setPreviewSize(width, height);
	params.setVideoSize(width, height);
	if(mId == CAM_CSI) {
#ifdef IMPLEMENTED_V4L2_CTRL
		mHC->getContrastCtrl(&ctrl);
		params.setContrastValue((ctrl.minimum + ctrl.maximum) / 2);

		mHC->getBrightnessCtrl(&ctrl);
		params.setBrightnessValue((ctrl.minimum + ctrl.maximum) / 2);

		mHC->getSaturationCtrl(&ctrl);
		params.setSaturationValue((ctrl.minimum + ctrl.maximum) / 2);
		mHC->getHueCtrl(&ctrl);
		params.setHueValue((ctrl.minimum + ctrl.maximum) / 2);
#endif
	} else {
		params.setPreviewFrameRate(BACK_CAM_FRAMERATE);
	}
	mHC->setParameters(&params);
#ifdef DISPLAY_READY
	mCD->setBottom();
#endif

	return 0;
}

#ifdef MOTION_DETECTION_ENABLE
void CdrCamera::startAWMD()
{
	if(!mAwmdTimerID) {
		if(mAwmdTimerCallback == NULL) {
			db_error("%s %d, awmd timer call back not set\n", __FUNCTION__, __LINE__);
			return;
		}
		if(createTimer(mCaller, &mAwmdTimerID, mAwmdTimerCallback) < 0) {
			db_error("%s %d, create timer failed\n", __FUNCTION__, __LINE__);
			return;	
		}
	}

	db_msg("%s %d\n", __FUNCTION__, __LINE__);
	if (mHC && (mAWMDing == false)) {
		db_msg("%s %d\n", __FUNCTION__, __LINE__);
		mHC->setAWMoveDetectionListener(this);
		mHC->startAWMoveDetection();
		mHC->setAWMDSensitivityLevel(1);
		mAWMDing = true;
	}
}

void CdrCamera::stopAWMD()
{
	db_msg("%s %d\n", __FUNCTION__, __LINE__);
	if (mHC && (mAWMDing)) {
		db_msg("%s %d\n", __FUNCTION__, __LINE__);
		mHC->stopAWMoveDetection();
		if(mAwmdTimerID) {
			stopTimer(mAwmdTimerID);
			deleteTimer(mAwmdTimerID);
			mAwmdTimerID = 0;
		}
		mAWMDing = false;
	}
}

void CdrCamera::setAWMD(bool val)
{
	Mutex::Autolock _l(mLock);
	if (mId != CAM_CSI) {
		return ;
	}
	mNeedAWMD = val;
	db_msg("%s %d, val is %d\n", __FUNCTION__, __LINE__, val);
	if (mHC && (mPreviewing == true)) {
		db_msg("%s %d\n", __FUNCTION__, __LINE__);
		if (val) {
			startAWMD();
		} else {
			stopAWMD();
		}
	}
}
#endif

#ifdef WATERMARK_ENABLE
void CdrCamera::setWaterMark(bool val)
{
	Mutex::Autolock _l(mLock);

	mNeedWaterMark = val;

	if (val) {
	    enableWaterMark();
	} else {
	    disableWaterMark();
	}
}
#endif

void CdrCamera::release()
{
	mHC->release();
}

#ifdef MOTION_DETECTION_ENABLE
void CdrCamera::setAwmdCallBack(void (*callback)(int value, void *caller))
{
	mAwmdCallback = callback;
}

void CdrCamera::onAWMoveDetection(int value, HerbCamera *pCamera)
{
	//	db_msg("%s %d\n", __FUNCTION__, __LINE__);
	if(mAwmdCallback) {
		(*mAwmdCallback)(value, mCaller);
	}
}
#endif

bool CdrCamera::isPreviewing(void)
{
	Mutex::Autolock _l(mLock);
	return mPreviewing;
}

#ifdef MOTION_DETECTION_ENABLE
void CdrCamera::setAwmdTimerCallBack(void (*callback)(sigval_t val))
{
	mAwmdTimerCallback = callback;
}

void CdrCamera::feedAwmdTimer(int seconds)
{
	if(mAwmdTimerID)	
		setOneShotTimer(seconds, 0, mAwmdTimerID);
}

int CdrCamera::queryAwmdExpirationTime(time_t* sec, long* nsec)
{
	if(mAwmdTimerID) {
		if(getExpirationTime(sec, nsec, mAwmdTimerID) < 0) {
			db_error("%s %d, get awmd timer expiration time failed\n", __FUNCTION__, __LINE__);
			return -1;
		}
	} else
		return -1;

	return 0;
}
#endif

void CdrCamera::setCaller(void* caller)
{
	mCaller = caller;
}

int CdrCamera::getInputSource()
{
	int val;
	mHC->getInputSource(&val);
	return val;
}
int CdrCamera::getTVinSystem()
{
	int val;
	mHC->getTVinSystem(&val);
	return val;
}

#ifdef WATERMARK_ENABLE
bool CdrCamera::needWaterMark()
{
	return mNeedWaterMark;
}
#endif
