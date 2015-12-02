#pragma once

#ifndef AUDIO_AEC_THREAD_H
#define AUDIO_AEC_THREAD_H


#include "AudioThread.h"


class AudioAECThread : public AudioThread
{
    typedef AudioThread super;
	DECLARE_DYNCREATE(AudioAECThread)
public:
	AudioAECThread();
	virtual ~AudioAECThread();

    BOOL CreateThread();

// Overrides
public:
    virtual void Start();
    virtual void Stop();

protected:
    virtual void DoJob();
};

#endif//AUDIO_AEC_THREAD_H