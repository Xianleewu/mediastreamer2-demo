#define LOG_TAG "RemoteClient"
#include <stdio.h>
#include <ProcessState.h>
#include <IPCThreadState.h>
#include <utils/Vector.h>
#include <sys/mman.h>

#include <utils/Log.h>
#include <binder/IServiceManager.h>

#include "RemoteBuffer.h"
using namespace rockchip;

Mutex drawlock;
Condition drawQueue;
Vector<RemotePreviewBuffer *> drawWorkQ;

class myPreviewCb : public RemoteBufferWrapper::RemotePreviewCallback
{
    public:
        myPreviewCb() {};
        ~myPreviewCb() {};
	virtual void onCall(unsigned char *pbuf, int size) {
                ALOGD("onCall E");
        }

	virtual void onPreviewData(int shared_fd, unsigned char *pbuf, int size) {
        ALOGD("onPreviewFrame E");

        RemotePreviewBuffer *pbuf1 = new RemotePreviewBuffer;
        RemotePreviewBuffer *data = (RemotePreviewBuffer *)pbuf;

        pbuf1->share_fd = dup(shared_fd);
        pbuf1->index = data->index;
        pbuf1->width = data->width;
        pbuf1->height = data->height;
        pbuf1->stride = data->stride;
        pbuf1->size = data->size;
        drawlock.lock();
        drawWorkQ.push_back(pbuf1);
        drawQueue.broadcast();
        drawlock.unlock();
    };
};

class PreviewThread : public Thread
{
public:
    RemoteBufferWrapper *mpService;
    PreviewThread(RemoteBufferWrapper *service) {
        mpService = service;
    };
    ~PreviewThread() {
    }
protected:
    virtual bool threadLoop() {

        RemotePreviewBuffer *pbuf;
        drawlock.lock();
        if(drawWorkQ.isEmpty()) {
            ALOGD("######### no remotebuffer ready to draw...");
            drawQueue.wait(drawlock);
            drawlock.unlock();
            return true;
        }
        pbuf = drawWorkQ.itemAt(0);
        drawWorkQ.removeAt(0);
        drawlock.unlock();

        ALOGD("draw preview Buffer: share_fd %d, index %d, w: %d h: %d , size: %d",
            pbuf->share_fd, pbuf->index, pbuf->width, pbuf->height, pbuf->size);
        unsigned char *vaddr = (unsigned char *)mmap(NULL, pbuf->size, PROT_READ | PROT_WRITE, MAP_SHARED, pbuf->share_fd, 0);

        //can do draw here
        //.....
        //....
        //ALOGD("draw content : %d ", *(int *)vaddr);

        //release this buffer
        munmap(vaddr, pbuf->size);
        mpService->releasePreviewFrame((unsigned char *)pbuf, sizeof(RemotePreviewBuffer));
        ALOGD("releasePreviewFrame: index %d end", pbuf->index);
        close(pbuf->share_fd);
        delete pbuf;
        return true;
    };
};

int main()
{
	ALOGV("[%s:%d] RemoteClient Start", __func__, __LINE__);

	RemoteBufferWrapper service;

    int size = 32;
    RemotePreviewBuffer *pbuf = new RemotePreviewBuffer;
    unsigned char *buf = (unsigned char *)pbuf;
    pbuf->index = 0x55;
    pbuf->size = sizeof(RemotePreviewBuffer);
    pbuf->share_fd = 0xaa;
    
	service.connect();
    service.setPreviewCallback(new myPreviewCb);
	service.call(buf, size);
	service.startPreview(0);

    sp<ProcessState> proc(ProcessState::self());
    sp<PreviewThread> pt = new PreviewThread(&service);
    pt->run("preview_thread");
    proc->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    
	service.disconnect();
	service.removeClient();

	return 0;
}
