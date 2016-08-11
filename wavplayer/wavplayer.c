#include <mediastreamer2/mscommon.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msfileplayer.h>
#include <wavplayer.h>

WavPlayer *creatPlayer(char *filename,int loop)
{
    WavPlayer *wavplayer;
    MSSndCard *sndCard;
    char *cardId = NULL;
    char *file = strdup(filename);
    int sampleRate = 44110;
    int nchannels = 2;
    int mloop = loop;

    wavplayer = (WavPlayer *)ms_malloc0(sizeof(WavPlayer));

    sndCard = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),cardId);
#ifdef __linux
    if (sndCard==NULL)
        sndCard = ms_alsa_card_new_custom(cardId, cardId);
#endif

    wavplayer->sndWriter = ms_snd_card_create_writer(sndCard);
    wavplayer->wavReader = ms_filter_new(MS_FILE_PLAYER_ID);

    ms_filter_call_method(wavplayer->wavReader,MS_FILE_PLAYER_OPEN,(void*)file);
    ms_filter_call_method(wavplayer->wavReader,MS_FILTER_GET_SAMPLE_RATE,&sampleRate);
    ms_filter_call_method(wavplayer->wavReader,MS_FILTER_GET_NCHANNELS,&nchannels);
    ms_filter_call_method(wavplayer->wavReader,MS_FILE_PLAYER_LOOP,&mloop);

    ms_filter_call_method(wavplayer->sndWriter,MS_FILTER_SET_SAMPLE_RATE,&sampleRate);
    ms_filter_call_method(wavplayer->sndWriter,MS_FILTER_SET_NCHANNELS,&nchannels);

    ms_filter_link(wavplayer->wavReader,0,wavplayer->sndWriter,0);
    ms_filter_call_method_noarg(wavplayer->wavReader,MS_FILE_PLAYER_START);

    return wavplayer;
}


void startPlay(WavPlayer *wavplayer)
{
    if(wavplayer->playTicker)
    {
        ms_filter_call_method_noarg(wavplayer->wavReader,MS_FILE_PLAYER_START);
        return;
    }
    else
    {
        if(wavplayer->wavReader && wavplayer->sndWriter)
        {
            wavplayer->playTicker = ms_ticker_new();
            ms_ticker_attach(wavplayer->playTicker,wavplayer->wavReader);
        }
        else
        {
            ortp_message("Wav player is not avaliable");
        }
    }
}


void stopPlay(WavPlayer *wavplayer)
{
    if(!wavplayer->playTicker)
    {
        ortp_message("palyer not avaliable");
    }
    else
    {
        ms_filter_call_method_noarg(wavplayer->wavReader,MS_FILE_PLAYER_STOP);
    }
}


void destroyPlayer(WavPlayer *wavplayer)
{
    stopPlay(wavplayer);
    ms_ticker_detach(wavplayer->playTicker,wavplayer->wavReader);
    ms_filter_unlink(wavplayer->wavReader,0,wavplayer->sndWriter,0);

    ms_filter_destroy(wavplayer->wavReader);
    ms_filter_destroy(wavplayer->sndWriter);
    ms_ticker_destroy(wavplayer->playTicker);

    ms_free(wavplayer);
}
