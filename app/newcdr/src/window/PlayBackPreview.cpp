/*************************************************************************
  > File Name: PlayBackPreview.c
  > Description: 
  > Author: CaoDan
  > Mail: caodan2519@gmail.com 
  > Created Time: 2014年04月18日 星期五 13:56:16
 ************************************************************************/

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <binder/IMemory.h>
#include <include_media/media/HerbMediaMetadataRetriever.h>
#include <private/media/VideoFrame.h>
//#include <CedarMediaServerCaller.h>

#include "windows.h"
#include "keyEvent.h"
#include "cdr_message.h"
#include "StorageManager.h"
#include "Dialog.h"
#undef LOG_TAG
#define LOG_TAG "PlayBackPreview.cpp"
#include "debug.h"

#define EMBEDDED_PICTURE_TYPE_ANY		0xFFFF
#define ONT_SHOT_TIMER	100
#define IMAGE_CONTROL_SHOW_BMP

using namespace android;

extern int scaler_bmp_init(int scn, int layer, CDR_RECT *rect);
extern int scaler_bmp_show(int screen, const BITMAP *bmpImage, CDR_RECT *rect=0);
extern int scaler_bmp_exit();

int PlayBackPreview::keyProc(int keyCode, int isLongPress)
{
	HWND hStatusBar;
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	switch(keyCode) {
	case CDR_KEY_OK:
		if(isLongPress == SHORT_PRESS) {
			return playBackVideo();
		}
		else if (isLongPress == LONG_PRESS) {
			db_msg("go to RecordPreview\n");
			#ifndef USE_NEWUI
			SendMessage(hStatusBar, STBM_CLEAR_LABEL1_TEXT, WINDOWID_PLAYBACKPREVIEW, 0);
			#endif
			return WINDOWID_RECORDPREVIEW;
		}
		break;
	case CDR_KEY_MODE:
		if(isLongPress == SHORT_PRESS) {
		} else if(isLongPress == LONG_PRESS) {
			db_msg("long press");
			StorageManager* sm;
			sm = StorageManager::getInstance();
			if(sm->isMount() == false) {
				ShowTipLabel(mHwnd, LABEL_NO_TFCARD, false);
				break;
			}
			if(fileInfo.fileCounts == 0) {
				db_msg("no file to delete\n");	
				ShowTipLabel(mHwnd, LABEL_FILELIST_EMPTY, false);
			} else
				deleteFileDialog();
		}
		break;
	case CDR_KEY_LEFT:
		if(isLongPress == SHORT_PRESS)
			prevPreviewFile();
		else if(isLongPress == LONG_PRESS)
			prevPagePreviewFile();
		break;
	case CDR_KEY_RIGHT:
		if(isLongPress == SHORT_PRESS)
			nextPreviewFile();
		else if(isLongPress == LONG_PRESS)
			nextPagePreviewFile();
		break;
	default:
		break;
	}
	return WINDOWID_PLAYBACKPREVIEW;
}

int PlayBackPreviewProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	PlayBackPreview* mPLBPreview = (PlayBackPreview*)GetWindowAdditionalData(hWnd);
	db_msg("message code: [%d], %s, wParam=%d, lParam=%lu\n", message, Message2Str(message), wParam, lParam);
	switch(message) {
	case MSG_CREATE:
		mPLBPreview->createSubWidgets(hWnd);
		#ifdef USE_NEWUI		
		SetWindowElementAttr(GetDlgItem(hWnd, ID_PLAYBACKPREVIEW_FC), 
			WE_FGC_WINDOW, Pixel2DWORD(HDC_SCREEN, 0xFFFFFFFF));
		SetWindowText(GetDlgItem(hWnd, ID_PLAYBACKPREVIEW_FC), "(00/00)");
		#endif
		break;
	case MSG_SHOWWINDOW:
		if(wParam == SW_SHOWNORMAL) {
			mPLBPreview->showPlaybackPreview();
		}
		break;
	case MSG_WINDOW_CHANGED:
		scaler_bmp_exit();
		break;
	#ifdef USE_NEWUI
	case MSG_SHOW_TIP_LABEL:
		ShowTipLabel(hWnd, (enum labelIndex)wParam, false, 3000);
		break;
	#endif
	case MSG_CDR_KEY:
		db_msg("key code is %d\n", wParam);
		return mPLBPreview->keyProc(wParam, lParam);
	case MSG_UPDATE_LIST:
		{
			int ret = mPLBPreview->updatePreviewFilelist();
			if(ret == 0) {
				mPLBPreview->showThumnail();
			} else if(ret == 1) {
				mPLBPreview->showBlank();
			}
		}
		break;
	case MSG_DESTROY:
#ifdef DISPLAY_READY
#ifndef IMAGE_CONTROL_SHOW_BMP
		scaler_bmp_exit();
#endif
#endif
#ifdef USE_NEWUI
		mPLBPreview->destroyWidgets();
#endif
		break;
	default:
		break;
	}
	return DefaultWindowProc(hWnd, message, wParam, lParam);
}

	PlayBackPreview::PlayBackPreview(CdrMain* pCdrMain)
	: mCdrMain(pCdrMain)
	, bmpIcon()
	, bmpImage()
	, fileInfo()
	, mDispScreen(0)
{
	mShowPlayBackThumbT = NULL;
	HWND hParent;
	RECT rect;
	CDR_RECT STBRect;
	ResourceManager* mRM;
	hParent = mCdrMain->getHwnd();
	mRM = ResourceManager::getInstance();
	mRM->getResRect(ID_STATUSBAR, STBRect);
	GetWindowRect(hParent, &rect);
	rect.top += (STBRect.h);
	mHwnd = CreateWindowEx(WINDOW_PLAYBACKPREVIEW, "",
			WS_NONE,
			WS_EX_TRANSPARENT | WS_EX_USEPARENTFONT,
			WINDOWID_PLAYBACKPREVIEW,
			rect.left, rect.top, RECTW(rect), RECTH(rect),
			hParent, (DWORD)this);
	if(mHwnd == HWND_INVALID) {
		db_error("create PlayBack Preview window failed\n");
		return;
	}
	ShowWindow(mHwnd, SW_HIDE);

#ifdef USE_NEWUI	
	mDispRect.x = rect.left;
	mDispRect.y = rect.top;
	mDispRect.w = RECTW(rect);
	mDispRect.h = RECTH(rect);
#else
	mRM->getResRect(ID_PLAYBACKPREVIEW_IMAGE, mDispRect);
	mDispRect.y += STBRect.h;
#endif
}
PlayBackPreview::~PlayBackPreview()
{
}

#ifdef USE_NEWUI
int PlayBackPreview::createSubWidgets(HWND hParent)
{
	HWND hWnd;
	ResourceManager* mRM;
	CDR_RECT rect;
	int retval;
	RECT mgRect;

	mRM = ResourceManager::getInstance();
	GetClientRect(hParent, &mgRect);
	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_TRANSPARENT,
			ID_PLAYBACKPREVIEW_IMAGE,
			mgRect.left, mgRect.top, RECTW(mgRect), RECTH(mgRect),
			hParent, 0);

	if(hWnd == HWND_INVALID) {
		db_error("create playback previe image label failed\n");
		return -1;
	}

	retval = mRM->getResBmp(ID_PLAYBACKPREVIEW_ICON, BMPTYPE_BASE, bmpIcon);
	if(retval < 0) {
		db_error("get RES_BMP_CURRENT_PLBPREVIEW_ICON failed\n");
		return -1;
	}

	rect.x = (RECTW(mgRect)/2) - (bmpIcon.bmWidth/2);
	rect.y = (RECTH(mgRect)/2) - (bmpIcon.bmHeight/2);

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_NONE | WS_EX_TRANSPARENT,
			ID_PLAYBACKPREVIEW_ICON,
			rect.x, rect.y, bmpIcon.bmWidth, bmpIcon.bmHeight,
			hParent, (LPARAM)&bmpIcon);
	if(hWnd == HWND_INVALID) {
		db_error("create playback preview icon label failed\n");
		return -1;
	}

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_CHILD | WS_VISIBLE | SS_SIMPLE | SS_VCENTER | SS_CENTER,
			WS_EX_TRANSPARENT,
			ID_PLAYBACKPREVIEW_FC,
			0, RECTH(mgRect) - 58, RECTW(mgRect), 58,
			hParent, 0);
	if(hWnd == HWND_INVALID) {
		db_error("create playback preview file count label failed\n");
		return -1;
	}
	mLogFont = CreateLogFont("*", "fixed", "GB2312-0", 
			FONT_WEIGHT_REGULAR, FONT_SLANT_ROMAN, FONT_FLIP_NIL,
			FONT_OTHER_AUTOSCALE, FONT_UNDERLINE_NONE, FONT_STRUCKOUT_NONE, 32, 0);
	SetWindowFont(hWnd, mLogFont);

	return 0;
}

