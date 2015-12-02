#pragma once

#ifndef AUDIO_PACK_THREAD_H
#define AUDIO_PACK_THREAD_H

#include "AudioThread.h"

class jrtplib::RTPSession;


class AudioPackThread : public AudioThread
{
    typedef AudioThread super;
	DECLARE_DYNCREATE(AudioPackThread)
public:
	AudioPackThread();
	virtual ~AudioPackThread();

    BOOL CreateThread(jrtplib::RTPSession* pSession);

// Overrides
public:
    virtual void Start();
    virtual void Stop();
protected:
    virtual void DoJob();

private:
    jrtplib::RTPSession* m_pSession;
};

#endif//AUDIO_PACK_THREAD_H