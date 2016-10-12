#include "PlayBack.h"
#undef LOG_TAG
#define LOG_TAG "PlayBack"
#include "debug.h"
#define DISPLAY_READY

int PlayBack::keyProc(int keyCode, int isLongPress)
{
	HWND hIcon;
	switch(keyCode) {
	case CDR_KEY_OK:
		if(isLongPress == SHORT_PRESS) {
			hIcon = GetDlgItem(mHwnd, ID_PLAYBACK_ICON);
			if(getCurrentState() == STATE_STARTED) {
				pause();
				KillTimer(mHwnd, PLAYBACK_TIMER);
				if(!IsWindowVisible(hIcon))	{
					ShowWindow(hIcon, SW_SHOWNORMAL);
				}
			} else if(getCurrentState() == STATE_PAUSED) {
				start();
				SetTimer(mHwnd, PLAYBACK_TIMER, PROGRESSBAR_REFRESH_TIME);
				if(IsWindowVisible(hIcon)) {
					ShowWindow(hIcon, SW_HIDE);
				}
			}
		} else if(isLongPress == LONG_PRESS) {
			stopPlayback();
			return WINDOWID_PLAYBACKPREVIEW;
		}
		break;
	case CDR_KEY_MODE:
		if(isLongPress == SHORT_PRESS) {
			stopPlayback();
			return WINDOWID_PLAYBACKPREVIEW;
		}
		break;
	case CDR_KEY_LEFT:
		seek(-2000);
		updateProgressBar();
		break;
	case CDR_KEY_RIGHT:
		seek(2000);
		updateProgressBar();
		break;
	}

	return WINDOWID_PLAYBACK;
}


int PlayBackProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	PlayBack *playBack = (PlayBack*)GetWindowAdditionalData(hWnd);
 	db_msg("message code: [%d], %s, wParam=%d, lParam=%lu\n", message, Message2Str(message), wParam, lParam);
	switch(message) {
	case MSG_CREATE:
		if(playBack->createSubWidgets(hWnd) < 0) {
			db_error("create subWidgets failed\n");
			break;
		}
		break;
	case MSG_CDR_KEY:
		return playBack->keyProc(wParam, lParam);
	case MSG_PREPARE_PLAYBACK:
		{
			String8* file;
			file = (String8*)wParam;
			db_msg("file is %s\n", file->string());
			return playBack->PreparePlayBack(*file);
		}
		break;
	case MWM_CHANGE_FROM_WINDOW:
		{
			playBack->prepareCamera4Playback();		/* 画面显示出来前clean layer */
			playBack->initPlayBack();
			db_msg("xxxxxx\n");
			usleep(200 * 1000);		/* 延时， 保证在layer被渲染后绘制窗口, 避免闪屏 */
			set_ui_alpha(150);
			db_msg("xxxxxx\n");
		}
		break;
	case MSG_TIMER:
		if(wParam == PLAYBACK_TIMER)
			playBack->updateProgressBar();
		break;
	case MSG_PLB_COMPLETE:
		db_msg("MSG_PLB_COMPLETE: wParam is %d\n", wParam);
		playBack->mCurrentState = STATE_PLAYBACK_COMPLETED;
		if(IsTimerInstalled(hWnd, PLAYBACK_TIMER) == TRUE)
			KillTimer(hWnd, PLAYBACK_TIMER);
		set_ui_alpha(255);
		if (wParam == 1) {
			playBack->mStopFlag = 1;
			playBack->stop();
			playBack->reset();
			/* if wParam is 1, the MSG_PLB_COMPLETE is send from the onCompletion, so playBack->stop is not need */
			SendMessage(GetParent(hWnd), MWM_CHANGE_WINDOW, WINDOWID_PLAYBACK, WINDOWID_PLAYBACKPREVIEW);
		} else {
			/* if wParam is 0, the MSG_PLB_COMPLETE is send from outside, the caller expected to stop the playback, so need to call playBack->stop */
			playBack->mStopFlag = 1;		/* set the flag, tell the onCompletion not send the MSG_PLB_COMPLETE msg */
			playBack->stop();
			playBack->reset();
		}
		break;
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

PlayBack::PlayBack(CdrMain *cdrMain) : 
	mCurrentState(STATE_IDLE), 
	mTargetState(STATE_IDLE), 
	mStopFlag(0), 
	bmpIcon(), 
	bmpProgressBar()
{
	RECT rect;
	HWND hMainWnd = cdrMain->getHwnd();

	GetWindowRect(hMainWnd, &rect);

	mHwnd = CreateWindowEx(WINDOW_PLAYBACK, "",
			WS_NONE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			WINDOWID_PLAYBACK,
			0, 0, RECTW(rect), RECTH(rect),
			hMainWnd, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		return;
	}
	showWindow(SW_HIDE);

	mCdrMain = cdrMain;
#ifdef DISPLAY_READY
	mCdrDisp = new CdrDisplay(0);//mCdrMain->getCommonDisp(0);
#endif
	mCMP = new CedarMediaPlayer;
	mPlayBackListener = new PlayBackListener(this);
}

PlayBack::~PlayBack()
{

}

#ifdef USE_NEWUI
int PlayBack::createSubWidgets(HWND hWnd)
{
	int retval;
	RECT pbRect;
	CDR_RECT rect;
	ResourceManager* rm;
	HWND retWnd;

	GetClientRect(hWnd, &pbRect);

	rm = ResourceManager::getInstance();
	retval = rm->getResBmp(ID_PLAYBACK_ICON, BMPTYPE_BASE, bmpIcon);
	if(retval < 0) {
		db_error("get current playback icon bmp failed\n");
		return -1;
	}
	
	rect.x = (RECTW(pbRect)/2) - (bmpIcon.bmWidth/2);
	rect.y = (RECTH(pbRect)/2) - (bmpIcon.bmHeight/2);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_TRANSPARENT,
			ID_PLAYBACK_ICON,		
			rect.x, rect.y, bmpIcon.bmWidth, bmpIcon.bmHeight,
			hWnd, (DWORD)&bmpIcon);
	if(retWnd == HWND_INVALID) {
		db_error("create playback icon label failed\n");
		return -1;
	}
	ShowWindow(retWnd, SW_HIDE);

	ProgressBarData_t PGBData;
	PGBData.bgcWidget = rm->getResColor(ID_PLAYBACK_PGB, COLOR_BGC);
	PGBData.fgcWidget = rm->getResColor(ID_PLAYBACK_PGB, COLOR_FGC);
	retWnd = CreateWindowEx(CTRL_CDRPROGRESSBAR, NULL,
			WS_VISIBLE,
			WS_EX_TRANSPARENT,
			ID_PLAYBACK_PGB,		
			0, RECTH(pbRect)-58, RECTW(pbRect), 58,
			hWnd, (DWORD)&PGBData);
	if(retWnd == HWND_INVALID) {
		db_error("create playback progress bar failed\n");
		return -1;
	}

	SetWindowBkColor(retWnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
	return 0;
}
#else
int PlayBack::createSubWidgets(HWND hWnd)
{
	int retval;
	CDR_RECT rect;
	ResourceManager* rm;
	HWND retWnd;

	rm = ResourceManager::getInstance();
	retval = rm->getResBmp(ID_PLAYBACK_ICON, BMPTYPE_BASE, bmpIcon);
	if(retval < 0) {
		db_error("get current playback icon bmp failed\n");
		return -1;
	}

	rm->getResRect(ID_PLAYBACK_ICON, rect);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_TRANSPARENT,
			ID_PLAYBACK_ICON,		
			rect.x, rect.y, bmpIcon.bmWidth, bmpIcon.bmHeight,
			hWnd, (DWORD)&bmpIcon);
	if(retWnd == HWND_INVALID) {
		db_error("create playback icon label failed\n");
		return -1;
	}
	ShowWindow(retWnd, SW_HIDE);

	ProgressBarData_t PGBData;
	rm->getResRect(ID_PLAYBACK_PGB, rect);
	PGBData.bgcWidget = rm->getResColor(ID_PLAYBACK_PGB, COLOR_BGC);
	PGBData.fgcWidget = rm->getResColor(ID_PLAYBACK_PGB, COLOR_FGC);
	retWnd = CreateWindowEx(CTRL_CDRPROGRESSBAR, NULL,
			WS_VISIBLE,
			WS_EX_NONE,
			ID_PLAYBACK_PGB,		
			rect.x, rect.y, rect.w, rect.h,
			hWnd, (DWORD)&PGBData);
	if(retWnd == HWND_INVALID) {
		db_error("create playback progress bar failed\n");
		return -1;
	}

	SetWindowBkColor(retWnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
	return 0;
}
#endif

int PlayBack::initPlayBack() {
    HWND hIcon;

    hIcon = GetDlgItem(mHwnd, ID_PLAYBACK_ICON);
    if(IsWindowVisible(hIcon)) {
        ShowWindow(hIcon, SW_HIDE);
    }

    db_msg("xxxxxxxxxxxxxxx\n");
    SetTimer(mHwnd, PLAYBACK_TIMER, PROGRESSBAR_REFRESH_TIME);
    mStopFlag = 0;

    return 0;
}
int PlayBack::startPlayBack()
{
	HWND hIcon;

	hIcon = GetDlgItem(mHwnd, ID_PLAYBACK_ICON);
	if(IsWindowVisible(hIcon)) {
		ShowWindow(hIcon, SW_HIDE);
	}

	db_msg("xxxxxxxxxxxxxxx\n");
	SetTimer(mHwnd, PLAYBACK_TIMER, PROGRESSBAR_REFRESH_TIME);
	initProgressBar();
	mStopFlag = 0;
	start();

	return 0;
}

int PlayBack::PreparePlayBack(String8 filePath)
{
	preparePlay(filePath);
	return 0;
}

playerState PlayBack::getCurrentState()
{
	return mCurrentState;
}

void PlayBack::setDisplay(int hlay)
{
#ifdef DISPLAY_READY
	mCMP->setDisplay(hlay);
#endif
}

int PlayBack::preparePlay(String8 filePath)
{
	/* Idle:	Set Listeners */
	mCMP->setOnPreparedListener(mPlayBackListener);

	mCMP->setOnVideoSizeChangedListener(mPlayBackListener);

	mCMP->setOnCompletionListener(mPlayBackListener);

	mCMP->setOnDecodeCompleteListener(mPlayBackListener);

	mCMP->setOnErrorListener(mPlayBackListener);

	mCMP->setOnInfoListener(mPlayBackListener);
	mCMP->setOnSeekCompleteListener(mPlayBackListener);

	mCMP->setDataSource(filePath);
	mCMP->setAudioStreamType(AUDIO_STREAM_MUSIC);	/*AudioManager.STREAM_MUSIC*/
	mCMP->setScreenOnWhilePlaying(true);
	/* Initialized */
	mCMP->prepareAsync();
	mCurrentState = STATE_PREPARING;
	return NO_ERROR;
}

void PlayBack::start()
{
	if(mCurrentState == STATE_PREPARED || mCurrentState == STATE_PAUSED)
	{
		if(mCMP != NULL) {
			mCMP->start();
			mCurrentState = STATE_STARTED;
		}
	}

	mTargetState = STATE_STARTED;
	noWork(false);
}

void PlayBack::pause()
{
	if(mCurrentState == STATE_STARTED) {
		if(mCMP != NULL) {
			mCMP->pause();
			mCurrentState = STATE_PAUSED;
		}
	}
	mTargetState = STATE_PAUSED;
}

bool PlayBack::isStarted()
{
	return (mTargetState == STATE_STARTED);
}

void PlayBack::seek(int msec)
{
	int total, current;
	if(msec == 0)
		return;
	if(mCurrentState == STATE_STARTED || mCurrentState == STATE_PAUSED) {
		total = mCMP->getDuration();
		current = mCMP->getCurrentPosition();
		if(total == -1 || current == -1)
			return;
		current += msec;
		if(current > total || current < 0)
			return;
		mCMP->seekTo(current);
	}
}

void PlayBack::stop()
{
	if(mCurrentState != STATE_IDLE) {
		if(mCMP != NULL) {
			mCMP->stop();
			mCurrentState = STATE_STOPPED;
			mTargetState  = STATE_STOPPED;
		}
	}
	if (mCdrMain->isBatteryOnline()) {
		/* noWork(true); */
	}
    if(mCdrDisp) {
        mDispLock.lock();
        mCdrDisp->close();
        mCdrDisp->releaseSurface();
        mDispLock.unlock();
    }
}

void PlayBack::release()
{
	if(mCMP != NULL) {
		mCMP->release();
		mCurrentState = STATE_END;
		mTargetState  = STATE_END;
	}
}

void PlayBack::reset()
{
	if(mCMP != NULL) {
		mCMP->reset();
		mCurrentState = STATE_IDLE;
		mTargetState  = STATE_IDLE;
	}
}

void PlayBack::noWork(bool idle)
{
	mCdrMain->noWork(idle);
}

void PlayBack::PlayBackListener::onPrepared(CedarMediaPlayer *pMp)
{
#ifdef DISPLAY_READY
    if(!mPB->mCdrDisp) {
        //mCdrDisp = mCdrMain->getCommonDisp(0);
        mPB->mCdrDisp = new CdrDisplay(0);
    }
    if(!mPB->mCdrDisp) {
        db_error("mCdrDisp is not avaliable\n");
        return;
    }

    int w, h;
    mPB->mCMP->getVideoWidth(&w);
    mPB->mCMP->getVideoHeight(&h);
    mPB->mCdrDisp->requestSurface(w, h, w * 3 / 2);
    mPB->setDisplay(mPB->mCdrDisp->getHandle());
    mPB->mCdrDisp->open();

    RECT rect;
    HWND hMainWnd = mPB->mCdrMain->getHwnd();
    CDR_RECT dispRect;
    GetWindowRect(hMainWnd, &rect);
    dispRect.x = rect.left;
    dispRect.y = rect.top;
    dispRect.w = RECTW(rect);
    dispRect.h = RECTH(rect);
    mPB->mCdrDisp->setRect(dispRect);

    db_msg("PreparePlayBack, w %d, h %d, disp rect(%d,%d,%d,%d)", w, h,dispRect.x,dispRect.y,dispRect.w,dispRect.h);
#endif

    mPB->mCurrentState = STATE_PREPARED;
    db_msg("start CedarMediaPlayer\n");
    mPB->initProgressBar();
    mPB->start();
}

void PlayBack::PlayBackListener::onCompletion(CedarMediaPlayer *pMp)
{
	db_msg("receive onCompletion message!, mStopFlag is %d\n", mPB->mStopFlag);
	if(mPB->mStopFlag == 1) {
		mPB->mStopFlag = 0;
	} else {
		SendMessage(mPB->getHwnd(), MSG_PLB_COMPLETE, 1, 0);
	}
}

bool PlayBack::PlayBackListener::onError(CedarMediaPlayer *pMp, int what, int extra)
{
	db_error("receive onError message!\n");
	mPB->stopPlayback();
	return false;
}


void PlayBack::PlayBackListener::onVideoSizeChanged(CedarMediaPlayer *pMp, int width, int height)
{
	db_msg("onVideoSizeChanged!\n");
}

bool PlayBack::PlayBackListener::onInfo(CedarMediaPlayer *pMp, int what, int extra)
{
	db_msg("receive onInfo message!\n");
	return false;
}

void PlayBack::PlayBackListener::onSeekComplete(CedarMediaPlayer *pMp)
{
	db_msg("receive onSeekComplete message!\n");
}

void PlayBack::PlayBackListener::onDecodeComplete(CedarMediaPlayer *pMp)
{
	db_msg("onDecodeComplete \n");
	//buffer updated, commit to display
#ifdef DISPLAY_READY
    if(STATE_STARTED == mPB->mCurrentState) {
        PreviewBuffer *buffer = new PreviewBuffer;
        memset(buffer, 0, sizeof(PreviewBuffer));
        buffer->format = HAL_PIXEL_FORMAT_YCrCb_NV12;
        mPB->mCMP->getVideoWidth(&buffer->width);
        mPB->mCMP->getVideoHeight(&buffer->height);
        buffer->stride = buffer->width;

        mPB->mDispLock.lock();
        if(buffer->width != 0 && buffer->height != 0 && mPB->mCdrDisp->getHandle() != 0) {
            buffer->share_fd = mPB->mCdrDisp->getHandle();
            mPB->mCdrDisp->commitToDisplay(buffer,false);
        }
        mPB->mDispLock.unlock();
        delete(buffer);
    }
#endif
}

void PlayBack::initProgressBar(void)
{
	int ms;
	PGBTime_t PGBTime;

	ms = mCMP->getDuration();
	db_msg("duration ms is %d\n", ms);

	PGBTime.min = ms / 1000 / 60;
	PGBTime.sec = ms / 1000 % 60;

	SendMessage(GetDlgItem(mHwnd, ID_PLAYBACK_PGB), PGBM_SETTIME_RANGE, 0, (LPARAM)&PGBTime);
}

void PlayBack::resetProgressBar(void)
{
	PGBTime_t PGBTime;

	PGBTime.min = 0;
	PGBTime.sec = 0;

	SendMessage(GetDlgItem(mHwnd, ID_PLAYBACK_PGB), PGBM_SETTIME_RANGE, 0, (LPARAM)&PGBTime);
}

void PlayBack::updateProgressBar(void)
{
	int ms;
	PGBTime_t curDuration;
	ms = mCMP->getCurrentPosition();
	curDuration.min = ms / 1000 / 60;
	curDuration.sec = ms / 1000 % 60;
	SendMessage(GetDlgItem(mHwnd, ID_PLAYBACK_PGB), PGBM_SETCURTIME, (WPARAM)&curDuration, 0);
}

void PlayBack::prepareCamera4Playback(void)
{
	mCdrMain->prepareCamera4Playback(false);
}

int PlayBack::stopPlayback(bool bRestoreAlpha)
{
	db_msg("stop playback\n");
	if(IsTimerInstalled(mHwnd, PLAYBACK_TIMER) == TRUE)
		KillTimer(mHwnd, PLAYBACK_TIMER);
	stop();
	reset();
	if (bRestoreAlpha) {
		set_ui_alpha(255);
	}
	return 0;
}

int PlayBack::restoreAlpha()
{
	return set_ui_alpha(255);
}
