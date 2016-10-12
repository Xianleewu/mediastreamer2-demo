
#include "StatusBar.h"
#undef LOG_TAG
#define LOG_TAG "StatusBar.cpp"
#include "debug.h"
#include "cutils/properties.h"
#include <network/wifiMonitor.h>

using namespace android;

static int wifi_signal_level = -1;

static pic_t pic_sets[] = {
	{ID_STATUSBAR_ICON_WINDOWPIC,	"WindowPic"},
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
			ResourceManager *rm;
			gal_pixel color;

			rm = ResourceManager::getInstance();
			color = rm->getResColor(ID_STATUSBAR, COLOR_BGC);
			SetWindowBkColor(hWnd, color);

			if(stb->createSTBWidgets(hWnd) < 0) {
				db_error("create sub widgets failed\n");	
			}

			color = rm->getResColor(ID_STATUSBAR, COLOR_FGC);
			db_msg("status bar fgc is 0x%lx\n", Pixel2DWORD(HDC_SCREEN, color));
			SetWindowElementAttr(GetDlgItem(hWnd, ID_STATUSBAR_LABEL1), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, color));
			SetWindowElementAttr(GetDlgItem(hWnd, ID_STATUSBAR_LABEL_TIME), WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, color));

			SetTimer(hWnd, STB_TIMER1, 100);
			stb->updateTime(hWnd);
			SetTimer(hWnd, WIFI_SIGNAL_UPDATE, WIFI_SIGNAL_INTERVAL);
		}
		break;
	case MSG_TIMER:
		{
			if(wParam == STB_TIMER1) {
				stb->updateTime(hWnd);
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
	case STBM_SETLABEL1_TEXT:
		/* lParam: msg str */
		if(lParam) {
			SetWindowText(GetDlgItem(hWnd, ID_STATUSBAR_LABEL1), (char*)lParam);
		}
		break;
	case STBM_CLEAR_LABEL1_TEXT:
		{
			if (stb->getCurrenWindowId() != wParam) {
				break;
			}
			HWND hLabel1;
			RECT rect;
			hLabel1 = GetDlgItem(hWnd, ID_STATUSBAR_LABEL1);
			GetClientRect(hLabel1, &rect);
			SetWindowText(hLabel1, "");
			InvalidateRect(hLabel1, &rect, TRUE);
		}
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

	mHwnd = CreateWindowEx(WINDOW_STATUSBAR, "",
			WS_VISIBLE | WS_CHILD,
			WS_EX_NONE | WS_EX_USEPARENTFONT,
			WINDOWID_STATUSBAR,		
			rect.x, rect.y, rect.w, rect.h,
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create status bar failed\n");
		return;
	}
	rm->setHwnd(WINDOWID_STATUSBAR, mHwnd);
	mCurWindowID = WINDOWID_RECORDPREVIEW;
}

StatusBar::~StatusBar()
{
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

	db_msg("initPic %p\n", po);
	rm = ResourceManager::getInstance();
	retval = rm->getResRect(po->getId(), rect);
	if(retval < 0) {
		db_error("get %s failed\n", po->mName.string());
		return -1;
	}
	db_msg("name: %s, ID[%d], %d %d %d %d\n", po->mName.string(), po->getId(), rect.x, rect.y, rect.w, rect.h);

	retval = rm->getResBmp(po->getId(), BMPTYPE_BASE, *po->getImage());
	if(retval < 0) {
		db_error("loadb bitmap of %s failed\n", po->mName.string());
	}

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE ,
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
	CDR_RECT rect;
	int retval = 0;
	HWND retWnd;
	ResourceManager *rm;

	rm = ResourceManager::getInstance();

	/* Label1 */
	retval = rm->getResRect(ID_STATUSBAR_LABEL1, rect);
	if(retval < 0) {
		db_error("get ID_STATUSBAR_LABEL1 failed\n");
		return -1;
	}
	db_msg("label1 rect x = %d\n", rect.x);
	db_msg("label1 rect y = %d\n", rect.y);
	db_msg("label1 rect w = %d\n", rect.w);
	db_msg("label1 rect h = %d\n", rect.h);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			ID_STATUSBAR_LABEL1,
			rect.x, rect.y, rect.w, rect.h,
			hParent, 0);
	if(retWnd == HWND_INVALID) {
		db_error("create status bar window name failed\n");
		return -1;
	}

	retval = rm->getResRect(ID_STATUSBAR_LABEL_RESERVE, rect);
	if(retval < 0) {
		db_error("get ID_STATUSBAR_LABEL_RESERVE failed\n");
		return -1;
	}
	db_msg("reserve label rect x = %d\n", rect.x);
	db_msg("reserve label rect y = %d\n", rect.y);
	db_msg("reserve label rect w = %d\n", rect.w);
	db_msg("reserve label rect h = %d\n", rect.h);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE ,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			ID_STATUSBAR_LABEL_RESERVE,
			rect.x, rect.y, rect.w, rect.h,
			hParent, 0);

	if(retWnd == HWND_INVALID) {
		db_error("create status bar reserved label failed\n");
		return -1;
	}

	/* Ê±¼ä */
	retval = rm->getResRect(ID_STATUSBAR_LABEL_TIME, rect);
	if(retval < 0) {
		db_error("get ID_STATUSBAR_LABEL_TIME failed\n");
		return -1;
	}
	db_msg("time label rect x = %d\n", rect.x);
	db_msg("time label rect y = %d\n", rect.y);
	db_msg("time label rect w = %d\n", rect.w);
	db_msg("time label rect h = %d\n", rect.h);

	retWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE ,
			WS_EX_TRANSPARENT,
			ID_STATUSBAR_LABEL_TIME,
			rect.x, rect.y, rect.w, rect.h,
			hParent, 0);

	if(retWnd == HWND_INVALID) {
		db_error("create status bar time failed\n");
		return -1;
	}

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

