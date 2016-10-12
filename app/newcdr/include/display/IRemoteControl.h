#ifndef __IRemoteControl_H__
#define __IRemoteControl_H__
#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/String8.h>

class RemoteControl {
public:
	RemoteControl(){}
	virtual ~RemoteControl(){}
	virtual void call(unsigned char *pbuf, int size) = 0;
	virtual String8 callSync(unsigned char *pbuf, int size) = 0;
	virtual int onCall(unsigned char *pbuf, int size) = 0;
	virtual void releaseBuffer(unsigned char *pbuf, int size) = 0;
};

#endif
