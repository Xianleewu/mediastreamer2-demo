#include <stdio.h>
#include <signal.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msfilerec.h>
#include <ortp/ortp.h>

bool_t RUNING = TRUE;

void stop(int signum);

int main(int argc,char **argv)
{
    MSFilter *mEncoder = NULL;
    MSFilter *mDecoder = NULL;
    MSFilter *mReader = NULL;
    MSFilter *mWriter = NULL;

    MSSndCard *mCapSndCard = NULL;
    MSSndCard *mPlaySndCard = NULL;
    MSTicker *mTicker;
    int sampleRate = 44110;
    int nchannels = 2;

    ms_init();
    ortp_init();
    ortp_set_log_level_mask(ORTP_DEBUG|ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL|ORTP_TRACE);

    mCapSndCard = ms_snd_card_manager_get_default_capture_card(ms_snd_card_manager_get());
    mPlaySndCard = ms_snd_card_manager_get_default_playback_card(ms_snd_card_manager_get());

    if(!mCapSndCard || !mPlaySndCard)
    {
        ortp_fatal("get sound card faild");
        exit(0);
    }

    mReader = ms_snd_card_create_reader(mCapSndCard);
    mWriter = ms_snd_card_create_writer(mPlaySndCard);

    mEncoder = ms_filter_create_encoder("SPEEX");
    mDecoder = ms_filter_create_decoder("SPEEX");

    if(!mEncoder || !mDecoder)
    {
        ortp_fatal("creat codec filters faild");
        exit(0);
    }

    ms_filter_call_method(mReader,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&sampleRate);
    ms_filter_call_method(mReader,MS_FILTER_SET_NCHANNELS,&nchannels);
    ms_filter_call_method(mWriter,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&sampleRate);
    ms_filter_call_method(mWriter,MS_FILTER_SET_NCHANNELS,&nchannels);

    ms_filter_call_method(mEncoder,MS_FILTER_SET_SAMPLE_RATE,&sampleRate);
    ms_filter_call_method(mDecoder,MS_FILTER_SET_SAMPLE_RATE,&sampleRate);

    ms_filter_link(mReader,0,mEncoder,0);
    ms_filter_link(mEncoder,0,mDecoder,0);
    ms_filter_link(mDecoder,0,mWriter,0);

    mTicker = ms_ticker_new();
    ms_ticker_attach(mTicker,mReader);

    signal(SIGINT,stop);
    while(RUNING)
    {
        sleep(2);
    }

    return 0;
}


void stop(int signum)
{
    RUNING = FALSE;
}
