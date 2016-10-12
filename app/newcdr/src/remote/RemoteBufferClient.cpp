/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */

//#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "RemoteBufferClient"
#endif
#include <utils/Log.h>
#include <IRemoteBufferClient.h>
#include <RemoteBufferClient.h>
#include <binder/IServiceManager.h>

using namespace android;

namespace rockchip {

RemoteBufferClient::RemoteBufferClient() 
    : mListener(NULL) {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
}

status_t RemoteBufferClient::onRemoteConnected(const sp<IRemoteBuffer>& rb)
{
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
    return 0;
}

void RemoteBufferClient::onRemoteDisonnected()
{
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
}

status_t RemoteBufferClient::onRemoteError(int32_t error)
{
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
    return 0;
}

status_t RemoteBufferClient::onCall(const sp<IRemoteBuffer>& rb, unsigned char *pbuf, int size) {
    ALOGV("[%s:%d] Enter, size %d, pbuf %x", __func__, __LINE__, size, *(int *)pbuf);
    if (mListener != NULL) {
        mListener->onCall(pbuf, size);
    }    
    return NO_ERROR;
}

status_t RemoteBufferClient::onPreviewData(const sp<IRemoteBuffer>& rb, int shared_fd, unsigned char *pbuf, int size) {
    ALOGV("[%s:%d] Enter, size %d, pbuf %x", __func__, __LINE__, size, *(int *)pbuf);
    if (mListener != NULL) {
        mListener->postPreviewData(shared_fd, pbuf, size);
    }
    return NO_ERROR;
}

RemoteBufferClient::~RemoteBufferClient() {
	ALOGV("[%s:%d] Enter", __func__, __LINE__);
    mListener = NULL;
}

}; /** namespace rockchip */

