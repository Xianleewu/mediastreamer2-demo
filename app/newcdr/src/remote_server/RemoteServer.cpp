#define LOG_TAG "RemoteServer"

#include <stdlib.h>
#include <fcntl.h>

#include <utils/Log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include "RemoteBuffer.h"

using namespace rockchip;

int main(int argc, char** argv)
{
	ALOGV("[%s:%d] RemoteServer Start", __func__, __LINE__);
	defaultServiceManager()->addService(String16(REMOTE_BUFFER_SERVICE), new RemoteBuffer());
	//RemoteBuffer::instantiate();

	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool();

	return 0;
}
