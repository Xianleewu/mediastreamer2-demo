#include <linux/videodev2.h>
#include <properties.h>
#include "RecordPreview.h"
#include "gps/GpsController.h"
#undef LOG_TAG
#define LOG_TAG "RecordPreview"
#include "debug.h"
#ifdef EVENTMANAGER_READY
#include "EventManager.h"
#endif
#include "StorageManager.h"
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <HerbMediaMetadataRetriever.h>

#ifdef MESSAGE_FRAMEWORK
#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"
#endif

#define GPS_SPEED_WATERMARK_LENGTH      8

static const char *WHITE_BALANCE[] = {
	"auto",
	"daylight",
	"cloudy-daylight",
	"incandescent",
	"fluorescent"
};

static int oldCarSpeed = 0;

#ifdef USE_NEWUI
#define IDC_RECORD_TIMER_ICON 250
#define IDC_RECORD_TIMER 251
#define RECORD_TIMER 150
#endif

#ifdef REMOTE_3G
long report_alarm_time = 0;
#define REPORT_DURATION_SECONDS		(10*60)  //10*60
#endif


extern String8 handle_remote_message_sync(unsigned char *pbuffer, int size);


int RecordPreview::keyProc(int keyCode, int isLongPress)
{
	StorageManager* sm;

	switch(keyCode) {
	case CDR_KEY_OK:
		if(isLongPress == SHORT_PRESS) {
			if(mRPWindowState == STATUS_RECORDPREVIEW || mRPWindowState == STATUS_AWMD) {
				if (isRecording()) {
					db_debug("===============key_ok to stopRecord\n");
				#ifdef MESSAGE_FRAMEWORK
					CommuReq req;

					db_msg("fwk_msg send stop record\n");
					memset(&req, 0x0, sizeof(req));
					req.cmdType = COMMU_TYPE_RECORD;
					req.operType = 0;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
				#else
					stopRecord();
				#endif
					add2Db(CAM_CSI);
					add2Db(CAM_UVC);
				} else {
					db_debug("===============key_ok to startRecord\n");
				#ifdef MESSAGE_FRAMEWORK
					CommuReq req;

					db_msg("fwk_msg send start record\n");
					memset(&req, 0x0, sizeof(req));
					req.cmdType = COMMU_TYPE_RECORD;
					req.operType = 1;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
				#else
					startRecord();
				#endif
				}
			} else if(mRPWindowState == STATUS_PHOTOGRAPH) {
			#ifdef MESSAGE_FRAMEWORK
				fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_TAKE_PICTURE_REQ, NULL, 0);
			#else
				takePicture();
			#endif
			}
		} else if(isLongPress == LONG_PRESS) {
			if(mRPWindowState == STATUS_RECORDPREVIEW && !isRecording()) {
				sm = StorageManager::getInstance();
				if (sm->isMount() == false) {
					SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_NO_TFCARD, 0);
					return WINDOWID_RECORDPREVIEW;
				}
#if 0
				db_debug("photograph\n");
				//oldPIPMode = mPIPMode;
				//transformPIP(CSI_ONLY);
				mRPWindowState = STATUS_PHOTOGRAPH;
				mCdrMain->setCurrentWindowState(mRPWindowState);
#else
				db_debug("go to PlayBackPreview\n");
				mRPWindowState = STATUS_RECORDPREVIEW;
				mCdrMain->setCurrentWindowState(mRPWindowState);					
				mUvcPlugFlag = false;
				if(mRPWindowState != STATUS_AWMD )
					mOldRPWindowState = mRPWindowState;
				else
					mOldRPWindowState = STATUS_RECORDPREVIEW;
				return WINDOWID_PLAYBACKPREVIEW;
#endif
			} else if(mRPWindowState == STATUS_PHOTOGRAPH) {
				db_debug("go to PlayBackPreview\n");
				mRPWindowState = STATUS_RECORDPREVIEW;
				mCdrMain->setCurrentWindowState(mRPWindowState);					
				mUvcPlugFlag = false;
				if(mRPWindowState != STATUS_AWMD )
					mOldRPWindowState = mRPWindowState;
				else
					mOldRPWindowState =STATUS_RECORDPREVIEW;
				return WINDOWID_PLAYBACKPREVIEW;
			}
		}
		break;
	case CDR_KEY_MODE:
		if(isLongPress == SHORT_PRESS) {
			if( !isRecording() ) {
				mUvcPlugFlag = false;
				if(mRPWindowState != STATUS_AWMD )
					mOldRPWindowState = mRPWindowState;
				else
					mOldRPWindowState = STATUS_RECORDPREVIEW;
				return WINDOWID_MENU;
			}
		} else if(isLongPress == LONG_PRESS) {
			#ifdef MESSAGE_FRAMEWORK
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_TAKE_PICTURE_REQ, NULL, 0);
			#else
			takePicture();
			#endif
		}
		break;
	case CDR_KEY_LEFT:
		if(isLongPress == SHORT_PRESS) {
			if(mPIPMode == UVC_ON_CSI)
				transformPIP(CSI_ON_UVC);
			else if(mPIPMode == CSI_ON_UVC)
				transformPIP(UVC_ONLY);
			else if(mPIPMode == UVC_ONLY)
				transformPIP(CSI_ONLY);
			else if(mPIPMode == CSI_ONLY)
				transformPIP(UVC_ON_CSI);
		} else if(isLongPress == LONG_PRESS) {
#ifdef MOTION_DETECTION_ENABLE
			switchAWMD();
#endif
		}
		break;
	case CDR_KEY_RIGHT:
		if(isLongPress == SHORT_PRESS) {
			if(mRPWindowState == STATUS_RECORDPREVIEW)
			{
			#ifdef MESSAGE_FRAMEWORK
				int result;
				result = getVoiceStatus();
				if(-1 == result)
				{
					//do nothing, No CSI
				}
				else
				{
					if(result)
					{
						CommuReq req;

						db_msg("fwk_msg send voice on--> off\n");
						memset(&req, 0x0, sizeof(req));
						req.cmdType = COMMU_TYPE_VOICE;
						req.operType = 0;
						fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
					}
					else
					{
						CommuReq req;

						db_msg("fwk_msg send voice off --> on\n");
						memset(&req, 0x0, sizeof(req));
						req.cmdType = COMMU_TYPE_VOICE;
						req.operType = 1;
						fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
					}
				}
			#else
				switchVoice();
			#endif
			}
		} else {
			if(isRecording())
				lockFile();
		}
		break;
	}
	return WINDOWID_RECORDPREVIEW;
}

void RecordPreview::lockFile()
{
	HWND hStatusBar;
	db_msg("Lockflag: %d", mLockFlag);
	mLockFlag = !mLockFlag;
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	SendMessage(hStatusBar, STBM_LOCK, mLockFlag, 0);
}

int RecordPreview::getVoiceStatus(void)
{
	bool voiceStatus = true;

	
	if (mRecorder[CAM_CSI]) {
		db_msg("getVoiceStatus CSI");
		voiceStatus = mRecorder[CAM_CSI]->getVoiceStatus();
	}
	else
	{
		db_msg("getVoiceStatus Non-CSI, return -1");
		return -1;
	}

	db_msg("getVoiceStatus voiceStatus=%d \n", voiceStatus);
	
	return voiceStatus;
}


void RecordPreview::switchVoice()
{
	bool voiceStatus;
	HWND hStatusBar;
	db_msg("xxxxxxxxxxxxxxxx");
	if (mRecorder[CAM_CSI]) {
		voiceStatus = mRecorder[CAM_CSI]->getVoiceStatus();
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		SendMessage(hStatusBar, STBM_VOICE, !voiceStatus, 0);
	}
}

int RecordPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	RecordPreview* recordPreview = (RecordPreview*)GetWindowAdditionalData(hWnd);
	//db_msg("message code: [%d], %s, wParam=%d, lParam=%lu\n", message, Message2Str(message), wParam, lParam);
	switch(message) {
	case MSG_CREATE:
		#ifdef USE_NEWUI
		recordPreview->createWidgets(hWnd);
		SetWindowElementAttr(GetDlgItem(hWnd, WINDOWID_RECORDPREVIEW+IDC_RECORD_TIMER), 
			WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, 0xFFFFFFFF));
		SetWindowText(GetDlgItem(hWnd, WINDOWID_RECORDPREVIEW+IDC_RECORD_TIMER), "00:00:00");
		#endif
		break;
	case MSG_CDR_KEY:
		db_debug("key code is %d\n", wParam);
		return recordPreview->keyProc(wParam, lParam);
	case MWM_CHANGE_FROM_WINDOW:
		recordPreview->restorePreviewOutSide(wParam);
		//usleep(200 * 1000);		/* 让 layer开始渲染后才允许绘制窗口 */
		db_msg("xxxxxxxx\n");
		break;
	case MSG_SHOWWINDOW:
		db_msg("MSG_SHOWWINDOW\n");
		if(wParam == SW_SHOWNORMAL) {
			db_msg("MSG_SHOWWINDOW\n");
#ifdef MOTION_DETECTION_ENABLE
			recordPreview->updateAWMDWindowPic();
#endif
		}
		break;
	case MSG_SHOW_TIP_LABEL:
		db_msg("MSG_SHOW_TIP_LABEL,wParam = %d",wParam);
		ShowTipLabel(hWnd, (enum labelIndex)wParam);
		break;
#ifdef USE_NEWUI
	case RPM_START_RECORD_TIMER:
		recordPreview->startRecordTimer(wParam);
		break;
	case RPM_STOP_RECORD_TIMER:
		recordPreview->stopRecordTimer();
		break;
	case MSG_TIMER:
		if(wParam == RECORD_TIMER) {
			recordPreview->increaseRecordTime();
			recordPreview->updateRecordTime(hWnd);
		}
		break;
	case MSG_DESTROY:
		recordPreview->destroyWidgets();
		break;
#endif
	default:
		recordPreview->msgCallback(message, (int)wParam);
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

void RecordPreview::takePicFinish(int cam_id)
{
	Mutex::Autolock _l(mLockPic);
	mAllowTakePic[cam_id] = true;
	isThumb = false;
}


void RecordPreview::add2Db(int cameraID)
{
	StorageManager* sm = StorageManager::getInstance();
	String8 newName("");
	char buf[128] = {0};
	db_msg("one file done %s", mRecordDBInfo[cameraID].fileString.string());
	if (mRecordDBInfo[cameraID].fileString.isEmpty()) {
		return;
	}
	if(!FILE_EXSIST(mRecordDBInfo[cameraID].fileString.string())){
		db_msg("%s not exists,do not add to db",mRecordDBInfo[cameraID].fileString.string());
		return;
	}
        if(sm->fileSize(mRecordDBInfo[cameraID].fileString) < (256 * 1024)) { //file is too small
		sm->deleteFile(mRecordDBInfo[cameraID].fileString.string());
		db_msg("file %s size is 0kb", mRecordDBInfo[cameraID].fileString.string());
	} else {
		Elem elem;
		elem.file = (char*)mRecordDBInfo[cameraID].fileString.string();
		if (mLockFlag) {
			elem.info = (char*)INFO_LOCK;
			newName   = sm->lockFile(elem.file);
			elem.file = (char*)newName.string();
		} else {
			elem.info = (char*)mRecordDBInfo[cameraID].infoString.string();
		}
		elem.type = (char*)TYPE_VIDEO;
		getDir(elem.file,buf);
		getBaseName(buf,buf);
		elem.dir = buf;
		elem.time = mRecordDBInfo[cameraID].time;
		sm->dbAddFile(&elem);
	}

}
static void jpegCallback(void *data, int size, void* caller, int id)
{
	RecordPreview* rp;
#ifdef MESSAGE_FRAMEWORK
	WRTC_TAKE_PICTURE_NOTIFY rsp;
#endif
	
	db_debug("picture call back %d data %p", id, data);
	if(caller == NULL) {
		db_error("caller not initialized\n");
		return;
	}
	rp = (RecordPreview*)caller;
	rp->savePicture(data, size, id);
	rp->takePicFinish(id);

#ifdef MESSAGE_FRAMEWORK
	memset(&rsp, 0, sizeof(rsp));
	rsp.size = strlen(rp->getSnapShotLocation());
	if(rsp.size > 0)
	{
		rsp.result = 0;
	}
	else
	{
		rsp.result = -1;
	}
	strncpy((char*)rsp.path, rp->getSnapShotLocation(), WRTC_MAX_FILE_NAME_LEN);
	db_debug("file_name=%s rsp.path=%s size=%d \n", rp->getSnapShotLocation(), rsp.path, rsp.size);
	fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_VIEW, MSG_ID_FWK_CONTROL_TAKE_PICTURE_RSP, &rsp, sizeof(rsp));
