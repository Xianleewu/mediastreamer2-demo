#include <CdrDisplay.h>
#undef LOG_TAG
#define LOG_TAG "CdrDisplay.cpp"
#include "debug.h"
static int g_ion_fd = -1;

CdrDisplay::CdrDisplay(int disp_id)
	: mDisplayCallback(NULL),
	mSurface(NULL),
	mZorder(0),
	format(0)
{
	mCD = CedarDisplay::getInstance(disp_id);
	mLayerOpened = true;
}

void CdrDisplay::setZorder(int zorder)
{
	mZorder = zorder;
}
void CdrDisplay::returnBuffer(void *lparam, void *rparam)
{
	CdrDisplay *disp = (CdrDisplay *)lparam;
	struct DispBufferInfo *buffer_info = (struct DispBufferInfo *)rparam;

	if((disp == NULL) || (disp->mDisplayCallback == NULL))
		return;

	//db_msg("CdrDisplay: return buffer disp->mDisplayCallback %x\n", disp->mDisplayCallback);
	disp->mDisplayCallback->returnFrame(buffer_info->previewBuffer);
	delete(buffer_info);
}
void CdrDisplay::commitToDisplay(struct PreviewBuffer *param, bool bAsyncMode)
{
	DispBufferInfo *buffer_info = new DispBufferInfo;

	//db_msg("commitToDisplay,  disp->mDisplayCallback %x, mSurface %x,	mZorder %d, format %d\n"
	//	, mDisplayCallback
	//	, mSurface
	//	, mZorder
	//	, format);

	if(mSurface && (param->share_fd == mSurface->shared_fd)) {
		param->vaddr = (void *)mSurface->vir_addr;
		param->share_fd = 0;
	}

	buffer_info->previewBuffer = (struct PreviewBuffer *)param;
	buffer_info->zorder = mZorder;
	buffer_info->x = mDispInfo.view.x;
	buffer_info->y = mDispInfo.view.y;
	buffer_info->w = mDispInfo.view.w;
	buffer_info->h = mDispInfo.view.h;
	buffer_info->owner = this;
	buffer_info->returnBuffer = bAsyncMode ? returnBuffer : NULL;
	mCD->commitToDisplay((void *)buffer_info, bAsyncMode);
}

void CdrDisplay::setDisplayCallback(DisplayCallback *callback)
{
	mDisplayCallback = callback;
}
ion_private_data_t* allocIonSurface (int w, int h, int pitch)
{
	ion_user_handle_t handle;
	size_t len, ret, ion_fd, shared_fd, map_fd;
	int align = 0;
	int alloc_flags = 0;
	int heap_mask = 1;
	int prot = PROT_READ | PROT_WRITE;
	int map_flags = MAP_SHARED;
	unsigned char *ptr;

	db_msg("allocIonSurface \n");

	alloc_flags = 0;//(ION_FLAG_CACHED|ION_FLAG_CACHED_NEEDS_SYNC);
	heap_mask = /*ION_HEAP(ION_VMALLOC_HEAP_ID) | */ION_HEAP(ION_CMA_HEAP_ID);

	if(g_ion_fd < 0) {
		g_ion_fd = ion_open();
	    if (g_ion_fd < 0)
	    return NULL;
	}

	ion_fd = g_ion_fd;

	len = h * pitch;
	ret = ion_alloc(ion_fd, len, align, heap_mask, alloc_flags, &handle);
	if (ret) {
	    db_msg("%s ion_alloc failed: %s\n", __func__, strerror(ret));
		return NULL;
	}

	ret = ion_share(ion_fd, handle, (int *)&shared_fd);
	if (ret != 0)
	{
		db_error("ion_share( %d ) failed", ion_fd);

		if (0 != ion_free(ion_fd, handle))
		{
			db_error("ion_free( %d ) failed", ion_fd);
		}
		return NULL;
	}

	//ret = ion_map(ion_fd, handle, len, prot, map_flags, 0, &ptr, &map_fd);
	ptr = (unsigned char *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
	if (MAP_FAILED == ptr)
	{
        db_error("%s ion_map failed\n", __func__);
		if (0 != ion_free(ion_fd, handle))
		{
			db_error("ion_free( %d ) failed", ion_fd);
		}

		close(shared_fd);
		return NULL;
	}

	ion_private_data_t *priv_data = (ion_private_data_t *)malloc(sizeof(ion_private_data_t));

	priv_data->ion_hnd = handle;
	priv_data->vir_addr = (int)ptr;
	priv_data->len = len;
	priv_data->shared_fd = shared_fd;
	priv_data->w = w;
	priv_data->h = h;
	priv_data->pitch = pitch;
	db_msg("allocIonSurface handle %d, shared_fd %d, addr %x, len %d\n", handle, shared_fd, ptr, len);

	return priv_data;
}

void freeIonSurface (ion_private_data_t* priv_data)
{
	int ret;
        db_msg("allocIonSurface handle %d, shared_fd %d\n", priv_data->ion_hnd, priv_data->shared_fd);
	ret = ion_free(g_ion_fd, priv_data->ion_hnd);
	if (ret) {
	    db_error("%s failed: %s %d\n", __func__, strerror(ret), priv_data->ion_hnd);
	    return;
	}

	munmap((void *)priv_data->vir_addr, priv_data->len);
	close(priv_data->shared_fd);
	free(priv_data);
}
int CdrDisplay::requestSurface(int w, int h, int pitch)
{
    if(NULL == mSurface)
	    mSurface = allocIonSurface(w, h, pitch);
	return 0;
}

void CdrDisplay::releaseSurface()
{
	if(mSurface) {
		freeIonSurface(mSurface);
		mSurface = NULL;
	}
}

int CdrDisplay::getHandle()
{
	if(mSurface)
		return mSurface->shared_fd;
	else
		return 0;
}

CdrDisplay::~CdrDisplay()
{
	db_msg("CdrDisplay Destructor\n");
	if(mSurface)
		freeIonSurface(mSurface);
}

void CdrDisplay::setBottom()
{
	//mCD->setPreviewBottom(mHlay);
}

void CdrDisplay::setRect(CDR_RECT &rect)
{
	ViewInfo vi;
	mDispInfo.view.x = rect.x;
	mDispInfo.view.y = rect.y;
	mDispInfo.view.w = rect.w;
	mDispInfo.view.h = rect.h;
}

void CdrDisplay::open()
{
	db_msg("open\n");
	mCD->open(mZorder, true);
}

void CdrDisplay::close()
{
	db_msg("close\n");
	mCD->open(mZorder, false);
}

void CdrDisplay::exchange(int hlay, int flag)
{
	//mCD->exchangeSurface(mHlay, hlay, flag);
}

void CdrDisplay::otherScreen(int screen, int hlay)
{
	//mCD->otherScreen(screen, mHlay, hlay);
}

void CdrDisplay::clean(void)
{
	//mCD->clearSurface(mHlay);
}