int PlayBackPreview::destroyWidgets()
{
	DestroyLogFont(mLogFont);
	return 0;
}

#else
int PlayBackPreview::createSubWidgets(HWND hParent)
{
	HWND hWnd;
	ResourceManager* mRM;
	CDR_RECT rect;
	int retval;
	RECT mgRect;
	mRM = ResourceManager::getInstance();
	mRM->getResRect(ID_PLAYBACKPREVIEW_IMAGE, rect);
	GetWindowRect(mHwnd, &mgRect);
	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_NONE,
			ID_PLAYBACKPREVIEW_IMAGE,
			rect.x, rect.y, rect.w, rect.h,
			hParent, 0);

	if(hWnd == HWND_INVALID) {
		db_error("create playback previe image label failed\n");
		return -1;
	}
	SetWindowBkColor(hWnd, RGBA2Pixel(HDC_SCREEN, 0x00, 0x00, 0x00, 0x00));
	mRM->getResRect(ID_PLAYBACKPREVIEW_ICON, rect);

	retval = mRM->getResBmp(ID_PLAYBACKPREVIEW_ICON, BMPTYPE_BASE, bmpIcon);
	if(retval < 0) {
		db_error("get RES_BMP_CURRENT_PLBPREVIEW_ICON failed\n");
		return -1;
	}

	hWnd = CreateWindowEx(CTRL_STATIC, "",
			WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_CENTERIMAGE,
			WS_EX_NONE | WS_EX_TRANSPARENT,
			ID_PLAYBACKPREVIEW_ICON,
			rect.x, rect.y, bmpIcon.bmWidth, bmpIcon.bmHeight,
			hParent, (LPARAM)&bmpIcon);
	if(hWnd == HWND_INVALID) {
		db_error("create playback preview image label failed\n");
		return -1;
	}
	//	ShowWindow(hWnd, SW_HIDE);

	return 0;
}

#endif

