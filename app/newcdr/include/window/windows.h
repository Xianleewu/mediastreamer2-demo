#ifndef _WINDOWS_H
#define _WINDOWS_H


#ifdef MEDIAPLAYER_READY
#include <CedarMediaPlayer.h>
#endif

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>

#include "RecordPreview.h"
#include "CdrCamera.h"
#include "CdrMediaRecorder.h"
#include "PlayBack.h"

#ifdef POWERMANAGER_READY
#include "PowerManager.h"
#endif
#include "StorageManager.h"
#include "resourceManager.h"
#include "cdr_widgets.h"
#ifdef EVENTMANAGER_READY
#include "EventManager.h"
#endif
#include "PicObj.h"
#include <CdrDisplay.h>
#include "RemoteBuffer.h"

#include "fwk_gl_def.h"

using namespace rockchip;

enum playerState {
	STATE_ERROR					= -1,
	STATE_IDLE					= 0,
	STATE_PREPARING				= 1,
	STATE_PREPARED				= 2,
	STATE_STARTED				= 3,
	STATE_PAUSED				= 4,
	STATE_PLAYBACK_COMPLETED	= 5,
	STATE_STOPPED				= 6,
	STATE_END					= 7
};

enum {
	FORBID_NORMAL = 0,
	FORBID_FORMAT,
};

#define WINDOW_STATUSBAR		"CDRStatusBar"
#define WINDOW_RECORDPREVIEW	"RecordPreview"
#define WINDOW_MENU				"CDRMenu"
#define WINDOW_PLAYBACKPREVIEW	"PlayBkPreview"
#define WINDOW_PLAYBACK			"PlayBack"



extern int RegisterCDRWindows(void);
extern void UnRegisterCDRWindows(void);

#ifdef USE_NEWUI
typedef enum {
	MENU_RECORDPREVIEW = 1,
	MENU_PHOTOGRAPH = 2,
	MENU_PLAYBACKPREVIEW = 3
}MenuWindow_t;
#endif

typedef enum {
	STATUS_RECORDPREVIEW = 1,
	STATUS_PHOTOGRAPH = 2,
	STATUS_AWMD	= 3
}RPWindowState_t;

typedef struct {
	unsigned int curWindowID;
	RPWindowState_t RPWindowState;
}windowState_t;

class CdrMain;
class PowerManager;
class BufferUsedCnt;
class MainWindow
{
public:
	MainWindow(){mHwnd = 0;}
	~MainWindow(){};
	HWND getHwnd() {
		return mHwnd;
	}

	void updateWindowFont(void)
	{
		HWND hChild;

		hChild = GetNextChild(mHwnd, 0);
		while( hChild && (hChild != HWND_INVALID) )
		{
			SetWindowFont(hChild, GetWindowFont(mHwnd));
			hChild = GetNextChild(mHwnd, hChild);
		}
	}
	void showWindow(int cmd) {
		ShowWindow(mHwnd, cmd);
	}
	void keyProc(int keyCode, int isLongPress);
protected:
	HWND mHwnd;
};

class StatusBar : public MainWindow
{
public:
	StatusBar(CdrMain* cdrMain);
	~StatusBar();

