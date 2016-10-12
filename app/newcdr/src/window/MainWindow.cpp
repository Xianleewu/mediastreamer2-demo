
#include "MainWindow.h"
#include <properties.h>
#ifdef LOG_TAG 
#undef LOG_TAG
#endif
#define LOG_TAG "MainWindow.cpp"
#include "debug.h"

#include "posixTimer.h"
#include "dateSetting.h"

#ifdef REMOTE_3G
extern long report_alarm_time;
#endif

using namespace android;


typedef struct 
{
	int idx;
	CdrMain *cdrMain;
	timer_t id;
	time_t time;
}timer_set;
static timer_set timer_data[3];

void ready2ShutDown(sigval_t sg)
{
	db_msg("whether to shut down\n");
	timer_set *data = (timer_set*)sg.sival_ptr;
	int idx = data->idx;
	CdrMain* cdrMain = data->cdrMain;

	db_msg("timer idx is %d\n", idx);
	if (cdrMain->isBatteryOnline()) {
		if (idx == TIMER_NOWORK_IDX2) {
			db_info("TIMER_NOWORK_IDX2\n");
			if(cdrMain->isShuttingdown() == false)
				SendNotifyMessage(cdrMain->getHwnd(), MSG_SHOW_TIP_LABEL, LABEL_30S_NOWORK_SHUTDOWN, 0);
			return;
		} else if (idx == TIMER_NOWORK_IDX){
			db_msg("timerid nowork\n");
			if (cdrMain->isRecording()) {
				db_msg("isRecording, no need to shutdown\n");
				goto out;
			}
#ifdef MEDIAPLAYER_READY
			if (cdrMain->isPlaying()) {
				db_msg("isStarted, no need to shutdown\n");
				goto out;
			}
#endif
		}
//   	SendNotifyMessage(cdrMain->getHwnd(), MSG_SHOW_TIP_LABEL, LABEL_SHUTDOWN_NOW, 0);
		cdrMain->shutdown();
		db_msg("now shutdown\n");
		return;
	}

out:
	if(idx == TIMER_NOWORK_IDX) {
		db_info("setOneShotTimer\n");
		setOneShotTimer(timer_data[TIMER_NOWORK_IDX2].time, 0, timer_data[TIMER_NOWORK_IDX2].id);
	}
}

void CdrMain::startShutdownTimer(int idx)
{
	int ret;
	db_msg("startShutdownTimer %d\n", idx);
	if (timer_data[idx].id == 0) {
		ret = createTimer((void*)&timer_data[idx], &timer_data[idx].id, ready2ShutDown);
		db_msg("createTimer, timerId is 0x%lx\n", (unsigned long)timer_data[idx].id);
	}
	if (timer_data[idx].id) {
		if (idx == TIMER_LOWPOWER_IDX || idx == TIMER_NOWORK_IDX2) {
			db_msg("setOneShotTimer\n");
			setOneShotTimer(timer_data[idx].time, 0, timer_data[idx].id);
		} else if(idx == TIMER_NOWORK_IDX){
			db_msg("setPeriodTimer\n");
			setPeriodTimer(timer_data[idx].time, 0, timer_data[idx].id);
		}
		db_msg("#Warning: System will be shut down\n");
	}
}

void CdrMain::initTimerData()
{
	timer_data[TIMER_LOWPOWER_IDX].cdrMain = this;
	timer_data[TIMER_LOWPOWER_IDX].id = 0;
	timer_data[TIMER_LOWPOWER_IDX].time = LOWPOWER_SHUTDOWN_TIME;
	timer_data[TIMER_LOWPOWER_IDX].idx = TIMER_LOWPOWER_IDX;
	timer_data[TIMER_NOWORK_IDX].cdrMain = this;
	timer_data[TIMER_NOWORK_IDX].id = 0;
	timer_data[TIMER_NOWORK_IDX].time = NOWORK_SHUTDOWN_TIME;
	timer_data[TIMER_NOWORK_IDX].idx = TIMER_NOWORK_IDX;
	timer_data[TIMER_NOWORK_IDX2].cdrMain = this;
	timer_data[TIMER_NOWORK_IDX2].id = 0;
	timer_data[TIMER_NOWORK_IDX2].time = NOWORK_SHUTDOWN_TIME2;
	timer_data[TIMER_NOWORK_IDX2].idx = TIMER_NOWORK_IDX2;
}

void CdrMain::noWork(bool idle)
{
	if (idle) {
		if(mShutdowning == true) {
			db_msg("shutting down, not set timer\n");
			return;
		}
		db_msg("no work, resume timer\n");
		startShutdownTimer(TIMER_NOWORK_IDX);
		startShutdownTimer(TIMER_NOWORK_IDX2);
	} else {
		db_msg("working, stop timer\n");
//		cancelShutdownTimer(TIMER_NOWORK_IDX);
		stopShutdownTimer(TIMER_NOWORK_IDX);
		stopShutdownTimer(TIMER_NOWORK_IDX2);
		CloseTipLabel();
	}
}

void CdrMain::cancelShutdownTimer(int idx)
{
	db_msg("cancelShutdownTimer");
	if (timer_data[idx].id != 0) {
		db_msg("deleteTimer");
		deleteTimer(timer_data[idx].id);
		timer_data[idx].id = 0;
	}
}

void CdrMain::stopShutdownTimer(int idx)
{
	db_msg("stopShutdownTimer\n");
	if (timer_data[idx].id != 0) {
		db_msg("stopShutdownTimer\n");
		stopTimer(timer_data[idx].id);
	}
}

bool CdrMain::isRecording()
{
	return mRecordPreview->isRecording();
}

bool CdrMain::isPlaying()
{
#ifdef MEDIAPLAYER_READY
	if (mPlayBack) {
		return mPlayBack->isStarted();
	}
#endif
	return false;
}