#endif
}

static void previewCallback(camera_preview_frame_buffer_t *data, void* caller, int id)
{
	RecordPreview* rp;

	if(caller == NULL) {
		db_error("caller not initialized\n");
		return;
	}
	rp = (RecordPreview*)caller;

	rp->showFrame(data, id);
}

#ifdef MOTION_DETECTION_ENABLE
static void awmdTimerCallback(sigval_t val)
{
	db_msg("posix timer data: %p\n", val.sival_ptr);
	RecordPreview *rp = (RecordPreview *)val.sival_ptr;
	rp->stopRecord(CAM_CSI);
}

static void awmdCallback(int value, void *pCaller)
{
	RecordPreview *rp = (RecordPreview *)pCaller;
	CdrRecordParam_t param;
#ifdef REMOTE_3G
	REMOTE_NETWORK_ALARM_T alarm_ind;
	long current_time = 0;
	long time_diff = 0;
#endif
	
	if(!IsWindowVisible(rp->getHwnd())) {
		return;
	}

#ifdef REMOTE_3G
		current_time = time(NULL);
		if(current_time > report_alarm_time)
		{
			time_diff = current_time - report_alarm_time;
		}
		else
		{
			time_diff = report_alarm_time - current_time;
		}
		db_msg("remote_network alarm current_time=%ld report_alarm_time=%ld time_diff=%ld REPORT_DURATION_SECONDS=%d\n", current_time, report_alarm_time, time_diff, REPORT_DURATION_SECONDS);
	
		if(time_diff > REPORT_DURATION_SECONDS)
		{
			db_msg("remote_network send MD alarm \n");
			report_alarm_time = current_time;
			memset(&alarm_ind, 0, sizeof(alarm_ind));
			alarm_ind.type = REMOTE_NETWORK_ALARM_TYPE_MD;
			fwk_send_message_ext(FWK_MOD_CONTROL,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_3G_REMOTE_ALARM_IND, &alarm_ind, sizeof(alarm_ind));
		}
#endif


	if(rp->needDelayStartAWMD() == true) {
		db_msg("need to delay awmd\n");
		rp->mAwmdDelayCount++;
		/* skip 100 times , after stop record */
		if(rp->mAwmdDelayCount >= 100) {
			db_msg("clear awmd delay flag\n");
			rp->mAwmdDelayCount = 0;
			rp->setDelayStartAWMD(false);
		}
		return;
	}

	db_msg("awmd : value- %d\n", value);
	
	if(value == 1) {
		if(rp->isRecording(CAM_CSI) == false) {
			param.fileSize = AWMD_PREALLOC_SIZE;
			param.video_type = VIDEO_TYPE_NORMAL;
			param.duration = 60 * 1000;
			rp->startRecord(CAM_CSI, param);
			rp->feedAwmdTimer(5);
		} else if(rp->isRecording(CAM_CSI) == true) {
			time_t sec;
			long nsec;
			if(rp->queryAwmdExpirationTime(&sec, &nsec) < 0) {
				db_error("query awmd timer expiration time failed\n");
				return;
			}
			if( (sec == 4 && nsec <= 500 * 1000 * 1000) || sec < 4)
				rp->feedAwmdTimer(5);
		}
	}
}
#endif

void RecordPreview::initCamera(int cam_type)
{
 	CDR_RECT rect;
	unsigned int width = 640, height = 360;
	int node = 0;
	unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;
#ifdef USE_NEWUI
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
	if(rm) {
		videoCurrent = rm->menuDataVM.current;
	}
#endif
	
	if(mCamExist[cam_type] == false)
		return;

	if (cam_type == CAM_CSI) {
		rect = rectBig;
		node = CAM_CSI;
	}
	if (cam_type == CAM_UVC) {
		rect = rectSmall;
		if(videoCurrent == VIDEO_MODE_BACK_CAMERA) {
			rect = rectBig;
		}
		mUVCNode = CAM_UVC;
		node = mUVCNode;
		if(mHasTVD) {
			node = CAM_TVD;
			width = 720;
			height = 480;
		}
	}
	mCamera[cam_type] = new CdrCamera(cam_type, rect, node);
	mCamera[cam_type]->initCamera(width, height);
	mCamera[cam_type]->setCaller(this);
	mCamera[cam_type]->setJpegCallback(jpegCallback);
	mCamera[cam_type]->setPreviewCallback(previewCallback);
	mCamera[cam_type]->getDisp()->setDisplayCallback(this);
#ifdef MOTION_DETECTION_ENABLE
	if (cam_type == CAM_CSI) {
		mCamera[cam_type]->setAwmdCallBack(awmdCallback);
		mCamera[cam_type]->setAwmdTimerCallBack(awmdTimerCallback);
	}
#endif
}

void RecordPreview::startPreview(int cam_type)
{
	if(mCamExist[cam_type] == false)
		return;

#if 0
       bool needWaterMark = true;
       if (mCamera[CAM_CSI]) {
               needWaterMark = mCamera[CAM_CSI]->needWaterMark();
       }
#endif
	if(mCamera[cam_type]) {
#ifdef WATERMARK_ENABLE
		ResourceManager* rm = ResourceManager::getInstance();
		if(rm) {
			mNeedWaterMark = rm->menuDataTWMEnable;
			mCamera[cam_type]->setWaterMark(mNeedWaterMark);
		}
#endif
		mCamera[cam_type]->startPreview();
	}
}

void RecordPreview::startPreview()
{
	Mutex::Autolock _l(mLock);
	status_t err;
	if (mCamExist[CAM_CSI]) {
		startPreview(CAM_CSI);
	}
	if (mCamExist[CAM_UVC]) {
		startPreview(CAM_UVC);
	}
#ifdef WATERMARK_ENABLE
	if(mUpdatePreviewWMT == NULL) {
		mUpdatePreviewWMT = new UpdatePreviewWaterMarkThread(this);
		err = mUpdatePreviewWMT->start();
		if (err != OK) {
			mUpdatePreviewWMT.clear();
		}
	}
#endif
}

void RecordPreview::setDeleteFlag(int cam_type, String8 file)
{
	Mutex::Autolock _l(mDeleteLock);
	db_msg("setDeleteFlag %s!", file.string());
	mNeedDelete[cam_type] = file;
}

void RecordPreview::clrDeleteFlag(int cam_type, String8 file)
{
	Mutex::Autolock _l(mDeleteLock);
	if (file == mNeedDelete[cam_type]) {
		db_msg("clrDeleteFlag!");
		mNeedDelete[cam_type] = "";
	} else {
		db_msg("should not be cleared %s", file.string());
	}
}

String8 RecordPreview::getDeleteFlag(int cam_type)
{
	Mutex::Autolock _l(mDeleteLock);
	return mNeedDelete[cam_type];
}

bool RecordPreview::isPreviewing(int cam_type)
{
	if(mCamera[cam_type])
		return mCamera[cam_type]->isPreviewing();

	return false;
}
bool RecordPreview::isPreviewing()
{
	if(isPreviewing(CAM_CSI) == true && isPreviewing(CAM_UVC))
		return true;
	return false;
}

void RecordPreview::errorListener(CdrMediaRecorder* cmr, int what, int extra)
{
	db_msg("Message what -> %X\n", what);
	switch (what) {
		case MEDIA_RECORDER_ERROR_FILE_ERROR:
		case MEDIA_RECORDER_ERROR_UNKNOWN:
			#ifdef MESSAGE_FRAMEWORK
			CommuReq req;

			db_msg("fwk_msg send stop record\n");
			memset(&req, 0x0, sizeof(req));
			req.cmdType = COMMU_TYPE_RECORD;
			req.operType = 0;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
			#else
			stopRecord();
			#endif
		default:
			break;
	}
}

void RecordPreview::recordListener(CdrMediaRecorder* cmr, int what, int extra)
{
#ifdef IMPLEMENTED_recordListener
	int cameraID, fd;
        char buf[128] = {0};
	CdrRecordParam_t recordParam;
	Elem elem;
	HerbCamera *pHc = NULL;
	StorageManager* sm = StorageManager::getInstance();
	cameraID = cmr->getCameraID();
	pHc = mCamera[cameraID]->getCamera();
	cmr->getRecordParam(recordParam);
	db_debug("cameraID is %d, what %d, extra is %d\n", cameraID, what, extra);
	String8 needDeleteFile;
	switch(what) {
	case MEDIA_RECORDER_INFO_MAX_DURATION_REACHED:
		db_debug("MEDIA_RECORDER_INFO_MAX_DURATION_REACHED\n");

		if((cmr->isRecordAudio()) && (++cmr->mMaxDurationReachedTimes < 2))
			break;
		cmr->mMaxDurationReachedTimes = 0;

		//Add current file to DataBase;
		do {
			db_debug("add to database \n");
			/*if(cmr->getImpactStatus() == true) {
				db_msg("mImpactFlag[%d] is %d\n", cameraID, mImpactFlag[cameraID]);
				if(mImpactFlag[cameraID] <= 0) {
					db_msg("impact file done, file is %s\n", mExtraDBInfo[cameraID].fileString.string());
					if(sm->fileSize(mExtraDBInfo[cameraID].fileString) < (256 * 1024)) {	//file is too small
						sm->deleteFile(mExtraDBInfo[cameraID].fileString.string());
						db_msg("file %s size is 0kb", mExtraDBInfo[cameraID].fileString.string());
					} else {
						elem.info = (char*)INFO_LOCK;
						elem.file = (char*)mExtraDBInfo[cameraID].fileString.string();
						elem.type = (char*)TYPE_VIDEO;
						elem.time = mExtraDBInfo[cameraID].time;
						getDir(elem.file,buf);
						getBaseName(buf,buf);
						elem.dir = buf;
						sm->dbAddFile(&elem);
					}
					cmr->stopImpactVideo();
					mRecordDBInfo[cameraID].fileString = mBakFileString[cameraID];
					mRecordDBInfo[cameraID].time = mBakNow[cameraID];
					mExtraDBInfo[cameraID].fileString.empty();
					break;
				}
				mImpactFlag[cameraID]--;
			}*/

			String8 newName("");
			db_msg("one file done %s", mRecordDBInfo[cameraID].fileString.string());
			if (mRecordDBInfo[cameraID].fileString.isEmpty()) {
				db_error("filename is empty. it's an accident ");
				break;
			}
			if(sm->fileSize(mRecordDBInfo[cameraID].fileString) < (256 * 1024)) { //file is too small
				sm->deleteFile(mRecordDBInfo[cameraID].fileString.string());
				db_msg("file %s size is 0kb", mRecordDBInfo[cameraID].fileString.string());
				break;
			} else {
				elem.file = (char*)mRecordDBInfo[cameraID].fileString.string();
				if (mLockFlag) {
					elem.info = (char*)INFO_LOCK;
					newName   = sm->lockFile(elem.file);
					elem.file = (char*)newName.string();
				} else {
					elem.info = (char*)mRecordDBInfo[cameraID].infoString.string();
				}
				elem.type = (char*)TYPE_VIDEO;
				getDir(elem.file,buf);
				getBaseName(buf,buf);
				elem.dir = buf;
				elem.time = mRecordDBInfo[cameraID].time;
				sm->dbAddFile(&elem);
			}
			clrDeleteFlag(cameraID, mRecordDBInfo[cameraID].fileString);
		} while (0);
		//generate next file
		{
			needDeleteFile = getDeleteFlag(cameraID);
			if (!needDeleteFile.isEmpty()) {
				if(sm->fileSize(needDeleteFile) == 0) {
					db_error("MEDIA_RECORDER_INFO_MAX_DURATION_REACHED %s is empty, will be deleted\n",needDeleteFile.string());
					sm->deleteFile(needDeleteFile.string());
					clrDeleteFlag(cameraID, needDeleteFile );
				}
			}
			db_msg("Please set the next fd.");
			fd = sm->generateFile(mBakNow[cameraID], mBakFileString[cameraID], cameraID, recordParam.video_type);
			isThumb = true;
			#ifdef MESSAGE_FRAMEWORK
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_TAKE_THUMB_REQ, NULL, 0);
			#else
			takePicture();
			#endif
			if(fd <= 0) {
				db_error("get fd failed\n");
				clrDeleteFlag(cameraID, mRecordDBInfo[cameraID].fileString);
				#ifdef MESSAGE_FRAMEWORK
				{
					CommuReq req;

					db_msg("fwk_msg send stop record\n");
					memset(&req, 0x0, sizeof(req));
					req.cmdType = COMMU_TYPE_RECORD;
					req.operType = 0;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
				}
				#else
				stopRecord();
				#endif
				if (fd == RET_IO_NO_RECORD) {
					ShowTipLabel(mHwnd, LABEL_TFCARD_FULL);
				}
			}
			setDeleteFlag(cameraID, mBakFileString[cameraID]);
			cmr->setNextFile(fd, 0);
		}
		mRecordDBInfo[cameraID].fileString = mBakFileString[cameraID];
		mRecordDBInfo[cameraID].time = mBakNow[cameraID];
		mRecordDBInfo[cameraID].infoString = INFO_NORMAL;
		break;
	case MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED:
		{
			db_debug("MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED\n");
		}
		break;
	case MEDIA_RECORDER_INFO_FILE_DONE:
	{
		if((cmr->isRecordAudio()) && (++cmr->mMaxFileDoneReachedTimes < 2))
			break;
		cmr->mMaxFileDoneReachedTimes = 0;

		cmr->stopVideoFile();
                //if(cmr->getImpactStatus() == true) {
				db_msg("mImpactFlag[%d] is %d\n", cameraID, mImpactFlag[cameraID]);
				//if(mImpactFlag[cameraID] <= 0) {
					db_msg("impact file done, file is %s\n", mExtraDBInfo[cameraID].fileString.string());
					if(!FILE_EXSIST(mExtraDBInfo[cameraID].fileString.string())) {	//file not exist
						db_msg("file %s not exists!!", mExtraDBInfo[cameraID].fileString.string());
					} else if(sm->fileSize(mExtraDBInfo[cameraID].fileString) < (256 * 1024)) {	//file is too small
						sm->deleteFile(mExtraDBInfo[cameraID].fileString.string());
						db_msg("file %s size is 0kb", mExtraDBInfo[cameraID].fileString.string());
					} else {
						elem.info = (char*)INFO_LOCK;
						elem.file = (char*)mExtraDBInfo[cameraID].fileString.string();
						elem.type = (char*)TYPE_VIDEO;
						elem.time = mExtraDBInfo[cameraID].time;
						getDir(elem.file,buf);
						getBaseName(buf,buf);
						elem.dir = buf;
						sm->dbAddFile(&elem);
					}
					clrDeleteFlag(cameraID, mExtraDBInfo[cameraID].fileString);
					cmr->stopImpactVideo();
					//break;
				//}
				//mImpactFlag[cameraID]--;
			//}


		db_error("associated video done (%s)",mExtraDBInfo[cameraID].fileString.string());
	}
	break;
	case MEDIA_RECORDER_FILE_WRITE_SPEED_MODE:
	{
		db_debug("MEDIA_RECORDER_FILE_WRITE_SPEED_MODE extra %d", extra);
		if (pHc == NULL) break;
		HerbCamera::Parameters preview_params;
		pHc->getParameters(&preview_params);
		preview_params.setFpsPercent((100 - (20 * extra)));
		pHc->setParameters(&preview_params);
	}
	break;
}

