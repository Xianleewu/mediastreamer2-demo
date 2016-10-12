/*   ----------------------------------------------------------------------
Copyright (C) 2014-2016 Fuzhou Rockchip Electronics Co., Ltd

     Sec Class: Rockchip Confidential

V1.0 Dayao Ji <jdy@rock-chips.com>
---------------------------------------------------------------------- */


#include <stdio.h>
#include <errno.h>
/* file ops */
#include <fcntl.h>

/* fork */
#include <unistd.h>
/* ioctl */
#include <sys/ioctl.h>
/* mmap */
#include <sys/mman.h>
/* fb ops */
#include <linux/fb.h>
#include <linux/kd.h>
/* wait */
#include <sys/types.h>
#include <sys/wait.h>

/* ion ops */
#include <linux/ion.h>
#include <ion/ion.h>
#include <linux/rockchip_ion.h>

/* binder ipc */
#include <ProcessState.h>
#include <IPCThreadState.h>

#include <HerbMediaRecorder.h>

#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "HerbMediaRecorderForRemote"
#include <utils/Log.h>
#include "Rtc.h"

#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"




extern void remote_preview_callback(int cam_type);
extern void notify_is_recording(void);
extern void notify_is_rejecting(void);

using namespace android;

#define FILE_PATH_VIDEO "/mnt/external_sd/video/%04d%02d%02d_%02d%02d%02dA_REMOTE.mp4"
char remote_video_path[WRTC_MAX_FILE_NAME_LEN+1] = {0};
time_t vTimestamp = 0;
int remote_type;
pthread_mutex_t s_remote_mutex = PTHREAD_MUTEX_INITIALIZER;
bool is_recording_finish_ok = false;

namespace android {

class InfoListener : public HerbMediaRecorder::OnInfoListener
{
public:
    InfoListener(int id, int *recording) {
		mId = id;
		mCnt = 1;
		mRecording = recording;
	};
    ~InfoListener() {};

	void onInfo(HerbMediaRecorder *pMr, int what, int extra) {
		
		int fd = -1;
		switch(what) {
		case MEDIA_RECORDER_INFO_MAX_DURATION_REACHED:
			pthread_mutex_lock(&s_remote_mutex);
			is_recording_finish_ok = true;
			pthread_mutex_unlock(&s_remote_mutex);
			
			*mRecording = 0;
			ALOGD("remote_network For Remote MEDIA_RECORDER_INFO_MAX_DURATION_REACHED \n");
			
			#if 0
			snprintf (path, 100, FILE_PATH, mId, ++mCnt);
			fd = open(path, O_CREAT | O_RDWR, "wb+");
			if (fd <= 0) {
				ALOGD("open failed : ERROR --> %s\n", strerror(errno));
				pMr->stop();
				pMr->reset();
				pMr->release();
				break;
			}
		    pMr->setOutputFile(fd);
			#endif
			break;
        default:
            ALOGD("remote_network >>>>>>>>>>>>>>>>>>>>> what HerbMediaRecorder::OnInfoListener= %d", what);
            break;
        }
    };
private:
	int mId;
	int mCnt;
    int *mRecording;
};

class ErrorListener : public HerbMediaRecorder::OnErrorListener
{
public:
    ErrorListener(int *recording) { mRecording = recording;};
    ~ErrorListener() {};

    void onError(HerbMediaRecorder *pMr, int what, int extra) {
		REMOTE_NETWORK_UPDATE_STATUS_IND_T status_ind_req;
		
		switch (what) {
			case MEDIA_RECORDER_ERROR_FILE_ERROR:
			case MEDIA_RECORDER_ERROR_UNKNOWN:
				*mRecording = 0;

				if(REMOTE_NETWORK_TYPE_REALTIME == remote_type)
				{
					#if 1
					notify_is_rejecting();
					#else
					memset(&status_ind_req, 0x0, sizeof(status_ind_req));
					status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
					fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
					#endif
				}
			default:
				break;
		}
        ALOGD("remote_network what = %d", what);
    };
private:
    int *mRecording;
};


class RecorderThread : public Thread
{
public:
    RecorderThread(int id, HerbCamera *pCamera) { mId = id; mCnt = 1; pHC = pCamera;};
protected:
    
