#ifndef _CDR_DISPLAY_H
#include <CedarDisplay.h>

#include "misc.h"
typedef struct __ion_private_data_t
{
    ion_user_handle_t ion_hnd;
	int shared_fd;
	int vir_addr;
	int len;
	int w;
	int h;
	int pitch;
}ion_private_data_t;

typedef struct view_info ViewInfo;

using namespace android;
class CdrDisplay
{
public:
    class DisplayCallback;
	CdrDisplay(int disp_id);
	~CdrDisplay();
	int getHandle();
	void setBottom();
	void setRect(CDR_RECT &rect);
	void open();
	void close();
	void exchange(int hlay, int flag); //flag:1-otherOnTop
	void otherScreen(int screen, int hlay);
	void clean(void);

	void setZorder(int zorder);

	int requestSurface(int w, int h, int pitch);
	void releaseSurface();
	ion_private_data_t * getSurface(){return mSurface;};

	void commitToDisplay(struct PreviewBuffer *param, bool bAsyncMode);
	static void returnBuffer(void *lparam, void *rparam);

    /**
     * Callback interface used to release display frames after
     * they are displayed.
     *
     * @see #setDisplayCallback(CdrDisplay.DisplayCallback)
     */
    class DisplayCallback
    {
    public:
		DisplayCallback(){}
		virtual ~DisplayCallback(){}
		virtual void returnFrame(struct PreviewBuffer *pBuf) = 0;
    };
	void setDisplayCallback(DisplayCallback *callback);

private:
	CedarDisplay* mCD;
	DisplayCallback *mDisplayCallback;
	int mHlay;
	ion_private_data_t *mSurface;
	int mZorder;
	int format;
	struct surface mDispInfo;
	bool mLayerOpened;
};

#define _CDR_DISPLAY_H
#endif
