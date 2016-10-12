/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */
#include <IRemoteBuffer.h>
#include <IRemoteBufferClient.h>
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "IRemoteBufferClient"
#endif

namespace rockchip {

enum {
    TRANSACTION_onRemoteConnected = IBinder::FIRST_CALL_TRANSACTION,
    TRANSACTION_onRemoteDisconnected,
    TRANSACTION_onRemoteError,
    TRANSACTION_onCall,
    TRANSACTION_onPreviewData,
};


class BpRemoteBufferClient : public BpInterface<IRemoteBufferClient> {
public:
	BpRemoteBufferClient(const sp<IBinder>& impl)
		: BpInterface<IRemoteBufferClient>(impl)
	{
	}

	status_t onRemoteConnected(const sp<IRemoteBuffer>& rb) {
                ALOGV("%s", __func__);
                Parcel data, reply;
		data.writeInterfaceToken(IRemoteBufferClient::getInterfaceDescriptor());
		data.writeStrongBinder(rb->asBinder());
		remote()->transact(TRANSACTION_onRemoteConnected, data, &reply);
		return reply.readInt32();
	}

        status_t onCall(const sp<IRemoteBuffer>& rb, unsigned char *pbuf, int size) {
		Parcel data, reply;
		data.writeInterfaceToken(IRemoteBufferClient::getInterfaceDescriptor());
		data.writeStrongBinder(rb->asBinder());
                data.writeByteArray(size, pbuf);
                ALOGD("%s, size %d, 0x%x", __func__, size, *(int *)pbuf);
                	remote()->transact(TRANSACTION_onCall, data, &reply);
                return reply.readInt32();
        }

    
        status_t onPreviewData(const sp<IRemoteBuffer>& rb, int shared_fd, unsigned char *pbuf, int size) {
        	Parcel data, reply;
        	data.writeInterfaceToken(IRemoteBufferClient::getInterfaceDescriptor());
        	data.writeStrongBinder(rb->asBinder());
                data.writeFileDescriptor(shared_fd);
                data.writeByteArray(size, pbuf);
                ALOGV("%s, size %d, 0x%x", __func__, size, *(int *)pbuf);
                remote()->transact(TRANSACTION_onPreviewData, data, &reply);
                int ret = reply.readInt32();
                ALOGV("%s, size %d, 0x%x end, reply %d", __func__, size, *(int *)pbuf, ret);
                return ret;
        }

	void onRemoteDisonnected()  {
	}

	status_t onRemoteError(int32_t error)  {
		return NO_ERROR;
	}
};

IMPLEMENT_META_INTERFACE(RemoteBufferClient, REMOTE_BUFFER_CLIENT_SERVER);

status_t BnRemoteBufferClient::onTransact( uint32_t code,
		const Parcel& data,
		Parcel* reply,
		uint32_t flags) {
	switch (code) {
		case TRANSACTION_onRemoteConnected:
		{
			ALOGE("BnRemoteBufferClient TRANSACTION_onRemoteConnected");
			CHECK_INTERFACE(IRemoteBufferClient, data, reply);
                        sp<IRemoteBuffer> c =  interface_cast<IRemoteBuffer>(data.readStrongBinder());
			reply->writeInt32(onRemoteConnected(c));
			break;
		}
		case TRANSACTION_onRemoteDisconnected:
		{
			CHECK_INTERFACE(IRemoteBufferClient, data, reply);
			break;
		}
		case TRANSACTION_onRemoteError:
		{
			CHECK_INTERFACE(IRemoteBufferClient, data, reply);
			break;
		}
		case TRANSACTION_onCall:
		{
			ALOGE("BnRemoteBuffer TRANSACTION_onCall");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBuffer> c =  interface_cast<IRemoteBuffer>(data.readStrongBinder());
                        int size = data.readInt32();
                        unsigned char *buf = new unsigned char[size];
                        data.read(buf, size);
                        ALOGD("TRANSACTION_call, size %d, 0x%x", size, *(int *)buf);
                        	reply->writeInt32(onCall(c,buf,size));
                        delete[] buf;
			break;
		}
		case TRANSACTION_onPreviewData:
		{
			ALOGV("BnRemoteBuffer TRANSACTION_onPreviewData");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBuffer> c =  interface_cast<IRemoteBuffer>(data.readStrongBinder());
                        int shared_fd = data.readFileDescriptor();
                        int size = data.readInt32();
                        unsigned char *buf = new unsigned char[size];
                        data.read(buf, size);
                        ALOGV("TRANSACTION_onPreviewData, size %d, 0x%x", size, *(int *)buf);
			reply->writeInt32(onPreviewData(c, shared_fd, buf, size));
                        delete[] buf;
                        close(shared_fd);
			break;
		}        
		default:
			ALOGE("Unknown transaction code.");
			return BBinder::onTransact(code, data, reply, flags);
	}

	return NO_ERROR;
}

}; /** namesapce rockchip */