	void msgCallback(int msgType, unsigned int data);
	void handleWindowPicEvent(windowState_t *windowState);
	int createSTBWidgets(HWND hParent);
	void updateTime(HWND hStatusBar);
	void updateRecordTime(HWND hStatusBar);
	void increaseRecordTime();
	int initPic(PicObj *po);
	int handleRecordTimerMsg(unsigned int message, unsigned int recordType);
	void setRecordVTL(unsigned int vtl);
	unsigned int getCurrenWindowId();
#ifdef USE_NEWUI
	PBITMAP getBgBmp();
#endif
private:
	Vector <PicObj*> mPic;
	#ifdef USE_NEWUI
	BITMAP mBgBmp;
	#endif
	CdrMain* mCdrMain;
	DWORD  mRecordTime;
	DWORD  mRecordVTL;
	DWORD  mCurRecordVTL;
	unsigned int mCurWindowID;
};
class RecordPreview : public MainWindow
	, public RecordListener
	, public ErrorListener
	, public CdrDisplay::DisplayCallback
	, public RemoteControl
{
private:
	bool SdcardState;
	bool isThumb;

	ImageQuality_t pictuQuality;
	ImageQuality_t thumbQuality;
public:
	RecordPreview(CdrMain* cdrMain);
	~RecordPreview();
	void init();
	int createWindow();
	virtual void recordListener(CdrMediaRecorder* cmr, int what, int extra);
	virtual void errorListener(CdrMediaRecorder* cmr, int what, int extra);
	int startRecord();
	int startRecord(int cam_type);
	int startRecord(int cam_type, CdrRecordParam_t param);
	int stopRecord(int cam_type);
	void stopRecord();
	int getRecordState();
	int setRecordState(int state);
	void waitForStorageMount(int timeout_ms);

	void setPictureQuality(int quality);
	
	void startPreview();
	void startPreview(int cam_type);
	void stopPreview(int cam_type);
	void stopPreview();

	void initCamera(int cam_type);
	void previewRelease(int cam_type);

	bool isRecording(int cam_type);
	bool isRecording();
	bool isPreviewing(int cam_type);
	bool isPreviewing();
#ifdef USE_NEWUI
	int startRecordTimer(unsigned int recordType);
	int stopRecordTimer();
	void updateRecordTime(HWND hWnd);
	void increaseRecordTime();
	int createWidgets(HWND hParent);
	int destroyWidgets();
#endif
#ifdef WATERMARK_ENABLE
	void setWaterMark(bool val);
	void updatePreviewWaterMarkInfo();
#endif
#ifdef DISPLAY_READY
	CdrDisplay *getCommonDisp(int disp_id);
	void showFrame(camera_preview_frame_buffer_t *data, int id);
	void returnFrame(struct PreviewBuffer *pBuf);
#endif
	void call(unsigned char *pbuf, int size);
	String8 callSync(unsigned char *pbuf, int size);
	int onCall(unsigned char *pbuf, int size);
	void releaseBuffer(unsigned char *pbuf, int size);
	void releaseBuffer(int camId, int index);
	void waitPreviewBufferReleased(int camId);

	int keyProc(int keyCode, int isLongPress);
	void msgCallback(int msgType, int idx);

	void setCamExist(int cam_type, bool bExist);
	bool isCameraExist(int cam_type);
	bool isCamExist(int cam_type);
	void feedAwmdTimer(int seconds);
	int queryAwmdExpirationTime(time_t* sec, long* nsec);
	void updateAWMDWindowPic();
	void savePicture(void* data, int size, int id);
	int impactOccur();
    void prepareCamera4Playback(bool closeLayer=true);
	void restorePreviewOutSide(unsigned int windowID);
	bool getPreviewStatus();

	void switchVoice();
	int getVoiceStatus(void);
	void setSdcardState(bool bExist);
	void otherScreen(int screen);
	void storagePlug(bool plugin);
	bool hasTVD(bool &hasTVD);
	bool needDelayStartAWMD(void);	/* set flag, delay some time after stopRecord, when awmd is open */
	void setDelayStartAWMD(bool value);
	void restoreZOrder();
	unsigned int mAwmdDelayCount;
	int getHdl(int disp_id);
	bool getUvcState();
	void takePicFinish(int cam_id);
	void add2Db(int cameraID);
	void takePicture();
	char* getSnapShotLocation(void);
	int getMP4Duration(const char * path);
	void clearSnapShotLocation(void);
	HerbCamera*  getHerbCameraPointer(int cam_type);
	void setPreviewcallback(int cam_type);
#ifdef WATERMARK_ENABLE
	class UpdatePreviewWaterMarkThread : public Thread
	{
	public:
		UpdatePreviewWaterMarkThread(RecordPreview* pContext) 
			: Thread(false),
			  mWindowContext(pContext) {
		}
		~UpdatePreviewWaterMarkThread(){}
		virtual bool threadLoop() {
			mWindowContext->updatePreviewWaterMarkInfo();	
			return true;
		}

		status_t start() {
			return run("updatePreviewWaterMarkInfo", PRIORITY_NORMAL);
		}

	private:
		RecordPreview* mWindowContext;
	};

#endif
private:
	void lockFile();
	int prepareRecord(int cam_type, CdrRecordParam_t param);


	void recordRelease(int cam_type);
	void recordRelease();

	void previewRelease();

	void setSilent(bool mode);
	void setVideoQuality(int idx);
	void setPicQuality(int idx);
	void setRecordTime(int idx);
	void setAWMD(bool val);
#ifdef WATERMARK_ENABLE
	//int createWatermarkWidgets();
#endif
	void switchAWMD();
	void switchAWMD(bool enable);
	void transformPIP(pipMode_t newMode);
#ifdef USE_NEWUI
	void switchVideoMode(int idx);
#endif
	void setDeleteFlag(int cam_type, String8 file);
	void clrDeleteFlag(int cam_type, String8 file);
	String8 getDeleteFlag(int cam_type);

	CdrMain *mCdrMain;
	CdrCamera *mCamera[CAM_CNT];
	CdrMediaRecorder *mRecorder[CAM_CNT];

	Mutex mLock;
	Mutex mLockPic;
	Size mVideoSize;
	RPWindowState_t mRPWindowState, mOldRPWindowState;
	pipMode_t mPIPMode, oldPIPMode;
	recordDBInfo_t mRecordDBInfo[CAM_CNT];
	recordDBInfo_t mExtraDBInfo[CAM_CNT];
	int mDuration;
	bool mCamExist[CAM_CNT];
	String8 mNeedDelete[CAM_CNT];
	CDR_RECT rectBig, rectSmall;
	bool mUvcPlugFlag;
	Mutex mImpactLock;
	Mutex mRecordLock;
	Mutex mDeleteLock;
	bool mLockFlag;
	int mImpactFlag[CAM_CNT];
	String8 mBakFileString[CAM_CNT];
	time_t mBakNow[CAM_CNT];
	bool mHasTVD;
	int mUVCNode;
	bool mAwmdDelayStart;
	Mutex mAwmdLock;
	bool mSilent;
	bool mAllowTakePic[CAM_CNT];
#ifdef WATERMARK_ENABLE
	bool mNeedWaterMark;
	sp <UpdatePreviewWaterMarkThread> mUpdatePreviewWMT;
#endif
#ifdef USE_NEWUI
	DWORD  mRecordTime;
	DWORD  mRecordNowTime;
	DWORD  mRecordLastTime;
	DWORD  mRecordVTL;
	DWORD  mCurRecordVTL;
	BITMAP mDotPic;
	PLOGFONT mLogFont;
	bool   mIsPreview;
#endif
	sp <RemoteBuffer> mRemoteBuffer;
	Mutex mBufferlock;
	Vector<BufferUsedCnt *> mBufferQueue;
	Mutex mSnapShotbuf;
	char snapshot_location[WRTC_MAX_FILE_NAME_LEN+1];
};

