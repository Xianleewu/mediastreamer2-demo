/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */
#ifndef __REMOTEBUFFERCLIENT_H__
#define __REMOTEBUFFERCLIENT_H__

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <IRemoteBufferClient.h>

using namespace android;

namespace rockchip {
	
class RemoteBufferListener: virtual public RefBase 
{
public:
	virtual void onCall(unsigned char *pbuf, int size) = 0;
	virtual void postPreviewData(int shared_fd, unsigned char *pbuf, int size) = 0;
};

class RemoteBufferClient : public BnRemoteBufferClient 
{
public:
	RemoteBufferClient();
	status_t onRemoteConnected(const sp<IRemoteBuffer>& rb);
	void onRemoteDisonnected();
	status_t onRemoteError(int32_t error);
	status_t onCall(const sp<IRemoteBuffer>& rb, unsigned char *pbuf, int size);
    status_t onPreviewData(const sp<IRemoteBuffer>& rb, int shared_fd, unsigned char *pbuf, int size);

	void setListener(RemoteBufferListener* listener) { mListener = listener;};

protected:
	virtual ~RemoteBufferClient();

private:
    RemoteBufferListener*  mListener;
};

};

#endif /* __REMOTEBUFFERCLIENT_H__ */
