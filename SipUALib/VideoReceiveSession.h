#pragma once

#ifndef VIDEO_RECEIVE_SESSION_H
#define VIDEO_RECEIVE_SESSION_H

#include "AVRTPSession.h"
#include "VideoUnpackThread.h"

class VideoReceiveSession : public AVRTPSession
{
    typedef AVRTPSession super;
public:
    VideoReceiveSession(jrtplib::RTPRandom *rnd = 0, jrtplib::RTPMemoryManager *mgr = 0);
    virtual ~VideoReceiveSession(void);

protected:
//#ifdef RTP_SUPPORT_THREAD
//	virtual void OnPollThreadError(int errcode);
//	virtual void OnPollThreadStep();
//	virtual void OnPollThreadStart(bool &stop);
//	virtual void OnPollThreadStop();
//#endif // RTP_SUPPORT_THREAD

    virtual void ProcessRTPPacket(const jrtplib::RTPSourceData &srcdat,const jrtplib::RTPPacket &rtppack);

public:
    void SetRemoteWindow(HWND hWnd)
    {
        //m_hWnd = hWnd;
        m_unpacker.SetRemoteWindow(hWnd);
    }

    virtual void Start()
    {
        super::Start();
        //m_unpacker.CreateThread(hWnd,
        //    getRawPkgQueue(), getFrameQueue());
        m_unpacker.Start();
    }

    virtual void Stop() {
        if (IsWorking())
        {
            DEBUG_INFO("VideoReceiveSession::Stop...");

            super::Stop();
            m_unpacker.Stop();
#ifdef ENABLE_QUEUE_CLEAR
            m_queueRaw.clear();
            m_queueFrm.clear();
#endif//ENABLE_QUEUE_CLEAR

            DEBUG_INFO(L"VideoReceiveSession::Stop DONE");
        }
    }

protected:
    VideoPkgQueue* getRawPkgQueue() { return &m_queueRaw; }
    VideoFrmQueue* getFrameQueue() { return &m_queueFrm; }

private:
    VideoPkgQueue m_queueRaw;
    VideoFrmQueue m_queueFrm;

    VideoUnpackThread m_unpacker;
    //HWND m_hWnd;
};

#endif//VIDEO_RECEIVE_SESSION_H