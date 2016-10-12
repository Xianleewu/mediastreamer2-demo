#ifndef _RECORD_PREVIEW_H
#define _RECORD_PREVIEW_H

#include <time.h>
#include "windows.h"
#include "keyEvent.h"
#include "cdr_message.h"
#include "resourceManager.h"
#include "camera.h"
#include "Dialog.h"

#define TMP_RECDRD_STATE "/tmp/recdrd_state"

enum{
   VIDEO_MODE_FONT_CAMERA = 0,
   VIDEO_MODE_BACK_CAMERA,
   VIDEO_MODE_DOUBLE_CAMERA
};

class BufferUsedCnt {
public:
	BufferUsedCnt(int camId, int index) : mCamId(camId), mIndex(index), mUsedCnt(0){};
	~BufferUsedCnt(){};
	//return current count
	int incUsedCnt(int cnt)
	{
		Mutex::Autolock lock(mBufferUsedCntMutex);
		mUsedCnt += cnt;
		return mUsedCnt;
	};
	int decUsedCnt(int cnt)
	{
		Mutex::Autolock lock(mBufferUsedCntMutex);
		mUsedCnt -= cnt;
		return mUsedCnt;
	};
	int getUsedCnt()
	{
		Mutex::Autolock lock(mBufferUsedCntMutex);
		return mUsedCnt;
	};
	int mCamId;
	int mIndex;

private:
	mutable Mutex mBufferUsedCntMutex;
	int mUsedCnt;
};

#endif