/************* MainWindow Proc ***************/
static int CDRWinProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	CdrMain* mCdrMain = (CdrMain*)GetWindowAdditionalData(hWnd);
	//db_msg("message is %s\n", Message2Str(message));
	switch(message) {
	case MSG_FONTCHANGED:
		mCdrMain->updateWindowFont();
		break;
	case MSG_CREATE:
		break;
		/* + + + keyEvent + + + */
	case MSG_KEYDOWN:
		if (!mCdrMain->winCreateFinish) {
			return 0;
		}
		if(mCdrMain->isKeyUp == true) {
			mCdrMain->downKey = wParam;	
			SetTimer(hWnd, ID_TIMER_KEY, LONG_PRESS_TIME);
			mCdrMain->isKeyUp = false;
		}
		break;
	case MSG_KEYUP:
		if (!mCdrMain->winCreateFinish) {
			return 0;
		}
		mCdrMain->isKeyUp = true;
		if(wParam == mCdrMain->downKey)	{
			KillTimer(hWnd, ID_TIMER_KEY);
			db_msg("short press\n");
			mCdrMain->keyProc(wParam, SHORT_PRESS);
		}
		break;
	case MSG_KEYLONGPRESS:
		if (!mCdrMain->winCreateFinish) {
			return 0;
		}
		mCdrMain->downKey = -1;
		db_msg("long press\n");
		mCdrMain->keyProc(wParam, LONG_PRESS);
		break;
	case MSG_TIMER:
		if(wParam == ID_TIMER_KEY) {
			if (!mCdrMain->winCreateFinish) {
				return 0;
			}
			mCdrMain->isKeyUp = true;			
			SendMessage(hWnd, MSG_KEYLONGPRESS, mCdrMain->downKey, 0);
			KillTimer(hWnd, ID_TIMER_KEY);
		}
		break;
		/* - - - keyEvent - - - */
	case MWM_CHANGE_WINDOW:
		mCdrMain->changeWindow(wParam, lParam);
		break;
	case MSG_CLOSE:
		DestroyMainWindow (hWnd);
		PostQuitMessage (hWnd);
		break;
	case MSG_SHOW_TIP_LABEL:
		ShowTipLabel(hWnd, (enum labelIndex)wParam);
		break;
	case MSG_DESTROY:
		break;
	default:
		mCdrMain->msgCallback(message, wParam);
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

static bool inline isKeyEvent(int msg)
{
	if (msg != MSG_KEYDOWN && msg != MSG_KEYUP && msg != MSG_KEYLONGPRESS) {
		return false;	
	}
	return true;
}

int MsgHook(void* context, HWND dst_wnd, int msg, WPARAM wparam, LPARAM lparam)
{
	int ret = HOOK_GOON;
	
	CdrMain *cdrMain = (CdrMain*)context;
	HWND hMainWnd = cdrMain->getHwnd();

	if(cdrMain->mShutdowning == true) {
		db_msg("shutdowning\n");
		return !HOOK_GOON;
	}
	if (isKeyEvent(msg)) {
		if(msg == MSG_KEYDOWN && (wparam == CDR_KEY_POWER || cdrMain->isScreenOn())){
			//playKeySound();
		}
		if (wparam == CDR_KEY_POWER) {  //power key, post to main proc
			SendMessage(hMainWnd, msg, wparam,lparam);
			return !HOOK_GOON; //need not other work
		} else {
			if(cdrMain->isScreenOn())
				cdrMain->setOffTime();
		}
	}
	if (!cdrMain->isScreenOn()) { //screen off
		if (msg == MSG_KEYUP) {
			cdrMain->screenOn();
		}
		return !HOOK_GOON; //nothing to do
	}
	return ret;
}

CdrMain::CdrMain()
	: mShutdowning(false)
	, mSTB(NULL)
	, mRecordPreview(NULL)
#ifdef MEDIAPLAYER_READY
	, mPlayBackPreview(NULL)
	, mPlayBack(NULL)
#endif
	, mMenu(NULL)
#ifdef POWERMANAGER_READY
	, mPM(NULL)
#endif
#ifdef EVENTMANAGER_READY
	, mEM(NULL)
#endif
	, mEnableRecord(FORBID_NORMAL)
	, curWindowState()
	, downKey(0)
	, isKeyUp(true)
	, isLongPress(false)
	, usbStorageConnected(false)
	, mTime(TIME_INFINITE)
	, winCreateFinish(false)
	, mFactoryReseting(false)
{
	sp<ProcessState> proc(ProcessState::self());
	ProcessState::self()->startThreadPool();

	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: start cdr, time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	ResourceManager* rm = ResourceManager::getInstance();
	if(rm->initStage1() < 0) {
		db_error("ResourceManager initStage1 failed\n");
	}

	mRecordPreview =  new RecordPreview(this);
}



CdrMain::~CdrMain()
{
	if(mRecordPreview) {
		delete mRecordPreview;
		mRecordPreview = NULL;
	}
	if(mSTB) {
		delete mSTB;
		mSTB = NULL;
	}
#ifdef MEDIAPLAYER_READY
	if(mPlayBackPreview) {
		delete mPlayBackPreview;
		mPlayBackPreview = NULL;
	}
	if(mPlayBack) {
		delete mPlayBack;
		mPlayBack= NULL;
	}
#endif
	if(mMenu) {
		delete mMenu;
		mMenu= NULL;
	}
	UnRegisterCDRWindows();
	UnRegisterCDRControls();
	ExitGUISafely(0);
	MainWindowThreadCleanup(mHwnd);
}

bool CdrMain::isBatteryOnline()
{
#ifdef EVENTMANAGER_READY
	if (mEM) {
		return mEM->isBatteryOnline();
	}
#endif
	return false;
}
void CdrMain::initPreview(void* none)
{
	status_t err;

	mInitPT = new InitPreviewThread(this);
	err = mInitPT->start();
	if (err != OK) {
		mInitPT.clear();
	}
}

int CdrMain::initPreview(void)
{
	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: initPreview , time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	mRecordPreview->init();
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: after init time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	mPOR.signal();

	mRecordPreview->startPreview();
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: after startPreview time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);


	return 0;
}

void CdrMain::initStage2(void* none)
{
	status_t err;

	mInitStage2T = new InitStage2Thread(this);
	err = mInitStage2T->start();
	if (err != OK) {
		db_msg("initStage2 start failed\n");
		mInitPT.clear();
	}
}

int CdrMain::initStage2(void)
{
	StorageManager* sm;
	ResourceManager* rm;
	int result;

	/* wait for the preview is started */
	mLock.lock();
	result  = mPOR.waitRelative(mLock, seconds(1));
	mLock.unlock();

	rm = ResourceManager::getInstance();
	if(rm->initStage2() < 0) {
		db_error("ResourceManager initStage2 failed\n");
		return -1;
	}

	if(createOtherWindows() < 0) {
		return -1;
	}
#ifdef EVENTMANAGER_READY
	bool hasTVD;
	bool tvdOnline;
#ifdef POWERMANAGER_READY
	mPM = new PowerManager(this);
#endif
	tvdOnline = mRecordPreview->hasTVD(hasTVD);
	mEM = new EventManager(tvdOnline, hasTVD);
	mEM->setEventListener(this);
	mEM->init(hasTVD);
	if(!mEM->isBatteryOnline()) {
		db_msg("battery is charging\n");
		SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHARGING, 0, 0);
	}
	initTimerData();
//	startShutdownTimer(TIMER_NOWORK_IDX);
//	noWork(true);
#endif

	sm = StorageManager::getInstance();
	sm->setEventListener(this);
	if(sm->isMount() == true) {
		db_msg("mounted\n");
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, 1, 0);
	} else {
		db_msg("not mount\n");
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, 0, 0);
	}

	configInit();
	winCreateFinish = true;
	return 0;
}

void CdrMain::stopPreview()
{
        mRecordPreview->setCamExist(CAM_UVC, false);

	mRecordPreview->stopPreview(CAM_CSI);
	mRecordPreview->previewRelease(CAM_CSI);
}

void CdrMain::startPreview()
{
	ResourceManager* rm;
	int val;

	rm = ResourceManager::getInstance();
	val = rm->getResIntValue(ID_MENU_LIST_VM, INTVAL_SUBMENU_INDEX);

	mRecordPreview->initCamera(CAM_CSI);

	if ((val == 0) || (val == 2)) {
                mRecordPreview->startPreview(CAM_CSI);
	}
}

bool CdrMain::isPreviewing()
{
	return mRecordPreview->getPreviewStatus();
}

void CdrMain::shutdown()
{
	db_msg("shutdown\n");
	mShutdowning = true;
	showShutdownLogo();
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
	rm->syncConfigureToDisk();
	mRecordPreview->stopRecord();
	mRecordPreview->stopPreview();
#ifdef MEDIAPLAYER_READY
	mPlayBack->stopPlayback(false);
#endif
#ifdef POWERMANAGER_READY
	mPM->powerOff();
#endif
}

void CdrMain::readyToShutdown()
{
	db_msg("readyToShutdown\n");
	mShutdowning = true;
    if(mFactoryReseting == false) {
    	ResourceManager* rm;
    	rm = ResourceManager::getInstance();
    	rm->syncConfigureToDisk();
    }
	mRecordPreview->stopRecord();
	stopPreview();
#ifdef MEDIAPLAYER_READY
	mPlayBack->stopPlayback(false);
#endif
}

