#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>
#include "GpsController.h"

int main() {
	ALOGE("mini gps start ...\n");
	enableGps();

	while(1) {
		sleep(1);
	}
	exit(1);
}