#ifdef MEDIAPLAYER_READY
class PlayBackPreview : public MainWindow
{
public:
	PlayBackPreview(CdrMain* pCdrMain);
	~PlayBackPreview();

	int keyProc(int keyCode, int isLongPress);
	int createSubWidgets(HWND hWnd);
#ifdef USE_NEWUI
	int destroyWidgets();
#endif
	int getFileInfo(void);
	int updatePreviewFilelist(void);
	void showThumnail();
	void showThumnailByThread();
	int getPicFileName(String8 &picFile);
	int getBmpPicFromVideo(const char* videoFile, const char* picFile);
	bool isCurrentVideoFile(void);
	void nextPreviewFile();
	void nextPagePreviewFile();
	void prevPreviewFile();
	void prevPagePreviewFile();
	void getCurrentFileName(String8 &file);
	void getCurrentFileInfo(PLBPreviewFileInfo_t &info);
	void setCurIdx(int idx);
	void showPlaybackPreview();
	void deleteFileDialog();
	void showBlank();
	void otherScreen(int screen);
	int beforeOtherScreen();
	int afterOtherScreen(int layer, int screen);
	class ShowPlayBackThumbThread : public Thread
	{
	public:
		ShowPlayBackThumbThread(PlayBackPreview* pContext) 
			: Thread(false),
			  mWindowContext(pContext) {
		}
		~ShowPlayBackThumbThread(){}
		virtual bool threadLoop() {
			mWindowContext->showThumnailByThread();	
			return false;
		}