#define IGNORE_UVC_STATE
int CdrMain::notify(int message, int val)
{
	int ret = 0;
	int camNum = 0;

	if(mShutdowning == true) {
		db_info("shutting down, ignore events\n");	
		return 0;
	}

	switch(message) {
#ifdef USE_NEWUI
	case EVENT_VIDEO_PHOTO_VALUE:
		SendMessage(mSTB->getHwnd(), STBM_VIDEO_PHOTO_VALUE, val, 0);
		break;
	case EVENT_VIDEO_MODE:
		SendMessage(mSTB->getHwnd(), STBM_VIDEO_MODE, val, 0);
		break;
#endif
	case EVENT_GPS:
		SendMessage(mSTB->getHwnd(), STBM_ENABLE_DISABLE_GPS, val, 0);
		break;
	case EVENT_WIFI:
		SendMessage(mSTB->getHwnd(), STBM_ENABLE_DISABLE_WIFI, val, 0);
		break;
	case EVENT_AP:
		SendMessage(mSTB->getHwnd(), STBM_ENABLE_DISABLE_AP, val, 0);
		break;
#ifdef MODEM_ENABLE
	case EVENT_CELLULAR:
		SendMessage(mSTB->getHwnd(), STBM_ENABLE_DISABLE_CELLULAR, val, 0);
		break;
#endif
	case EVENT_UVC_PLUG_IN:
#ifdef IGNORE_UVC_STATE
        if (mRecordPreview->isCameraExist(CAM_UVC)) {
            ALOGD("uvc has pluged in");
            return 0;
        }
		camNum = HerbCamera::getNumberOfCameras();
		if(camNum <= 1) {
			ALOGD("catch uvc pluged in, but camerahal not report uvc camera num, maybe fast plugout! egnor the msg!");
			return 0;
		}
		property_set("persist.sys.usbcamera.status","added");
		mRecordPreview->setCamExist(CAM_UVC, true);
		if(curWindowState.curWindowID == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 1, 0);
		}
		break;
	case EVENT_TVD_PLUG_IN:
#ifdef IGNORE_UVC_STATE
		mRecordPreview->setCamExist(CAM_TVD, true);
		if(curWindowState.curWindowID == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 1, 0);
		}

		break;
	case EVENT_UVC_PLUG_OUT:
#ifdef IGNORE_UVC_STATE
        if (!mRecordPreview->isCameraExist(CAM_UVC)) {
            ALOGD("uvc has pluged out");
            return 0;
		}
		property_set("persist.sys.usbcamera.status","removed");
		mRecordPreview->setCamExist(CAM_UVC, false);
		if(curWindowState.curWindowID == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 0, 0);
		}
		break;
	case EVENT_TVD_PLUG_OUT:
#ifdef IGNORE_UVC_STATE
		mRecordPreview->setCamExist(CAM_TVD, false);
		if(curWindowState.curWindowID == WINDOWID_RECORDPREVIEW)
#endif
		{
			SendMessage(mSTB->getHwnd(), STBM_UVC, 0, 0);
		}
		break;
	case EVENT_CONNECT2PC:
		{
			db_msg("EVENT_CONNECT2PC\n");
			if (!isScreenOn()) { //screen off
				screenOn();
			}
			SendNotifyMessage(mHwnd, MSG_CONNECT2PC, 0, 0);
			if (!isRecording() && !isPlaying()) {
				noWork(false);
			}
		}
		break;
	case EVENT_DISCONNECFROMPC:
		{
			int ret;
			db_msg("EVENT_DISCONNECFROMPC\n");
#ifdef POWERMANAGER_READY
			ret = mPM->getBatteryLevel();
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
#endif

			SendNotifyMessage(mHwnd, MSG_DISCONNECT_FROM_PC, 0, 0);
			if (!isRecording() && !isPlaying()) {
				noWork(false);
//				noWork(true);
			}
		}
		break;
	case EVENT_IMPACT:
#ifdef MEDIAPLAYER_READY
		mRecordPreview->impactOccur();
#endif
		break;
	case EVENT_BATTERY:
#ifdef POWERMANAGER_READY
		ret = mPM->setBatteryLevel(val);
		if (ret > BATTERY_CHANGE_BASE) {
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
			if (ret == BATTERY_LOW_LEVEL) {	
				db_error("LOW LEVEL!!!!");
				if (mEM->isBatteryOnline()) {
					SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_LOW_POWER_SHUTDOWN, 0);
					shutdown();
				}
			}
		}
#endif
		break;
	case EVENT_MOUNT:
		SendMessage(mSTB->getHwnd(), STBM_MOUNT_TFCARD, val, 0);
		if (!val) {
			if (curWindowState.curWindowID == WINDOWID_RECORDPREVIEW) {
				db_msg("-----------umount, stopRecord and setSdcardState(false)\n");
#ifdef MEDIAPLAYER_READY
				mRecordPreview->storagePlug(false);
			} else if (curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW) {
				db_msg("current window is playbackview\n");
				SendMessage(mSTB->getHwnd(), STBM_CLEAR_LABEL1_TEXT, WINDOWID_PLAYBACKPREVIEW, 0);
				SendMessage(mHwnd, MWM_CHANGE_WINDOW, WINDOWID_PLAYBACKPREVIEW, WINDOWID_RECORDPREVIEW);
			} else if (curWindowState.curWindowID == WINDOWID_PLAYBACK) {
				db_msg("current window is playback\n");
				SendMessage(mPlayBack->getHwnd(), MSG_PLB_COMPLETE, 0, 0);
				SendMessage(mHwnd, MWM_CHANGE_WINDOW, WINDOWID_PLAYBACK, WINDOWID_RECORDPREVIEW);
#endif
			}
		} else {
#ifdef MEDIAPLAYER_READY
			if (curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW) {
				SendMessage(mPlayBackPreview->getHwnd(), MSG_UPDATE_LIST, 0, 0);
			}
			if (curWindowState.curWindowID == WINDOWID_RECORDPREVIEW)
				mRecordPreview->storagePlug(true);
#endif
		}
		break;
	case EVENT_DELAY_SHUTDOWN:
		if (!isScreenOn()) { //screen off
			screenOn();
		}
#ifdef EVENTMANAGER_READY
		if(mEM->isBatteryOnline()) {
			int ret;
			ret = mPM->getBatteryLevel();
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHANGED, ret, 0);
		}
		if (!isRecording()&&!isPlaying()) {
			startShutdownTimer(TIMER_LOWPOWER_IDX);
			db_msg("xxxxxxxxxx\n");
			SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_10S_SHUTDOWN, 0);
		}
#endif
		break;
	case EVENT_CANCEL_SHUTDOWN:
		if (!isScreenOn()) { //screen off
			screenOn();
		}
#ifdef EVENTMANAGER_READY
		if(!mEM->isBatteryOnline()) {
			SendMessage(mSTB->getHwnd(), MSG_BATTERY_CHARGING, 0, 0);
		}
		if (!isRecording()&& !isPlaying()) {
			cancelShutdownTimer(TIMER_LOWPOWER_IDX);
			db_msg("xxxxxxxxxx\n");
			CloseTipLabel();
			noWork(false);
		}
#endif
		break;	
	case EVENT_HDMI_PLUGIN:
		{
#ifdef MEDIAPLAYER_READY
			#ifdef EVENT_HDMI
			mEM->hdmiAudioOn(true);
			#endif
			if ( curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW ) {
				mPlayBackPreview->beforeOtherScreen();
			}
#endif
			mRecordPreview->otherScreen(1);
#ifdef MEDIAPLAYER_READY
			if ( curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW ) {
				#ifdef DISPLAY_READY
				mPlayBackPreview->afterOtherScreen(getHdl(0), 1);
				#endif
			}
			mPlayBackPreview->otherScreen(1);
#endif
		}
		break;
	case EVENT_HDMI_PLUGOUT: 
		{
#ifdef MEDIAPLAYER_READY
			#ifdef EVENT_HDMI
			mEM->hdmiAudioOn(false);
			#endif
			if ( curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW ) {
				mPlayBackPreview->beforeOtherScreen();
			}
#endif
			mRecordPreview->otherScreen(0);
#ifdef MEDIAPLAYER_READY
			if ( curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW ) {
				#ifdef DISPLAY_READY
				mPlayBackPreview->afterOtherScreen(getHdl(0), 0);
				#endif
			}
			mPlayBackPreview->otherScreen(0);
#endif
		}
		break;
	case EVENT_TVOUT_PLUG_IN:
		{
			mRecordPreview->otherScreen(2);
		}
		break;
	case EVENT_TVOUT_PLUG_OUT: 
		{
			mRecordPreview->otherScreen(0);
		}
		break;
	}
	return ret;
}

