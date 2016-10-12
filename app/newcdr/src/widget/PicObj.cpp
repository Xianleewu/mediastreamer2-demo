
#include "PicObj.h"
#undef LOG_TAG
#define LOG_TAG "PicObj.cpp"
#include "debug.h"

using namespace android;

PicObj::PicObj(String8 name)
{
	mName = name;
	memset(&mBmp, 0, sizeof(BITMAP));
}

PicObj::~PicObj()
{

}

PBITMAP PicObj::getImage()
{
	return &mBmp;
}

enum ResourceID PicObj::getId()
{
	return mId;
}

void PicObj::setId(enum ResourceID id)
{
	mId = id;
}

void PicObj::update()
{
	ResourceManager *rm;
	int retval;

	db_msg("update-----val:%d mId%d\n", mVal, mId);
	rm = ResourceManager::getInstance();
	if(mId != ID_STATUSBAR_ICON_AUDIO) {
		rm->setCurrentIconValue(mId, mVal);
	}
	else {
		rm->setResBoolValue(mId, mVal);
	}

	rm->unloadBitMap(mBmp);
	retval = rm->getResBmp(mId, BMPTYPE_BASE, mBmp);
	if (retval) {
		db_msg("get pic %s failed\n", mName.string());
	}
	
	HWND hWnd;
	RECT rect;

	db_msg("mParent is 0x%x\n", mParent);
	hWnd = GetDlgItem(mParent, mId);
	db_msg("hWnd is 0x%x\n", hWnd);
	GetClientRect(hWnd, &rect);

	InvalidateRect(hWnd, &rect, true);
	SendMessage(hWnd, STM_SETIMAGE, (WPARAM)&mBmp, 0);
}

void PicObj::update(enum BmpType type)
{
	HWND hWnd;
	RECT rect;
	int retval;
	ResourceManager* rm;

	rm = ResourceManager::getInstance();

	rm->unloadBitMap(mBmp);
	retval = rm->getResBmp(mId, type, mBmp);
	if(retval < 0) {
		db_msg("get pic %s failed\n", mName.string());
		return;
	}

	hWnd = GetDlgItem(mParent, mId);
	GetClientRect(hWnd, &rect);

	InvalidateRect(hWnd, &rect, true);
	SendMessage(hWnd, STM_SETIMAGE, (WPARAM)&mBmp, 0);
}

void PicObj::set(int val)
{
	mVal = val;
}

int PicObj::get(void)
{
	return mVal;
}

