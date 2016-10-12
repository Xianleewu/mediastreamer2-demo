/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */

//#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "RemoteBuffer"
#endif
#include <utils/Log.h>
#include <IRemoteBufferClient.h>
#include <RemoteBuffer.h>
#include <RemoteBufferClient.h>
#include <binder/IServiceManager.h>

#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"

using namespace android;

namespace rockchip {

RemoteBuffer::RemoteBuffer()
    : mIsConnected(false)
{
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
}

status_t RemoteBuffer::connect(const sp<IRemoteBufferClient>& client) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);

	Mutex::Autolock lock(mLock);
	/** No need to check camera buffer producer is ready here. */
/*	for (size_t i = 0; i < mClients.size(); i++) {
		sp<IRemoteBufferClient> c = mClients[i].promote();
		if (c == client) {
			ALOGW("Dupplicate connect with same client. Ignored.");
			return NO_ERROR;
		}
	}*/
	//wp<IRemoteBufferClient> c = client;
	//mClients.add(c);
        mClient = client;
        mClient->asBinder()->linkToDeath(this);
	client->onRemoteConnected(this);
	ALOGV("[%s:%d] End", __func__, __LINE__);
        mIsConnected = true;

	return NO_ERROR;
}

void RemoteBuffer::removeClient(wp<IRemoteBufferClient> client) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);

	Mutex::Autolock lock(mLock);
	//mClients.remove(client);
}

status_t RemoteBuffer::disconnect(const sp<IRemoteBufferClient> &client) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);

	Mutex::Autolock lock(mLock);
	/*for (size_t i = 0; i < mClients.size(); i++) {
		wp<IRemoteBufferClient> wc = mClients[i];
		sp<IRemoteBufferClient> c = wc.promote();
		c->onRemoteDisonnected();
		mClients.remove(wc);
	}*/
	//mClients.remove(client);
	if(mClient != NULL) {
                mClient->asBinder()->unlinkToDeath(this);
                mClient = NULL;
        }
        mIsConnected = false;
	return NO_ERROR;
}

void RemoteBuffer::binderDied(const wp<IBinder>& who) {
	ALOGE("[%s:%d] Enter, bufferQueue size %d", __func__, __LINE__, mBufferQueue.size());
        RemotePreviewBuffer *temp;

        disconnect(mClient);

	mBufferlock.lock();
        while(!mBufferQueue.isEmpty()) {
                temp = mBufferQueue.itemAt(0);
                mBufferQueue.removeAt(0);
                if(mControl != NULL)
                        mControl->releaseBuffer((unsigned char *)temp, sizeof(RemotePreviewBuffer));
                delete temp;
        }
        mBufferlock.unlock();
}

status_t RemoteBuffer::call(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) {
	ALOGV("[%s:%d] Enter, size %d, pbuf %x, %p", "RemoteBuffer::Call", __LINE__, size, *(int *)pbuf, mControl);

    if(mControl == NULL)
        return NO_ERROR;
	#if 0
    mControl->call(pbuf, size);
	#else
	fwk_send_message_ext(FWK_MOD_VIEW, FWK_MOD_CONTROL, MSG_ID_FWK_CONTROL_WRTC_CALL_REQ, pbuf, size);
	#endif
    return NO_ERROR;
}

String8 RemoteBuffer::callSync(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) {
    ALOGD("[%s:%d] Enter, size %d, pbuf %x, %p", "RemoteBuffer::callSync", __LINE__, size, *(int *)pbuf, mControl);
    String8 result;

    if(mControl)
        result = mControl->callSync(pbuf, size);

    return result;
}

status_t RemoteBuffer::releaseBuffer(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) {
        RemotePreviewBuffer *temp;
        RemotePreviewBuffer *current = (RemotePreviewBuffer *)pbuf;
        ALOGD("[%s:%d]:index %d, mBufferQueue.size() %d", __func__,__LINE__, current->index, mBufferQueue.size());

        if(mControl == NULL)
                return NO_ERROR;
        mControl->releaseBuffer(pbuf, size);

	mBufferlock.lock();
        for(int i = 0; i < mBufferQueue.size(); i++) {
                temp = mBufferQueue.itemAt(i);
                if(temp->index == current->index) {
                        mBufferQueue.removeAt(i);
                        delete temp;
                        break;
                }
        }
        mBufferlock.unlock();
        return NO_ERROR;
}