static void connectToPCCallback(HWND hWnd, int id, int nc, DWORD context)
{
	CdrMain* cdrMain;
	cdrMain = (CdrMain*)context;
	ResourceManager* rm;
	const char* ptr;
	HWND hDlg;

	hDlg = GetParent(hWnd);

	rm = ResourceManager::getInstance();
	if(nc == BN_CLICKED) {
		if(id == ID_CONNECT2PC)	{
#ifdef EVENTMANAGER_READY
			if(cdrMain->getUSBStorageMode() == false) {
				db_msg("open usb storage mode\n");
				cdrMain->setUSBStorageMode(true);
				ptr = rm->getLabel(LANG_LABEL_CLOSE_USB_STORAGE_DEVICE);
				if(ptr)
					SetWindowText(hWnd, ptr);
				else {
					db_error("get label failed\n");
				}
			} else if(cdrMain->getUSBStorageMode() == true) {
				db_msg("close usb storage mode, hDlg is 0x%x\n", hDlg);
				cdrMain->setUSBStorageMode(false);	
				ptr = rm->getLabel(LANG_LABEL_OPEN_USB_STORAGE_DEVICE);
				if(ptr)
					SetWindowText(hWnd, ptr);
				else {
					db_error("get label failed\n");
				}
			}
#endif
		} else if(id == ID_CONNECT2PC + 1) {
			db_msg("charge mode, hDlg is 0x%x\n", hDlg);
			SendNotifyMessage(hDlg, MSG_CLOSE_CONNECT2PC_DIALOG, 0, 0);
			if(!cdrMain->isBatteryOnline()) {
				db_msg("battery is charging\n");
				SendMessage(cdrMain->getWindowHandle(WINDOWID_STATUSBAR), MSG_BATTERY_CHARGING, 0, 0);
			}
		}
	}
}

static int connectToPCDialog(HWND hWnd, void* context)
{
	int retval = 0;
	PopUpMenuInfo_t info;
	ResourceManager* rm;
	static bool hasOpened = false;

	if(hasOpened == true) {
		db_info("dialog has been opened\n");
		return 0;
	}

	rm = ResourceManager::getInstance();

	memset(&info, 0, sizeof(info));
	info.pLogFont = rm->getLogFont();
	retval = rm->getResRect(ID_CONNECT2PC, info.rect);
	if(retval < 0) {
		db_error("get connectToPC rect failed\n");	
		return -1;
	}

	retval = rm->getResIntValue(ID_CONNECT2PC, INTVAL_ITEMWIDTH);
	if(retval < 0) {
		db_error("get connectToPC item_width failed");
		return -1;
	}
	info.item_width = retval;

	retval = rm->getResIntValue(ID_CONNECT2PC, INTVAL_ITEMHEIGHT);
	if(retval < 0) {
		db_error("get connectToPC item_height failed");
		return -1;
	}
	info.item_height = retval;

	info.style = POPMENUS_V | POPMENUS_VCENTER;
	info.item_nrs = 2;
	info.id = ID_CONNECT2PC;
	info.name[0] = rm->getLabel(LANG_LABEL_OPEN_USB_STORAGE_DEVICE);
	if(info.name[0] == NULL) {
		db_error("get the LANG_LABEL_OPEN_USB_STORAGE_DEVICE failed\n");
		return -1;
	}
	info.name[1] = rm->getLabel(LANG_LABEL_CHARGE_MODE);
	if(info.name[1] == NULL) {
		db_error("get the LANG_LABEL_CHARGE_MODE failed\n");
		return -1;
	}
	info.callback[0] = connectToPCCallback;
	info.callback[1] = connectToPCCallback;
	info.context = (DWORD)context;

	info.end_key = CDR_KEY_MODE;
	info.flag_end_key = 1;		/* key up to end the dialog */
	info.push_key = CDR_KEY_OK;
	info.table_key = CDR_KEY_RIGHT;

	info.bgc_widget = rm->getResColor(ID_CONNECT2PC, COLOR_BGC);
	info.bgc_item_focus = rm->getResColor(ID_CONNECT2PC, COLOR_BGC_ITEMFOCUS);
	info.bgc_item_normal = rm->getResColor(ID_CONNECT2PC, COLOR_BGC_ITEMNORMAL);
	info.mainc_3dbox = rm->getResColor(ID_CONNECT2PC, COLOR_MAIN3DBOX);

	#ifdef USE_NEWUI
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_CONFIRM, info.btnConfirmPic);
	rm->getResBmp(ID_MENU_LIST_MB, BMPTYPE_CANCEL, info.btnCancelPic);
	#endif

	hasOpened = true;
	retval = PopUpMenu(hWnd, &info);
	db_msg("retval is %d\n", retval);
	hasOpened = false;

#ifdef USE_NEWUI
	if (info.btnConfirmPic.bmBits)
		UnloadBitmap(&info.btnConfirmPic);
	if (info.btnCancelPic.bmBits)
		UnloadBitmap(&info.btnCancelPic);
#endif

	return retval;
}

static void closeConnectToPCDialog(void)
{
	BroadcastMessage(MSG_CLOSE_CONNECT2PC_DIALOG, 0, 0);
}

void CdrMain::msgCallback(int msgType, unsigned int data)
{
	ResourceManager* rm = ResourceManager::getInstance();
	int idx = (int)data;
	switch(msgType) {
	case MSG_RM_LANG_CHANGED:
		{
			PLOGFONT logFont;
			logFont = rm->getLogFont();
			if(logFont == NULL) {
				db_error("invalid log font\n");
				return;
			}
			db_msg("type:%s, charset:%s, family:%s\n", logFont->type, logFont->charset, logFont->family);
			SetWindowFont(mHwnd, logFont);
		}
		break;
	case MSG_RM_SCREENSWITCH:
#ifdef POWERMANAGER_READY
		{
			db_info("MSG_RM_SCREENSWITCH, data is %d, idx is %d\n", data, idx);
			if (idx < 3) {
				if(mPM) {
					mTime = (idx+1)*10;
					mPM->setOffTime(mTime);
				}
			} else {
				if(mPM) {
					mTime = TIME_INFINITE;
					mPM->setOffTime(mTime);
				}
			}
		}
#endif
		break;
	case MSG_CONNECT2PC:
		db_msg("connect2Pc dialog\n");
		if (!isShuttingdown()) {
			connectToPCDialog(mHwnd, this);
		}
		break;
	case MSG_DISCONNECT_FROM_PC:
		db_msg("close connect2Pc dialog\n");
		closeConnectToPCDialog();
#ifdef EVENTMANAGER_READY
		setUSBStorageMode(false);
#endif
		break;
	}
}

void CdrMain::forbidRecord(int type)
{
	mEnableRecord = type;
}

int CdrMain::isEnableRecord()
{
	return mEnableRecord;
}

unsigned int CdrMain::getCurWindowID()
{
	return curWindowState.curWindowID;
}

