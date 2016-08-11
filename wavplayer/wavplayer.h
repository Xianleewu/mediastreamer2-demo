#ifndef WAVPLAYER_H
#define WAVPLAYER_H

typedef struct _WavPlayer
{
    MSFilter *wavReader;
    MSFilter *sndWriter;
    MSTicker *playTicker;

} WavPlayer;


WavPlayer *creatPlayer(char *filename,int loop);

void startPlay(WavPlayer *wavplayer);

void stopPlay(WavPlayer *wavplayer);

void destroyPlayer(WavPlayer *wavplayer);


#endif // WAVPLAYER_H

