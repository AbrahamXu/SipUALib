#pragma once

#ifndef AUDIO_THREAD_H
#define AUDIO_THREAD_H


#include "mmsystem.h"
#include "WorkThread.h"

class AudioThread : public WorkThread
{
    typedef WorkThread super;
    DECLARE_DYNCREATE(AudioThread)
public:
    AudioThread();
    virtual ~AudioThread();

    void InitFormat();

protected:
    WAVEFORMATEX m_wfx;
};

#endif//AUDIO_THREAD_H