int PlayBackPreview::getFileInfo(void)
{
	int retval;
	StorageManager* mSTM;
	dbRecord_t dbRecord;

	mSTM = StorageManager::getInstance();
	retval = mSTM->dbCount((char*)TYPE_VIDEO);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	db_msg("video fileCounts is %d\n", retval);
	fileInfo.fileCounts = retval;

	retval = mSTM->dbCount((char*)TYPE_PIC);
	if(retval < 0) {
		db_error("get db count failed\n");
		return -1;
	}
	db_msg("photo fileCounts is %d\n", retval);
	fileInfo.fileCounts += retval;

	db_msg("curIdx is %d\n", fileInfo.curIdx);
	if(fileInfo.fileCounts == 0) {
		db_msg("empty database\n");	
		return 0;
	}
	if(mSTM->dbGetSortByTimeRecord(dbRecord, fileInfo.curIdx) < 0) {
		db_error("get sort by time record failed\n");
		return -1;
	}
	fileInfo.curFileName = dbRecord.file;
	fileInfo.curTypeString = dbRecord.type;
	fileInfo.curInfoString = dbRecord.info;
	db_msg("cur file is %s\n", fileInfo.curFileName.string());
	db_msg("cur Type is %s\n", fileInfo.curTypeString.string());
	db_msg("cur info is %s\n", fileInfo.curInfoString.string());
	return 0;
}
int PlayBackPreview::updatePreviewFilelist(void)
{
	HWND hStatusBar;
	char buf[50] = {0};
	char buf2[50] = {0};
	int ret = 0;
	if(getFileInfo() < 0)
		return 1;
	StorageManager *sm = StorageManager::getInstance();
	if(!sm->isMount() || (fileInfo.fileCounts == 0)) {
		sprintf(buf, "(00/00)");
		ret = 1;
	} else {
		if(getBaseName(fileInfo.curFileName.string(), buf2) != NULL)
			sprintf(buf, "%s (%02d/%02d)", buf2, fileInfo.curIdx + 1, fileInfo.fileCounts);
	}
	#ifdef USE_NEWUI	
	SetWindowText(GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_FC), buf);
	#else
	hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
	SendMessage(hStatusBar, STBM_SETLABEL1_TEXT, 0, (LPARAM)buf);
	#endif
	return ret;
}
#define SHOW_JPEG_PIC 1
int PlayBackPreview::getPicFileName(String8 &bmpFile)
{
	char buf[128] = {0};
	char buf2[128] = {0};
	char bufdir[128] = {0};
	char picFile[128] = {0};
	char thumbDir[128] = {0};
	int ret = 0;
	if(strncmp(fileInfo.curTypeString.string(), TYPE_PIC, 5) == 0) {	//generate pic thumbnail
    #if SHOW_JPEG_PIC
	    snprintf(picFile, 128, "%s",fileInfo.curFileName.string());
    #else
		/* hardware decode jpeg */
		if(getBaseName(fileInfo.curFileName.string(), buf2) == NULL) {
			db_error("get base name failed, file is %s\n", fileInfo.curFileName.string());
			return -1;
		}
		if(removeSuffix(buf2, buf) == NULL) {
			db_error("remove the file suffix failed, file is %s\n", fileInfo.curFileName.string());
			return -1;
		}
                if(getDir(fileInfo.curFileName.string(), buf2) == NULL) {
			db_error("get dir name failed, file is %s\n", fileInfo.curFileName.string());
			return -1;
		}
		sprintf(picFile, "%s%s/%s.bmp", bufdir,THUMBNAIL_DIR,buf);
		sprintf(thumbDir, "%s/%s", bufdir,THUMBNAIL_DIR);
		if(access(thumbDir, F_OK) != 0) {
			if(mkdir(thumbDir, 0777) != 0) {
				db_error("create dir %s failed\n",thumbDir);	
				return -1;
			}
		}
		if(access(picFile , F_OK) != 0) {
			/*CedarMediaServerCaller *caller = new CedarMediaServerCaller();
			int fd = open(fileInfo.curFileName.string(), O_RDWR);
			if (fd < 0) {
				db_error("fail to read %s", fileInfo.curFileName.string());
			}
			if (caller == NULL) {
				close(fd);
			    ALOGE("Failed to alloc CedarMediaServerCaller");
				return -1;
			}
			sp<IMemory> mem = caller->jpegDecode(fd);
			if (mem == NULL) {
			    ALOGE("jpegDecode error!");
			    delete caller;
				close(fd);
			    return -1;
			}
			close(fd);
			VideoFrame *videoFrame = NULL;
			videoFrame = static_cast<VideoFrame *>(mem->pointer());
			if (videoFrame == NULL) {
				db_error("getFrameAtTime: videoFrame is a NULL pointer");
				ret = -1;
				return ret;
			}
			StorageManager *sm = StorageManager::getInstance();
			ret = sm->saveVideoFrameToBmpFile(videoFrame, picFile);*/
		}
    #endif
	} else { //generate video thumbnail
		if(getBaseName(fileInfo.curFileName.string(), buf2) == NULL) {
			db_error("get base name failed, file is %s\n", fileInfo.curFileName.string());
			return -1;
		}
		db_msg("basename is %s\n", buf2);
		if(removeSuffix(buf2, buf) == NULL) {
			db_error("remove the file suffix failed, file is %s\n", fileInfo.curFileName.string());
			return -1;
		}
		db_msg("buf is %s\n", buf);
		if(getDir(fileInfo.curFileName.string(), bufdir) == NULL) {
			db_error("get dir name failed, file is %s\n", fileInfo.curFileName.string());
			return -1;
                }
                   
		sprintf(picFile, "%s/%s%s.bmp", bufdir,THUMBNAIL_DIR,buf);
		db_msg("picFile is %s\n", picFile);
		sprintf(thumbDir, "%s/%s", bufdir,THUMBNAIL_DIR);
		if(access(thumbDir, F_OK) != 0) {
			if(mkdir(thumbDir, 0777) != 0) {
				db_error("create dir %s failed\n", thumbDir);	
				return -1;
			}
		}
		if(access(picFile , F_OK) != 0) {	//there is no thunbnail
			if(getBmpPicFromVideo(fileInfo.curFileName.string(), picFile) != 0) {
				db_error("get picture from video failed\n");	
				return -1;
			}
		}
	}
	bmpFile = String8(picFile);
	return 0;
}