#endif
}

#ifdef USE_NEWUI
void RecordPreview::increaseRecordTime()
{
	HWND hStatusBar;
	time_t timer;
	struct tm *t_tm;

	time(&timer);
	t_tm = localtime(&timer);
	mRecordLastTime = mRecordNowTime;
	mRecordNowTime = t_tm->tm_sec;
	if (mRecordNowTime - mRecordLastTime > 0)
		mRecordTime++;
	if(mRecordTime >= mCurRecordVTL / 1000) {
		mRecordTime = 0;
		if(mCurRecordVTL == AFTIMEMS) {
			mCurRecordVTL = mRecordVTL;
			db_msg("reset the mCurRecordVTL to %ld ms\n", mCurRecordVTL);
			hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
			SendMessage(hStatusBar, STBM_LOCK, 0, 0);
		}
	}
}

void RecordPreview::updateRecordTime(HWND hWnd)
{
	if (mCdrMain->isScreenOn()) { //screen on
		char timeStr[30] = {0};
		
		sprintf(timeStr, "%02d:%02d:%02d", mRecordTime/3600, mRecordTime/60, mRecordTime%60);
		SetWindowText(GetDlgItem(hWnd, WINDOWID_RECORDPREVIEW+IDC_RECORD_TIMER), timeStr);
		UpdateWindow(GetDlgItem(hWnd, WINDOWID_RECORDPREVIEW+IDC_RECORD_TIMER), false);
	}
}

int RecordPreview::startRecordTimer(unsigned int recordType)
{
	HWND hStatusBar;
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	time_t timer;
	struct tm *t_tm;

	db_msg("recordType is %d, mRecordVTL is %ld ms, mCurRecordVTL is %ld ms\n", recordType, mRecordVTL, mCurRecordVTL);
	if(mRecordVTL == 0) {
		db_msg("start record timer failed, record vtl not initialized\n");
		return -1;
	}
	if(IsTimerInstalled(mHwnd, RECORD_TIMER) == TRUE) {
		if (ResetTimer(mHwnd, RECORD_TIMER, 20) == TRUE)
			db_msg("ResetTimer\n");	
	} else {
		if (SetTimer(mHwnd, RECORD_TIMER, 20) == TRUE)
			db_msg("SetTimer\n");
	}

	mRecordTime = 0;
	time(&timer);
	t_tm = localtime(&timer);
	mRecordNowTime = mRecordLastTime = t_tm->tm_sec;
	updateRecordTime(mHwnd);
	if(recordType == 1) {
		/* if recordType is impact record, set the mCurRecordVTL with AFTIMEMS
		 * when time expired, compare the mCurRecordVTL with mRecordVTL, 
		 * if not equal, then reset mCurRecordVTL with mRecordVTL
		 * */
		mCurRecordVTL = AFTIMEMS;
		SendMessage(hStatusBar, STBM_LOCK, 1, 0);
	}
	return 0;
}

int RecordPreview::stopRecordTimer()
{
	HWND hStatusBar;
	db_msg("enter\n");
	
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	KillTimer(mHwnd, RECORD_TIMER);
	mRecordTime = 0;
	mRecordNowTime = 1;
	mRecordLastTime = 0;
	updateRecordTime(mHwnd);
	SendMessage(hStatusBar, STBM_LOCK, 0, 0);
	return 0;
}

int RecordPreview::createWidgets(HWND hParent)
{
	HWND hWnd;
	ResourceManager *rm;
	RECT rect;

	rm = ResourceManager::getInstance();
	rm->getResBmp(ID_RECORD_PREVIEW1, BMPTYPE_BASE, mDotPic);
	
	GetWindowRect(hParent, &rect);

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | SS_REALSIZEIMAGE,
			WS_EX_TRANSPARENT,
			WINDOWID_RECORDPREVIEW + IDC_RECORD_TIMER_ICON,
			0, RECTH(rect)-58, 40, 58,
			hParent, (DWORD)&mDotPic);
	if(hWnd == HWND_INVALID) {
		db_error("create record Preview timer icon window failed\n");
		return -1;
	}

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE | SS_VCENTER,
			WS_EX_TRANSPARENT,
			WINDOWID_RECORDPREVIEW + IDC_RECORD_TIMER,
			40, RECTH(rect)-58, RECTW(rect)-40, 58,
			hParent, 0);
	if(hWnd == HWND_INVALID) {
		db_error("create record Preview timer window failed\n");
		return -1;
	}

	mLogFont = CreateLogFont("*", "fixed", "GB2312-0", 
			FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
			FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 32, 0);
	SetWindowFont(hWnd, mLogFont);
	
	return 0;
}

int RecordPreview::destroyWidgets()
{
	DestroyLogFont(mLogFont);
	
	if (mDotPic.bmBits != NULL)
		UnloadBitmap(&mDotPic);
	return 0;
}
#endif

int RecordPreview::createWindow(void)
{
	RECT rect;
	CDR_RECT STBRect;
	ResourceManager* rm;
	HWND hParent;
	HWND hStatusBar;

	rm = ResourceManager::getInstance();
	rm->getResRect(ID_STATUSBAR, STBRect);
	hParent = mCdrMain->getHwnd();
	GetWindowRect(hParent, &rect);

	rect.top += STBRect.h;

	mHwnd = CreateWindowEx(WINDOW_RECORDPREVIEW, "",
			WS_CHILD | WS_VISIBLE,
#ifdef USE_NEWUI
			WS_EX_TRANSPARENT,
#else
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
#endif
			WINDOWID_RECORDPREVIEW,
			rect.left, rect.top, RECTW(rect), RECTH(rect),
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create record Preview window failed\n");
		return -1;
	}
	rm->setHwnd(WINDOWID_RECORDPREVIEW, mHwnd);

#if	0
	createWatermarkWidgets();
	gal_pixel color = 0xFFDCDCDC;
	SetWindowElementAttr(GetDlgItem(mHwnd, ID_STATUSBAR_LABEL1), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, color));
	SetWindowElementAttr(GetDlgItem(mHwnd, ID_PREVIEW_WATERMARK_GPSINFO), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, color));
#endif

	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	if(mCamExist[CAM_UVC] == true)
		SendNotifyMessage(hStatusBar, STBM_UVC, 1, 0);
	else
		SendNotifyMessage(hStatusBar, STBM_UVC, 0, 0);

	return 0;
}

bool checkDev(int cam_type, bool &hasTVD, int &uvcnode)
{

	char dev[32];
	int i = 0;
	struct v4l2_capability cap; 
	int ret;

	if (cam_type == CAM_CSI) {
		sprintf(dev, "/dev/video%d", cam_type);
		return !access(dev, F_OK);
	}

	for (i = 1; i < 64; ++i) {
		sprintf(dev, "/dev/video%d", i);
		if (!access(dev, F_OK)) {
			int fd = open(dev, O_RDWR | O_NONBLOCK, 0);
			if (fd < 0) {
				continue;
			}
			ret = ioctl (fd, VIDIOC_QUERYCAP, &cap);
			if (ret < 0) {
				close(fd);
				continue;
			}
			if (strcmp((char*)cap.driver, "uvcvideo") == 0) {
				db_debug("uvcvideo device node is %s\n", dev);
				hasTVD = false;	//Tvd could not exist at all.
				close(fd);
				uvcnode = i;
				return true;
			} else if (strcmp((char*)cap.driver, "tvd") == 0) {
				db_debug("TVD device node is %s\n", dev);
				hasTVD = true;
#ifdef EVENTMANAGER_READY
#ifdef EVENT_TVD
				if (detectPlugIn(fd) == 0) {
					db_msg("tvd:plug in\n");
					close(fd);
					return true;
				}
#endif
#endif
				//else {
					//close(fd);
					//return false;
				//}
				
				//close(fd);	//UVC may be existed.
				//return false;
			}
			close(fd);
			fd = -1;
		}
	}
	db_error("Could not find device for device ID %d!\n", cam_type);
	return false;
}

RecordPreview::RecordPreview(CdrMain *cdrMain)
	: mCdrMain(cdrMain),
	mRPWindowState(STATUS_RECORDPREVIEW),
	mOldRPWindowState(STATUS_RECORDPREVIEW), 
	mUvcPlugFlag(false),
	SdcardState(true),
	isThumb(false),
	pictuQuality(ImageQuality1080P),
	thumbQuality(ImageQualityQQVGA),
#ifdef WATERMARK_ENABLE
	mNeedWaterMark(false),
#endif
	mUVCNode(1)
#ifdef MOTION_DETECTION_ENABLE
    ,
	mAwmdDelayStart(false),
	mIsPreview(true)
#endif
{
	ResourceManager* rm = ResourceManager::getInstance();
	mVideoSize.width  = 1920;
	mVideoSize.height = 1080;
	mRecorder[CAM_CSI] = NULL;
	mRecorder[CAM_UVC] = NULL;
	mCamera[CAM_CSI] = NULL;
	mCamera[CAM_UVC] = NULL;
	mUpdatePreviewWMT = NULL;
	mDuration = 60*1000;	

	if (rm->getResRect(ID_RECORD_PREVIEW1, rectBig)) {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 480;
		rectBig.h = 272;
	}
	if (rm->getResRect(ID_RECORD_PREVIEW2, rectSmall)) {
		rectSmall.x = 300;
		rectSmall.y = 22;
		rectSmall.w = 180;
		rectSmall.h = 102;
	}
	
	mHasTVD = false;
	
	mCamExist[CAM_CSI] = checkDev(CAM_CSI, mHasTVD, mUVCNode);
	mCamExist[CAM_UVC] = checkDev(CAM_UVC, mHasTVD, mUVCNode);
	int camNum = HerbCamera::getNumberOfCameras();
	if(camNum > 1) {
		property_set("persist.sys.usbcamera.status","added");
		mCamExist[CAM_UVC] = true;
		db_msg("camera hal report %d camera", camNum);
	}else {
		db_msg("camera hal report only %d camera", camNum);
		mCamExist[CAM_UVC] = false;
	}
	
	mNeedDelete[CAM_CSI]="";
	mNeedDelete[CAM_UVC]="";
	mLockFlag = false;
	mSilent = true;
	if(mCamExist[CAM_CSI] == false && mCamExist[CAM_UVC] == false)
		mPIPMode = NO_PREVIEW;
	else if(mCamExist[CAM_CSI] == true && mCamExist[CAM_UVC] == true)
		mPIPMode = UVC_ON_CSI;
	else if(mCamExist[CAM_CSI] == true)
		mPIPMode = CSI_ONLY;
	else
		mPIPMode = UVC_ONLY;
	mAllowTakePic[CAM_CSI] = true;
	mAllowTakePic[CAM_UVC] = true;
	db_msg("mPIPMode is %d\n", mPIPMode);

	mRemoteBuffer = new RemoteBuffer();
        mRemoteBuffer->setRemoteControl(this);
	defaultServiceManager()->addService(String16(REMOTE_BUFFER_SERVICE), mRemoteBuffer);
}

