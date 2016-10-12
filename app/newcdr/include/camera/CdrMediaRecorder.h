#ifndef _CDR_MEDIA_RECORDER_H
#define _CDR_MEDIA_RECORDER_H

#include <HerbMediaRecorder.h>
#include "misc.h"
#include "camera.h"
#define BFTIMEMS (1000*5)       /* the record time before impact, ms */
#define AFTIMEMS (1000*10)      /* the record time after impact */

enum speedMode {
    FAST,
    MIDDLE,
    SLOW,
    FREEZE
};

struct speedModeInfo {
    enum speedMode mode;
    int bitRate;
    int frameRate;
};

typedef struct {
	fileType_t video_type;
	uint32_t fileSize;
	int duration;		/* unit: s*/
}CdrRecordParam_t;
using namespace android;
class CdrMediaRecorder;
class RecordListener
{
public:
	RecordListener(){}
	virtual ~RecordListener(){}
	virtual void recordListener(CdrMediaRecorder* cmr, int what, int extra) = 0;
};

class ErrorListener
{
public:
	ErrorListener(){}
	virtual ~ErrorListener(){}
	virtual void errorListener(CdrMediaRecorder* cmr, int what, int extra) = 0;
};

typedef enum{
	DISABLED_MODE = 0,
	ENABLED_MODE  = 1
}MuteMode_t;

typedef enum{
	RECORDER_ERROR			= 0,
	RECORDER_NOT_INITIAL	= 1 << 0,
	RECORDER_IDLE			= 1 << 1,
	RECORDER_PREPARED		= 1 << 2,
	RECORDER_RECORDING		= 1 << 3
}CdrRecorderState_t;

class CdrMediaRecorder : public HerbMediaRecorder::OnInfoListener
						,public HerbMediaRecorder::OnErrorListener
						,public HerbMediaRecorder::OnDataListener
{
public:
	CdrMediaRecorder(int cameraId);
	~CdrMediaRecorder();
	void setRecordParam(CdrRecordParam_t &param);
	void getRecordParam(CdrRecordParam_t &param);
    void setModeInfo(struct speedModeInfo *modeInfo);
	void onInfo(HerbMediaRecorder *pMr, int what, int extra);
	void onError(HerbMediaRecorder *pMr, int what, int extra);
	void onData(HerbMediaRecorder *pMr, int what, int extra);
	status_t initRecorder(HerbCamera *mHC, Size &videoSize, uint32_t frameRate, int fd, uint32_t fileSize);
	status_t start(void);
	void setOnInfoCallback(RecordListener* listener);
	void setOnErrorCallback(ErrorListener* listener);
	int stop(void);
	void release();
	void setDuration(int ms);
	void setNextFile(int fd, unsigned int filesize);
	void setFile(int fd, unsigned int filesize);
	void setSilent(bool mode);
	void setRecordTime();
	int getCameraID(void);
	CdrRecorderState_t getRecorderState(void);
//	int startExtraFile(int fd);
//	int stopExtraFile();
	void setSdcardState(bool bExist);
	bool getVoiceStatus();
	int startImpactVideo(int fd, unsigned long size);
	void stopVideoFile();
	void stopImpactVideo();
	bool getImpactStatus();
	bool isRecordAudio() {return mRecordAudio;};
	int mMaxDurationReachedTimes;
	int mMaxFileDoneReachedTimes;

private:
	void setDuration();
	HerbMediaRecorder *mHMR;
	HerbCamera *mHc;
	CdrRecordParam_t mParam;
	RecordListener* mListener;
	ErrorListener* mErrListener;
	int mCamId;
	int mDuration;
	Mutex mLock;
	bool mSilent;
	CdrRecorderState_t mCurrentState;
	bool mStartImpactFile;
	bool mRecordAudio;
};

#endif
