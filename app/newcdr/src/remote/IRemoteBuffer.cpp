/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */
#include <IRemoteBuffer.h>
#include <IRemoteBufferClient.h>
//#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "IRemoteBuffer"
#endif

namespace rockchip {

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define CHECK(condition)                                \
	LOG_ALWAYS_FATAL_IF(                                \
			!(condition),                               \
			"%s",                                       \
			__FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
			" CHECK(" #condition ") failed.")

enum {
    TRANSACTION_connect = IBinder::FIRST_CALL_TRANSACTION,
    TRANSACTION_disconnect,
    TRANSACTION_removeClient,
    TRANSACTION_call,
    TRANSACTION_releaseBuffer,
    TRANSACTION_callSync,
};


class BpRemoteBuffer : public BpInterface<IRemoteBuffer> {
public:
	BpRemoteBuffer(const sp<IBinder>& impl)
		: BpInterface<IRemoteBuffer>(impl)
	{
	}

	virtual status_t connect(const sp<IRemoteBufferClient>& client) {
		Parcel data, reply;
		data.writeInterfaceToken(IRemoteBuffer::getInterfaceDescriptor());
		data.writeStrongBinder(client->asBinder());
		remote()->transact(TRANSACTION_connect, data, &reply);
		return reply.readInt32();
	}

	virtual status_t disconnect(const sp<IRemoteBufferClient>& client) {
		Parcel data, reply;
		data.writeInterfaceToken(IRemoteBuffer::getInterfaceDescriptor());
		data.writeStrongBinder(client->asBinder());
		remote()->transact(TRANSACTION_disconnect, data, &reply);
		return reply.readInt32();
	}

	virtual void removeClient(wp<IRemoteBufferClient> client)  {
		Parcel data, reply;
		data.writeInterfaceToken(IRemoteBuffer::getInterfaceDescriptor());
		sp<IRemoteBufferClient> c = client.promote();
		data.writeStrongBinder(c->asBinder());
		remote()->transact(TRANSACTION_removeClient, data, &reply);
		return;
	}

    virtual status_t call(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) {
		Parcel data, reply;
		data.writeInterfaceToken(IRemoteBuffer::getInterfaceDescriptor());
		data.writeStrongBinder(client->asBinder());
                data.writeByteArray(size, pbuf);
                ALOGD("%s, size %d, 0x%x", __func__, size, *(int *)pbuf);
		remote()->transact(TRANSACTION_call, data, &reply);
                return reply.readInt32();
    }

    virtual String8 callSync(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) {
        Parcel data, reply;
        data.writeInterfaceToken(IRemoteBuffer::getInterfaceDescriptor());
        data.writeStrongBinder(client->asBinder());
        data.writeByteArray(size, pbuf);
        ALOGD("%s, size %d, 0x%x", __func__, size, *(int *)pbuf);
        remote()->transact(TRANSACTION_callSync, data, &reply);
        return reply.readString8();
    }
    
    status_t releaseBuffer(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size) {
		Parcel data, reply;
		data.writeInterfaceToken(IRemoteBuffer::getInterfaceDescriptor());
		data.writeStrongBinder(client->asBinder());
                data.writeByteArray(size, pbuf);
                ALOGV("%s, size %d, 0x%x", __func__, size, *(int *)pbuf);
                remote()->transact(TRANSACTION_releaseBuffer, data, &reply);
                return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(RemoteBuffer, REMOTE_BUFFER_SERVICE);

status_t BnRemoteBuffer::onTransact( uint32_t code,
		const Parcel& data,
		Parcel* reply,
		uint32_t flags) {
	switch (code) {
		case TRANSACTION_connect:
		{
			ALOGE("BnRemoteBuffer TRANSACTION_connect");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBufferClient> c =  interface_cast<IRemoteBufferClient>(data.readStrongBinder());
			reply->writeInt32(connect(c));
			break;
		}
		case TRANSACTION_disconnect:
		{
			ALOGE("BnRemoteBuffer TRANSACTION_disconnect");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBufferClient> c =  interface_cast<IRemoteBufferClient>(data.readStrongBinder());
			reply->writeInt32(disconnect(c));
			break;
		}
		case TRANSACTION_removeClient:
		{
			ALOGE("BnRemoteBuffer TRANSACTION_remove");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBufferClient> c =  interface_cast<IRemoteBufferClient>(data.readStrongBinder());
			removeClient(c);
			break;
		}
		case TRANSACTION_call:
		{
			ALOGE("BnRemoteBuffer TRANSACTION_call");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBufferClient> c =  interface_cast<IRemoteBufferClient>(data.readStrongBinder());
                        int size = data.readInt32();
                        unsigned char *buf = new unsigned char[size + 1];
                        data.read(buf, size);
                        buf[size] = '\0';
                        ALOGD("TRANSACTION_call, size %d, 0x%x", size, *(int *)buf);
			reply->writeInt32(call(c,buf,size));
                        delete[] buf;
			break;
		}
		case TRANSACTION_releaseBuffer:
		{
			ALOGV("BnRemoteBuffer TRANSACTION_releaseBuffer");
			CHECK_INTERFACE(IRemoteBuffer, data, reply);
			sp<IRemoteBufferClient> c =  interface_cast<IRemoteBufferClient>(data.readStrongBinder());
                        int size = data.readInt32();
                        unsigned char *buf = new unsigned char[size];
                        data.read(buf, size);
                        ALOGV("TRANSACTION_releaseBuffer, size %d, 0x%x", size, *(int *)buf);
			reply->writeInt32(releaseBuffer(c, buf, size));
                        delete[] buf;
			break;
        }
        case TRANSACTION_callSync:
        {
            ALOGE("BnRemoteBuffer TRANSACTION_callSync");
            CHECK_INTERFACE(IRemoteBuffer, data, reply);
            sp<IRemoteBufferClient> c =  interface_cast<IRemoteBufferClient>(data.readStrongBinder());
            int size = data.readInt32();
            unsigned char *buf = new unsigned char[size + 1];
            data.read(buf, size);
            buf[size] = '\0';
            ALOGD("TRANSACTION_callSync, size %d, 0x%x", size, *(int *)buf);
            reply->writeString8(callSync(c,buf,size));
            delete[] buf;
            break;
        }
		default:
			ALOGE("Unknown transaction code.");
			return BBinder::onTransact(code, data, reply, flags);
	}

	return NO_ERROR;
}

}; /** namesapce rockchip */

