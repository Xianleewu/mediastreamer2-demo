
#include "StatusBar.h"
#undef LOG_TAG
#define LOG_TAG "NewStatusBar.cpp"
#include "debug.h"
#include "cutils/properties.h"
#include <network/wifiMonitor.h>

using namespace android;

static int wifi_signal_level = -1;

static pic_t pic_sets[] = {
	{ID_STATUSBAR_ICON_WINDOWPIC,	"WindowPic"},
	{ID_STATUSBAR_ICON_VALUE,			"Value"},
	{ID_STATUSBAR_ICON_MODE,				"Mode"},
	{ID_STATUSBAR_ICON_AWMD,		"AWMD"},
	{ID_STATUSBAR_ICON_UVC,			"UVC"},
	{ID_STATUSBAR_ICON_WIFI,		"Wifi"},
	{ID_STATUSBAR_ICON_AP,			"ap"},
	{ID_STATUSBAR_ICON_LOCK,		"Lock"},
	{ID_STATUSBAR_ICON_AUDIO,		"Voice"},
	{ID_STATUSBAR_ICON_TFCARD,		"TFCard"},
	{ID_STATUSBAR_ICON_GPS,			"Gps"},
#ifdef MODEM_ENABLE
	{ID_STATUSBAR_ICON_CELLULAR,                 "Cellular"},
#endif
	{ID_STATUSBAR_ICON_BAT,			"Bat"},
};

enum iconIndex{
	ICON_INDEX_WINDOWPIC = 0,
	ICON_INDEX_VALUE,
	ICON_INDEX_MODE,
	ICON_INDEX_AWMD,
	ICON_INDEX_UVC,
	ICON_INDEX_WIFI,
	ICON_INDEX_AP,
	ICON_INDEX_LOCK,
	ICON_INDEX_AUDIO,
	ICON_INDEX_TFCARD,
	ICON_INDEX_GPS,
#ifdef MODEM_ENABLE
	ICON_INDEX_CELLULAR,
#endif
	ICON_INDEX_BAT,
};

#define PIC_CNT		(sizeof(pic_sets)/sizeof(pic_sets[0]))

int StatusBarProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	StatusBar* stb = (StatusBar*)GetWindowAdditionalData(hWnd);
	//db_msg("message is %s\n", Message2Str(message));
	switch(message) {
	case MSG_FONTCHANGED:
		stb->updateWindowFont();
		break;
	case MSG_CREATE:
		{
			SetWindowBkColor(hWnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
			if(stb->createSTBWidgets(hWnd) < 0) {
				db_error("create sub widgets failed\n");	
			}
			SetTimer(hWnd, WIFI_SIGNAL_UPDATE, WIFI_SIGNAL_INTERVAL);
		}
		break;
	case MSG_PAINT:
		{
			HDC hdc;
			hdc = BeginPaint (hWnd);
			int x = 0, y = 0, w, h;			
			CDR_RECT rect;
			RECT rcClient;
			PBITMAP pBgPic;
			ResourceManager *rm = ResourceManager::getInstance();

			db_info("paint ----------status bar----------\n");

			rm->getResRect(ID_STATUSBAR_ICON_WINDOWPIC, rect);

			pBgPic = stb->getBgBmp();
			//截取.9图片的中间部分进行缩放
			FillBitmapPartInBox(hdc, 0, 0, rect.w, rect.h,
									pBgPic, 10, 10, (pBgPic->bmWidth/2)-10, pBgPic->bmHeight-20);
			GetClientRect (hWnd, &rcClient);
			FillBitmapPartInBox(hdc, rect.w, 0, RECTW(rcClient) - rect.w, rect.h,
					stb->getBgBmp(), (pBgPic->bmWidth/2)+10, 10, (pBgPic->bmWidth/2)-20, pBgPic->bmHeight-20);
			EndPaint (hWnd, hdc);
		}
		break;
	case MSG_TIMER:
		{
			if(wParam == STB_TIMER1) {
//				stb->updateTime(hWnd);
			} else if(wParam == STB_TIMER2) {
				stb->increaseRecordTime();
				stb->updateRecordTime(hWnd);
			} else if (wParam == WIFI_SIGNAL_UPDATE) {
				int wifi_level;
				char wifi_level_str[PROPERTY_VALUE_MAX] = {0};
				property_get(WIFI_SIGNAL_LEVEL, wifi_level_str, "-1");
				wifi_level = atoi(wifi_level_str);
				if (wifi_level > 0 && wifi_signal_level != wifi_level) {
					wifi_signal_level = wifi_level;
					stb->msgCallback(STBM_ENABLE_DISABLE_WIFI, wifi_level);
					db_msg("%s, wifi_level=%d", __func__, wifi_level);
				} else if (wifi_level < 0) {
					wifi_signal_level = wifi_level;
				}
			}
		}
		break;
	case STBM_START_RECORD_TIMER:
	case STBM_RESUME_RECORD_TIMER:
	case STBM_STOP_RECORD_TIMER:
	case STBM_GET_RECORD_TIME:
		return stb->handleRecordTimerMsg(message, wParam);
		break;
	case MSG_CLOSE:
		KillTimer(hWnd, STB_TIMER1);
		KillTimer(hWnd, WIFI_SIGNAL_UPDATE);
	case MSG_DESTROY:
		break;
	default:
		stb->msgCallback(message, wParam);
	}

	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

	StatusBar::StatusBar(CdrMain *cdrMain)
: mCdrMain(cdrMain)
{
	CDR_RECT rect;
	HWND hParent;
	ResourceManager *rm;
	PicObj* pic[PIC_CNT];
	int retval = 0;

	rm = ResourceManager::getInstance();
	hParent = mCdrMain->getHwnd();
	rm->getResRect(ID_STATUSBAR, rect);
	db_msg("rect.x = %d\n", rect.x);
	db_msg("rect.y = %d\n", rect.y);
	db_msg("rect.w = %d\n", rect.w);
	db_msg("rect.h = %d\n", rect.h);

	db_msg("pic size :%u\n", PIC_CNT);
	for(unsigned int i = 0; i < PIC_CNT; i++ ) {
		pic[i] = new PicObj(String8(pic_sets[i].name));
		pic[i]->setId(pic_sets[i].id);
		mPic.add(pic[i]);
	}
	db_msg("mPic size :%u\n", mPic.size());

	/* statusbar background pic */
	retval = rm->getResBmp(ID_STATUSBAR, BMPTYPE_BASE, mBgBmp);
	if(retval < 0) {
		db_error("loadb bitmap of ID_STATUSBAR failed\n");
	}

	mHwnd = CreateWindowEx(WINDOW_STATUSBAR, "",
			WS_VISIBLE | WS_CHILD,
			WS_EX_USEPARENTFONT,
			WINDOWID_STATUSBAR,
			rect.x, rect.y, rect.w, rect.h,
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create status bar failed\n");
		return;
	}
	rm->setHwnd(WINDOWID_STATUSBAR, mHwnd);
	mCurWindowID = MENU_RECORDPREVIEW;
}

StatusBar::~StatusBar()
{
	if(mBgBmp.bmBits != NULL) {
		UnloadBitmap(&mBgBmp);
	}

	db_msg("StatusBar Destructor\n");
	for(unsigned int i = 0; i < mPic.size(); i++) {
		delete mPic.itemAt(i);
	}
	db_msg("StatusBar Destructor\n");
}

int StatusBar::initPic(PicObj *po)
{
	int retval;
	CDR_RECT rect;
	HWND retWnd;
	ResourceManager* rm;
	HDC hdc;

	db_msg("initPic %p\n", po);
	rm = ResourceManager::getInstance();
	retval = rm->getResRect(po->getId(), rect);
	if(retval < 0) {
		db_error("get %s failed\n", po->mName.string());
		return -1;
	}
	db_msg("name: %s, ID[%d], %d %d %d %d\n", po->mName.string(), po->getId(), rect.x, rect.y, rect.w, rect.h);

	if (po->getId() == ID_STATUSBAR_ICON_WINDOWPIC)
		retval = rm->getResBmp(po->getId(), BMPTYPE_WINDOWPIC_RECORDPREVIEW, *po->getImage());
	else 
		retval = rm->getResBmp(po->getId(), BMPTYPE_BASE, *po->getImage());
	if(retval < 0) {
		db_error("loadb bitmap of %s failed\n", po->mName.string());
	}

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | SS_REALSIZEIMAGE,
			WS_EX_TRANSPARENT,
			po->getId(),
			rect.x, rect.y, rect.w, rect.h,
			po->mParent, (DWORD)(po->getImage()));
	if(retWnd == HWND_INVALID) {
		db_error("create status bar volume failed\n");
		return -1;
	}

	return 0;
}