int CdrMain::createMainWindows(void)
{
	CDR_RECT rect;
	MAINWINCREATE CreateInfo;
	ResourceManager* rm = ResourceManager::getInstance();

	if(RegisterCDRControls() < 0) {
		db_error("register CDR controls failed\n");
		return -1;
	}
	if(RegisterCDRWindows() < 0) {
		db_error("register CDR windows failed\n");
		return -1;
	}

	CreateInfo.dwStyle = WS_VISIBLE;
#ifdef USE_NEWUI
	CreateInfo.dwExStyle = WS_EX_NONE | WS_EX_AUTOSECONDARYDC;
#else
	CreateInfo.dwExStyle = WS_EX_NONE/* | WS_EX_AUTOSECONDARYDC*/;
#endif
	CreateInfo.spCaption = "";
	CreateInfo.hMenu = 0;
	CreateInfo.hCursor = GetSystemCursor(0);
	CreateInfo.hIcon = 0;
	CreateInfo.MainWindowProc = CDRWinProc;

	rm->getResRect(ID_SCREEN, rect);
	CreateInfo.lx = rect.x;
	CreateInfo.ty = rect.y;
	CreateInfo.rx = rect.w;
	CreateInfo.by = rect.h;

	CreateInfo.iBkColor = PIXEL_black;		/* the color key */
	CreateInfo.dwAddData = (DWORD)this;
	CreateInfo.hHosting = HWND_DESKTOP;

	mHwnd = CreateMainWindow(&CreateInfo);
	if (mHwnd == HWND_INVALID) {
		db_error("create Mainwindow failed\n");
		return -1;
	}
	ShowWindow(mHwnd, SW_SHOWNORMAL);
	rm->setHwnd(WINDOWID_MAIN, mHwnd);

	SetWindowBkColor(mHwnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));

	mSTB = new StatusBar(this);
	if(mSTB->getHwnd() == HWND_INVALID) {
		db_error("create StatusBar failed\n");
		return -1;
	}

	if(mRecordPreview->createWindow() < 0) {
		db_error("create RecordPreview Window failed\n");
		return -1;
	}

	windowState_t windowState;
	windowState.curWindowID = WINDOWID_RECORDPREVIEW;
	windowState.RPWindowState = STATUS_RECORDPREVIEW;
	setCurrentWindowState(windowState);
	ShowWindow(mRecordPreview->getHwnd(), SW_SHOWNORMAL);
	initKeySound("/res/others/key.wav");
#if 1
	RegisterKeyMsgHook((void*)this, MsgHook);
#endif

	return 0;
}

int CdrMain::createOtherWindows(void)
{


#ifdef MEDIAPLAYER_READY
	/* create PlayBackPreview */
	mPlayBackPreview = new PlayBackPreview(this);
	if(mPlayBackPreview->getHwnd() == HWND_INVALID) {
		db_error("create PlayBackPreview Window failed\n");
		return -1;
	}
	db_msg("xxxxxxxxxxxxxxxxxxxxDEBUG\n");

	/* create PlayBack */
	mPlayBack = new PlayBack(this);
	if(mPlayBack->getHwnd() == HWND_INVALID) {
		db_error("create PlayBack Window failed\n");
		return -1;
	}
	db_msg("xxxxxxxxxxxxxxxxxxxxDEBUG\n");
#endif
	
	/* CreateMenu */
	mMenu = new Menu(this);
	if(mMenu->getHwnd() == HWND_INVALID) {
		db_error("create PlayBack Window failed\n");
		return -1;
	}
	db_msg("xxxxxxxxxxxxxxxxxxxxDEBUG\n");

	return 0;
}

void CdrMain::msgLoop(void)
{
	MSG Msg;
	while (GetMessage (&Msg, this->mHwnd)) {
		TranslateMessage (&Msg);
		DispatchMessage (&Msg);
	}
}

HWND CdrMain::getWindowHandle(unsigned int windowID)
{
	switch(windowID) {
	case WINDOWID_RECORDPREVIEW:
		return mRecordPreview->getHwnd();
		break;
#ifdef MEDIAPLAYER_READY
	case WINDOWID_PLAYBACKPREVIEW:
		return mPlayBackPreview->getHwnd();
		break;
	case WINDOWID_PLAYBACK:
		return mPlayBack->getHwnd();
		break;
#endif
	case WINDOWID_MENU:
		return mMenu->getHwnd();
		break;
	case WINDOWID_STATUSBAR:
		return mSTB->getHwnd();
		break;
	default:
		db_error("invalid window id: %d\n", windowID);
	}

	return HWND_INVALID;
}

MainWindow* CdrMain::windowID2Window(unsigned int windowID)
{
	switch(windowID) {
	case WINDOWID_RECORDPREVIEW:
		return mRecordPreview;
		break;
#ifdef MEDIAPLAYER_READY
	case WINDOWID_PLAYBACKPREVIEW:
		return mPlayBackPreview;
		break;
	case WINDOWID_PLAYBACK:
		return mPlayBack;
		break;
#endif
	case WINDOWID_MENU:
		return mMenu;
		break;
	case WINDOWID_STATUSBAR:
		return mSTB;
		break;
	default:
		db_error("invalid window id: %d\n", windowID);
	}

	return NULL;
}

void CdrMain::setCurrentWindowState(windowState_t windowState)
{
	curWindowState = windowState;
	SendNotifyMessage(mSTB->getHwnd(), MSG_CDRMAIN_CUR_WINDOW_CHANGED, (WPARAM)&curWindowState, 0);
#ifdef USE_NEWUI
	if (mMenu)
		SendNotifyMessage(mMenu->getHwnd(), MSG_CDRMAIN_CUR_WINDOW_CHANGED, (WPARAM)&curWindowState, 0);
#endif
}

void CdrMain::setCurrentWindowState(RPWindowState_t status)
{
	curWindowState.RPWindowState = status;
	SendNotifyMessage(mSTB->getHwnd(), MSG_CDRMAIN_CUR_WINDOW_CHANGED, (WPARAM)&curWindowState, 0);
#ifdef USE_NEWUI
	if (mMenu)
		SendNotifyMessage(mMenu->getHwnd(), MSG_CDRMAIN_CUR_WINDOW_CHANGED, (WPARAM)&curWindowState, 0);
#endif
}

void CdrMain::setPhotoGraph(int state)
{
	SendMessage(mSTB->getHwnd(), STBM_PHOTO_GRAPH, state, 0);
}

void CdrMain::cleanPlaybackFileIdx()
{
	mPlayBackPreview->setCurIdx(0);
}

void CdrMain::changeWindow(unsigned int windowID)
{
	HWND toHwnd;
	if (isShuttingdown() || isRecording()) {
		return ;
	}
	if ((curWindowState.curWindowID == WINDOWID_RECORDPREVIEW) && isRecording()) {
		return ;
	}
	toHwnd = getWindowHandle(windowID);
	if(toHwnd == HWND_INVALID) {
		db_error("invalid toHwnd\n");	
		return;
	}
#ifdef MEDIAPLAYER_READY
	if ( windowID == WINDOWID_PLAYBACKPREVIEW ) {
		prepareCamera4Playback(false);
	}
#endif
	if( windowID == WINDOWID_RECORDPREVIEW || windowID == WINDOWID_PLAYBACK) {
		//SetWindowBkColor(mHwnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00,0x00));	/* the color key */
	} else {
		//SetWindowBkColor(mHwnd, RGBA2Pixel(HDC_SCREEN, 0x11, 0x11, 0x11, 0xff));	/* the main window color */
	}
#ifdef MEDIAPLAYER_READY
	if( windowID == WINDOWID_PLAYBACK ) {
		ShowWindow(getWindowHandle(WINDOWID_STATUSBAR), SW_HIDE);
	} else if(curWindowState.curWindowID == WINDOWID_PLAYBACK ) {
		ShowWindow(getWindowHandle(WINDOWID_STATUSBAR), SW_SHOWNORMAL);
	}
#endif
	ShowWindow(getWindowHandle(curWindowState.curWindowID), SW_HIDE);
	ShowWindow(toHwnd, SW_SHOWNORMAL);
	SendNotifyMessage(toHwnd, MWM_CHANGE_FROM_WINDOW, curWindowState.curWindowID, 0);
#ifdef MEDIAPLAYER_READY
	if (curWindowState.curWindowID == WINDOWID_PLAYBACKPREVIEW) {
		SendMessage(getWindowHandle(WINDOWID_PLAYBACKPREVIEW), MSG_WINDOW_CHANGED, 0, 0);
	}
	if (curWindowState.curWindowID == WINDOWID_RECORDPREVIEW && windowID == WINDOWID_PLAYBACKPREVIEW)
		cleanPlaybackFileIdx();
#endif
	curWindowState.curWindowID = windowID;
	setCurrentWindowState(curWindowState);	
}

