/*
 * Copyright (C) 2008 The Android Open Source Project
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

//#include "sysdeps.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
#include <utils/Log.h>







int main(int argc, char **argv)
{
    char update_val[128];
    int ret;
    
    memset(update_val, 0, sizeof(update_val));
    ret = snprintf(update_val, sizeof(update_val), "/system/bin/recovery %s", "/data/update.zip");
    if (ret >= (int) sizeof(update_val)) {
        printf("update_val string too long. length=%d\n", ret);
    }
    
	  system(update_val);
    
    return 0;
    
}