int StatusBar::createSTBWidgets(HWND hParent)
{
	db_msg("for initPic\n");
	for( unsigned int i = 0; i < mPic.size(); i++ ) {
		mPic.itemAt(i)->mParent = hParent;
		if(initPic(mPic.itemAt(i)) < 0) {
			db_msg("init pic mId: %d failed\n", mPic.itemAt(i)->getId());
			return -1;
		}
	}

	return 0;
}

void StatusBar::updateRecordTime(HWND hWnd)
{
	char timeStr[30] = {0};

	sprintf(timeStr, "%02d:%02d", (unsigned int)mRecordTime/60, (unsigned int)mRecordTime%60);
	//SetWindowText(GetDlgItem(hWnd, ID_STATUSBAR_LABEL1), timeStr);
	//UpdateWindow(GetDlgItem(hWnd, ID_STATUSBAR_LABEL1), false);
}

void StatusBar::increaseRecordTime()
{
	
	mRecordTime++;

	if(mRecordTime >= mCurRecordVTL / 1000) {
		mRecordTime = 0;
		if(mCurRecordVTL == AFTIMEMS) {
			mCurRecordVTL = mRecordVTL;
			db_msg("reset the mCurRecordVTL to %ld ms\n", mCurRecordVTL);
			SendMessage(mHwnd, STBM_LOCK, 0, 0);
		}
	}
}

