/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

#include <cutils/properties.h>
#include <cutils/android_reboot.h>
#include "install.h"


extern "C" int __reboot(int, int, int, void*);











int
main(int argc, char **argv) {
    time_t start = time(NULL);
	const char *update_package = NULL;
	int arg = 0;
	int status = INSTALL_SUCCESS;
	int wipe_cache = 0;
	char buf[100];    
	char property_val[PROPERTY_VALUE_MAX];
	int ret;


	printf("%s argc=%d.  \n", __FUNCTION__, argc);
	printf("Command:");
    for (arg = 0; arg < argc; arg++) {
        printf(" \"%s\"", argv[arg]);
    }
    printf("\n");
	
	if(argc != 2)
	{
		printf("Usage: recovery filename. eg: recovery update.zip \n");
		return -1;
	}
	
    //printf("Starting recovery (pid %d) on %s. argv[1]=%s \n", getpid(), ctime(&start), argv[1]);
	update_package = argv[1];
	

   

    if (update_package) {
		printf("update_package = %s \n", update_package);
		status = install_package(update_package, &wipe_cache, NULL, false);
		if (status != INSTALL_SUCCESS) {
			printf("Installation aborted.\n");
            printf("OTA failed! Please power off the device to keep it in this state and file a bug report!\n");
			return -1;
		}
    }
    

	
	printf("\n %s line=%d  Reboot to bootloader to continue...\n", __FUNCTION__, __LINE__);
	memset(buf, 0, sizeof(buf));
	memset(property_val, 0, sizeof(property_val));

	//sync before reboot
    sync();    

	ret = snprintf(property_val, sizeof(property_val), "reboot,%s", "ota"); 
	if (ret >= (int) sizeof(property_val)) { 
		printf("reboot string too long. length=%d\n", ret);    
	} 
	ret = property_set(ANDROID_RB_PROPERTY, property_val);
	if (ret < 0) {
		printf("reboot failed: %d\n", ret);    
	}   
	// Don't return early. Give the reboot command time to take effect   
	while(1) { pause(); }


    return 0;
}