void CdrMain::changeWindow(unsigned int fromID, unsigned int toID)
{
	if(fromID == curWindowState.curWindowID) {
		changeWindow(toID);
	}
}

void CdrMain::keyProc(int keyCode, int isLongPress)
{
	int ret;
	db_msg("key %x %d\n", keyCode, isLongPress);
	MainWindow* curWin;
	if(keyCode == CDR_KEY_POWER) {
		if (isLongPress) {
			CloseTipLabel();
			ret = CdrDialog(mHwnd, CDR_DIALOG_SHUTDOWN);
			if(ret == DIALOG_OK) {
				//				mPM->powerOff();
#ifdef MEDIAPLAYER_READY
				mPlayBack->restoreAlpha();
#endif
				shutdown();
			} else if(ret == DIALOG_CANCEL) {
				db_msg("cancel\n");	
			} else {
				if (curWindowState.curWindowID == WINDOWID_MENU) {
					#ifndef USE_NEWUI
					mMenu->cancelAlphaEffect();
					#endif
				}
				db_msg("ret is %d\n", ret);	
			}
		} else {
			db_msg("xxxxxxxxxx\n");
#ifdef POWERMANAGER_READY
			mPM->screenSwitch();
#endif
		}
		return;
	}

	curWin = windowID2Window(curWindowState.curWindowID);
	db_msg("curWindowID is %d\n", curWindowState.curWindowID);
	if (curWin) {
		db_msg("xxxxxxxxxx\n");
		DispatchKeyEvent(curWin, keyCode, isLongPress);
	}
}

#ifdef DISPLAY_READY
CdrDisplay *CdrMain::getCommonDisp(int disp_id)
{
	return mRecordPreview->getCommonDisp(disp_id);
}

int CdrMain::getHdl(int disp_id)
{
	return mRecordPreview->getHdl(disp_id);
}
#endif

/*
 * DispatchKeyEvent message to mWin
 * */
void CdrMain::DispatchKeyEvent(MainWindow *mWin, int keyCode, int isLongPress)
{
	HWND hWnd;
	unsigned int windowID;

	hWnd = mWin->getHwnd();
	if( hWnd && (hWnd != HWND_INVALID)) {
		windowID = SendMessage(hWnd, MSG_CDR_KEY, keyCode, isLongPress);
		db_msg("windowID is %d\n", windowID);
		if((windowID != curWindowState.curWindowID) )
			changeWindow(windowID);
	}
}

int CdrMain::getTiltAngle()
{
	if(!mEM) {
		return -90;
	}
	return mEM->getTiltAngle();
}

void CdrMain::getCoordinate(float *x, float *y, float *z)
{
	if(mEM) {
		mEM->getCoordinate(x, y, z);
	}
}

void CdrMain::reboot()
{
	mPM->reboot();
}

bool CdrMain::isScreenOn()
{
#ifdef POWERMANAGER_READY
	if (mPM) {
		return mPM->isScreenOn();
	}
#endif
	return true;
}

int CdrMain::screenOn()
{
#ifdef POWERMANAGER_READY
	if (mPM) {
#ifdef POWERMANAGER_ENABLE_DEEPSLEEP
		mPM->pmWakeUp();
#else
                mPM->systemResume();
                mPM->screenOn();
#endif                
	}
#endif
	setOffTime();
	return 0;
}

void CdrMain::setOffTime()
{
#ifdef POWERMANAGER_READY
	if(mPM)
		mPM->setOffTime(mTime);
#endif
}

void CdrMain::configInit()
{
	bool val;
	ResourceManager* rm = ResourceManager::getInstance();

	/*TODO*/
	int time = rm->getResIntValue(ID_MENU_LIST_SS, INTVAL_SUBMENU_INDEX);
	if (time < 3) {
		time = (time+1)*10;
	} else {
		time = TIME_INFINITE;
	}

#ifdef POWERMANAGER_READY
	mPM->setOffTime(time);
	rm->notifyAll();
#endif
//	usleep(1000*1000);
	val = rm->getResBoolValue(ID_MENU_LIST_AWMD);
	if(val == true) {
		db_info("AWMD mode\n");
	} else {
		val = rm->getResBoolValue(ID_MENU_LIST_POR); //power on record
		if ((mRecordPreview->getRecordState() == -1 && val == true) ||
			mRecordPreview->getRecordState() == 1) {
			db_info("POR: power on record\n");
			if (curWindowState.curWindowID == WINDOWID_RECORDPREVIEW) {
				mRecordPreview->waitForStorageMount(6000);
			#ifdef MESSAGE_FRAMEWORK
					CommuReq req;

					db_msg("fwk_msg send start record\n");
					memset(&req, 0x0, sizeof(req));
					req.cmdType = COMMU_TYPE_RECORD;
					req.operType = 1;
					fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
			#else
				mRecordPreview->startRecord();
			#endif
			}
		}
	}
#ifdef WATERMARK_ENABLE
	val = rm->getResBoolValue(ID_MENU_LIST_TWM);
	mRecordPreview->setWaterMark(val);
#endif

#ifdef POWERMANAGER_READY
	/* mPM->adjustCpuFreq(CPU_FREQ_MID); */
#endif
}

void CdrMain::exit()
{
#ifdef EVENTMANAGER_READY
	delete mEM;
	mEM = NULL;
	delete mPM;
	mPM = NULL;
#endif
}

#ifdef MEDIAPLAYER_READY
void CdrMain::prepareCamera4Playback(bool closeLayer)
{
	if(mRecordPreview)
		mRecordPreview->prepareCamera4Playback(closeLayer);
}
#endif

#ifdef EVENTMANAGER_READY
void CdrMain::setUSBStorageMode(bool enable)
{
	if(enable == true) {
		if(usbStorageConnected == false) {
			db_msg("enable USB Storage mode\n");
			if (isRecording()) {
			#ifdef MESSAGE_FRAMEWORK
				CommuReq req;

				db_msg("fwk_msg send stop record\n");
				memset(&req, 0x0, sizeof(req));
				req.cmdType = COMMU_TYPE_RECORD;
				req.operType = 0;
				fwk_send_message_ext(FWK_MOD_VIEW,FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_COMMU_SWITCH_REQ, &req, sizeof(req));
			#else
				mRecordPreview->stopRecord();
			#endif
			}
			mEM->connect2PC();
			usbStorageConnected = true;
		}
	} else {
		if(usbStorageConnected == true)	{
			db_msg("disable USB Storage mode\n");
			mEM->disconnectFromPC();
			//StorageManager *sm = StorageManager::getInstance();
			//sm->dbUpdate();
			usbStorageConnected = false;
		}
	}
}
#endif