void StatusBar::msgCallback(int msgType, unsigned int data)
{
	ALOGV("recived msg %x, data is 0x%x\n", msgType, data);
	switch(msgType) {
	case MSG_CDRMAIN_CUR_WINDOW_CHANGED:
		windowState_t *curWindowState;
		curWindowState = (windowState_t*)data;
		if(curWindowState == NULL) {
			db_error("invalid windowState\n");
			break;
		}
		handleWindowPicEvent(curWindowState);
		break;
	case MSG_BATTERY_CHANGED:
		mPic.itemAt(ICON_INDEX_BAT)->set(data);
		mPic.itemAt(ICON_INDEX_BAT)->update();
		break;
	case MSG_BATTERY_CHARGING:
		mPic.itemAt(ICON_INDEX_BAT)->set(5);
		mPic.itemAt(ICON_INDEX_BAT)->update();
		break;
	case STBM_MOUNT_TFCARD:
		db_msg("STBM_MOUNT_TFCARD\n");
		mPic.itemAt(ICON_INDEX_TFCARD)->set(data);
		mPic.itemAt(ICON_INDEX_TFCARD)->update();
		break;
	case STBM_ENABLE_DISABLE_GPS:
		db_msg("STBM_ENABLE_DISABLE_GPS\n");
		mPic.itemAt(ICON_INDEX_GPS)->set(data);
		mPic.itemAt(ICON_INDEX_GPS)->update();
		break;
	case STBM_ENABLE_DISABLE_WIFI:
		db_msg("STBM_ENABLE_DISABLE_WIFI\n");
		mPic.itemAt(ICON_INDEX_WIFI)->set(data);
		mPic.itemAt(ICON_INDEX_WIFI)->update();
		break;
	case STBM_ENABLE_DISABLE_AP:
		db_msg("STBM_ENABLE_DISABLE_AP\n");
		mPic.itemAt(ICON_INDEX_AP)->set(data);
		mPic.itemAt(ICON_INDEX_AP)->update();
		break;
#ifdef MODEM_ENABLE
	case STBM_ENABLE_DISABLE_CELLULAR:
		db_msg("STBM_ENABLE_DISABLE_CELLULAR\n");
		mPic.itemAt(ICON_INDEX_CELLULAR)->set(data);
		mPic.itemAt(ICON_INDEX_CELLULAR)->update();
		break;
#endif
	case STBM_VOICE:
		db_msg("STBM_VOICE\n");
		if(mPic.itemAt(ICON_INDEX_AUDIO)->get() != (int)data) {
			mPic.itemAt(ICON_INDEX_AUDIO)->set(data);
			mPic.itemAt(ICON_INDEX_AUDIO)->update();
		}
		break;
	case STBM_LOCK:
		mPic.itemAt(ICON_INDEX_LOCK)->set(data);
		mPic.itemAt(ICON_INDEX_LOCK)->update();
		break;
	case STBM_UVC:
		mPic.itemAt(ICON_INDEX_UVC)->set(data);
		mPic.itemAt(ICON_INDEX_UVC)->update();
		break;
	case STBM_VIDEO_PHOTO_VALUE:
		mPic.itemAt(ICON_INDEX_VALUE)->set(data);
		mPic.itemAt(ICON_INDEX_VALUE)->update();
		break;
	case STBM_VIDEO_MODE:
		mPic.itemAt(ICON_INDEX_MODE)->set(data);
		mPic.itemAt(ICON_INDEX_MODE)->update();
		break;
	case STBM_AWMD:
		mPic.itemAt(ICON_INDEX_AWMD)->set(data);
		mPic.itemAt(ICON_INDEX_AWMD)->update();
		break;
	case STBM_PHOTO_GRAPH:
		db_msg("STBM_PHOTO_GRAPH data = %d\n", data);
		if (data) {
			mPic.itemAt(ICON_INDEX_WINDOWPIC)->update(BMPTYPE_WINDOWPIC_PHOTOGRAPH);
		} else {
			mPic.itemAt(ICON_INDEX_WINDOWPIC)->update(BMPTYPE_WINDOWPIC_RECORDPREVIEW);
		}
	default:
		break;
	}
}

