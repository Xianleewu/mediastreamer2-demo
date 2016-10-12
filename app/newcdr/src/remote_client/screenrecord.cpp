#define LOG_TAG "ScreenRecord"
#include <stdio.h>
#include <ProcessState.h>
#include <IPCThreadState.h>
#include <utils/Vector.h>
#include <sys/mman.h>
#include <pthread.h>

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include "screenrecord.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>  

#include "RemoteBuffer.h"
using namespace rockchip;

Mutex drawlock;
Mutex callbacklock;
Condition drawQueue;
Vector<RemotePreviewBuffer *> drawWorkQ;
android::MsgCallback *msgCallback = NULL;

class myPreviewCb : public RemoteBufferWrapper::RemotePreviewCallback
{
    public:
        myPreviewCb() {};
        ~myPreviewCb() {};

	virtual void onCall(unsigned char *pbuf, int size) {
                if(NULL != msgCallback) {
                        msgCallback->OnMsg(pbuf, size);
                }
        }
	virtual void onPreviewData(int shared_fd, unsigned char *pbuf, int size) {
        //ALOGD("onPreviewFrame shared_fd %d E", shared_fd);

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

namespace android {
static RemoteBufferWrapper service;
pthread_t mThread;
bool isRunning = false;
ScreenRecordCallback *previewCallback = NULL;
static myPreviewCb *previewCb = NULL;

#define FOURCC(a, b, c, d)                                        \
      ((static_cast<uint32_t>(a)) | (static_cast<uint32_t>(b) << 8) | \
       (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24))

void record(void *buf, int size) {
    int mRecordFd;
    mRecordFd = open("/mnt/external_sd/raw.nv12",  O_CREAT | O_LARGEFILE | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
        if(mRecordFd < 0) {
           ALOGE("open record file failed");
           return;
        }
    int ret = write(mRecordFd, buf, size);
    if(ret < size) {
        ALOGE("write failed ret %d < size %d",ret,size);
    }
    ALOGE("write size %d",size);
}

void WrapFrame(int width, int height, void *mPixelBuf,  int64_t timestampUs, struct ScreenFrame &screenFrame) {
    static bool bFirstFrame = true;
    static int64_t time_stamp = 0;

    if(bFirstFrame) {
        bFirstFrame = false;
        record(mPixelBuf, width * height * 3 >> 1);
    }

    size_t headerLen = sizeof(uint32_t) * 10;
    size_t rgbDataLen = width * height * 3 >> 1;//kOutBytesPerPixel;
    uint8_t header[headerLen];

    screenFrame.width = width;
    screenFrame.height= height;
    screenFrame.fourcc= FOURCC('N','V','1','2');
    screenFrame.pixel_width = 1;
    screenFrame.pixel_height = 1;
    screenFrame.elapsed_time = (time_stamp == 0) ? 0 : ((timestampUs - time_stamp) * 1000);
    screenFrame.time_stamp = timestampUs * 1000;
    screenFrame.data_size = rgbDataLen;
    screenFrame.data = mPixelBuf;

    time_stamp += timestampUs;
}

static bool networkBusy() {
        static bool odd = true;

        //half fps
        odd = (odd == true) ? false : true;
        return odd;
}
// static
static void *previewThread(void *pthis) {
    ALOGD("ThreadWrapper: %p", pthis);
    prctl(PR_SET_NAME, "previewThread");  
    RemoteBufferWrapper *pservice = static_cast<RemoteBufferWrapper *>(pthis);
    while(isRunning) {
        RemotePreviewBuffer *pbuf;
        drawlock.lock();
        if(drawWorkQ.isEmpty()) {
            //ALOGD("######### no remotebuffer ready to draw...");
            drawQueue.wait(drawlock);
            drawlock.unlock();
            continue;
        }
        pbuf = drawWorkQ.itemAt(0);
        drawWorkQ.removeAt(0);
        drawlock.unlock();

        ALOGD("draw preview Buffer: share_fd %d, index %d, w: %d h: %d , size: %d, previewCallback %p",
            pbuf->share_fd, pbuf->index, pbuf->width, pbuf->height, pbuf->size, previewCallback);
        unsigned char *vaddr = (unsigned char *)mmap(NULL, pbuf->size, PROT_READ | PROT_WRITE, MAP_SHARED, pbuf->share_fd, 0);

        //can do draw here
        //.....
        //....
        //ALOGD("draw content : %d ", *(int *)vaddr);
        struct ScreenFrame frame;
        int64_t timestampUs = systemTime() / 1000ll;
        WrapFrame(pbuf->width, pbuf->height, vaddr, timestampUs,frame);

        callbacklock.lock();
        if((NULL != previewCallback) && !networkBusy())
                previewCallback->OnFrame(frame);
        callbacklock.unlock();

        //release this buffer
        munmap(vaddr, pbuf->size);
        pservice->releasePreviewFrame((unsigned char *)pbuf, sizeof(RemotePreviewBuffer));
        //ALOGD("close fd : %d ", (pbuf->share_fd));
        close(pbuf->share_fd);
        delete pbuf;
    }
    
    ALOGD("previewThread: exit ");
    return NULL;
}

bool StartScreenRecordThread(int width, int height, ScreenRecordCallback *callback) {
        if(NULL == previewCb) {
                previewCb = new myPreviewCb;
        }
        service.connect();
        service.setPreviewCallback(previewCb);
	//service.call(buf, size);
        service.startPreview(0);
        sp<ProcessState> proc(ProcessState::self());
        proc->startThreadPool();
        
        callbacklock.lock();
        previewCallback = callback;
        callbacklock.unlock();

        if(isRunning == false) {
                isRunning = true;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                int err = pthread_create(&mThread, &attr, previewThread, &service);
                pthread_attr_destroy(&attr);
        }

        return true;
}

bool StopScreenRecordThread() {
        callbacklock.lock();
        previewCallback = NULL;
        ALOGD("StopScreenRecord, callback = %p", previewCallback);
        callbacklock.unlock();
        service.disconnect();
        return true;
}

bool IsScreenRecordThreadRunning() {
    return isRunning;
}

bool JoinRecordThread() {
    return true;
}

bool QueryScreenFormat(int *width, int *height, int *mFps, int *fourcc) {
    return true;
}

int RemoteCall(unsigned char *buf, int length) {
        return service.call(buf, length);
}

bool RegisterMsgCallback(MsgCallback *callback) {
        msgCallback = callback;
        return true;
}
}