		status_t start() {
			return run("ShowPlayBackThumb", PRIORITY_NORMAL);
		}

	private:
		PlayBackPreview* mWindowContext;
	};



private:
	CdrMain* mCdrMain;
	BITMAP bmpIcon;
	BITMAP bmpImage;
	PLBPreviewFileInfo_t fileInfo;
	int playBackVideo();
	int mDispScreen;
	CDR_RECT mDispRect;
#ifdef USE_NEWUI
	PLOGFONT mLogFont;
#endif
	sp <ShowPlayBackThumbThread> mShowPlayBackThumbT;
};

class PlayBack : public MainWindow
{
public:
	PlayBack(CdrMain *cdrMain);
	~PlayBack();
	int createSubWidgets(HWND hWnd);
	int keyProc(int keyCode, int isLongPress);
	void setDisplay(int hlay);
	int preparePlay(String8 filePath);
	int PreparePlayBack(String8 filePath);
	int startPlayBack();
	int initPlayBack();
	void start();
	void pause();
	void seek(int msec);
	void stop();
	void release();
	void reset();
	playerState getCurrentState();
	void initProgressBar(void);
	void resetProgressBar(void);
	void updateProgressBar(void);
	bool isStarted();
	void noWork(bool idle);
	void prepareCamera4Playback();
	int stopPlayback(bool bRestoreAlpha=true);
	int restoreAlpha();
	class PlayBackListener : public CedarMediaPlayer::OnPreparedListener
							, public CedarMediaPlayer::OnCompletionListener
							, public CedarMediaPlayer::OnErrorListener
							, public CedarMediaPlayer::OnVideoSizeChangedListener
							, public CedarMediaPlayer::OnInfoListener
							, public CedarMediaPlayer::OnSeekCompleteListener
							, public CedarMediaPlayer::OnDecodeCompleteListener
	{
	public:
		PlayBackListener(PlayBack *pb){mPB = pb;};
		void onPrepared(CedarMediaPlayer *pMp);
		void onCompletion(CedarMediaPlayer *pMp);
		bool onError(CedarMediaPlayer *pMp, int what, int extra);
		void onVideoSizeChanged(CedarMediaPlayer *pMp, int width, int height);
		bool onInfo(CedarMediaPlayer *pMp, int what, int extra);
		void onSeekComplete(CedarMediaPlayer *pMp);
		void onDecodeComplete(CedarMediaPlayer *pMp);
	private:
		PlayBack *mPB;
	};
public:
	playerState mCurrentState;
	playerState mTargetState;
	/* if user send the msg MSG_PLB_COMPLETE with wParam 0, 
	 * then mStopFlag will be set to 1, indicate onCompletion not send MSG_PLB_COMPLETE again*/
	unsigned int mStopFlag;
	Mutex mDispLock;
private:
	CdrMain *mCdrMain;
//#ifdef DISPLAY_READY
	CdrDisplay *mCdrDisp;
//#endif
	CedarMediaPlayer *mCMP;
	PlayBackListener *mPlayBackListener;
	BITMAP bmpIcon;
	BITMAP bmpProgressBar;

};

