#pragma once

#ifndef AUDIO_RECEIVE_SESSION_H
#define AUDIO_RECEIVE_SESSION_H

#include "AVRTPSession.h"
#include "AudioPlayThread.h"

class AudioReceiveSession : public AVRTPSession
{
    typedef AVRTPSession super;
public:
    AudioReceiveSession(jrtplib::RTPRandom *rnd = 0, jrtplib::RTPMemoryManager *mgr = 0);
    virtual ~AudioReceiveSession(void);

protected:
//#ifdef RTP_SUPPORT_THREAD
//	virtual void OnPollThreadError(int errcode);
//	virtual void OnPollThreadStep();
//	virtual void OnPollThreadStart(bool &stop);
//	virtual void OnPollThreadStop();
//#endif // RTP_SUPPORT_THREAD

    virtual void ProcessRTPPacket(const jrtplib::RTPSourceData &srcdat,const jrtplib::RTPPacket &rtppack);

public:
    virtual void Start() {
        super::Start();
        m_playThread.Start();
    }

    virtual void Stop() {
        if (IsWorking())
        {
            DEBUG_INFO("AudioReceiveSession::Stop...");

            super::Stop();
            m_playThread.Stop();
#ifdef ENABLE_QUEUE_CLEAR
            m_pkgQueue.clear();
#endif//ENABLE_QUEUE_CLEAR

            DEBUG_INFO(L"AudioReceiveSession::Stop DONE");
        }
    }

    PlayPkgQueue* getPackageQueue() { return &m_pkgQueue; }

private:
    PlayPkgQueue m_pkgQueue;

    AudioPlayThread m_playThread;
};

#endif//AUDIO_RECEIVE_SESSION_H