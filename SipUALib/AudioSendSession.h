#pragma once

#ifndef AUDIO_SEND_SESSION_H
#define AUDIO_SEND_SESSION_H

#include "AVRTPSession.h"
#include "PackageOueue.h"
#include "AudioCaptureThread.h"
#include "AudioAECThread.h"



class AudioSendSession : public AVRTPSession
{
    typedef AVRTPSession super;
public:
    AudioSendSession(jrtplib::RTPRandom *rnd = 0, jrtplib::RTPMemoryManager *mgr = 0);
    virtual ~AudioSendSession(void);

public:
    RecPkgQueue* getPackageQueue() { return &m_pkgQueueOut; }
#ifdef ENABLE_AUDIO_AEC
    RecPkgQueue* getMicQueue() { return &m_pkgQueueMic; }
    RecPkgQueue* getRefQueue() { return &m_pkgQueueRef; }
#endif//ENABLE_AUDIO_AEC

    virtual void Start() {
        super::Start();

#ifdef ENABLE_AUDIO_AEC
        m_threadAEC.Start();
#endif//ENABLE_AUDIO_AEC

        m_threadCapture.Start();
    }

    virtual void Stop() {
        if (IsWorking())
        {
            DEBUG_INFO("AudioSendSession::Stop...");

            super::Stop();
            m_threadCapture.Stop();

#ifdef ENABLE_QUEUE_CLEAR
            m_pkgQueueOut.clear();
#endif//ENABLE_QUEUE_CLEAR

#ifdef ENABLE_AUDIO_AEC
            m_threadAEC.Stop();
#endif//ENABLE_AUDIO_AEC

            DEBUG_INFO(L"AudioSendSession::Stop DONE");
        }
    }

private:
    RecPkgQueue m_pkgQueueOut;
#ifdef ENABLE_AUDIO_AEC
    RecPkgQueue m_pkgQueueMic;
    RecPkgQueue m_pkgQueueRef;

    AudioAECThread m_threadAEC;
#endif//ENABLE_AUDIO_AEC

    AudioCaptureThread m_threadCapture;
};


#endif//AUDIO_SEND_SESSION_H