#endif

#ifndef USE_NEWUI
class Menu : public MainWindow , public CommonListener
{
public:
	Menu(CdrMain* pCdrMain);
	~Menu();

	int createSubWidgets(HWND hWnd);
	int createMenuListContents(HWND hWnd);

	int keyProc(int keyCode, int isLongPress);
	int updateNewSelected(int oldSel, int newSel);

	int ShowSubMenu(unsigned int menuIndex, bool &isModified);

	int HandleSubMenuChange(unsigned int menuIndex, int newSel);
	int HandleSubMenuChange(unsigned int menuIndex, bool newValue);

	int updateMenuTexts(void);
	int updateSwitchIcons(void);

	int ShowMessageBox(unsigned int menuIndex);
	int ShowAPSettingDialog();
	int ShowDateDialog();
	int ShowFormattingTip();

	void doFormatTFCard(void);

	void doFactoryReset(void);
	void finish(int what, int extra);
	void cancelAlphaEffect(void);
private:
	int getMenuListAttr(menuListAttr_t &attr);
	int getItemImages(MENULISTITEMINFO*  mlii);
	int getItemStrings(MENULISTITEMINFO* mlii);
	int getFirstValueStrings(MENULISTITEMINFO *mlii, int menuIndex);
	int getFirstValueImages(MENULISTITEMINFO* mlii, int menuIndex);
	int getSecondValueImages(MENULISTITEMINFO* mlii, MLFlags* mlFlags);

	int getSubMenuData(unsigned subMenuIndex, subMenuData_t &subMenuData);
	int getCheckBoxStatus(unsigned int menuIndex, bool &isChecked);
	int getMessageBoxData(unsigned int menuIndex, MessageBox_t &messageBoxData);

	void showAlphaEffect(void);
	CdrMain* mCdrMain;
	BITMAP *itemImageArray;
	BITMAP *value1ImageArray;
	BITMAP unfoldImageLight;
	BITMAP unfoldImageDark;
};
#else
class Menu : public MainWindow  , public CommonListener
{
public:
	Menu(CdrMain* pCdrMain);
	~Menu();

	int createSubWidgets(HWND hWnd);
	int createMenuBar(HWND hWnd);
	int createMenulists(HWND hWnd, enum ResourceID* pResID, int count);
	void updateMenuListsTexts(void);
	int updateMenuTextsWithVQ(int oldSel, int newSel);
	int updateMenuTextsWithVTL(int newSel);
	int updateMenuTextsWithVideoSettings(int newSel, enum ResourceID resID);
	void handleWindowChanged(windowState_t *windowState);
	int keyProc(int keyCode, int isLongPress);
	void doFormatTFCard(void);
	PBITMAP getBgBmp();
	PBITMAP getLightBgBmp();
	PBITMAP getVerticalLineBmp();
	PBITMAP getMlBgBmp();
	PBITMAP getHorizontalLineBmp();
	PBITMAP getMlLightBmp();
	int getCurActiveIcon();
	int getCurIconCount();
	MenuWindow_t getCurMenuWindow();
	void updateActiveWindowFont();
	int ShowFormattingTip();
	void doFactoryReset(void);
private:
	int getMenuListAttr(menuListAttr_t &attr, enum ResourceID resID);
	int getMenuContentCount(enum ResourceID resID);
	int getItemStrings(enum ResourceID resID, MENULISTITEMINFO* mlii);
	int createMenuListContents(enum ResourceID resID, HWND hMenuList);
	int getSubMenuData(enum ResourceID resID, subMenuData_t &subMenuData);
	int ShowSubMenu(HWND hMenuList, enum ResourceID resID);
	int ShowAPSettingDialog(void);
	int ShowDateDialog(void);
	void finish(int what, int extra);
	int updateMenuTexts(enum ResourceID *pMlResourceID, int mlCount);
	int getMessageBoxData(enum ResourceID resID, MessageBox_t &messageBoxData);
	int ShowMessageBox(enum ResourceID resID);