int PlayBackPreview::beforeOtherScreen()
{
	return 0;
}

int PlayBackPreview::afterOtherScreen(int layer, int screen)
{
    return 0;
}

void PlayBackPreview::otherScreen(int screen)
{
	mDispScreen = screen;
	if (screen == 1) {
		mDispRect.x = 50;
		mDispRect.y = 50;
		mDispRect.w = 1200;
		mDispRect.h = 700;
	} else {
		RECT rect;
		CDR_RECT STBRect;
		ResourceManager* mRM;
		mRM = ResourceManager::getInstance();
		mRM->getResRect(ID_STATUSBAR, STBRect);
		GetWindowRect(GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE), &rect);
		mDispRect.x = rect.left;
		mDispRect.y = rect.top;
		mDispRect.y += STBRect.h;
		mDispRect.w = RECTW(rect);
		mDispRect.h = RECTH(rect);
	}
}


void PlayBackPreview::showBlank()
{
 	HWND retWnd;
	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE);

	db_msg("erase PLBPREVIEW_IMAGE background!");
#ifdef IMAGE_CONTROL_SHOW_BMP
	//if (mDispScreen == 1) {
		SendMessage(retWnd, STM_SETIMAGE, (WPARAM)0, 0);
		ShowWindow(GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE), SW_SHOWNORMAL);
	//}
#endif
#ifdef DISPLAY_READY
#ifndef IMAGE_CONTROL_SHOW_BMP
	scaler_bmp_show(mDispScreen, 0, &mDispRect);
#endif
#endif
	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_ICON);
	ShowWindow(retWnd, SW_HIDE);
}


void PlayBackPreview::showThumnailByThread(void)
{
       while(1) {
           //ALOGD(" ywz-------------fileInfo.curIdx(%d),fileInfo.showedIndx(%d)",fileInfo.curIdx,fileInfo.showedIndx);
           if(fileInfo.curIdx != fileInfo.showedIndx){
               fileInfo.showedIndx = fileInfo.curIdx;
               #ifdef IMAGE_CONTROL_SHOW_BMP
               SendMessage(mCdrMain->getWindowHandle(WINDOWID_PLAYBACKPREVIEW), MSG_UPDATE_LIST, 0, 0);
               #else
               int ret = updatePreviewFilelist();
               if(ret == 0) {
                   showThumnail();
               } else if(ret == 1) {
                   showBlank();
               }
               #endif
               usleep(50*1000);
               continue;
           }
           usleep(50*1000);
       }
}

void PlayBackPreview::showThumnail(void)
{
	int ret;
	String8 bmpFile;
	HWND retWnd;
	if(fileInfo.fileCounts == 0) {
		db_msg("no file to preview\n");
		return;
	}
	db_msg("current file type is %s\n", fileInfo.curTypeString.string());
	db_msg("current file name is %s\n", fileInfo.curFileName.string());
	if(getPicFileName(bmpFile) < 0)	{
		db_error("get pic file name failed\n");
		showBlank();
		return ;
	}
	if(bmpImage.bmBits)
		UnloadBitmap(&bmpImage);
	if((ret = LoadBitmapFromFile(HDC_SCREEN, &bmpImage, bmpFile.string())) != ERR_BMP_OK) {
		db_error("load %s failed, ret is %d\n", bmpFile.string(), ret);	
		return;
	}

	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE);
