
#ifndef __PICOBJ_H__
#define __PICOBJ_H__

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <utils/String8.h>

#include "resourceManager.h"

class PicObj
{
public:
	PicObj(android::String8 name);
	~PicObj();

	PBITMAP getImage();
	enum ResourceID getId();
	void setId(enum ResourceID id);
	void update();
	void update(enum BmpType type);
	void set(int val);
	int get(void);
	HWND mParent;
	android::String8 mName;
private:
	enum ResourceID mId;
	int mVal;
	BITMAP mBmp;
};

typedef struct pic_t
{
	enum ResourceID id;
	const char *name;
}pic_t;

#endif