static unsigned char *getShutdownLogo()
{
	int fd;
	unsigned char *buf = NULL;
	int buf_size = 512*1024;
	
	fd = open("/dev/shutlogo", O_RDWR);
    if( fd < 0) {
    	db_error("open ifm fail\n");
    }
	buf  = (unsigned char *)malloc(buf_size);
	read(fd, buf, buf_size);
	
	return buf;
}
void CdrMain::showShutdownLogo(void)
{
	HWND hStatusBar, hMenu, hPBP;
#ifdef DISPLAY_READY
	CdrDisplay* cd;
#endif
	unsigned char *buf;

#ifdef DISPLAY_READY 
	cd = getCommonDisp(CAM_CSI);
	if(cd)
		ck_off(cd->getHandle());
	cd = getCommonDisp(CAM_UVC);
	if(cd)
		ck_off(cd->getHandle());
#endif
	hStatusBar = getWindowHandle(WINDOWID_STATUSBAR);
	hMenu = getWindowHandle(WINDOWID_MENU);
#ifdef MEDIAPLAYER_READY
	hPBP = getWindowHandle(WINDOWID_PLAYBACKPREVIEW);
#endif
	ShowWindow(hStatusBar, SW_HIDE);
	SendMessage(hStatusBar, MSG_CLOSE, 0, 0);	//cancel statusbar timer
	ShowWindow(hMenu, SW_HIDE);
	ShowWindow(hPBP, SW_HIDE);
	//showImageWidget(mHwnd, IMAGEWIDGET_POWEROFF);
	buf = getShutdownLogo();
	CloseTipLabel();
	showImageWidget(mHwnd, IMAGEWIDGET_POWEROFF, buf);
	free(buf);
}

bool CdrMain::getUSBStorageMode(void)
{
	return usbStorageConnected;
}

bool CdrMain::isShuttingdown(void)
{
	return mShutdowning;
}

void CdrMain::setSTBRecordVTL(unsigned int vtl)
{
	mSTB->setRecordVTL(vtl);
}

bool CdrMain::getUvcState()
{
#ifdef IGNORE_UVC_STATE
	return false;
#else
	return mRecordPreview->getUvcState();
#endif
}

int CdrMain::notifyWebRtc(unsigned char *pbuf, int size)
{
	return mRecordPreview->onCall(pbuf,size);
}

int CdrMain::take_picture(void)
{
	mRecordPreview->takePicture();
	return 0;
}

int CdrMain::impact_occur(void) 
{
    return mRecordPreview->impactOccur();
}

int CdrMain::setVideoModeWebRTC(int value)
{
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
    if(rm->setResIntValue(ID_MENU_LIST_VQ, INTVAL_SUBMENU_INDEX, value) < 0) {
		db_error("set ID_MENU_LIST_VQ to %d failed\n",value);	
		return -1;
	}
#ifdef USE_NEWUI
	if(value)
	{
		mMenu->updateMenuTextsWithVQ(0, value);
	}
	else
	{
		mMenu->updateMenuTextsWithVQ(1, value);
	}
#endif
	
	return 0;
}

int CdrMain::setPicModeWebRTC(int value)
{
	mRecordPreview->setPictureQuality(value);
	return 0;
}


int  CdrMain::setRecordTimeWebRTC(int value)
{
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
    if(rm->setResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX, value) < 0) {
		db_error("set ID_MENU_LIST_VTL to %d failed\n",value);	
		return -1;
	}
#ifdef USE_NEWUI
	mMenu->updateMenuTextsWithVTL(value);
#endif
	
	return 0;
}

int  CdrMain::setExposureWebRTC(int value)
{
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
    if(rm->setResIntValue(ID_MENU_LIST_EXPOSURE, INTVAL_SUBMENU_INDEX, value) < 0) {
		db_error("set ID_MENU_LIST_EXPOSURE to %d failed\n",value);	
		return -1;
	}
	
	return 0;
}

int  CdrMain::setWhiteBalanceWebRTC(int value)
{
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
    if(rm->setResIntValue(ID_MENU_LIST_WB, INTVAL_SUBMENU_INDEX, value) < 0) {
		db_error("set ID_MENU_LIST_WB to %d failed\n",value);	
		return -1;
	}
	
	return 0;
}

int  CdrMain::setContrastWebRTC(int value)
{
	ResourceManager* rm;
	rm = ResourceManager::getInstance();
    if(rm->setResIntValue(ID_MENU_LIST_CONTRAST, INTVAL_SUBMENU_INDEX, value) < 0) {
		db_error("set ID_MENU_LIST_CONTRAST to %d failed\n",value);	
		return -1;
	}
	
	return 0;
}




int  CdrMain::setGpsWebRTC(int value)
{
	ResourceManager* rm;
	bool newVal = value ? true : false;

#ifdef USE_NEWUI	
	rm = ResourceManager::getInstance();
    if(rm->setResBoolValue(ID_MENU_LIST_GPS, newVal) < 0) {
		db_error("set ID_MENU_LIST_GPS to %d failed\n",newVal);	
		return -1;
	}
#endif

	return 0;
}

int  CdrMain::setCellularWebRTC(int value)
{
	ResourceManager* rm;
	bool newVal = value ? true : false;

#ifdef USE_NEWUI		
	rm = ResourceManager::getInstance();
    if(rm->setResBoolValue(ID_MENU_LIST_CELLULAR, newVal) < 0) {
		db_error("set ID_MENU_LIST_CELLULAR to %d failed\n",newVal);	
		return -1;
	}
#endif

	return 0;
}

int  CdrMain::setTimeWaterMarkWebRTC(int value)
{
	ResourceManager* rm;
	bool newVal = value ? true : false;

#ifdef USE_NEWUI		
	rm = ResourceManager::getInstance();
    if(rm->setResBoolValue(ID_MENU_LIST_TWM, newVal) < 0) {
		db_error("set ID_MENU_LIST_TWM to %d failed\n",newVal);	
		return -1;
	}
#endif

	return 0;
}

int  CdrMain::setAWMDWebRTC(int value)
{
	ResourceManager* rm;
	bool newVal = value ? true : false;

#ifdef USE_NEWUI		
	rm = ResourceManager::getInstance();
    if(rm->setResBoolValue(ID_MENU_LIST_AWMD, newVal) < 0) {
		db_error("set ID_MENU_LIST_AWMD to %d failed\n",newVal);	
		return -1;
	}
#endif

#ifdef REMOTE_3G
	report_alarm_time = 0;
#endif
	return 0;
}

int  CdrMain::setFormatWebRTC(void)
{
	StorageManager *sm = StorageManager::getInstance();
	
	if(sm->isInsert() == false) {
		db_msg("tfcard not mount\n");
		return -1;
	}
	mMenu->doFormatTFCard();


	return 0;
}

bool  CdrMain::isCardInsert(void)
{
	StorageManager *sm = StorageManager::getInstance();
	
	if(sm->isInsert() == false) {
		db_msg("tfcard not mount\n");
		return false;
	}

	return true;
}


int  CdrMain::setFactoryResetWebRTC(void)
{
	
	mMenu->doFactoryReset();

    mFactoryReseting = true;
#ifdef USE_NEWUI
	reboot();
#endif

	return 0;
}	

int  CdrMain::setRecordWebRTC(int value)
{
	bool newVal = value ? true : false;

	if(newVal)
	{
		if(mRecordPreview->isRecording())
		{
			return 0;
		}
		else
		{
			mRecordPreview->startRecord();
		}
	}
	else
	{
		if(mRecordPreview->isRecording())
		{
			mRecordPreview->stopRecord();
		}
		else
		{
			return 0;
		}
	}

	return 0;
}

int  CdrMain::setSoundWebRTC(int value)
{
	bool newVal = value ? true : false;
	int result;
	
	result = mRecordPreview->getVoiceStatus();
	if(-1 == result)
	{
		return -1;
	}
	
	if(newVal)
	{
		if(result)
		{
			return 0;
		}
		else
		{
			mRecordPreview->switchVoice();
		}
		
	}
	else
	{
		if(result)
		{
			mRecordPreview->switchVoice();
		}
		else
		{
			return 0;
		}
	}
		
	return 0;
}