#ifdef IMAGE_CONTROL_SHOW_BMP
        // show jpg photo on minigui, else show bmp frame(from video) with layer 0, to fix show wrong image of layer 0 when start play video
        if(isCurrentVideoFile() == false) {
		SendMessage(retWnd, STM_SETIMAGE, (WPARAM)&bmpImage, 0);
		ShowWindow(GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE), SW_SHOWNORMAL);
	} else {
		SendMessage(retWnd, STM_SETIMAGE, (WPARAM)&bmpImage, 0);
		ShowWindow(GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_IMAGE), SW_SHOWNORMAL);	
	return;
		HWND hParent = mCdrMain->getHwnd();
                CDR_RECT cdr_rect;
                RECT rect;
	        GetWindowRect(hParent, &rect);
                cdr_rect.x = rect.left;
                cdr_rect.y = rect.top;
                cdr_rect.w = RECTW(rect);
                cdr_rect.h = RECTH(rect);
                scaler_bmp_show(mDispScreen, &bmpImage, &cdr_rect);
        }
#endif
#ifdef DISPLAY_READY
#ifndef IMAGE_CONTROL_SHOW_BMP
	scaler_bmp_show(mDispScreen, &bmpImage, &mDispRect);
#endif
#endif
	retWnd = GetDlgItem(mHwnd, ID_PLAYBACKPREVIEW_ICON);
	if(strncmp(fileInfo.curTypeString.string(), TYPE_PIC, 5) == 0) {
		if(IsWindowVisible(retWnd)) {
			db_msg("xxxxxxxxxx\n");	
			ShowWindow(retWnd, SW_HIDE);
		}
	} else {
		if(!IsWindowVisible(retWnd)) {
			ShowWindow(retWnd, SW_SHOWNORMAL);
		}
	}
}

int PlayBackPreview::getBmpPicFromVideo(const char* videoFile, const char* picFile)
{
	status_t ret;
	int retval = 0;
	sp<IMemory> frameMemory = NULL;
	HerbMediaMetadataRetriever *retriever;
	StorageManager* mSTM;

	mSTM = StorageManager::getInstance();
	retriever = new HerbMediaMetadataRetriever();
	if (retriever == NULL) {
		db_error("Failed to alloc HerbMediaMetadataRetriever");
		return -1;
	}
	ret = retriever->setDataSource(videoFile);
	if (ret != NO_ERROR) {
		db_error("setDataSource failed(%d)", ret);
		retval = -1;
		goto out;
	}
	frameMemory = retriever->getEmbeddedPicture(EMBEDDED_PICTURE_TYPE_ANY);
	if (frameMemory != NULL) {
		/*MediaAlbumArt* mediaAlbumArt = NULL;
		db_msg("Have EmbeddedPicture");
		mediaAlbumArt = static_cast<MediaAlbumArt *>(frameMemory->pointer());
		if (mediaAlbumArt == NULL) {
			db_error("getEmbeddedPicture: Call to getEmbeddedPicture failed.");
			retval = -1;
			goto out;
		}*/
		//		unsigned int len = mediaAlbumArt->mSize;
		//		char *data = (char*)mediaAlbumArt + sizeof(MediaAlbumArt);
		/*
		 * TODO: how to get the width and height
		 * */
	} else {
		VideoFrame *videoFrame = NULL;
		db_msg("getFrameAtTime");

		frameMemory = retriever->getFrameAtTime();
		if (frameMemory == NULL) {
			db_error("Failed to get picture");
			retval = -1;
			goto out;
		}

		videoFrame = static_cast<VideoFrame *>(frameMemory->pointer());
		if (videoFrame == NULL) {
			db_error("getFrameAtTime: videoFrame is a NULL pointer");
			retval = -1;
			goto out;
		}

		db_msg("videoFrame: width=%d, height=%d, disp_width=%d,disp_height=%d, size=%d, RotationAngle=%d",
				videoFrame->mWidth, videoFrame->mHeight, videoFrame->mDisplayWidth, videoFrame->mDisplayHeight, videoFrame->mSize, videoFrame->mRotationAngle);

		ret = mSTM->saveVideoFrameToBmpFile(videoFrame, picFile);
		if (ret != ERR_BMP_OK) {
			db_msg("Failed to save picture, ret is %d", ret);
			retval = -1;
			goto out;
		}
	}

out:
	if(retriever)
		delete retriever;
	return retval;
}

