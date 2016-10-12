/**
 * Author : Cody Xie <cody.xie@rock-chips.com>
 *
 * Description : A ION allocated buffer for usage cross processes.
 */
#ifndef __REMOTEBUFFER_H__
#define __REMOTEBUFFER_H__

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <IRemoteBuffer.h>
#include <IRemoteBufferClient.h>
#include <RemoteBufferClient.h>
#include <IRemoteControl.h>

using namespace android;

struct RemotePreviewBuffer{
    int share_fd;
    int map_fd;
    int index;
    void *vddr;
    int width;
    int height;
    int format;
    int stride;
    int size;
    bool used;
};

namespace rockchip {

class RemoteBuffer : 
	public virtual BnRemoteBuffer,
	public IBinder::DeathRecipient {
public:
	RemoteBuffer();
	//remote api
	virtual status_t connect(const sp<IRemoteBufferClient>& client);
	virtual status_t disconnect(const sp<IRemoteBufferClient> &client);
	virtual void removeClient(wp<IRemoteBufferClient> client);
	virtual status_t call(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size);
	virtual status_t releaseBuffer(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size);
	virtual String8 callSync(const sp<IRemoteBufferClient>& client, unsigned char *pbuf, int size);

	//local api
	void setRemoteControl(RemoteControl *control) { mControl = control;};
	bool isConnected() { return mIsConnected;};
	void onPreviewData(int shared_fd, unsigned char *pbuf, int size);
	void onCall(unsigned char *pbuf, int size);

	//die
	void binderDied(const wp<IBinder>& /*who*/);
protected:
	virtual ~RemoteBuffer();

private:
	mutable Mutex mLock;
	//SortedVector < wp<IRemoteBufferClient> > mClients;
	sp<IRemoteBufferClient> mClient;
	RemoteControl *mControl;
	bool mIsConnected;
	Mutex mBufferlock;
	Vector<RemotePreviewBuffer *> mBufferQueue;
};

class RemoteBufferWrapper : public virtual RemoteBufferListener {
public:
	RemoteBufferWrapper();
	~RemoteBufferWrapper();

	int connect(void);
	int disconnect(void);
	int removeClient(void);
	int call(unsigned char *buf, int size);
	String8 callSync(unsigned char *buf, int size);
	void startPreview(int camId);
	
	class RemotePreviewCallback: virtual public RefBase 
	{
	public:
		virtual void onCall(unsigned char *pbuf, int size) = 0;
		virtual void onPreviewData(int shared_fd, unsigned char *pbuf, int size) = 0;
	};
	void setPreviewCallback(RemotePreviewCallback *pCb) { mpPreviewCallback = pCb;};
	void postPreviewData(int shared_fd, unsigned char *pbuf, int size);
	void releasePreviewFrame(unsigned char *pbuf, int size);
	void onCall(unsigned char *pbuf, int size);

private:
	sp<RemoteBufferClient> mclient;
	sp<IBinder> mbinder;
	sp<IRemoteBuffer> mservice;
	sp<RemotePreviewCallback> mpPreviewCallback;
};

};

#endif /* __REMOTEBUFFER_H__ */
