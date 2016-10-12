#include "CdrMediaRecorder.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "CdrMediaRecorder"
#endif
#include "debug.h"

struct speedModeInfo gModeInfo[2][4] = {
    {
        { FAST,   CAM_CSI_BITRATE, FRONT_CAM_FRAMERATE},
        { MIDDLE, CAM_CSI_BITRATE * 9 / 10, FRONT_CAM_FRAMERATE * 9 / 10 },
        { SLOW,   CAM_CSI_BITRATE * 8 / 10, FRONT_CAM_FRAMERATE * 8 / 10 },
        { FREEZE, CAM_CSI_BITRATE * 3 / 10, FRONT_CAM_FRAMERATE * 8 / 10 },
    },
    {
        { FAST,   CAM_UVC_BITRATE, BACK_CAM_FRAMERATE },
        { MIDDLE, CAM_UVC_BITRATE * 9 / 10, BACK_CAM_FRAMERATE * 9 / 10 },
        { SLOW,   CAM_UVC_BITRATE * 8 / 10, BACK_CAM_FRAMERATE * 8 / 10 },
        { FREEZE, CAM_UVC_BITRATE * 3 / 10, BACK_CAM_FRAMERATE * 8 / 10 },
    }
};



CdrMediaRecorder::CdrMediaRecorder(int cam_id)
	: mHMR(NULL),
	  mListener(NULL),
	  mErrListener(NULL),
	  mCamId(-1),
	  mCurrentState(RECORDER_NOT_INITIAL),
	  mStartImpactFile(false)
{
	mHMR = new HerbMediaRecorder();
	mHMR->setOnInfoListener(this);
	mHMR->setOnDataListener(this);
	mHMR->setOnErrorListener(this);
	mCamId = cam_id;
	mSilent = false;
	mDuration = 60;
	//memset(&record_param, 0, sizeof(record_param));
	
	mCurrentState = RECORDER_IDLE;
}

void CdrMediaRecorder::setModeInfo(struct speedModeInfo *modeInfo) {
    db_debug("setModeInfo mode %d, bitrate = %d, framerate = %d\n",
            modeInfo->mode,
            modeInfo->bitRate,
            modeInfo->frameRate);
    if (mHMR != NULL) {
		char params[64];
    	sprintf(params, "video-param-encoder-bitrate=%d", modeInfo->bitRate);
        mHMR->setParameterEx(String8(params));

		sprintf(params, "video-param-encoder-framerate=%d", modeInfo->frameRate);
        mHMR->setParameterEx(String8(params));
    }
}

void CdrMediaRecorder::onInfo(HerbMediaRecorder *pMR, int what, int extra)
{
	if(mListener) {
		mListener->recordListener(this, what, extra);
	}

    if (what == MEDIA_RECORDER_FILE_WRITE_SPEED_MODE) {
        db_debug("MEDIA_RECORDER_FILE_WRITE_SPEED_MODE mode %d\n", extra);
        setModeInfo(&gModeInfo[mCamId][extra]);
        //if (extra == FREEZE) {
        //    stop();
        //}
    }
}

void CdrMediaRecorder::onError(HerbMediaRecorder *pMR, int what, int extra)
{
	if(mErrListener) {
		mErrListener->errorListener(this, what, extra);
	}
}

void CdrMediaRecorder::onData(HerbMediaRecorder *pMR, int what, int extra)
{
	if(mListener) {
		mListener->recordListener(this, what, extra);
	}
}
void CdrMediaRecorder::setRecordParam(CdrRecordParam_t &param)
{
	mParam = param;
}
void CdrMediaRecorder::getRecordParam(CdrRecordParam_t &param)
{
	param = mParam;
}
CdrMediaRecorder::~CdrMediaRecorder()
{
	delete mHMR;
	mHMR = NULL;
}
void CdrMediaRecorder::setOnInfoCallback(RecordListener* listener)
{
	mListener = listener;	
}

void CdrMediaRecorder::setOnErrorCallback(ErrorListener* listener)
{
	mErrListener = listener;	
}