bool PlayBackPreview::isCurrentVideoFile(void)
{
	if(access(fileInfo.curFileName.string(), F_OK) != 0) {
		db_error("the current file %s not exsist\n", fileInfo.curFileName.string());
		return false;
	}
	if(strncmp(fileInfo.curTypeString.string(), TYPE_VIDEO, 5) == 0)
		return true;
	else 
		return false;
}

void PlayBackPreview::nextPreviewFile(void)
{
	if(fileInfo.fileCounts == 0)
		return;

	fileInfo.curIdx++;
	if(fileInfo.curIdx >= fileInfo.fileCounts)
		fileInfo.curIdx = 0;
	/*int ret = updatePreviewFilelist();
	if(ret == 0) {
		showThumnail();
	} else if(ret == 1) {
		showBlank();
	}*/
}

void PlayBackPreview::nextPagePreviewFile(void)
{
	int cnt;

	if(fileInfo.fileCounts == 0)
		return;

	if (fileInfo.fileCounts <= 10) {
		fileInfo.curIdx = fileInfo.fileCounts - 1;
		return;
	}
	cnt = fileInfo.curIdx;
	cnt += 10;
	if (cnt <= fileInfo.fileCounts - 1)
		fileInfo.curIdx += 10;
	else if (fileInfo.curIdx == fileInfo.fileCounts - 1)
		fileInfo.curIdx = 9;
	else if (cnt > fileInfo.fileCounts - 1)
		fileInfo.curIdx = fileInfo.fileCounts - 1;
}

void PlayBackPreview::prevPreviewFile(void)
{
	if(fileInfo.fileCounts == 0)
		return;

	if(fileInfo.curIdx == 0)
		fileInfo.curIdx = fileInfo.fileCounts - 1;
	else
		fileInfo.curIdx--;

	/*int ret = updatePreviewFilelist();
	if(ret == 0) {
		showThumnail();
	} else if(ret == 1) {
		showBlank();
	}*/
}

void PlayBackPreview::prevPagePreviewFile(void)
{
	int cnt;

	if(fileInfo.fileCounts == 0)
		return;

	if (fileInfo.fileCounts <= 10) {
		fileInfo.curIdx = 0;
		return;
	}
	cnt = fileInfo.curIdx;
	cnt -= 10;
	if (cnt >= 0)
		fileInfo.curIdx -= 10;
	else if (fileInfo.curIdx == 0)
		fileInfo.curIdx = fileInfo.fileCounts - 10;
	else if (cnt < 0)
		fileInfo.curIdx = 0;
}

void PlayBackPreview::getCurrentFileName(String8 &file)
{
	file = fileInfo.curFileName;
}

void PlayBackPreview::getCurrentFileInfo(PLBPreviewFileInfo_t &info)
{
	info = fileInfo;
}

void PlayBackPreview::setCurIdx(int idx)
{
	fileInfo.curIdx = idx;
}

void PlayBackPreview::showPlaybackPreview()
{
        status_t err;
        if(mShowPlayBackThumbT == NULL) {
                mShowPlayBackThumbT = new ShowPlayBackThumbThread(this);
                err = mShowPlayBackThumbT->start();
                if (err != OK) {
                        mShowPlayBackThumbT.clear();
                }
        }
#ifdef DISPLAY_READY
	scaler_bmp_init(mDispScreen, 0, &mDispRect);
#endif
	int ret = updatePreviewFilelist();
	if(ret == 0) {
		showThumnail();
	} else if(ret == 1) {
		showBlank();
		#ifdef USE_NEWUI
		SendNotifyMessage(mHwnd, MSG_SHOW_TIP_LABEL, LABEL_FILELIST_EMPTY, 0);
		#endif
	}

}