RecordPreview::~RecordPreview()
{

}

bool RecordPreview::hasTVD(bool &hasTVD)
{
	hasTVD = mHasTVD;
	db_msg("hasTVD:%d\n", hasTVD);
	return (mHasTVD && mCamExist[CAM_UVC]);
}

void RecordPreview::init(void)
{
#ifdef DISPLAY_READY
	CdrDisplay* cd;	
#endif

	unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;
#ifdef USE_NEWUI
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
	if(rm) {
		videoCurrent = rm->menuDataVM.current;
	}
#endif

	if(mCamExist[CAM_CSI] == true) {
		if(VIDEO_MODE_BACK_CAMERA != videoCurrent) {
			initCamera(CAM_CSI);
			startPreview(CAM_CSI);
			mRecorder[CAM_CSI] = new CdrMediaRecorder(CAM_CSI);
			mRecorder[CAM_CSI]->setOnInfoCallback(this);
			mRecorder[CAM_CSI]->setOnErrorCallback(this);
		}
	}
	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	if(mCamExist[CAM_UVC] == true && VIDEO_MODE_FONT_CAMERA != videoCurrent) {
		db_msg("Test Time: start uvc , time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

		initCamera(CAM_UVC);
		mRecorder[CAM_UVC] = new CdrMediaRecorder(CAM_UVC);
		mRecorder[CAM_UVC]->setOnInfoCallback(this);
		mRecorder[CAM_UVC]->setOnErrorCallback(this);
#ifdef DISPLAY_READY
		if(mCamExist[CAM_CSI] && mCamera[CAM_CSI]) {
			cd = mCamera[CAM_CSI]->getDisp();
			cd->setBottom();
		}
#endif
	}

	if(!mCamExist[CAM_CSI] && !mCamExist[CAM_UVC]) {
		mPIPMode = NO_PREVIEW;
	} else if(mCamExist[CAM_CSI] && !mCamExist[CAM_UVC]) {
		if(VIDEO_MODE_BACK_CAMERA != videoCurrent) {
			mPIPMode = CSI_ONLY;
		}

		if(VIDEO_MODE_BACK_CAMERA == videoCurrent) {
			mPIPMode = NO_PREVIEW;
		}
	} else if(!mCamExist[CAM_CSI] && mCamExist[CAM_UVC]) {
		if(VIDEO_MODE_FONT_CAMERA != videoCurrent) {
			mPIPMode = UVC_ONLY;
		}

		if(VIDEO_MODE_FONT_CAMERA == videoCurrent) {
			mPIPMode = NO_PREVIEW;
		}
	} else if(mCamExist[CAM_CSI] && mCamExist[CAM_UVC]) {
		if(VIDEO_MODE_FONT_CAMERA == videoCurrent) {
			mPIPMode = CSI_ONLY;
		}

		if(VIDEO_MODE_BACK_CAMERA == videoCurrent) {
			mPIPMode = UVC_ONLY;
		}

		if(VIDEO_MODE_DOUBLE_CAMERA == videoCurrent) {
			mPIPMode = UVC_ON_CSI;
		}
	}
}

static void display_setareapercent(CDR_RECT *rect, int percent)
{
	int valid_width  = (percent*rect->w)/100;
	int valid_height = (percent*rect->h)/100;
	
	rect->x = (rect->w - valid_width) / 2;
	rect->w = valid_width;
	rect->y = (rect->h - valid_height) / 2;
	rect->h = valid_height;
}

//rect align with parent_rect on the top-right
static void display_setareapercent(CDR_RECT *rect, const CDR_RECT *parent_rect, int percent)
{
	int valid_width  = (percent*rect->w)/100;
	int valid_height = (percent*rect->h)/100;
	
	rect->x = (parent_rect->x + parent_rect->w) - valid_width;
	rect->w = valid_width;
	rect->y = parent_rect->y;
	rect->h = valid_height;
}

void RecordPreview::otherScreen(int screen)
{
#ifdef DISPLAY_READY
	CdrDisplay *cdCSI, *cdUVC;
	int hlay_uvc = 0;
	cdCSI = getCommonDisp(CAM_CSI);
	cdUVC = getCommonDisp(CAM_UVC);
	if (cdUVC) {
		hlay_uvc = cdUVC->getHandle();
	}
	if (cdCSI) {
		cdCSI->otherScreen(screen, hlay_uvc);
	}
	if (screen == 1) {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 1280;
		rectBig.h = 720;
		display_setareapercent(&rectBig, 95);
		rectSmall.x = 920;
		rectSmall.y = 0;
		rectSmall.w = 360;
		rectSmall.h = 240;
		display_setareapercent(&rectSmall, &rectBig, 95);
	} else if (screen == 2) {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 720;
		rectBig.h = 480;
		display_setareapercent(&rectBig, 95);
		rectSmall.x = 480;
		rectSmall.y = 0;
		rectSmall.w = 240;
		rectSmall.h = 160;
		display_setareapercent(&rectSmall, &rectBig, 95);
	} else {
		rectBig.x = 0;
		rectBig.y = 0;
		rectBig.w = 480;
		rectBig.h = 272;
		rectSmall.x = 300;
		rectSmall.y = 22;
		rectSmall.w = 180;
		rectSmall.h = 102;
	}
	restoreZOrder();
#endif
}

void RecordPreview::storagePlug(bool plugin)
{
	ResourceManager* rm = ResourceManager::getInstance();
	bool val;

	val = rm->getResBoolValue(ID_MENU_LIST_POR);
	if (plugin) { //tfcard plug in
		setSdcardState(true);
		if (val) {
			ALOGD(" auto start record when sdcard plug in");
		#ifdef MESSAGE_FRAMEWORK
			CommuReq req;

			db_msg("fwk_msg send start record\n");
			memset(&req, 0x0, sizeof(req));
			req.cmdType = COMMU_TYPE_RECORD;
			req.operType = 1;
			fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
		#else
			startRecord();
		#endif
		} else {
			CloseTipLabel();
		}
	} else {	//tfcard plug out
	#ifdef MESSAGE_FRAMEWORK
		CommuReq req;

		db_msg("fwk_msg send stop record\n");
		memset(&req, 0x0, sizeof(req));
		req.cmdType = COMMU_TYPE_RECORD;
		req.operType = 0;
		fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
	#else
		stopRecord();
	#endif
		setSdcardState(false);

		//hide by wjh(usb camera don't start preview if the video mode is VIDEO_MODE_FONT_CAMERA)
		//stopPreview(CAM_UVC);
		//startPreview(CAM_UVC);
	}
}

void RecordPreview::restoreZOrder()
{
#ifdef DISPLAY_READY
	CdrDisplay *cdCSI = getCommonDisp(CAM_CSI);
	CdrDisplay *cdUVC = getCommonDisp(CAM_UVC);
	switch(mPIPMode) {
		case NO_PREVIEW:
		break;
		case CSI_ONLY:
			
			if (cdCSI) {
				cdCSI->setRect(rectBig);
				cdCSI->setBottom();
			}
			if (cdUVC) {
				//db_msg("UVC layer should be close!!!");
				cdUVC->setBottom();
			}
		break;
		case UVC_ONLY:
			
			if (cdUVC) {
				cdUVC->setRect(rectBig);
				cdUVC->setBottom();
			}
			if (cdCSI) {
				//db_msg("CSI layer should be close!!!");
				cdCSI->setBottom();
			}
		break;
		case CSI_ON_UVC:
			if (cdCSI) {
				cdCSI->setRect(rectSmall);
			}
			cdCSI->setBottom();
			if (cdUVC) {
				cdUVC->setRect(rectBig);
			}
			cdUVC->setBottom();
		break;
		case UVC_ON_CSI:
			if (cdUVC) {
				cdUVC->setRect(rectSmall);
			}
			cdUVC->setBottom();
			if (cdCSI) {
				cdCSI->setRect(rectBig);
			}
			cdCSI->setBottom();
		break;
	};
#endif
}


void RecordPreview::transformPIP(pipMode_t newMode)
{
#ifdef DISPLAY_READY
	CdrDisplay *cdCSI, *cdUVC;

	db_msg("mPIPMode is %d, newMode is %d\n", mPIPMode, newMode);
	switch(mPIPMode) {
	case NO_PREVIEW:	
		{
			if(newMode == UVC_ONLY) {
				if(mCamExist[CAM_UVC] == false)
					break;
				if(!mCamera[CAM_UVC])	{
					initCamera(CAM_UVC);
				}
				cdUVC = getCommonDisp(CAM_UVC);
				cdUVC->setRect(rectBig);
				startPreview(CAM_UVC);
				mPIPMode = UVC_ONLY;
				db_msg("-----------PIP_MODE: NO_PREVIEW ---> UVC_ONLY\n");
			}
		}
		break;
	case CSI_ONLY:
		{
			if(newMode == UVC_ON_CSI) {
				if(mCamExist[CAM_UVC] == false)
					break;
				if(!mCamera[CAM_UVC]){
					initCamera(CAM_UVC);
					cdCSI = getCommonDisp(CAM_CSI);
					cdCSI->setBottom();
					startPreview(CAM_UVC);
					if((SdcardState) && isRecording(CAM_CSI)) {
                                            startRecord(CAM_UVC);
                                            ALOGD("usb camera start record");
					} else {
                                            ALOGD("no sdcard,record not start");
					}
					mPIPMode = UVC_ON_CSI;
					db_msg("-----------PIP_MODE: CSI_ONLY ---> UVC_ON_CSI\n");
				} else {
					cdUVC = getCommonDisp(CAM_UVC);
					cdUVC->setRect(rectSmall);
					cdUVC->open();

					cdCSI = getCommonDisp(CAM_CSI);
					cdCSI->setBottom();
					startPreview(CAM_UVC);
					mPIPMode = UVC_ON_CSI;
					db_msg("-----------PIP_MODE: CSI_ONLY ---> UVC_ON_CSI\n");
				}
			} else if(newMode == CSI_ONLY) {
				db_msg("-----------PIP_MODE: CSI_ONLY ---> CSI_ONLY\n");
			}
		}
		break;
	case UVC_ONLY:
		{
			if(newMode == CSI_ONLY) {
				if(mCamExist[CAM_CSI] == false)
					break;
				if(!mCamera[CAM_CSI]) {
					initCamera(CAM_CSI);
					startPreview(CAM_CSI);

					cdUVC = getCommonDisp(CAM_UVC);
					cdUVC->close();
					cdUVC->setRect(rectSmall);
					mPIPMode = CSI_ONLY;
					db_msg("-----------PIP_MODE: UVC_ONLY ---> CSI_ONLY\n");
				} else {
					cdUVC = getCommonDisp(CAM_UVC);
					cdCSI = getCommonDisp(CAM_CSI);
					cdCSI->setRect(rectSmall);
					cdUVC->setBottom();

					cdUVC->exchange(cdCSI->getHandle(), 1);
					cdCSI->open();
					cdUVC->close();
					startPreview(CAM_CSI);
					mPIPMode = CSI_ONLY;
					db_msg("-----------PIP_MODE: UVC_ONLY ---> CSI_ONLY\n");
				}
			} else if(newMode == NO_PREVIEW) {
				cdUVC = getCommonDisp(CAM_UVC);
				cdUVC->close();
				mPIPMode = NO_PREVIEW;
				db_msg("-----------PIP_MODE: UVC_ONLY ---> NO_PREVIEW\n");
			} else if(newMode == UVC_ONLY) {
				if(mCamExist[CAM_UVC] == false)
					break;
				if(!mCamera[CAM_UVC])	{
					initCamera(CAM_UVC);
				}
				cdUVC = getCommonDisp(CAM_UVC);
				cdUVC->setRect(rectBig);
				startPreview(CAM_UVC);
				mPIPMode = UVC_ONLY;
				db_msg("-----------PIP_MODE: UVC_ONLY ---> UVC_ONLY\n");
			}
		}
		break;
	case CSI_ON_UVC:
		if(newMode == UVC_ONLY) {
			cdCSI = getCommonDisp(CAM_CSI);
			cdCSI->setRect(rectSmall);
			cdCSI->close();

			mPIPMode = UVC_ONLY;
			db_msg("-----------PIP_MODE: CSI_ON_UVC ---> UVC_ONLY\n");
		} else if(newMode == CSI_ONLY) {
			cdUVC = getCommonDisp(CAM_UVC);
			cdCSI = getCommonDisp(CAM_CSI);
			cdCSI->setRect(rectSmall);
			cdUVC->setRect(rectBig);
			cdUVC->setBottom();

			cdCSI->exchange(cdUVC->getHandle(), 1);
			cdUVC->close();
			mPIPMode = CSI_ONLY;
			db_msg("-----------PIP_MODE: CSI_ON_UVC ---> CSI_ONLY\n");
		}
		break;
	case UVC_ON_CSI:
		cdUVC = getCommonDisp(CAM_UVC);	
		cdCSI = getCommonDisp(CAM_CSI);
		cdUVC->setRect(rectSmall);
		cdCSI->setRect(rectBig);
		cdCSI->setBottom();
		if(newMode == CSI_ONLY) {
			cdUVC->close();
			mPIPMode = CSI_ONLY;
			db_msg("-----------PIP_MODE: UVC_ON_CSI ---> CSI_ONLY\n");
		} else if(newMode == CSI_ON_UVC) {
			cdCSI->exchange(cdUVC->getHandle(), 1);
			mPIPMode = CSI_ON_UVC;
			db_msg("-----------PIP_MODE: UVC_ON_CSI ---> CSI_ON_UVC\n");
		} else if(newMode == UVC_ONLY) {
			cdCSI->close();

			mPIPMode = UVC_ONLY;
			db_msg("-----------PIP_MODE: UVC_ON_CSI ---> UVC_ONLY\n");
		}
		break;
	}
#endif
}

bool RecordPreview::isCameraExist(int cam_type){
    return mCamExist[cam_type];
}

void RecordPreview::setCamExist(int cam_type, bool bExist)
{
	db_msg("-----------cam_type: %d %d\n", cam_type, bExist);
	unsigned int curWindowID;
	unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;
	int hasTVD = cam_type;
	if (cam_type == CAM_TVD) {
		mHasTVD = bExist;
		cam_type = CAM_UVC;
	} else if (cam_type == CAM_UVC) {
		mHasTVD = !bExist;
	}
	
	if (mCamExist[cam_type] == bExist) {
		return ;
	}
	Mutex::Autolock _l(mLock);
	mCamExist[cam_type] = bExist;

#ifdef USE_NEWUI
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
	if(rm) {
		videoCurrent = rm->menuDataVM.current;
	}
#endif

	curWindowID = mCdrMain->getCurWindowID();
	if(curWindowID != WINDOWID_PLAYBACKPREVIEW && curWindowID != WINDOWID_PLAYBACK) {
		/* RecordPreview and Menu */
		if(bExist == true) {
			/* plug in */	
			if(mPIPMode == NO_PREVIEW) {
#ifdef USE_NEWUI
				if(videoCurrent != VIDEO_MODE_FONT_CAMERA) {
					transformPIP(UVC_ONLY);
				}
#else
				transformPIP(UVC_ONLY);
#endif
			} else if(mPIPMode == CSI_ONLY) {
				usleep(5 * 1000);
#ifdef USE_NEWUI
				if(videoCurrent == VIDEO_MODE_DOUBLE_CAMERA) {
					transformPIP(UVC_ON_CSI);
				} else if(videoCurrent == VIDEO_MODE_BACK_CAMERA) {
					transformPIP(UVC_ONLY);
				}
#else
				transformPIP(UVC_ON_CSI);
#endif

			} else if(mPIPMode == UVC_ONLY) {
#ifdef USE_NEWUI
				if(videoCurrent == VIDEO_MODE_DOUBLE_CAMERA) {
					transformPIP(UVC_ON_CSI);
				} else if(videoCurrent == VIDEO_MODE_BACK_CAMERA) {
					transformPIP(UVC_ONLY);
				}
#else
				transformPIP(UVC_ON_CSI);
#endif
			}
		} else {
			/* plug out */
			if(SdcardState) {
                            stopRecord(CAM_UVC);
                        }
			if(mCamExist[CAM_CSI]) {
#ifdef USE_NEWUI
				if(videoCurrent != VIDEO_MODE_BACK_CAMERA) {
					transformPIP(CSI_ONLY);
				} else {
					if(mCamera[CAM_UVC]) {
						CdrDisplay * cdCSI;
						cdCSI = mCamera[CAM_UVC]->getDisp();
						cdCSI->close();
					}
				}
#else
				transformPIP(CSI_ONLY);	
#endif
			} else {
				transformPIP(NO_PREVIEW);
			}

			if (cam_type == CAM_UVC) {
				stopPreview(CAM_UVC);
			}
			if(mCamera[CAM_UVC]) {
				if (hasTVD != CAM_TVD) {
					delete mCamera[CAM_UVC];
					mCamera[CAM_UVC] = NULL;
				}
			}
		}
	} else {
		/* PlayBackPreview and PlayBack */
		mUvcPlugFlag = true;
		if(bExist == false) {
			if(mCamera[CAM_UVC]) {
				stopPreview(CAM_UVC);
				if (hasTVD != CAM_TVD) {
					delete mCamera[CAM_UVC];
					mCamera[CAM_UVC] = NULL;
				}
			}
		}
	}
}

void RecordPreview::waitForStorageMount(int timeout_ms)
{
	StorageManager* sm = StorageManager::getInstance();
	int i, count;

	count = timeout_ms / 500;
	for (i = 0; i < count; i++) {
		if (sm->isMount()) {
			break;
		}
		usleep(500*1000l);
	}
}

int RecordPreview::setRecordState(int state)
{
	int fd = 0;
	int ret;
	char buf[10];

	fd =  open(TMP_RECDRD_STATE, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(fd < 0) {
		db_error("open TMP_RECDRD_STATE fail\n");
		return -1;
    }
	sprintf(buf, "%d", state);
	ret = write(fd, buf, strlen(buf));
	if(ret > 0)
		db_msg("set TMP_RECDRD_STATE success!\n");
	else {
		db_error(" write Error (%s),ret=%d,fd=%d \n",strerror(errno),ret,fd);
	}
    close(fd);
    return (ret > 0) ? 0 : -1;
}

int RecordPreview::getRecordState()
{
	int fd = 0;
	int ret;
	char buf[10];

	fd =  open(TMP_RECDRD_STATE, O_RDWR);
	if(fd < 0) {
		db_error("open TMP_RECDRD_STATE fail\n");
		return -1;
	}
	memset(buf, 0, sizeof(buf));
	ret = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if(ret > 0)
		db_msg("get TMP_RECDRD_STATE success!\n");
	else {
		db_error(" read Error (%s),ret=%d,fd=%d \n",strerror(errno),ret,fd);
		return 0;
	}

	return atoi(buf);
}


HerbCamera*  RecordPreview::getHerbCameraPointer(int cam_type)
{
	return mCamera[cam_type]->getCamera();
}

void RecordPreview::setPreviewcallback(int cam_type)
{
	if (NULL != mCamera[cam_type]->getCamera()) {
		mCamera[cam_type]->setPreviewCallback(previewCallback);
	}
	return;
}

int RecordPreview::prepareRecord(int cam_type, CdrRecordParam_t param)
{
	int fd;
	int retval;
	time_t now;
	String8 file;
	HerbCamera *mHC;
	Size size(1280, 720);
	int framerate = BACK_CAM_FRAMERATE;
	int i = 0;
	StorageManager* sm = StorageManager::getInstance();
	
	isThumb = true;
	mBakFileString[cam_type].clear();
	mBakNow[cam_type] = 0;
	
	fd = sm->generateFile(now, file, cam_type, param.video_type);
	mBakNow[0] = now;
	mBakFileString[0] = file;
	#ifdef MESSAGE_FRAMEWORK
	fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_TAKE_THUMB_REQ, NULL, 0);
	#else
	takePicture();
	#endif
	
	if (fd <= 0) {
		SdcardState = false;
		db_error("fail to generate file, retval is %d\n", fd);
		return fd;
	}
	setDeleteFlag(cam_type, file);
	//Mutex::Autolock _l(mLock);
	mHC = mCamera[cam_type]->getCamera();
	mRecorder[cam_type]->setRecordParam(param);

	mRecordDBInfo[cam_type].fileString = file;
	mRecordDBInfo[cam_type].time = now;
	mRecordDBInfo[cam_type].infoString = INFO_NORMAL;
	mRecorder[cam_type]->setDuration(param.duration);
	if (cam_type == CAM_CSI) {
		size = mVideoSize;
		framerate = FRONT_CAM_FRAMERATE;
	} else if (cam_type == CAM_UVC) {
		//get max supported preview size of usb camera
		Vector<Size> sizes;
		int max_width = 0;
		int max_height = 0;
		HerbCamera::Parameters preview_params;

		mHC->getParameters(&preview_params);
		preview_params.getSupportedPreviewSizes(sizes);
		for(int i = 0; i < sizes.size(); i++) {
			if (sizes[i].width > max_width) {
				max_width = sizes[i].width;
				max_height = sizes[i].height;
			}
		}
		size.width = max_width;
		size.height = max_height;
	} else {
		if (mHasTVD) {
			size.width = 720;
			size.height = 480;
		}
	}
	retval = mRecorder[cam_type]->initRecorder(mHC, size, framerate, fd, 0);
	return retval;
}


int RecordPreview::startRecord(int cam_type, CdrRecordParam_t param)
{
	int retval;
	HWND hStatusBar;
	int isEnableRecord;

	{
		Mutex::Autolock _l(mRecordLock);
		if(mRecorder[cam_type] == NULL) {
			mRecorder[cam_type] = new CdrMediaRecorder(cam_type);
			mRecorder[cam_type]->setOnInfoCallback(this);
			mRecorder[cam_type]->setOnErrorCallback(this);
		}
		if(mRecorder[cam_type]->getRecorderState() == RECORDER_RECORDING) {
			db_error("cam_type: %d, recording is started\n", cam_type);
			return 0;
		}

		isEnableRecord = mCdrMain->isEnableRecord();
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		db_msg("isEnableRecord is %d\n", isEnableRecord);

		if (isEnableRecord != FORBID_NORMAL) {
			db_msg("xxxxxxxx\n");
			return -1;
		}
		db_msg("xxxxxxxx cam_type %d, mCamExist %d\n", cam_type, mCamExist[cam_type]);
		
		retval = prepareRecord(cam_type, param);
		if (retval < 0 ) {
			db_error("fail to prepare");
			if (mRecorder[CAM_CSI] && mRecorder[CAM_CSI]->getRecorderState() != RECORDER_RECORDING) {
				if(retval == RET_NOT_MOUNT) {
					//ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
					SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_NO_TFCARD, 0);
				} else if(retval == RET_IO_NO_RECORD) {
					//ShowTipLabel(mHwnd, LABEL_TFCARD_FULL);
					SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_TFCARD_FULL, 0);
				}
			}
			return retval;
		}

#ifdef USE_NEWUI
		ResourceManager* rm;
		unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;
		rm = ResourceManager::getInstance();
		if(rm) {
			videoCurrent = rm->menuDataVM.current;
		}
		if(videoCurrent == VIDEO_MODE_FONT_CAMERA || videoCurrent == VIDEO_MODE_BACK_CAMERA
			|| (videoCurrent == VIDEO_MODE_DOUBLE_CAMERA && cam_type == CAM_CSI)) {
			mCurRecordVTL = param.duration;
			mRecordVTL = param.duration;
			SendMessage(mHwnd, RPM_START_RECORD_TIMER, 0, 0);
		}
#else
		if(cam_type == CAM_CSI) {
			mCdrMain->setSTBRecordVTL(param.duration);
			SendMessage(hStatusBar, STBM_START_RECORD_TIMER, 0, 0);
		}
#endif
		retval = mRecorder[cam_type]->start();
                mRecorder[cam_type]->setSilent(mSilent);
	}
	if (NULL != mCamera[cam_type]->getCamera()) {
		mCamera[cam_type]->setPreviewCallback(previewCallback);
	}
	setRecordState(1);
	return 0;
}

