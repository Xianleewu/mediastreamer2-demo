#include <stdio.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/mscommon.h>
#include <mediastreamer2/mssndcard.h>
#include <mediastreamer2/msfilerec.h>


int main(void)
{
    MSFilter *msReader = NULL;
    MSFilter *msRecorder = NULL;
    MSFilter *msPlayer = NULL;
    MSFilter *msTee = NULL;

    MSTicker  *mTicker = NULL;
    MSSndCard *recoCard = NULL;
    MSSndCard *playCard = NULL;

    int sampRate = 44110;
    int nChannels = 2;

    ortp_init();
    ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
    ms_init();

    recoCard = ms_snd_card_manager_get_default_capture_card(ms_snd_card_manager_get());
    playCard = ms_snd_card_manager_get_default_playback_card(ms_snd_card_manager_get());

    if(!recoCard || !playCard)
    {
        ortp_error("can not get an sound card !");
    }

    msReader = ms_snd_card_create_reader(recoCard);
    msPlayer = ms_snd_card_create_writer(playCard);
    msRecorder = ms_filter_new(MS_FILE_REC_ID);
    msTee = ms_filter_new(MS_TEE_ID);

    ms_filter_call_method(msReader,MS_FILTER_SET_SAMPLE_RATE,&sampRate);
    ms_filter_call_method(msReader,MS_FILTER_SET_NCHANNELS,&nChannels);

    ms_filter_call_method(msPlayer,MS_FILTER_SET_SAMPLE_RATE,&sampRate);
    ms_filter_call_method(msPlayer,MS_FILTER_SET_NCHANNELS,&nChannels);


    ms_filter_call_method(msRecorder,MS_FILTER_SET_SAMPLE_RATE,&sampRate);
    ms_filter_call_method(msRecorder,MS_FILTER_SET_NCHANNELS,&nChannels);
    ms_filter_call_method(msRecorder,MS_FILE_REC_OPEN,"/home/xianlee/recorder.wav");
    ms_filter_call_method_noarg(msRecorder,MS_FILE_REC_START);

    mTicker = ms_ticker_new();

    ms_filter_link(msReader,0,msTee,0);
    ms_filter_link(msTee,0,msRecorder,0);
    ms_filter_link(msTee,1,msPlayer,0);
    ms_ticker_attach(mTicker,msReader);

    sleep(5);

    ms_ticker_detach(mTicker,msReader);
    ms_filter_unlink(msReader,0,msTee,0);
    ms_filter_unlink(msTee,0,msRecorder,0);
    ms_filter_unlink(msTee,1,msPlayer,0);

    ms_filter_call_method_noarg(msRecorder,MS_FILE_REC_CLOSE);

    ms_filter_destroy(msReader);
    ms_filter_destroy(msRecorder);
    ms_filter_destroy(msPlayer);
    ms_filter_destroy(msTee);

    ortp_message("see output in /home/xianlee/ !");
    return 0;
}