static void deleteDialogCallback(HWND hDlg, void* data)
{
	PlayBackPreview* playbackPreview;
	StorageManager* sm;
	PLBPreviewFileInfo_t fileInfo;

	playbackPreview = (PlayBackPreview*)data;
	sm = StorageManager::getInstance();

	/* delete file */
	playbackPreview->getCurrentFileInfo(fileInfo);
	db_msg("delete the %dth file %s\n", fileInfo.curIdx, fileInfo.curFileName.string());
	sm->deleteFile(fileInfo.curFileName.string());
	sm->dbDelFile(fileInfo.curFileName.string());
	if (fileInfo.curIdx == fileInfo.fileCounts - 1 && fileInfo.curIdx > 0)
		playbackPreview->setCurIdx(fileInfo.curIdx - 1);

	int ret = playbackPreview->updatePreviewFilelist();
	if(ret == 0) {
		playbackPreview->showThumnail();
	} else if(ret == 1) {
		playbackPreview->showBlank();
	}

	EndDialog(hDlg, IDC_BUTTON_OK);
}

void PlayBackPreview::deleteFileDialog(void)
{
	int retval;
	const char* ptr;
	MessageBox_t messageBoxData;
	ResourceManager* rm;
	String8 textInfo;

	rm = ResourceManager::getInstance();
	memset(&messageBoxData, 0, sizeof(messageBoxData));
	messageBoxData.dwStyle = MB_OKCANCEL | MB_HAVE_TITLE | MB_HAVE_TEXT;
	messageBoxData.flag_end_key = 0; /* end the dialog when endkey keydown */

	ptr = rm->getLabel(LANG_LABEL_DELETEFILE_TITLE);
	if(ptr == NULL) {
		db_error("get deletefile title failed\n");
		return;
	}
	messageBoxData.title = ptr;

	ptr = rm->getLabel(LANG_LABEL_DELETEFILE_TEXT);
	if(ptr == NULL) {
		db_error("get deletefile text failed\n");
		return;
	}
	messageBoxData.text = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_OK);
	if(ptr == NULL) {
		db_error("get ok string failed\n");
		return;
	}
	messageBoxData.buttonStr[0] = ptr;

	ptr = rm->getLabel(LANG_LABEL_SUBMENU_CANCEL);
	if(ptr == NULL) {
		db_error("get ok string failed\n");
		return;
	}
	messageBoxData.buttonStr[1] = ptr;

	messageBoxData.pLogFont = rm->getLogFont();

	rm->getResRect(ID_MENU_LIST_MB, messageBoxData.rect);
	messageBoxData.fgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_FGC);
	messageBoxData.bgcWidget = rm->getResColor(ID_MENU_LIST_MB, COLOR_BGC);
	messageBoxData.linecTitle = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_TITLE);
	messageBoxData.linecItem = rm->getResColor(ID_MENU_LIST_MB, COLOR_LINEC_ITEM);
	messageBoxData.confirmCallback = deleteDialogCallback;
	messageBoxData.confirmData = (void*)this;

	db_msg("this is %p\n", this);
	retval = showMessageBox(mHwnd, &messageBoxData);
	db_msg("retval is %d\n", retval);
}

int PlayBackPreview::playBackVideo(void)
{
	int retval;
	HWND hPlayBack, hStatusBar;
	StorageManager* sm;

	sm = StorageManager::getInstance();
	if(sm->isMount() == false) {
		ShowTipLabel(mHwnd, LABEL_NO_TFCARD);
		goto out;
	}
	if(fileInfo.fileCounts == 0) {
		ShowTipLabel(mHwnd, LABEL_FILELIST_EMPTY);
		goto out;
	}

	if(isCurrentVideoFile() == true) {
		db_msg("start playback\n");
		hPlayBack = mCdrMain->getWindowHandle(WINDOWID_PLAYBACK);
		retval = SendMessage(hPlayBack, MSG_PREPARE_PLAYBACK, (WPARAM)&(fileInfo.curFileName), 0);
		if(retval < 0) {
			db_msg("prepare playback failed\n");	
			ShowTipLabel(mHwnd, LABEL_PLAYBACK_FAIL);
			goto out;
		}

		#ifndef USE_NEWUI
		hStatusBar = mCdrMain->getWindowHandle(WINDOWID_STATUSBAR);
		SendMessage(hStatusBar, STBM_CLEAR_LABEL1_TEXT, WINDOWID_PLAYBACKPREVIEW, 0);
		#endif
		return WINDOWID_PLAYBACK;
	}

out:
	return WINDOWID_PLAYBACKPREVIEW;;
}

