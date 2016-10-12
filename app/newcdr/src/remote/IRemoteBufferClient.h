/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */
#ifndef __IREMOTEBUFFERCLIENT_H__
#define __IREMOTEBUFFERCLIENT_H__

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/BpBinder.h>
#include <binder/Parcel.h>

#include <IRemoteBuffer.h>

#define REMOTE_BUFFER_CLIENT_SERVER ("rockchip.IRemoteBufferClient")

using namespace android;

namespace rockchip {

class IRemoteBufferClient : public IInterface {
public:
	DECLARE_META_INTERFACE(RemoteBufferClient);
	
	virtual status_t onRemoteConnected(const sp<IRemoteBuffer>& rb) = 0;

	virtual void onRemoteDisonnected() = 0;

	virtual status_t onRemoteError(int32_t error) = 0;

    virtual status_t onCall(const sp<IRemoteBuffer>& rb, unsigned char *pbuf, int size) = 0;
    virtual status_t onPreviewData(const sp<IRemoteBuffer>& rb, int shared_fd, unsigned char *pbuf, int size) = 0;	
};

class BnRemoteBufferClient : public BnInterface<IRemoteBufferClient> {
    virtual status_t    onTransact( uint32_t code,
            const Parcel& data,
            Parcel* reply,
            uint32_t flags = 0); 
};

};

#endif /* __IREMOTEBUFFERCLIENT_H__ */
