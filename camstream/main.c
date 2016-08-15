#include <stdio.h>
#include <signal.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/mscommon.h>
#include <mediastreamer2/msmediaplayer.h>
#include <mediastreamer2/mswebcam.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/msrtp.h>
#include <mediastreamer2/msh264rec.h>
#include <ortp/ortp.h>


static void stopRec(int signum);
bool_t RUNING = TRUE;

int main()
{
    MSFilter *disWin = NULL;
    MSFilter *imgReader = NULL;
    MSFilter *pixConv = NULL;
    MSFilter *mTee = NULL;
    MSFilter *mEncoder = NULL;
    MSFilter *mH264Rec = NULL;

    MSWebCam *webCam = NULL;
    MSTicker *mTicker = NULL;
    MSPixFmt camFmt = MS_YUV420P;
    MSVideoSize vs = {640,360};
    bool_t   show = 1;
    float fps = 30.0;
    int br = 1024000;

    ortp_init();
    ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
    ms_init();


    webCam = ms_web_cam_manager_get_cam(ms_web_cam_manager_get(),"V4L2: /dev/video1");
    //webCam = ms_web_cam_manager_get_default_cam(ms_web_cam_manager_get());
    disWin = ms_filter_new(MS_X11VIDEO_ID);
    pixConv = ms_filter_new(MS_PIX_CONV_ID);
    mTee = ms_filter_new(MS_TEE_ID);
    mEncoder = ms_filter_create_encoder("H264");
    mH264Rec = ms_filter_new(MS_H264_REC_ID);

    if(!disWin || !webCam || !mH264Rec)
    {
        ortp_error("video out filter creat faild !");
        //exit(0);
    }

    imgReader = ms_web_cam_create_reader(webCam);

    ms_filter_call_method(mH264Rec,MS_H264_REC_OPEN,"/home/xianlee/xianlee.h264");
    ms_filter_call_method_noarg(mH264Rec,MS_H264_REC_START);

    ms_filter_call_method(imgReader,MS_FILTER_SET_FPS,&fps);
    ms_filter_call_method(imgReader,MS_FILTER_SET_VIDEO_SIZE,&vs);

    ms_filter_call_method(imgReader,MS_FILTER_GET_VIDEO_SIZE,&vs);
    ms_filter_call_method(pixConv,MS_FILTER_SET_VIDEO_SIZE,&vs);

    ms_filter_call_method(imgReader,MS_FILTER_GET_PIX_FMT,&camFmt);
    ms_filter_call_method(pixConv,MS_FILTER_SET_PIX_FMT,&camFmt);

    //set video size last
    ms_filter_call_method(mEncoder,MS_FILTER_SET_BITRATE,&br);
    ms_filter_call_method(mEncoder,MS_FILTER_SET_FPS,&fps);
    ms_filter_call_method(mEncoder,MS_FILTER_SET_VIDEO_SIZE,&vs);

    ms_filter_call_method(disWin,MS_VIDEO_DISPLAY_SHOW_VIDEO,&show);


    mTicker = ms_ticker_new();

    ms_filter_link(imgReader,0,pixConv,0);

    ms_filter_link(pixConv,0,mTee,0);

    ms_filter_link(mTee,0,disWin,0);

    ms_filter_link(mTee,1,mEncoder,0);

    ms_filter_link(mEncoder,0,mH264Rec,0);

    ms_ticker_attach(mTicker,imgReader);

    signal(SIGINT,stopRec);
    while(RUNING)
    {
        sleep(1);
    }

    ms_ticker_detach(mTicker,imgReader);
    ms_ticker_destroy(mTicker);

    ms_filter_unlink(imgReader,0,pixConv,0);
    ms_filter_unlink(pixConv,0,mTee,0);
    ms_filter_unlink(mTee,0,disWin,0);
    ms_filter_unlink(mTee,1,mEncoder,0);
    ms_filter_unlink(mEncoder,0,mH264Rec,0);

    ms_filter_call_method_noarg(mH264Rec,MS_H264_REC_CLOSE);

    ms_filter_destroy(imgReader);
    ms_filter_destroy(pixConv);
    ms_filter_destroy(mTee);
    ms_filter_destroy(mEncoder);
    ms_filter_destroy(mH264Rec);

    return 0;
}


static void stopRec(int signum)
{
    RUNING = FALSE;
}