void StatusBar::handleWindowPicEvent(windowState_t *windowState)
{
	ResourceManager *rm;
	RECT rect;

	rm = ResourceManager::getInstance();
	mCurWindowID = windowState->curWindowID;
	GetClientRect(mHwnd, &rect);

	switch(windowState->curWindowID) {
	case WINDOWID_RECORDPREVIEW:
		{
			RPWindowState_t state;
			state = windowState->RPWindowState;
			if(state == STATUS_RECORDPREVIEW) {
				if(mCdrMain->getUvcState()){
					mPic.itemAt(ICON_INDEX_UVC)->set(1);
					mPic.itemAt(ICON_INDEX_UVC)->update();
				} else {
					mPic.itemAt(ICON_INDEX_UVC)->set(0);
					mPic.itemAt(ICON_INDEX_UVC)->update();
				}
			} else if(state == STATUS_PHOTOGRAPH) {				
				if(mCdrMain->getUvcState()){
					mPic.itemAt(ICON_INDEX_UVC)->set(1);
					mPic.itemAt(ICON_INDEX_UVC)->update();
				} else {
					mPic.itemAt(ICON_INDEX_UVC)->set(0);
					mPic.itemAt(ICON_INDEX_UVC)->update();
				}
			}
			if(state == STATUS_AWMD) {
				db_msg("xxxxxxxxxx\n");
				mPic.itemAt(ICON_INDEX_AWMD)->set(1);
				mPic.itemAt(ICON_INDEX_AWMD)->update();
			} else {
				mPic.itemAt(ICON_INDEX_AWMD)->set(0);
				mPic.itemAt(ICON_INDEX_AWMD)->update();
			}

			if(state == STATUS_RECORDPREVIEW || state == STATUS_AWMD) {
				mPic.itemAt(ICON_INDEX_WINDOWPIC)->update(BMPTYPE_WINDOWPIC_RECORDPREVIEW);
			} else if(state == STATUS_PHOTOGRAPH) {
				mPic.itemAt(ICON_INDEX_WINDOWPIC)->update(BMPTYPE_WINDOWPIC_PHOTOGRAPH);
			}
			SetWindowBkColor(mHwnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
			InvalidateRect(mHwnd, &rect, TRUE);
		}
		break;
	case WINDOWID_PLAYBACKPREVIEW:
		mPic.itemAt(ICON_INDEX_WINDOWPIC)->update(BMPTYPE_WINDOWPIC_PLAYBACKPREVIEW);
		if(mCdrMain->getUvcState()){
			mPic.itemAt(ICON_INDEX_UVC)->set(1);
			mPic.itemAt(ICON_INDEX_UVC)->update();
		} else {
			mPic.itemAt(ICON_INDEX_UVC)->set(0);
			mPic.itemAt(ICON_INDEX_UVC)->update();
		}
		SetWindowBkColor(mHwnd, RGBA2Pixel(HDC_SCREEN, 0x55, 0x55, 0x55, 0xFF));
		InvalidateRect(mHwnd, &rect, TRUE);
		break;
	default:
		db_error("invalide windowID: %d\n", windowState->curWindowID);
		break;
	}
}

int StatusBar::handleRecordTimerMsg(unsigned int message, unsigned int recordType)
{
	switch(message) {
	case STBM_START_RECORD_TIMER:
		{
			db_msg("recieved STBM_START_RECORD_TIMER, recordType is %d, mRecordVTL is %ld ms, mCurRecordVTL is %ld ms\n", recordType, mRecordVTL, mCurRecordVTL);
			if(mRecordVTL == 0) {
				db_msg("start record timer failed, record vtl not initialized\n");
				break;
			}
			if(IsTimerInstalled(mHwnd, STB_TIMER2) == TRUE) {
				ResetTimer(mHwnd, STB_TIMER2, 100);
			} else {
				SetTimer(mHwnd, STB_TIMER2, 100);
			}
			mRecordTime = 0;
			updateRecordTime(mHwnd);
			if(recordType == 1) {
				/* if recordType is impact record, set the mCurRecordVTL with AFTIMEMS
				 * when time expired, compare the mCurRecordVTL with mRecordVTL, 
				 * if not equal, then reset mCurRecordVTL with mRecordVTL
				 * */
				mCurRecordVTL = AFTIMEMS;
				SendMessage(mHwnd, STBM_LOCK, 1, 0);
			}
		}
		break;
#if 0
	case STBM_RESUME_RECORD_TIMER:
		{
			db_msg("recieved STBM_RESUME_RECORD_TIMER, period is %d ms\n", period);
			ResetTimer(mHwnd, STB_TIMER2, 100);
			mRecordTime = 0;
			updateRecordTime(mHwnd);
		}
		break;
#endif
	case STBM_STOP_RECORD_TIMER:
		{
			db_msg("recieved STBM_STOP_RECORD!\n");
			mRecordTime = 0;	
			KillTimer(mHwnd, STB_TIMER2);
			//SetWindowText(GetDlgItem(mHwnd, ID_STATUSBAR_LABEL1), "");
			//UpdateWindow(GetDlgItem(mHwnd, ID_STATUSBAR_LABEL1), false);
			SendMessage(mHwnd, STBM_LOCK, 0, 0);
		}
		break;
	case STBM_GET_RECORD_TIME:
		return mRecordTime;
		break;
	}

	return 0;
}

/*
 * set the record video time length, unit(ms)
 * */
void StatusBar::setRecordVTL(unsigned int vtl)
{
	mRecordVTL = vtl;
	mCurRecordVTL = vtl;

}

unsigned int StatusBar::getCurrenWindowId()
{
	return mCurWindowID;
}

PBITMAP StatusBar::getBgBmp()
{
	return &mBgBmp;
}
