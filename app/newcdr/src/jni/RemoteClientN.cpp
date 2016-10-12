
#include <android/log.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include "RemoteBuffer.h"


#define LOG_TAG "RemoteClient"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


using namespace rockchip;

int main(void)
{
	LOGI("[%s:%d] RemoteClientN Start", __func__, __LINE__);


	RemoteBufferWrapper service;

	service.connect();
	service.disconnect();
	service.removeClient();

	LOGI("[%s:%d] RemoteClientN End", __func__, __LINE__);

	
	while(1){
		sleep(1);
	}



	return 0;
}
