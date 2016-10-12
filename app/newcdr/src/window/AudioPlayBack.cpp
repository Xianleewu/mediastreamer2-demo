#include "AudioPlayBack.h"
#undef LOG_TAG
#define LOG_TAG "AudioPlayBack"
#include "debug.h"

AudioPlayBack::AudioPlayBack() : 
	mCurrentState(STATE_IDLE)
{
	mCMP = new CedarMediaPlayer;
	mAudioPlayBackListener = new AudioPlayBackListener(this);
}

AudioPlayBack::~AudioPlayBack()
{

}

int AudioPlayBack::startAudioPlayBack()
{
	start();
	return 0;
}

int AudioPlayBack::PrepareAudioPlayBack(String8 filePath)
{
	int wait_count = 0;
	preparePlay(filePath);
	return 0;
}

playerState AudioPlayBack::getCurrentState()
{
	return mCurrentState;
}

int AudioPlayBack::preparePlay(String8 filePath)
{
	/* Idle:	Set Listeners */
	mCMP->setOnPreparedListener(mAudioPlayBackListener);

	mCMP->setOnCompletionListener(mAudioPlayBackListener);

	mCMP->setOnErrorListener(mAudioPlayBackListener);

	mCMP->setOnInfoListener(mAudioPlayBackListener);

	mCMP->setDataSource(filePath);
	/* Initialized */
	mCMP->prepareAsync();
	mCurrentState = STATE_PREPARING;
	return NO_ERROR;
}

void AudioPlayBack::start()
{
	if(mCurrentState == STATE_PREPARED || mCurrentState == STATE_PAUSED)
	{
		if(mCMP != NULL) {
			mCMP->start();
			mCurrentState = STATE_STARTED;
		}
	}
}

void AudioPlayBack::pause()
{
	if(mCurrentState == STATE_STARTED) {
		if(mCMP != NULL) {
			mCMP->pause();
			mCurrentState = STATE_PAUSED;
		}
	}
}
void AudioPlayBack::stop()
{
	if(mCurrentState != STATE_IDLE) {
		if(mCMP != NULL) {
			mCMP->stop();
			mCurrentState = STATE_STOPPED;
		}
	}
}

void AudioPlayBack::release()
{
	if(mCMP != NULL) {
		mCMP->release();
		mCurrentState = STATE_END;
	}
}

void AudioPlayBack::reset()
{
	if(mCMP != NULL) {
		mCMP->reset();
		mCurrentState = STATE_IDLE;
	}
}


void AudioPlayBack::AudioPlayBackListener::onPrepared(CedarMediaPlayer *pMp)
{
	mPB->mCurrentState = STATE_PREPARED;

	db_msg("start CedarMediaPlayer\n");
	mPB->start();
}

void AudioPlayBack::AudioPlayBackListener::onCompletion(CedarMediaPlayer *pMp)
{
    db_msg("stop CedarMediaPlayer\n");
    mPB->stop();
	mPB->reset();
}

bool AudioPlayBack::AudioPlayBackListener::onError(CedarMediaPlayer *pMp, int what, int extra)
{
	db_error("receive onError message!\n");
	return false;
}


bool AudioPlayBack::AudioPlayBackListener::onInfo(CedarMediaPlayer *pMp, int what, int extra)
{
	db_msg("receive onInfo message!\n");
	return false;
}


int AudioPlayBack::stopAudioPlayback(bool bRestoreAlpha)
{
	db_msg("stop AudioPlayBack\n");
	stop();
	reset();
	return 0;
}