    virtual bool threadLoop() {
        HerbCamera::Parameters params;
        uint32_t width = /*640*/1280;
        uint32_t height = /*480*/720;
        //pHC = /*HerbCamera::open(mId)*/gHC;
        //pHC->getParameters(&params);
        //params.setPreviewSize(width, height);
        params.setVideoSize(width, height);
        //pHC->setParameters(&params);
        int handle;
        unsigned char *ptr;
		REMOTE_NETWORK_UPDATE_STATUS_IND_T status_ind_req;
		REMOTE_NETWORK_UPLOAD_REQ_T upload_req;
        //allocIon(&handle, &ptr);
        //pHC->setPreviewDisplay(handle);
        //pHC->startPreview();

        pMr = new HerbMediaRecorder();
        pHC->unlock();
        pMr->setCamera(pHC);
        pMr->setAudioSource(HerbMediaRecorder::AudioSource::CAMCORDER);
        pMr->setVideoSource(HerbMediaRecorder::VideoSource::CAMERA);
        pMr->setOutputFormat(HerbMediaRecorder::OutputFormat::OUTPUT_FORMAT_MPEG4_FOR_DRIVING);
        pMr->setAudioEncoder(HerbMediaRecorder::AudioEncoder::AAC);
        pMr->setVideoEncoder(HerbMediaRecorder::VideoEncoder::H264);
        pMr->setVideoSize(/*640*/1280, /*480*/720);
		pMr->setVideoEncodingBitRate(1000000);  /*10000000*/
        pMr->setVideoFrameRate(30);

        
		struct tm *tm=NULL;

		pthread_mutex_lock(&s_remote_mutex);
		memset(remote_video_path, 0x0, sizeof(remote_video_path));
		//getDateTime(&tm);
		
		tm = localtime(&vTimestamp);
        snprintf (remote_video_path, WRTC_MAX_FILE_NAME_LEN, FILE_PATH_VIDEO, tm->tm_year + 1900, tm->tm_mon + 1,tm->tm_mday, tm->tm_hour,tm->tm_min, tm->tm_sec);
        int fd = open(remote_video_path, O_CREAT | O_RDWR, "wb+");
		pthread_mutex_unlock(&s_remote_mutex);
        if (fd <= 0) {
            ALOGD("open failed : ERROR --> %s\n", strerror(errno));
			if(REMOTE_NETWORK_TYPE_REALTIME == remote_type)
			{
				#if 1
				notify_is_rejecting();
				#else
				memset(&status_ind_req, 0x0, sizeof(status_ind_req));
				status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
				#endif
			}
					
            pMr->release();
            //pHC->release();
            return 1;
        }

        ALOGD("setMaxDuration");
        pMr->setMaxDuration(15*1000);
        ALOGD("setOutputFile %d", fd);
        pMr->setOutputFile(fd);
        pMr->setMaxFileSize(-1);
        ALOGD("prepare");
        pMr->prepare();
        mRecording = 1;
        pMr->setOnErrorListener(new ErrorListener(&mRecording));
        pMr->setOnInfoListener(new InfoListener(mId, &mRecording));
        ALOGD("start");
		#if 1
		if(REMOTE_NETWORK_TYPE_REALTIME == remote_type)
		{
			notify_is_recording();
		}
		#else
		memset(&status_ind_req, 0x0, sizeof(status_ind_req));
		status_ind_req.type = REMOTE_STATUS_TYPE_RECORDING;
		fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
		#endif
        pMr->start();
		remote_preview_callback(0);
        while (mRecording) sleep(1);
        ALOGD("remote_network stop");
        pMr->stop();
        ALOGD("remote_network reset");
        pMr->reset();
        ALOGD("remote_network release");
        pMr->release();
        ALOGD("remote_network release");
        //pHC->release();
        close(fd);

		pthread_mutex_lock(&s_remote_mutex);
		memset(&upload_req, 0, sizeof(upload_req));
		if(is_recording_finish_ok)
		{
			strncpy(upload_req.file_name, remote_video_path, WRTC_MAX_FILE_NAME_LEN);
			upload_req.timestamp = vTimestamp;
			upload_req.type = (REMOTE_NETWORK_TYPE)remote_type;
			upload_req.data_type = REMOTE_NETWORK_DATA_TYPE_VIDEO;
		}
		pthread_mutex_unlock(&s_remote_mutex);
		if(strlen(upload_req.file_name) > 0)
		{
			fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_UPLOAD_REQ, &upload_req, sizeof(upload_req));
		}
		else
		{
			if(REMOTE_NETWORK_TYPE_REALTIME == remote_type)
			{
				#if 1
				notify_is_rejecting();
				#else
				memset(&status_ind_req, 0x0, sizeof(status_ind_req));
				status_ind_req.type = REMOTE_STATUS_TYPE_REJECTED;
				fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_Reserverd1, MSG_ID_FWK_CONTROL_3G_REMOTE_STATUS_IND, &status_ind_req, sizeof(status_ind_req));
				#endif
			}
		}
		
        return 0;
    };
private:
    int mRecording;
    int mId;
	int mCnt;
    HerbCamera *pHC;
    HerbMediaRecorder *pMr;
};


};



int record_for_remote(HerbCamera* pCamera, time_t timestamp, int type)
{
	if(pCamera != NULL)
	{
		pthread_mutex_lock(&s_remote_mutex);
		vTimestamp = timestamp;
		remote_type = type;
		is_recording_finish_ok = false;
		pthread_mutex_unlock(&s_remote_mutex);

		sp<RecorderThread> tr0 = new RecorderThread(0,pCamera);
		tr0->run("ISPRecordingThread");
	}
	return 0;
}