int RecordPreview::startRecord(int cam_type)
{
	if(mCamExist[cam_type] == false)
		return 0;

	CdrRecordParam_t param;
	param.video_type = VIDEO_TYPE_NORMAL;
	if(cam_type == CAM_CSI)
		param.fileSize = CSI_PREALLOC_SIZE;
	else
		param.fileSize = UVC_PREALLOC_SIZE;
	param.duration = mDuration;
	return startRecord(cam_type, param);
}
int RecordPreview::startRecord()
{
	int ret;
	status_t err;
	unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;

#ifdef USE_NEWUI
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
	if(rm) {
		videoCurrent = rm->menuDataVM.current;
	}
#endif

	db_msg("xxxxxxxx\n");
	if(VIDEO_MODE_BACK_CAMERA != videoCurrent) {
		if(mCamera[CAM_CSI]) {
			if((ret = startRecord(CAM_CSI)) < 0) {
				db_error("startRecord CAM_CSI failed, ret is %d\n", ret);
				return -1;
			}
		}
	}

	if(VIDEO_MODE_FONT_CAMERA != videoCurrent) {
		if(mCamera[CAM_UVC]) {
			if((ret = startRecord(CAM_UVC)) < 0) {
				db_error("startRecord CAM_UVC failed, ret is %d\n", ret);
				return -1;
			}
		}
	}
	mCdrMain->noWork(false);
	return 0;
}


