#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/mseventqueue.h"
#include "mediastreamer2/msrtp.h"


int main(int argc, char **argv)
{
	MSWebCam *mWebCam = NULL;
	MSFilter *mSource = NULL;
	MSFilter *mEncoder = NULL;
	MSFilter *mRtpSink = NULL;
	MSFilter *imgReader = NULL;
	MSFilter *mTee = NULL;
	MSTicker *mTicker = NULL;
	MSFilter *pixConv = NULL;

	RtpSession *mSession = NULL;

	MSPixFmt camFmt = MS_YUV420P;
	MSVideoSize mSize = {640, 360};
	float fps = 30.0;
	char *local_ip = "127.0.0.1";
	char *remote_ip = "127.0.0.1";
	int remote_port = 6060;
	int socket_buf_size = 2000000;
	RtpProfile *profile = NULL;
	PayloadType *pt = NULL;
	OrtpEvQueue *mEventQ = NULL;
	JBParameters jbp;

	if(argc > 1)
	{
		remote_ip = argv[1];
		ortp_message("remote ip:%s", remote_ip);
	}

	ortp_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	ms_init();

	mSession = ms_create_duplex_rtp_session("172.16.14.192", 6050, 6051);
	profile = rtp_profile_new("H264");
	pt = payload_type_clone(&payload_type_h264);
	rtp_profile_set_payload(profile, 96, pt);
	rtp_session_set_profile(mSession,profile);
	rtp_session_set_remote_addr_full(mSession, remote_ip, remote_port, remote_ip, remote_port + 1);
	rtp_session_resync(mSession);
	rtp_session_set_payload_type(mSession,pt);
	rtp_session_enable_rtcp(mSession, TRUE);

	rtp_session_set_jitter_compensation(mSession, -1);
	rtp_session_get_jitter_buffer_params(mSession,&jbp);
	jbp.max_packets=1000;//needed for high resolution video
	rtp_session_set_jitter_buffer_params(mSession,&jbp);
	rtp_session_set_rtp_socket_recv_buffer_size(mSession,socket_buf_size);
	rtp_session_set_rtp_socket_send_buffer_size(mSession,socket_buf_size);

	mWebCam = ms_web_cam_manager_get_cam(ms_web_cam_manager_get(),"V4L2: /dev/video1");

	imgReader = ms_web_cam_create_reader(mWebCam);
	pixConv = ms_filter_new(MS_PIX_CONV_ID);
	mEncoder = ms_filter_create_encoder("H264");
	mRtpSink = ms_filter_new(MS_RTP_SEND_ID);
	mEventQ = ortp_ev_queue_new();

	ms_filter_call_method(mRtpSink,MS_RTP_SEND_SET_SESSION,mSession);

    ms_filter_call_method(imgReader,MS_FILTER_SET_FPS,&fps);
    ms_filter_call_method(imgReader,MS_FILTER_SET_VIDEO_SIZE,&mSize);

    ms_filter_call_method(imgReader,MS_FILTER_GET_VIDEO_SIZE,&mSize);
    ms_filter_call_method(pixConv,MS_FILTER_SET_VIDEO_SIZE,&mSize);

    ms_filter_call_method(imgReader,MS_FILTER_GET_PIX_FMT,&camFmt);
    ms_filter_call_method(pixConv,MS_FILTER_SET_PIX_FMT,&camFmt);

    //set video size last
    ms_filter_call_method(mEncoder,MS_FILTER_SET_FPS,&fps);
    ms_filter_call_method(mEncoder,MS_FILTER_SET_VIDEO_SIZE,&mSize);

	mTicker = ms_ticker_new();

    ms_filter_link(imgReader,0,pixConv,0);
    ms_filter_link(pixConv,0,mEncoder,0);
    ms_filter_link(mEncoder,0,mRtpSink,0);

    ms_ticker_attach(mTicker,imgReader);

	rtp_session_register_event_queue(mSession, mEventQ);
	rtp_session_resync(mSession);

	while(1)
	{
		rtp_session_resync(mSession);
	}

	return 0;
}