	Vector <PicObj*> mIconPic;
	CdrMain* mCdrMain;
	BITMAP mBgBmp;
	BITMAP mLightBg;
	BITMAP mVerticalLine;
	BITMAP mMlBgPic;
	BITMAP mHorizontalLine;
	BITMAP mMlLightBg;
	BITMAP mMlSelectIcon;
	MenuWindow_t mCurMenuWindow;
	unsigned int mCurActiveIcon;
	unsigned int mCurIconCount;
};
#endif

#define TIMER_LOWPOWER_IDX 0
#define TIMER_NOWORK_IDX 1
#define TIMER_NOWORK_IDX2 2

class CdrMain : public MainWindow
                , EventListener
{
public:
	CdrMain();
	~CdrMain();
	void initPreview(void* none);
	void initStage2(void* none);
	void msgCallback(int msgType, unsigned int data);
	void exit();
	int createMainWindows(void);
	int createOtherWindows(void);
	void msgLoop();
	HWND getWindowHandle(unsigned int windowID);
	MainWindow* windowID2Window(unsigned int windowID);
	void setCurrentWindowState(windowState_t windowState);
	void setCurrentWindowState(RPWindowState_t status);
	void setPhotoGraph(int state);
	void cleanPlaybackFileIdx();
	void changeWindow(unsigned int windowID);
	void changeWindow(unsigned int fromID, unsigned int toID);
	void DispatchKeyEvent(MainWindow *mWin, int keyCode, int isLongPress);
	void keyProc(int keyCode, int isLongPress);
	int getTiltAngle();
	void getCoordinate(float *x, float *y, float *z);
#ifdef DISPLAY_READY
	CdrDisplay *getCommonDisp(int disp_id);
#endif
	void configInit();
	int notify(int message, int data);
	void forbidRecord(int val);
	int isEnableRecord();
	unsigned int getCurWindowID();
	void prepareCamera4Playback(bool closeLayer=true);
	void reboot();
	bool isScreenOn();
	int  screenOn();
	void setOffTime();
	void setUSBStorageMode(bool enable);
	bool getUSBStorageMode();
	void startPreview();
	void stopPreview();
	bool isPreviewing();
	void shutdown();
	void readyToShutdown();
	void noWork(bool idle);
	bool isRecording();
	bool isPlaying();
	bool isBatteryOnline();
	void showShutdownLogo();
	bool isShuttingdown();
	bool mShutdowning;
	void setSTBRecordVTL(unsigned int vtl);
	int getHdl(int disp_id);
	bool getUvcState();
	int notifyWebRtc(unsigned char *pbuf, int size);
	int take_picture(void);
    int impact_occur(void);
	
	int setPicModeWebRTC(int value);
	int setVideoModeWebRTC(int value);
	int setRecordTimeWebRTC(int value);
	int  setExposureWebRTC(int value);
	int  setWhiteBalanceWebRTC(int value);
	int  setContrastWebRTC(int value);
	int  setGpsWebRTC(int value);
	int  setCellularWebRTC(int value);
	int  setTimeWaterMarkWebRTC(int value);
	int  setAWMDWebRTC(int value);
	int  setFormatWebRTC(void);
	int  setFactoryResetWebRTC(void);
	int  getSettingsWebRTC(WRTC_SETTINGS* pSettings);
	int  setRecordWebRTC(int value);
	int  setSoundWebRTC(int value);
	int  getVideoDuration(const char* path);
	void clearSnapshotPath(void);
	char* getSnapshotPath(void);
	bool  isCardInsert(void);
	bool isFrontCamExist(void);
	bool isBackCamExist(void);
	HerbCamera* getHerbCamera(int cam_type);
	void setCameraCallback(int cam_type);
	
	class InitPreviewThread : public Thread
	{
	public:
		InitPreviewThread(CdrMain* pContext) 
			: Thread(false),
			  mWindowContext(pContext) {
		}
		~InitPreviewThread(){}
		virtual bool threadLoop() {
			mWindowContext->initPreview();	
			return false;
		}

		status_t start() {
			return run("InitPreview", PRIORITY_NORMAL);
		}

	private:
		CdrMain* mWindowContext;
	};

	class InitStage2Thread : public Thread
	{
	public:
		InitStage2Thread(CdrMain* pContext) 
			: Thread(false),
			  mWindowContext(pContext) {
		}
		~InitStage2Thread(){}
		virtual bool threadLoop() {
			mWindowContext->initStage2();	
			return false;
		}

		status_t start() {
			return run("InitStage2", PRIORITY_NORMAL);
		}

	private:
		CdrMain* mWindowContext;
	};

private:
	void initTimerData();
	void startShutdownTimer(int idx);
	void cancelShutdownTimer(int idx);
	void stopShutdownTimer(int idx);
	int initPreview();
	int initStage2();
	StatusBar *mSTB;
	RecordPreview *mRecordPreview;
#ifdef MEDIAPLAYER_READY
	PlayBackPreview* mPlayBackPreview;
	PlayBack *mPlayBack;
#endif
	Menu* mMenu;
	windowState_t curWindowState;
	sp <InitPreviewThread> mInitPT;
	sp <InitStage2Thread> mInitStage2T;
	PowerManager *mPM;
#ifdef EVENTMANAGER_READY
	EventManager *mEM;
#endif
	bool mEnableRecord;
	bool usbStorageConnected;
	
	unsigned int mTime;
	Condition  mPOR;
	Mutex mLock;
	bool mFactoryReseting;
public:
	
	WPARAM downKey;
	bool isKeyUp;
	bool isLongPress;
	bool winCreateFinish;

};