int RecordPreview::stopRecord(int cam_type)
{
	int ret = 0;
	StorageManager* sm = StorageManager::getInstance();
	HWND hStatusBar;
	String8 needDeleteFile;

	{
		Mutex::Autolock _l(mRecordLock);		
		if(mRecorder[cam_type] == NULL) {
			db_msg("mRecorder[%d] is NULL\n", cam_type);
			return 0;
		}
		if(mRecorder[cam_type]->getRecorderState() != RECORDER_RECORDING) {
			db_error("cam_type: %d, recording is stopped\n", cam_type);
			return 0;
		}
		
		ret = mRecorder[cam_type]->stop();
		if (ret < 0) {
			return ret;
		}
		mRecorder[cam_type]->stopImpactVideo();
                mRecorder[cam_type]->release();
                delete(mRecorder[cam_type]);
                mRecorder[cam_type] = NULL;
        
		needDeleteFile = getDeleteFlag(cam_type);
		db_error("~~~~~~~~~~STOP record ,check need delete file:%d %s\n", !needDeleteFile.isEmpty(), needDeleteFile.string());
		if (!needDeleteFile.isEmpty()) {
			if(sm->fileSize(needDeleteFile) == 0) {
				sm->deleteFile(needDeleteFile.string());
				clrDeleteFlag(cam_type, needDeleteFile );
			}
		}
	}

#ifdef USE_NEWUI
	ResourceManager* rm;
	unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;
	rm = ResourceManager::getInstance();
	if(rm) {
		videoCurrent = rm->menuDataVM.current;
	}
	if(videoCurrent == VIDEO_MODE_FONT_CAMERA || videoCurrent == VIDEO_MODE_BACK_CAMERA
		|| (videoCurrent == VIDEO_MODE_DOUBLE_CAMERA && cam_type == CAM_CSI)) {
		stopRecordTimer();
		SendMessage(mHwnd, RPM_STOP_RECORD_TIMER, 0, 0);
	}
#else
	if(cam_type == CAM_CSI) {
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		SendMessage(hStatusBar, STBM_STOP_RECORD_TIMER, 0, 0);
	}
#endif

	if(mLockFlag == true) {
		lockFile();
	}
	if (!isRecording())
		setRecordState(0);
	db_msg("stopRecord finished\n");
	return ret;
}

void RecordPreview::stopRecord()
{
	int ret = 0;
#ifdef MOTION_DETECTION_ENABLE
	if(mRPWindowState == STATUS_AWMD) {
		if(isRecording(CAM_CSI)) {
			db_msg("need delay start the awmd record\n");
			setDelayStartAWMD(true);
		}
	}
#endif
	ret = stopRecord(CAM_CSI);
	if (ret < 0) {
		return ;
	}

	ret = stopRecord(CAM_UVC);
	if (ret < 0) {
		return ;
	}
	if (mCdrMain->isBatteryOnline()) {
		/* mCdrMain->noWork(true); */
	}
}

void RecordPreview::recordRelease(int cam_type)
{
	if (mRecorder[cam_type]) {
		mRecorder[cam_type]->release();
	}
}

void RecordPreview::recordRelease()
{
	recordRelease(CAM_CSI);
}

void RecordPreview::previewRelease(int cam_type)
{
	if(mCamExist[cam_type] == true && mCamera[cam_type]) {
		mCamera[cam_type]->release();
                delete mCamera[cam_type];
		mCamera[cam_type] = NULL;
	}
}

void RecordPreview::previewRelease()
{
	previewRelease(CAM_CSI);
}

void RecordPreview::stopPreview(int cam_type)
{
	if(mCamera[cam_type]) {
		mCamera[cam_type]->stopPreview();
                //wait buffer released by display thread
                waitPreviewBufferReleased(cam_type);
	}
}

void RecordPreview::stopPreview()
{
	stopPreview(CAM_CSI);
	stopPreview(CAM_UVC);
}

#ifdef DISPLAY_READY
CdrDisplay *RecordPreview::getCommonDisp(int disp_id)
{
	if(mCamera[disp_id])
		return mCamera[disp_id]->getDisp();
	else
		return NULL;
}
#endif

#ifdef DISPLAY_READY
int RecordPreview::getHdl(int disp_id)
{
       CdrDisplay *cd = getCommonDisp(disp_id);
       return cd->getHandle();
}
#endif

void RecordPreview::setSilent(bool mode)
{
	db_error("-------------------setSilent:%d\n", mode);
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setSilent(mode);
	}
	if (mRecorder[CAM_UVC]) {
		mRecorder[CAM_UVC]->setSilent(mode);
	}
	mSilent = mode;
}

void RecordPreview::setVideoQuality(int idx)
{
	Mutex::Autolock _l(mLock);
	db_error("----------------setVideoQuality, %d", idx);
	if (idx == 0) {
		mVideoSize = Size(1920, 1080);
	} else if (idx == 1) {
		mVideoSize = Size(1280, 720);
	}
}

void RecordPreview::setPictureQuality(int quality)
{
	pictuQuality = (ImageQuality_t)quality;
}
	
void RecordPreview::setPicQuality(int idx)
{
	Mutex::Autolock _l(mLock);
	db_error("----------------setPicQuality %d", idx);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setImageQuality((ImageQuality_t)idx);
	}
	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setImageQuality((ImageQuality_t)idx);
	}
}

void RecordPreview::setRecordTime(int idx)
{
	Mutex::Autolock _l(mLock);
	db_msg("----------------setRecordTime %d!", idx);
	if(idx < 0 || idx >= 2)
		return;
	int time = ((idx+1)*60*1000);
	mDuration = time;
#if 0
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setDuration(time);
	}
#endif
}

#ifdef MOTION_DETECTION_ENABLE
void RecordPreview::setAWMD(bool val)
{
	db_msg("----------------setAWMD %d!", val);
	if (mCamera[CAM_CSI]) {
		mCamera[CAM_CSI]->setAWMD(val);
	}

	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setAWMD(val);
	}
}
#endif

#ifdef WATERMARK_ENABLE
void RecordPreview::setWaterMark(bool val)
{
    mNeedWaterMark = val;
	if(mCamera[CAM_CSI]) {
        mCamera[CAM_CSI]->setWaterMark(mNeedWaterMark);
    }

	if (mCamera[CAM_UVC]) {
		mCamera[CAM_UVC]->setWaterMark(mNeedWaterMark);
	}
}

void RecordPreview::updatePreviewWaterMarkInfo()
{
#if	1
		int newCarSpeed = (int)getSpeed();
		if(newCarSpeed != oldCarSpeed) {
                        char newCarSpeedString[GPS_SPEED_WATERMARK_LENGTH];
                        sprintf(newCarSpeedString, "%dkm/h", newCarSpeed);
                        if (mCamera[CAM_CSI]) {
                                mCamera[CAM_CSI]->updatePreviewWaterMarkInfo(newCarSpeedString);
                        }

                        if (mCamera[CAM_UVC]) {
                                mCamera[CAM_UVC]->updatePreviewWaterMarkInfo(newCarSpeedString);
                        }
			oldCarSpeed = newCarSpeed;
                }

		usleep(1500000);
#else
		if((mHwnd != NULL) && (mCdrMain != NULL) &&
			(mCdrMain->getCurWindowID() == WINDOWID_RECORDPREVIEW)){
			if(mNeedWaterMark) {
				char info[GPS_WATERMARK_LENGTH];
				time_t loctime;
				struct tm *ptr;
				char osd[30] = {0};

				loctime = time(NULL);
		    	ptr = localtime(&loctime);
		    	sprintf(osd, "%04d-%02d-%02d %02d:%02d:%02d\0",
				ptr->tm_year+1900,
				ptr->tm_mon+1,
				ptr->tm_mday,
				ptr->tm_hour,
				ptr->tm_min,
				ptr->tm_sec);

				SetWindowText(GetDlgItem(mHwnd, ID_PREVIEW_WATERMARK_TIME), osd);
				
				if(!getGpsInfo(info, GPS_WATERMARK_LENGTH)) {
					SetWindowText(GetDlgItem(mHwnd, ID_PREVIEW_WATERMARK_GPSINFO), info);
				}
				
			} else {
				SetWindowText(GetDlgItem(mHwnd, ID_PREVIEW_WATERMARK_TIME), "");
				SetWindowText(GetDlgItem(mHwnd, ID_PREVIEW_WATERMARK_GPSINFO), "");
			}
		}
		sleep(1);
#endif
}

#endif

void RecordPreview::msgCallback(int msgType, int idx)
{
	ResourceManager* rm = ResourceManager::getInstance();
	switch(msgType) {
	case MSG_REFRESH:
		{
			idx = rm->getResIntValue(ID_MENU_LIST_VQ, INTVAL_SUBMENU_INDEX);
			setVideoQuality(idx);

			idx = rm->getResIntValue(ID_MENU_LIST_PQ, INTVAL_SUBMENU_INDEX);
			setPicQuality(idx);

			idx = rm->getResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX);
			setRecordTime(idx);

			idx = rm->getResIntValue(ID_MENU_LIST_WB, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setWhiteBalance(WHITE_BALANCE[idx]);
			}
			if (mCamera[CAM_UVC]) {
				mCamera[CAM_UVC]->setWhiteBalance(WHITE_BALANCE[idx]);
			}
			idx = rm->getResIntValue(ID_MENU_LIST_CONTRAST, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setContrast(idx);
			}
			idx = rm->getResIntValue(ID_MENU_LIST_EXPOSURE, INTVAL_SUBMENU_INDEX);
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setExposure(idx-3);
			}
			if(mCamera[CAM_UVC]) {
				mCamera[CAM_UVC]->setExposure(idx-3);
			}
			bool val;
			val = rm->getResBoolValue(ID_MENU_LIST_SILENTMODE);
			setSilent(val);

			val = rm->getResBoolValue(ID_MENU_LIST_TWM);
#ifdef WATERMARK_ENABLE
			setWaterMark(rm->menuDataTWMEnable);
#endif

#ifdef MOTION_DETECTION_ENABLE
			val = rm->getResBoolValue(ID_MENU_LIST_AWMD);
			setAWMD(val);
			updateAWMDWindowPic();
#endif
		}
		break;
	case MSG_RM_VIDEO_QUALITY:
		{
			setVideoQuality(idx);
		}
		break;
	case MSG_RM_PIC_QUALITY:
		{
			setPicQuality(idx);
		}
		break;
#ifdef USE_NEWUI
	case MSG_RM_VIDEO_MODE:
		{
			switchVideoMode(idx);
		}
		break;
#endif
	case MSG_RM_VIDEO_TIME_LENGTH:
		{
			setRecordTime(idx);
		}
		break;
	case MSG_RM_WHITEBALANCE:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setWhiteBalance(WHITE_BALANCE[idx]);
			}

			if (mCamera[CAM_UVC]) {
				mCamera[CAM_UVC]->setWhiteBalance(WHITE_BALANCE[idx]);
			}
		}
		break;
	case MSG_RM_CONTRAST:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setContrast(idx);
			}
		}
		break;
	case MSG_RM_EXPOSURE:
		{
			if (mCamera[CAM_CSI]) {
				mCamera[CAM_CSI]->setExposure(idx-3);
			}

			if(mCamera[CAM_UVC]) {
				mCamera[CAM_UVC]->setExposure(idx-3);
			}
		}
		break;
	case MSG_AUDIO_SILENT:
		{
			setSilent((bool)idx);
		}
		break;
#ifdef MOTION_DETECTION_ENABLE
	case MSG_AWMD:
		setAWMD((bool)idx);
		break;
