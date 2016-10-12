#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <asm-generic/ioctl.h>
#include <signal.h>

#include <cutils/log.h>
#include "misc.h"
#include "debug.h"

#include <CdrDisplay.h>
#include <system/graphics.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#undef LOG_TAG
#define LOG_TAG "LayerBmpShow"

#include "drv_disp.h"

static CdrDisplay *sDisp = NULL;
static Mutex sDispLock;
int scaler_bmp_init(int scn, int layer, CDR_RECT* dispRect)
{
	db_debug("scaler_bmp_init \n");
	if(sDisp == NULL) {
		CdrDisplay *disp = new CdrDisplay(0);
		
		disp->setZorder(0);
		disp->setRect(*dispRect);
		disp->setDisplayCallback(NULL);
		sDisp = disp;
	}
        sDisp->open();
	return 0;
}


int scaler_bmp_show(int scn, const BITMAP *bmpImage, CDR_RECT *rect)
{
	///return 0;
	db_debug("scaler_bmp_show %x, (%d,%d,%d,%d)\n",bmpImage,rect->x,rect->y,rect->w,rect->h);
	if(sDisp == NULL)
		return -1;

	if(bmpImage == NULL) {
		ALOGV("show blank");
	#if 0
		sDisp->setRect(*rect);
		sDisp->setDisplayCallback(NULL);
		PreviewBuffer *buffer = new PreviewBuffer;

		buffer->index = 1;
		buffer->format = HAL_PIXEL_FORMAT_RGBX_8888;
		buffer->width = rect->w;
		buffer->height = rect->h;
		buffer->stride = rect->w;
		buffer->share_fd = 0;
		buffer->vaddr = new char[rect->w * rect->h * 4];
		memset(buffer->vaddr, 0x00, rect->w * rect->h * 4);
		sDisp->commitToDisplay(buffer, false);
		delete(buffer->vaddr);
		delete(buffer); 
	#endif
	} else {
		db_msg("show thumbnail bmp addr %x\n", bmpImage->bmBits);
		sDisp->setRect(*rect);
		sDisp->setDisplayCallback(NULL);
		PreviewBuffer *buffer = new PreviewBuffer;
		buffer->index = 1;
		buffer->format = HAL_PIXEL_FORMAT_RGBX_8888;
		buffer->width = bmpImage->bmWidth;
		buffer->height = bmpImage->bmHeight;
		buffer->stride = bmpImage->bmWidth;
		buffer->share_fd = 0;


                sDispLock.lock();
                //  if display src picture size changed,  realloc surface is needed
                ion_private_data_t *surface = sDisp->getSurface();
                if((NULL != surface) && 
                        ((surface->w != bmpImage->bmWidth) || 
                        (surface->h != bmpImage->bmHeight) ||
                        (surface->pitch !=  bmpImage->bmWidth * 4))) {
                        sDisp->releaseSurface();
                }

                // alloc surface
                if(NULL == sDisp->getSurface())
                        sDisp->requestSurface(bmpImage->bmWidth, bmpImage->bmHeight, bmpImage->bmWidth * 4);

                surface = sDisp->getSurface();

                memcpy((void *)surface->vir_addr, bmpImage->bmBits, surface->len);
		buffer->share_fd = surface->shared_fd;
		sDisp->commitToDisplay(buffer, false);
		delete(buffer);
                sDispLock.unlock();
	}
	return 0;
}

int scaler_bmp_exit()
{
	db_debug("scaler_bmp_exit \n");
        
        sDispLock.lock();
        if(sDisp != NULL) {
                sDisp->releaseSurface();
                sDisp->close();
        }
        sDispLock.unlock();
	return 0;
}