status_t CdrMediaRecorder::initRecorder(HerbCamera *mHC, Size &videoSize, uint32_t frameRate, int fd, uint32_t fileSize)
{
	status_t ret = 0;
	mHc = mHC;
	mHc->unlock();
	ret = mHMR->setCamera(mHc);
    if (ret != NO_ERROR) {
    	db_msg("setCamera Failed(%d)\n", ret);
   		return ret;
    }
	if(CAM_CSI == mCamId) {
		ret = mHMR->setAudioSource(HerbMediaRecorder::AudioSource::CAMCORDER);
		if (ret != NO_ERROR) {
			return ret;
		}
		mRecordAudio = true;
	} else {
		mRecordAudio = false;
	}
	mMaxDurationReachedTimes = 0;

	ret = mHMR->setVideoSource(HerbMediaRecorder::VideoSource::CAMERA);
	if (ret != NO_ERROR) {
    	db_error("setVideoSource Failed(%d)", ret);
   		return ret;
    }

	ret = mHMR->setOutputFormat(HerbMediaRecorder::OutputFormat::OUTPUT_FORMAT_MPEG4_FOR_DRIVING);
	if (ret != NO_ERROR) {
    	db_error("setOutputFormat Failed(%d)", ret);
   		return ret;
    }

	ret = mHMR->setAudioEncoder(HerbMediaRecorder::AudioEncoder::AAC);
	if (ret != NO_ERROR) {
		return ret;
	}
	
	ret = mHMR->setVideoEncoder(HerbMediaRecorder::VideoEncoder::H264);
	if (ret != NO_ERROR) {
    	db_error("setVideoEncoder Failed(%d)", ret);
   		return ret;
    }

	ret = mHMR->setVideoSize(videoSize.width, videoSize.height);
	if (ret != NO_ERROR) {
    	db_error("setVideoSize Failed(%d)", ret);
   		return ret;
    }

	ret = mHMR->setVideoFrameRate(frameRate);
	if (ret != NO_ERROR) {
    	db_error("setVideoFrameRate Failed(%d)", ret);
   		return ret;
    }

	setDuration();
#ifdef IMPLEMENTED_setImpactFileDuration
	ret = mHMR->setImpactFileDuration(BFTIMEMS, AFTIMEMS);
	if (ret != NO_ERROR) {
		db_error("setImpactFileDuration Failed(%d)", ret);
		return ret;
	}
#endif

	setFile(fd, 0);
	ret = mHMR->prepare();
	if (ret != NO_ERROR) {
    	db_error("prepare Failed(%d)", ret);
   		return ret;
    }

	mCurrentState = RECORDER_PREPARED;
	
	return NO_ERROR;
}

status_t CdrMediaRecorder::start(void)
{
	status_t ret;
	mCurrentState = RECORDER_RECORDING;
	setSilent(mSilent);
	ret = mHMR->start();

#ifdef IMPLEMENTED_setVideoEncodingBitRate
    int bitrate;
    if(CAM_CSI == mCamId)
        bitrate = CAM_CSI_BITRATE;
    else
        bitrate = CAM_UVC_BITRATE;
    ret = mHMR->setVideoEncodingBitRate(bitrate);
    if (ret != NO_ERROR) {
        db_error("setVideoEncodingBitRate Failed(%d)", ret);
        return ret;
    }
#endif
	return ret;
}

int CdrMediaRecorder::stop(void)
{
	mCurrentState = RECORDER_IDLE;
	return mHMR->stop();
}

void CdrMediaRecorder::release()
{
	mHMR->release();
	mCurrentState = RECORDER_NOT_INITIAL;
}

void CdrMediaRecorder::setDuration()
{
	mHMR->setMaxDuration(mDuration);
}

void CdrMediaRecorder::setDuration(int ms)
{
	Mutex::Autolock _l(mLock);
	mDuration = ms;
	setDuration();
}

void CdrMediaRecorder::setNextFile(int fd, unsigned int filesize)
{
	mHMR->setOutputFileSync(fd, filesize);
	close(fd);
}

void CdrMediaRecorder::setFile(int fd, unsigned int filesize)
{
	mHMR->setOutputFile(fd, filesize);
	close(fd);
}

void CdrMediaRecorder::setSilent(bool mode)
{
	db_debug("setMuteMode %d", mode);
	Mutex::Autolock _l(mLock);
	mSilent = mode;
	if (mHMR && (mCurrentState&RECORDER_RECORDING)) {
		db_debug("silent %d", mode);
#ifdef IMPLEMENTED_setMuteMode
        if(mode)
		    mHMR->setMuteMode(ENABLED_MODE);
		else
			mHMR->setMuteMode(DISABLED_MODE);
#endif
	}
}

bool CdrMediaRecorder::getVoiceStatus()
{
	Mutex::Autolock _l(mLock);
	return mSilent;
}

int CdrMediaRecorder::getCameraID(void)
{
	return mCamId;
}

CdrRecorderState_t CdrMediaRecorder::getRecorderState(void)
{
	return mCurrentState;
}

#if 0
int CdrMediaRecorder::startExtraFile(int fd)
{
	mHMR->setExtraFileFd(fd);
	mHMR->startExtraFile();
	close(fd);
	return 0;
}

int CdrMediaRecorder::stopExtraFile()
{
	mHMR->stopExtraFile();
	return 0;
}
#endif

int CdrMediaRecorder::startImpactVideo(int fd, unsigned long size)
{
	Mutex::Autolock _l(mLock);
	//mHMR->setImpactOutputFile(fd, size);
        mHMR->triggerImpact(fd);
	close(fd);
	mStartImpactFile = true;
	db_debug("start impact video\n");

	return 0;
}

void CdrMediaRecorder::stopVideoFile() {
    ALOGD("%s E",__func__);
	Mutex::Autolock _l(mLock);
	mStartImpactFile = false;

	mHMR->CompleteLastStep();
	
}
void CdrMediaRecorder::stopImpactVideo()
{
        ALOGD("%s E",__func__);
	Mutex::Autolock _l(mLock);
	mStartImpactFile = false;
}

bool CdrMediaRecorder::getImpactStatus()
{
	return mStartImpactFile;
}

void CdrMediaRecorder::setSdcardState(bool bExist)
{
	//mHMR->setSdcardState(bExist);
}