#endif
#ifdef WATERMARK_ENABLE
	case MSG_WATER_MARK:
		setWaterMark((bool)idx);
		break;
#endif
	default:
		break;
	}
}

#ifdef MOTION_DETECTION_ENABLE
void RecordPreview::feedAwmdTimer(int seconds)
{
	if(mCamera[CAM_CSI])
		mCamera[CAM_CSI]->feedAwmdTimer(seconds);
}

int RecordPreview::queryAwmdExpirationTime(time_t* sec, long* nsec)
{
	if(mCamera[CAM_CSI])
		return mCamera[CAM_CSI]->queryAwmdExpirationTime(sec, nsec);
	else
		return -1;
}


void RecordPreview::updateAWMDWindowPic()
{
	bool awmd;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();
	awmd = rm->getResBoolValue(ID_MENU_LIST_AWMD);
	if(awmd == true) {
		db_msg("updateAWMDWindowPic true\n");
		mRPWindowState = STATUS_AWMD;
		mCdrMain->setCurrentWindowState(mRPWindowState);					
	} else {
		db_msg("updateAWMDWindowPic false\n");
		mRPWindowState = mOldRPWindowState;
		//	mRPWindowState = STATUS_RECORDPREVIEW;	
		mCdrMain->setCurrentWindowState(mRPWindowState);					
	}
}

void RecordPreview::switchAWMD()
{
	if(mRPWindowState != STATUS_RECORDPREVIEW && mRPWindowState != STATUS_AWMD)
		return;
	if(mRPWindowState == STATUS_AWMD) {
		db_msg("disable awmd\n");	
		switchAWMD(false);
		setDelayStartAWMD(false);
	} else {
		db_msg("enable awmd\n");	
		switchAWMD(true);
	}
}

void RecordPreview::switchAWMD(bool enable)
{
	ResourceManager* rm;
	time_t sec = 0;
	long nsec = 0;
	CdrRecordParam_t param;

	rm = ResourceManager::getInstance();
	if(enable == true) {
		stopRecord(CAM_CSI);
		rm->setResBoolValue(ID_MENU_LIST_AWMD, true);
		mRPWindowState = STATUS_AWMD;
		mCdrMain->setCurrentWindowState(mRPWindowState);					
		usleep(500);
		setAWMD(true);
	} else {
		if(queryAwmdExpirationTime(&sec, &nsec) < 0) {
			db_error("query awmd timer expiration time failed\n");
		}
		setAWMD(false);
		if(isRecording(CAM_CSI) == true) {
			if(sec > 0 || (sec == 0 && nsec > 500 * 1000 * 1000)) { 
				param.video_type = VIDEO_TYPE_NORMAL;
				param.fileSize = CSI_PREALLOC_SIZE;
				param.duration = mDuration;
				db_msg("set duration %d ms\n", mDuration);
				mRecorder[CAM_CSI]->setDuration(mDuration);
				mRecorder[CAM_CSI]->setRecordParam(param);
			}
		}
		rm->setResBoolValue(ID_MENU_LIST_AWMD, false);
		mRPWindowState = STATUS_RECORDPREVIEW;
		mCdrMain->setCurrentWindowState(mRPWindowState);					
	}
}
#endif

#ifdef USE_NEWUI
void RecordPreview::switchVideoMode(int idx)
{
	CdrDisplay *cdCSI = NULL, *cdUVC = NULL;
	pipMode_t newPipMode;
	int ret;

	switch(idx) {
		case VIDEO_MODE_FONT_CAMERA:
		{
			db_msg("switch video mode to VIDEO_MODE_FONT_CAMERA");
#ifdef DISPLAY_READY
			newPipMode = CSI_ONLY;
                        // 1. stop all display;
			if(mCamera[CAM_UVC]) {
				stopPreview(CAM_UVC);
			}
                        if(mCamera[CAM_CSI]) {
				stopPreview(CAM_CSI);
				//cdCSI = mCamera[CAM_CSI]->getDisp();
				//cdCSI->close();
                        }

                        // 2. start new display
                        if(mCamExist[CAM_CSI]) {
				if(!mCamera[CAM_CSI])
					initCamera(CAM_CSI);

                                mCamera[CAM_CSI]->setPreviewRect(rectBig, true);
				startPreview(CAM_CSI);
			}
			mPIPMode = newPipMode;
#endif
		}
		break;
		case VIDEO_MODE_BACK_CAMERA:
		{
			db_msg("switch video mode to VIDEO_MODE_BACK_CAMERA");
#ifdef DISPLAY_READY
			newPipMode = UVC_ONLY;
                        // 1. stop all display;
			if(mCamera[CAM_CSI]) {
				stopPreview(CAM_CSI);
			}
                        if(mCamera[CAM_UVC]) {
				stopPreview(CAM_UVC);
				//cdCSI = mCamera[CAM_UVC]->getDisp();
				//cdCSI->close();
                        }

                        // 2. start new display
                        if(mCamExist[CAM_UVC]) {
				if(!mCamera[CAM_UVC])
					initCamera(CAM_UVC);

                                mCamera[CAM_UVC]->setPreviewRect(rectBig, true);
				startPreview(CAM_UVC);
			}

			mPIPMode = newPipMode;
#endif
		}
		break;
		case VIDEO_MODE_DOUBLE_CAMERA:
		{
			db_msg("switch video mode to VIDEO_MODE_DOUBLE_CAMERA");
#ifdef DISPLAY_READY
                        // 1. stop all display;
                        if(mCamera[CAM_CSI]) {
				stopPreview(CAM_CSI);
				//cdCSI = mCamera[CAM_CSI]->getDisp();
				//cdCSI->close();
                        }
                        if(mCamera[CAM_UVC]) {
				stopPreview(CAM_UVC);
				//cdCSI = mCamera[CAM_UVC]->getDisp();
				//cdCSI->close();
                        }

                        // 2. start new display
			if(!mCamera[CAM_CSI])
				initCamera(CAM_CSI);

			if(!mCamExist[CAM_UVC]) {
			   newPipMode = CSI_ONLY;
			} else {
			   if(!mCamera[CAM_UVC]) {
					initCamera(CAM_UVC);
			   }
			   newPipMode = UVC_ON_CSI;
			}

			if (newPipMode == UVC_ON_CSI) {
                                mCamera[CAM_UVC]->setPreviewRect(rectSmall, false);
                                startPreview(CAM_UVC);
			}
                        mCamera[CAM_CSI]->setPreviewRect(rectBig, true);
			startPreview(CAM_CSI);
			mPIPMode = newPipMode;

#endif
		}
		break;
	    default:
	    break;
	}

}
#endif

void RecordPreview::takePicture()
{
	HWND hStatusBar;
	const char* ptr;
	StorageManager* sm;
	ResourceManager* rm;
	bool takePicture_status = false;
	
	if(mCamera[CAM_CSI] || mCamera[CAM_UVC]) {
		sm = StorageManager::getInstance();
		if(sm->isMount() == false) {
			ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
			return;
		}
		/*if(sm->isDiskFull() == true) {
			ShowTipLabel(mHwnd, LABEL_TFCARD_FULL);
			return;
		}*/
		rm = ResourceManager::getInstance();
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		{
			Mutex::Autolock _l(mLockPic);
			if (mCamera[CAM_CSI] && mCamera[CAM_CSI]->isPreviewing()) {
				if (mAllowTakePic[CAM_CSI]) {
					db_msg("csi take pic");
					
					if(isThumb == true)
					{
						mCamera[CAM_CSI]->setImageQuality(thumbQuality);
					}
					else
					{
						mCamera[CAM_CSI]->setImageQuality(pictuQuality);
					}
				
					mAllowTakePic[CAM_CSI] = false;
#ifdef HAVE_takePicture
					mCamera[CAM_CSI]->takePicture();
					takePicture_status = true;
#endif
				} else {
					db_msg("csi last action hasn't finished");
				}
			}
			if (mCamera[CAM_UVC] && mCamera[CAM_UVC]->isPreviewing()) {
				
				if (mAllowTakePic[CAM_UVC]) {
					mAllowTakePic[CAM_UVC] = false;
					db_msg("uvc take pic");
#ifdef HAVE_takePicture
					mCamera[CAM_UVC]->takePicture();
					takePicture_status = true;
#endif
				} else {
					db_msg("uvc last action hasn't finished");
				}
			}
		}
		if (takePicture_status) {
			mCdrMain->setPhotoGraph(1);
		}
		ptr = rm->getLabel(LANG_LABEL_TAKE_PICTURE);
		if(ptr)
			SendMessage(hStatusBar, STBM_SETLABEL1_TEXT, 0, (LPARAM)ptr);
	}
}

char* RecordPreview::getSnapShotLocation(void)
{
	return snapshot_location;
}

void RecordPreview::clearSnapShotLocation(void)
{
	mSnapShotbuf.lock();
	memset(snapshot_location, 0x0, sizeof(snapshot_location));
	mSnapShotbuf.unlock();
}


void RecordPreview::savePicture(void*data, int size, int id)
{
	int fd;
	time_t now;
	char buf[128] = {0};
	HWND hStatusBar;
	StorageManager* sm;
	String8 file;
	Elem elem;

	sm = StorageManager::getInstance();
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	
	if(isThumb == true)
	{
		fd = sm->generateFile2(mBakNow[0], file, 0, PHOTO_TYPE_NORMAL);
	}
	else
	{
		fd = sm->generateFile(now, file, id, PHOTO_TYPE_NORMAL);
	}
	
	if(fd <= 0) {
		db_error("generate picture file failed, retval is %d\n", fd);
		return;	
	}
	db_msg("picture is %s\n", file.string());

	sm->savePicture(data, size, fd);	
	elem.file = (char*)file.string();
	elem.time = now;
	elem.type = (char*)TYPE_PIC;
	elem.info = (char*)"";
	getDir(elem.file,buf);
	getBaseName(buf,buf);
	elem.dir = buf;
	sm->dbAddFile(&elem);

	SendMessage(hStatusBar, STBM_CLEAR_LABEL1_TEXT, WINDOWID_RECORDPREVIEW, 0);
	mCdrMain->setPhotoGraph(0);

	mSnapShotbuf.lock();
	memset(snapshot_location, 0, sizeof(snapshot_location));
	strncpy(snapshot_location,(char*)file.string(),WRTC_MAX_FILE_NAME_LEN);
	mSnapShotbuf.unlock();
}

bool RecordPreview::isRecording(int cam_type)
{
	Mutex::Autolock _l(mRecordLock);
	if(mRecorder[cam_type]) {
		if(mRecorder[cam_type]->getRecorderState() == RECORDER_RECORDING)	
			return true;
	}
	return false;
}
bool RecordPreview::isRecording()
{
	if(isRecording(CAM_CSI) == true || isRecording(CAM_UVC) == true)
		return true;
	return false;
}

int RecordPreview::impactOccur()
{
	db_msg("impact Occur");
	int fd;
	time_t now;
	StorageManager* sm;
	String8 file;
	HWND hStatusBar;
	unsigned int flag = 0;

	if(!isRecording()) {
		db_error("record is not started\n");
		return -1;
	}

	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	//if(SendMessage(hStatusBar, STBM_GET_RECORD_TIME, 0, 0) <= (BFTIMEMS / 1000) ) {
	//	db_error("record time less than %d ms\n", BFTIMEMS);
	//	return -1;
	//}

	Mutex::Autolock _l(mImpactLock);

	if(mCamExist[CAM_CSI] && mRecorder[CAM_CSI] && (mRecorder[CAM_CSI]->getImpactStatus() == true) ) {
		db_msg("CSI impact video has not finished\n");
		return -1;
	}
	if(mCamExist[CAM_UVC] && mRecorder[CAM_UVC] && (mRecorder[CAM_UVC]->getImpactStatus() == true) ) {
		db_msg("UVC impact video has not finished\n");
		return -1;
	}

	sm = StorageManager::getInstance();
	if ( isRecording(CAM_CSI) ) {
		fd = sm->generateFile(now, file, CAM_CSI, VIDEO_TYPE_IMPACT);
		if(fd > 0) {
			mRecorder[CAM_CSI]->startImpactVideo(fd, SZ_1M * 30);
			mExtraDBInfo[CAM_CSI].fileString = file;
			mExtraDBInfo[CAM_CSI].infoString = INFO_LOCK;
			mExtraDBInfo[CAM_CSI].time = now;
			mImpactFlag[CAM_CSI] = 1;
			flag = 1;
			#ifdef USE_NEWUI
			//SendMessage(mHwnd, RPM_START_RECORD_TIMER, 1, 0);
			//startRecordTimer(1);
			#else
			SendMessage(hStatusBar, STBM_START_RECORD_TIMER, 1, 0);
			#endif
		} else {
			db_msg("generate CSI impact file failed\n");
		}
	}
#if 0
	if ( isRecording(CAM_UVC) ) {
		fd = sm->generateFile(now, file, CAM_UVC, VIDEO_TYPE_IMPACT);
		if(fd > 0) {
			mRecorder[CAM_UVC]->startImpactVideo(fd, SZ_1M * 30);
			mExtraDBInfo[CAM_UVC].fileString = file;
			mExtraDBInfo[CAM_UVC].infoString = INFO_LOCK;
			mExtraDBInfo[CAM_UVC].time = now;
			mImpactFlag[CAM_UVC] = 1;
			flag = 1;
		} else {
			db_msg("generate UVC impact file failed\n");
		}
	}
#endif

#if 0
	if(flag == 1)
		lockFile();
#endif

	return 0;
}

