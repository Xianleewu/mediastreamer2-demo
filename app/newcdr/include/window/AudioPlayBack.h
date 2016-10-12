#ifndef __AUDIOPLAYBACK_H__
#define __AUDIOPLAYBACK_H__

#include "windows.h"

class AudioPlayBack 
{
public:
	AudioPlayBack();
	~AudioPlayBack();

	int preparePlay(String8 filePath);
	int PrepareAudioPlayBack(String8 filePath);
	int startAudioPlayBack();
	void start();
	void pause();
	void stop();
	void release();
	void reset();
	playerState getCurrentState();
	int stopAudioPlayback(bool bRestoreAlpha=true);
	class AudioPlayBackListener : public CedarMediaPlayer::OnPreparedListener
							, public CedarMediaPlayer::OnCompletionListener
							, public CedarMediaPlayer::OnErrorListener
							, public CedarMediaPlayer::OnInfoListener
	{
	public:
		AudioPlayBackListener(AudioPlayBack *pb){mPB = pb;};
		void onPrepared(CedarMediaPlayer *pMp);
		void onCompletion(CedarMediaPlayer *pMp);
		bool onError(CedarMediaPlayer *pMp, int what, int extra);
		bool onInfo(CedarMediaPlayer *pMp, int what, int extra);
	private:
		AudioPlayBack *mPB;
	};
public:
	playerState mCurrentState;

	CedarMediaPlayer *mCMP;
	AudioPlayBackListener *mAudioPlayBackListener;
};


#endif
