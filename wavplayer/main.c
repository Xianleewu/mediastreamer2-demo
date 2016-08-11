#include "stdio.h"
#include <mediastreamer2/mscommon.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msfileplayer.h>
#include <wavplayer.h>
#include <signal.h>
#include <pthread.h>

static WavPlayer *player = NULL;

static void stopplaying(int signum);
static void playThread(void);

int main(int argc,char **argv)
{
    char *file = "/home/xianlee/wavtest.wav";
    int loop = 1;
    pthread_t thid;

    ortp_init();
    ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
    ms_init();

    if(argc > 1)
    {
        file = argv[1];
    }

    player = creatPlayer(file,loop);
    if (pthread_create(&thid,NULL,(void *)playThread,NULL) != 0)
    {
        ortp_error(" creat thread error\n");
        exit(1);
    }

    sleep(4);
    //destroyPlayer(player);
    stopPlay(player);

    signal(SIGINT ,stopplaying);
    while(1)
    {
        sleep(10);
    }

    return 0;
}


static void stopplaying(int signum)
{
    if(player)
    {
        destroyPlayer(player);
    }
}

static void playThread()
{
    startPlay(player);
    ortp_message("play thread exit !");
    pthread_exit(0);
}