bool RecordPreview::getPreviewStatus()
{
	return mIsPreview;
}

/*
 * camera CSI should stop preview, keep layer open, and keep layer rectBig
 * camera UVC should stop preview, keep layer close
 * */
void RecordPreview::prepareCamera4Playback(bool closeLayer)
{
#ifdef DISPLAY_READY
	CdrDisplay *cdCSI = NULL, *cdUVC = NULL;
	db_msg("%s, closelayer %d\n", __func__, closeLayer);
	mIsPreview = false;
	if(mCamera[CAM_CSI]) {
		cdCSI = mCamera[CAM_CSI]->getDisp();
		cdCSI->close();
		cdCSI->setRect(rectBig);
		stopPreview(CAM_CSI);
		if (cdCSI && closeLayer) {
			cdCSI->clean();
		}
	}

	if(mCamera[CAM_UVC]) {
		cdUVC = mCamera[CAM_UVC]->getDisp();
		cdUVC->close();
		stopPreview(CAM_UVC);
		if(cdUVC && closeLayer) {
			cdUVC->clean();
		}
	}
#endif
}

void RecordPreview::restorePreviewOutSide(unsigned int windowID)
{
#ifdef DISPLAY_READY
	CdrDisplay *cdCSI = NULL, *cdUVC = NULL;//, *top = NULL, *bot=NULL;
	pipMode_t newPipMode;
	unsigned int videoCurrent = VIDEO_MODE_DOUBLE_CAMERA;

	if(windowID == WINDOWID_MENU) {
		db_info("return from menu\n");
		return;
	}

	mIsPreview = true;
#ifdef USE_NEWUI
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
	if(rm) {
		videoCurrent = rm->menuDataVM.current;
	}

	if(VIDEO_MODE_FONT_CAMERA == videoCurrent) {
		if(mCamExist[CAM_CSI] == true && !mCamera[CAM_CSI]) {
			initCamera(CAM_CSI);
		}

		if (mCamera[CAM_CSI]) {
			cdCSI = mCamera[CAM_CSI]->getDisp();
			//cdCSI->clean();
			startPreview(CAM_CSI);
			cdCSI->setBottom();
			cdCSI->setRect(rectBig);
			cdCSI->open();
		}
		newPipMode = CSI_ONLY;
	} else if(VIDEO_MODE_BACK_CAMERA == videoCurrent) {
		if(mCamExist[CAM_UVC] == true && !mCamera[CAM_UVC]) {
			initCamera(CAM_UVC);
		}

		if (mCamera[CAM_UVC]) {
			cdUVC = mCamera[CAM_UVC]->getDisp();
			//cdUVC->clean();
			startPreview(CAM_UVC);
			cdUVC->setBottom();
			cdUVC->setRect(rectBig);
			cdUVC->open();
		}
		newPipMode = UVC_ONLY;
	} else if(VIDEO_MODE_DOUBLE_CAMERA == videoCurrent) {
		if(mCamExist[CAM_CSI] == true && !mCamera[CAM_CSI]) {
			initCamera(CAM_CSI);
		}

		if(mCamExist[CAM_UVC] == true && !mCamera[CAM_UVC]) {
			initCamera(CAM_UVC);
		}

		if (mCamera[CAM_UVC]) {
			cdUVC = mCamera[CAM_UVC]->getDisp();
			//cdUVC->clean();
			startPreview(CAM_UVC);
			cdUVC->setBottom();
			cdUVC->setRect(rectSmall);
			cdUVC->open();
		}

		if (mCamera[CAM_CSI]) {
			cdCSI = mCamera[CAM_CSI]->getDisp();
			//cdCSI->clean();
			startPreview(CAM_CSI);
			cdCSI->setBottom();
			cdCSI->setRect(rectBig);
			cdCSI->open();
		}
		newPipMode = UVC_ON_CSI;
	}

	mPIPMode = newPipMode;
	
#else
	if(!mCamera[CAM_CSI])
		return;

	if(!mCamExist[CAM_UVC]) {
		newPipMode = CSI_ONLY;
	} else {
		if(!mCamera[CAM_UVC]) {
			initCamera(CAM_UVC);
		}
		newPipMode = UVC_ON_CSI;
	}
	db_msg("pipmode :%d", newPipMode);

	if (newPipMode == UVC_ON_CSI) {
		cdUVC = mCamera[CAM_UVC]->getDisp();
	//	cdUVC->clean();
		startPreview(CAM_UVC);
		cdUVC->setBottom();
		cdUVC->setRect(rectSmall);
		cdUVC->open();
	}
	cdCSI = mCamera[CAM_CSI]->getDisp();
	//cdCSI->clean();
	startPreview(CAM_CSI);
	cdCSI->setBottom();
	cdCSI->setRect(rectBig);
	cdCSI->open();
	mPIPMode = newPipMode;
#endif
#endif
}

void RecordPreview::setSdcardState(bool bExist)
{
	if (mRecorder[CAM_CSI]) {
		mRecorder[CAM_CSI]->setSdcardState(bExist);
	}
	if (mRecorder[CAM_UVC]) {
		mRecorder[CAM_UVC]->setSdcardState(bExist);
	}
}
#ifdef MOTION_DETECTION_ENABLE
bool RecordPreview::needDelayStartAWMD(void)
{
	Mutex::Autolock _l(mAwmdLock);
	return mAwmdDelayStart;
}

void RecordPreview::setDelayStartAWMD(bool value)
{
	Mutex::Autolock _l(mAwmdLock);
	mAwmdDelayStart = value;
	if(value == true)
		mAwmdDelayCount = 0;
}
#endif

bool RecordPreview::getUvcState()
{
	return mCamExist[CAM_UVC];
}

void RecordPreview::showFrame(camera_preview_frame_buffer_t *data, int id)
{
    BufferUsedCnt *buffCnt = new BufferUsedCnt(id, data->index);
    mBufferlock.lock();
    mBufferQueue.push_back(buffCnt);
    mBufferlock.unlock();

    //for remote display
    if((id == CAM_CSI) && (mRemoteBuffer->isConnected())) {
        ALOGD("%s, remote camid %d, index %d",__func__, id, data->index);
        struct RemotePreviewBuffer *buf = new struct RemotePreviewBuffer;
        buf->share_fd = data->share_fd;
        buf->index = data->index;
        buf->width = data->width;
        buf->height = data->height;
        buf->stride = data->stride;
        buf->size = data->size;

        buffCnt->incUsedCnt(1);
        mRemoteBuffer->onPreviewData(buf->share_fd, (unsigned char*)buf, sizeof(RemotePreviewBuffer));
    }
    //for local display
    {
        PreviewBuffer *pbuf = new PreviewBuffer;
        pbuf->share_fd = dup(data->share_fd);
        pbuf->index = data->index;
        pbuf->width = data->width;
        pbuf->height = data->height;
        pbuf->stride = data->stride;
        pbuf->size = data->size;
        pbuf->cam_id = id;

        unsigned char *vaddr = (unsigned char *)mmap(NULL, pbuf->size, PROT_READ | PROT_WRITE, MAP_SHARED, pbuf->share_fd,
0);
        pbuf->vaddr = vaddr;
        close(pbuf->share_fd);
        pbuf->share_fd = 0;

        pbuf->format = data->format;

        buffCnt->incUsedCnt(1);
        mCamera[id]->getDisp()->commitToDisplay(pbuf, true);
    }

}

//for local display
void RecordPreview::returnFrame(struct PreviewBuffer *pBuf)
{
	munmap(pBuf->vaddr, pBuf->size);
        releaseBuffer(pBuf->cam_id, pBuf->index);
	delete(pBuf);
}


void RecordPreview::call(unsigned char *pbuf, int size) {
    ALOGD("RecordPreview::call cmd :%s, len %d\n", pbuf, size);
    // to do ...
    char reply[128];
    snprintf(reply, 128, "%s reply ok", pbuf);
    mRemoteBuffer->onCall((unsigned char *)reply, 128);
}

String8 RecordPreview::callSync(unsigned char *pbuf, int size) {
    ALOGD("RecordPreview::callSync cmd :%s, len %d\n", pbuf, size);
	
	return handle_remote_message_sync(pbuf, size);
}

int RecordPreview::onCall(unsigned char *pbuf, int size) {
    ALOGD("RecordPreview::onCall cmd :%s, len %d\n", pbuf, size);
    mRemoteBuffer->onCall(pbuf, size);
	return 0;
}

int RecordPreview::getMP4Duration(const char * path)
{
	int duration = 0;
	HerbMediaMetadataRetriever *retriever;
	retriever = new HerbMediaMetadataRetriever();

	retriever->setDataSource(path);
	duration = atoi((char *)retriever->extractMetadata(METADATA_KEY_DURATION));
	delete retriever;
	return duration;
}


//for remote display
void RecordPreview::releaseBuffer(unsigned char *pbuf, int size) {
    struct RemotePreviewBuffer *buf = (struct RemotePreviewBuffer *)pbuf;
    ALOGD("RecordPreview::releaseBuffer index %d\n", buf->index);
    releaseBuffer(0, buf->index);
}

void RecordPreview::releaseBuffer(int camId, int index) {
        BufferUsedCnt *temp = NULL;
        mBufferlock.lock();
        for(int i = 0; i < mBufferQueue.size(); i++) {
                temp = mBufferQueue.itemAt(i);
                if(temp->mCamId == camId && temp->mIndex == index) {
                        temp->decUsedCnt(1);
                        if (temp->getUsedCnt() == 0) {
                                mBufferQueue.removeAt(i);
                        }
                        break;
                }
        }
        mBufferlock.unlock();

        if (temp && temp->getUsedCnt() == 0) {
                if(mCamera[camId])
                        mCamera[camId]->releasePreviewFrame(index);
                delete temp;
        }
}

//must be called after stopPreview(camId) which include close display
void RecordPreview::waitPreviewBufferReleased(int camId) {
        BufferUsedCnt *temp = NULL;
        ALOGD("%s camId %d enter\n", __func__, camId);
        while(1) {
                bool bBufferEmpty = true;
                mBufferlock.lock();
                for(int i = 0; i < mBufferQueue.size(); i++) {
                        temp = mBufferQueue.itemAt(i);
                        if(temp->mCamId == camId) {
                                bBufferEmpty = false;
                                break;
                        }
                }
                mBufferlock.unlock();
                if(bBufferEmpty)
                        break;
                else
                        usleep(10*1000);
        }
        ALOGD("%s camId %d done\n", __func__, camId);
}
#ifdef WATERMARK_ENABLE
#if	0
int RecordPreview::createWatermarkWidgets()
{
	CDR_RECT rect;
	RECT parentRect;
	int retval = 0;
	HWND retWnd;
	/*
	ResourceManager *rm;
	rm = ResourceManager::getInstance();
	retval = rm->getResRect(ID_STATUSBAR_LABEL1, rect);
	if(retval < 0) {
		db_error("get ID_STATUSBAR_LABEL1 failed\n");
		return -1;
	}*/

	GetWindowRect(mHwnd, &parentRect);
	rect.x = 20;
	rect.y = RECTH(parentRect) - 32;
	rect.w = RECTW(parentRect) - 40;
	rect.h = 16;

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			ID_PREVIEW_WATERMARK_TIME,
			rect.x, rect.y, rect.w, rect.h,
			mHwnd, 0);
	if(retWnd == HWND_INVALID) {
		db_error("create watermark time widgets failed\n");
		return -1;
	}

	rect.x = 20;
	rect.y = RECTH(parentRect) - 16;
	rect.w = RECTW(parentRect) - 40;
	rect.h = 16;
	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			ID_PREVIEW_WATERMARK_GPSINFO,
			rect.x, rect.y, rect.w, rect.h,
			mHwnd, 0);
	if(retWnd == HWND_INVALID) {
		db_error("create watermark gps info widgets failed\n");
		return -1;
	}

	return 0;
}
#endif

#endif