void RemoteBuffer::onPreviewData(int shared_fd, unsigned char *pbuf, int size) {
        RemotePreviewBuffer *current = (RemotePreviewBuffer *)pbuf;
        ALOGD("[%s:%d]: fd %d, index %d, mBufferQueue.size() %d", __func__,__LINE__, shared_fd, current->index, mBufferQueue.size());
        
	mBufferlock.lock();
        mBufferQueue.push_back(current);
        mBufferlock.unlock();

        mClient->onPreviewData(this, shared_fd, pbuf, size);
}

void RemoteBuffer::onCall(unsigned char *pbuf, int size) {
        ALOGV("line [%d]: size %d, pbuf %x", __LINE__, size, *(int *)pbuf);
	if(mClient == NULL)
		return;
	mClient->onCall(this, pbuf, size);
}

RemoteBuffer::~RemoteBuffer() {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
	//disconnect();
}

RemoteBufferWrapper::RemoteBufferWrapper() {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
	mbinder = NULL;
	mservice = NULL;

        mclient = new RemoteBufferClient();
        mclient->setListener(this);
    
	mbinder = defaultServiceManager()->getService(String16(REMOTE_BUFFER_SERVICE));
	if(mbinder == NULL) {
		ALOGE("Failed to get remote service: %s.\n", "REMOTE_BUFFER_SERVICE");
	}else{
		mservice = IRemoteBuffer::asInterface(mbinder);
		if(mservice == NULL) {
			ALOGE("Failed to get remote service interface.\n");
		}
	}

}

RemoteBufferWrapper::~RemoteBufferWrapper(){
	ALOGV("[%s:%d] Enter", __func__, __LINE__);

}

int RemoteBufferWrapper::connect(void){
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
	if(mservice == NULL){
		ALOGE("[%s %d] service is NULL",__func__, __LINE__);
		return -1;
	}

	mservice->connect(mclient);

	return 0;
}

int RemoteBufferWrapper::call(unsigned char *buf, int size) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
	if(mservice == NULL){
		ALOGE("[%s %d] service is NULL",__func__, __LINE__);
		return -1;
	}

	return mservice->call(mclient, buf, size);
}

String8 RemoteBufferWrapper::callSync(unsigned char *buf, int size) {
    String8 result;
    ALOGV("[%s:%d] Enter", __func__, __LINE__);
    if(mservice == NULL){
        ALOGE("[%s %d] service is NULL",__func__, __LINE__);
        return result;
    }

    return mservice->callSync(mclient, buf, size);
}

int RemoteBufferWrapper::disconnect(void) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
	if(mservice == NULL){
		ALOGE("[%s %d] service is NULL",__func__, __LINE__);
		return -1;
	}
	
	mservice->disconnect(mclient);
	return 0;
}

int RemoteBufferWrapper::removeClient(void) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
	if(mservice == NULL){
		ALOGE("[%s %d] service is NULL",__func__, __LINE__);
		return -1;
	}

	mservice->removeClient(mclient);
	return 0;
}

void RemoteBufferWrapper::startPreview(int camId) {
}

void RemoteBufferWrapper::postPreviewData(int shared_fd, unsigned char *pbuf, int size) {
        ALOGV("[%s:%d] Enter", __func__, __LINE__);
        if(mpPreviewCallback == NULL)
                return;
        mpPreviewCallback->onPreviewData(shared_fd, pbuf, size);
}

void RemoteBufferWrapper::onCall(unsigned char *pbuf, int size) {
        ALOGV("[%s:%d] Enter", __func__, __LINE__);
        if(mpPreviewCallback == NULL)
                return;
        mpPreviewCallback->onCall(pbuf, size);
}

void RemoteBufferWrapper::releasePreviewFrame(unsigned char *pbuf, int size) {
        ALOGV("[%s:%d] Enter", __func__, __LINE__);
        mservice->releaseBuffer(mclient, pbuf, size);
}

}; /** namespace rockchip */

