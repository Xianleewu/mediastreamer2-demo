/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */
#ifndef __IREMOTEBUFFER_H__
#define __IREMOTEBUFFER_H__

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/BpBinder.h>
#include <binder/Parcel.h>
#include <utils/String8.h>

#define REMOTE_BUFFER_SERVICE	("rockchip.IRemoteBuffer")

using namespace android;

namespace rockchip {

class IRemoteBufferClient;

class IRemoteBuffer : public IInterface {
public:
	DECLARE_META_INTERFACE(RemoteBuffer);

	virtual status_t connect(const sp<IRemoteBufferClient> &client) = 0;
	virtual status_t disconnect(const sp<IRemoteBufferClient> &client) = 0;
	virtual void removeClient(wp<IRemoteBufferClient> client) = 0;
    virtual status_t call(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) = 0;
    virtual status_t releaseBuffer(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) = 0;
    virtual String8 callSync(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) = 0;
};

class BnRemoteBuffer : public BnInterface<IRemoteBuffer> {
    virtual status_t    onTransact( uint32_t code,
            const Parcel& data,
            Parcel* reply,
            uint32_t flags = 0); 
};

};

#endif /* __IREMOTEBUFFER_H__ */