int  CdrMain::getSettingsWebRTC(WRTC_SETTINGS* pSettings)
{
	ResourceManager* rm;
	int temp=0;
	int result = 0;
	date_t dt;

	if(NULL == pSettings)
		return -1;

	rm = ResourceManager::getInstance();
	strncpy(pSettings->szName,WRTC_CMD_GET_ALL_PARAMS, WRTC_MAX_NAME_LEN);

	
	/*Resolutionl*/
	strncpy(pSettings->items[0].itemName, WRTC_CMD_SET_RESOLUTION, WRTC_MAX_NAME_LEN);
	pSettings->items[0].valueType = 1;
	temp = rm->getResIntValue(ID_MENU_LIST_VQ, INTVAL_SUBMENU_INDEX);
	if(0 == temp)
	{
		strncpy(pSettings->items[0].valueStr, WRTC_VALUE_RESOLUTION_1080P, WRTC_MAX_VALUE_LEN);
	}
	else if(1 == temp)
	{
		strncpy(pSettings->items[0].valueStr, WRTC_VALUE_RESOLUTION_720P, WRTC_MAX_VALUE_LEN);
	}
	
	/*RecordTime*/
	strncpy(pSettings->items[1].itemName, WRTC_CMD_SET_RECORD_TIME, WRTC_MAX_NAME_LEN);
	pSettings->items[1].valueType = 0;
	temp = rm->getResIntValue(ID_MENU_LIST_VTL, INTVAL_SUBMENU_INDEX);
	pSettings->items[1].value = (temp+1)*60;
	
	/*Exposure*/
	strncpy(pSettings->items[2].itemName, WRTC_CMD_SET_EXPOSURE, WRTC_MAX_NAME_LEN);
	pSettings->items[2].valueType = 0;
	temp = rm->getResIntValue(ID_MENU_LIST_EXPOSURE, INTVAL_SUBMENU_INDEX);
	pSettings->items[2].value = temp-3;
	
	/*WhiteBalance*/
	strncpy(pSettings->items[3].itemName, WRTC_CMD_SET_WB, WRTC_MAX_NAME_LEN);
	pSettings->items[3].valueType = 1;
	temp = rm->getResIntValue(ID_MENU_LIST_WB, INTVAL_SUBMENU_INDEX);
	if(0 == temp)
	{
		strncpy(pSettings->items[3].valueStr, WRTC_VALUE_WB_AUTO, WRTC_MAX_VALUE_LEN);
	}
	else if(1 == temp)
	{
		strncpy(pSettings->items[3].valueStr, WRTC_VALUE_WB_DAYLIGHT, WRTC_MAX_VALUE_LEN);
	}
	else if(2 == temp)
	{
		strncpy(pSettings->items[3].valueStr, WRTC_VALUE_WB_CLOUDY, WRTC_MAX_VALUE_LEN);
	}
	else if(3 == temp)
	{
		strncpy(pSettings->items[3].valueStr, WRTC_VALUE_WB_INCANDESCENT, WRTC_MAX_VALUE_LEN);
	}
	else if(4 == temp)
	{
		strncpy(pSettings->items[3].valueStr, WRTC_VALUE_WB_FLUORESCENT, WRTC_MAX_VALUE_LEN);
	}
	
	/*Contrast*/
	strncpy(pSettings->items[4].itemName, WRTC_CMD_SET_CONTRAST, WRTC_MAX_NAME_LEN);
	pSettings->items[4].valueType = 0;
	temp = rm->getResIntValue(ID_MENU_LIST_CONTRAST, INTVAL_SUBMENU_INDEX);
	pSettings->items[4].value = temp;
	
	/*WaterMark*/
	strncpy(pSettings->items[5].itemName, WRTC_CMD_SET_WATERMARK, WRTC_MAX_NAME_LEN);
	pSettings->items[5].valueType = 2;
	pSettings->items[5].bValue = rm->getResBoolValue(ID_MENU_LIST_TWM);
	
	/*Moving*/
	strncpy(pSettings->items[6].itemName, WRTC_CMD_SET_MOVING, WRTC_MAX_NAME_LEN);
	pSettings->items[6].valueType = 2;
	pSettings->items[6].bValue = rm->getResBoolValue(ID_MENU_LIST_AWMD);
	
	/*Gps*/
	strncpy(pSettings->items[7].itemName, WRTC_CMD_SET_GPS, WRTC_MAX_NAME_LEN);
	pSettings->items[7].valueType = 2;
	pSettings->items[7].bValue = rm->getResBoolValue(ID_MENU_LIST_GPS);
	
	/*Mobile*/
	strncpy(pSettings->items[8].itemName, WRTC_CMD_SET_MOBILE, WRTC_MAX_NAME_LEN);
	pSettings->items[8].valueType = 2;
	pSettings->items[8].bValue = rm->getResBoolValue(ID_MENU_LIST_CELLULAR);

	/*Voice*/
	strncpy(pSettings->items[9].itemName, WRTC_CMD_SET_SOUND, WRTC_MAX_NAME_LEN);
	pSettings->items[9].valueType = 2;
	result = mRecordPreview->getVoiceStatus();
	if(-1 == result)
	{
		pSettings->items[9].bValue = false;
	}
	else
	{
		if(result)
		{
			pSettings->items[9].bValue = true;
		}
		else
		{
			pSettings->items[9].bValue = false;
		}
	}

	/*record*/
	strncpy(pSettings->items[10].itemName, WRTC_CMD_SET_RECORD, WRTC_MAX_NAME_LEN);
	pSettings->items[10].valueType = 2;
	if(mRecordPreview->isRecording())
	{
		pSettings->items[10].bValue = true;
	}
	else
	{
		pSettings->items[10].bValue = false;
	}

	/*date*/
	memset(&dt, 0x0, sizeof(dt));
	getSystemDate(&dt);
	strncpy(pSettings->items[11].itemName, WRTC_CMD_SET_DATE_TIME, WRTC_MAX_NAME_LEN);
	pSettings->items[11].valueType = 3;
	pSettings->items[11].dt.year = dt.year;
	pSettings->items[11].dt.month = dt.month;
	pSettings->items[11].dt.day = dt.day;
	pSettings->items[11].dt.hour = dt.hour;
	pSettings->items[11].dt.minute = dt.minute;
	pSettings->items[11].dt.second = dt.second;
	

	pSettings->nCount = 12;
	
	return 0;
}


int CdrMain::getVideoDuration(const char* path)
{
	return mRecordPreview->getMP4Duration(path);
}

void CdrMain::clearSnapshotPath(void)
{
	mRecordPreview->clearSnapShotLocation();
}

char* CdrMain::getSnapshotPath(void)
{
	return mRecordPreview->getSnapShotLocation();
}


bool CdrMain::isFrontCamExist(void)
{
	return mRecordPreview->isCameraExist(CAM_CSI);
}

bool CdrMain::isBackCamExist(void)
{
	return mRecordPreview->isCameraExist(CAM_UVC);
}

HerbCamera* CdrMain::getHerbCamera(int cam_type)
{
	return mRecordPreview->getHerbCameraPointer(cam_type);
}

void CdrMain::setCameraCallback(int cam_type)
{
	mRecordPreview->setPreviewcallback(cam_type);
	return;
}




int CDRMain(int argc, const char *argv[], CdrMain* cdrMain)
{
	struct timespec measureTime1;
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: int CDRMain time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	if(cdrMain->createMainWindows() < 0) {
		delete cdrMain;
		return -1;
	}
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: after createMainWindows time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	cdrMain->initStage2(NULL);
	clock_gettime(CLOCK_MONOTONIC, &measureTime1);
	db_msg("Test Time: after initStage2 time is %ld secs, %ld nsecs\n", measureTime1.tv_sec, measureTime1.tv_nsec);

	cdrMain->msgLoop();

	delete cdrMain;
	return 0;
}

#ifdef _MGRM_THREADS
#include <minigui/dti.c>
#endif