class PowerManager
{
public:
	PowerManager(CdrMain *cm);
	~PowerManager();

	void setOffTime(unsigned int time);
	void pulse();
	void powerOff();
	int screenOff();
	int screenOn();

	int readyToOffScreen();
	void screenSwitch();
	int setBatteryLevel(int level);
	int getBatteryLevel(void);
	int backlightOff();
	int backlightOn();
	void runReboot();
	void reboot();
	bool isScreenOn();
	void adjustCpuFreq(char *cpu_freq);
	int systemSuspend(void);
	int systemResume(void);
	int pmSleep();
	int pmWakeUp();
	int pm_get_cam_preview_mode();
	class ScreenSwitchThread : public Thread
	{
	public:
		ScreenSwitchThread(PowerManager *pm)
			: Thread(false)
			, mPM(pm)
		{
		}
		void startThread()
		{
			run("ScreenSwitchThread", PRIORITY_NORMAL);
		}
		void stopThread()
		{
			requestExitAndWait();
		}
		bool threadLoop()
		{
			mPM->readyToOffScreen();
			return true;
		}
	private:
		PowerManager *mPM;
	};

	class poweroffThread : public Thread
	{
	public:
		poweroffThread(PowerManager* pm) : mPM(pm){}
		~poweroffThread(){}
		virtual bool threadLoop() {
			mPM->poweroff();	
			return false;
		}

		status_t start() {
			return run("poweroffThread", PRIORITY_NORMAL);
		}

	private:
		PowerManager* mPM;
	};


private:
	CdrMain *mCdrMain;
	sp<ScreenSwitchThread> mSS;
	sp<poweroffThread> mPO;
	int mDispFd;
	unsigned int mOffTime;
	Mutex mLock;
	Condition mCon;
	int mState;
	int mBatteryLevel;
	int mBacklight_level;
	int mBacklight_status;
	int poweroff();
	void pm_startpreview_by_resume();
	void pm_stoppreview_by_suspend();
};


#endif //_WINDOW_H
