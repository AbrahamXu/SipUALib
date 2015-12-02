#pragma once

#ifndef VIDEO_SEND_SESSION_H
#define VIDEO_SEND_SESSION_H

#include "AVRTPSession.h"
#include "PackageOueue.h"
#include "VideoCaptureThread.h"

class VideoSendSession : public AVRTPSession
{
    typedef AVRTPSession super;
public:
    VideoSendSession(jrtplib::RTPRandom *rnd = 0, jrtplib::RTPMemoryManager *mgr = 0);
    virtual ~VideoSendSession(void);

public:
    void SetPreviewWindow(HWND hWnd)
    {
        m_hWnd = hWnd;
    }

    virtual void Start()
    {
        super::Start();
        m_threadCapture.SetPreviewWindow(m_hWnd);
        m_threadCapture.Start();
    }

    virtual void Stop() {
        if (IsWorking())
        {
            DEBUG_INFO("VideoSendSession::Stop...");

            super::Stop();
            m_threadCapture.Stop();
#ifdef ENABLE_QUEUE_CLEAR
            m_queueFrames.clear();
#endif//ENABLE_QUEUE_CLEAR

            DEBUG_INFO(L"VideoSendSession::Stop DONE");
        }
    }

protected:
    VideoFrmQueue* getFrames() { return &m_queueFrames; }

private:
    HWND m_hWnd;
    VideoFrmQueue m_queueFrames;
    VideoCaptureThread m_threadCapture;
};

#endif//VIDEO_SEND_SESSION_H