void StatusBar::updateTime(HWND hWnd)
{

	struct tm* localTime;
	char timeStr[30] = {0};


	getDateTime(&localTime);

	sprintf(timeStr, "%02d:%02d", localTime->tm_hour, localTime->tm_min);
	SetWindowText(GetDlgItem(hWnd, ID_STATUSBAR_LABEL_TIME), timeStr);
}

void StatusBar::updateRecordTime(HWND hWnd)
{
	char timeStr[30] = {0};

	sprintf(timeStr, "%02d:%02d", (unsigned int)mRecordTime/60, (unsigned int)mRecordTime%60);
	SetWindowText(GetDlgItem(hWnd, ID_STATUSBAR_LABEL1), timeStr);
	UpdateWindow(GetDlgItem(hWnd, ID_STATUSBAR_LABEL1), false);
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
		mPic.itemAt(ICON_INDEX_BAT)->set(6);
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
	default:
		break;
	}
}

void StatusBar::handleWindowPicEvent(windowState_t *windowState)
{
	ResourceManager *rm;

	rm = ResourceManager::getInstance();
	mCurWindowID = windowState->curWindowID;
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
			} else if(state == STATUS_PHOTOGRAPH){
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
		break;
	case WINDOWID_MENU:
		mPic.itemAt(ICON_INDEX_WINDOWPIC)->update(BMPTYPE_WINDOWPIC_MENU);
		if(mCdrMain->getUvcState()){
			mPic.itemAt(ICON_INDEX_UVC)->set(1);
			mPic.itemAt(ICON_INDEX_UVC)->update();
		} else {
			mPic.itemAt(ICON_INDEX_UVC)->set(0);
			mPic.itemAt(ICON_INDEX_UVC)->update();
		}
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
			SetWindowText(GetDlgItem(mHwnd, ID_STATUSBAR_LABEL1), "");
			UpdateWindow(GetDlgItem(mHwnd, ID_STATUSBAR_LABEL1), false);
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
