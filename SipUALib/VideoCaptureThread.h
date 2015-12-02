#pragma once

#ifndef VIDEO_CAPTURE_THREAD_H
#define VIDEO_CAPTURE_THREAD_H


#include "WorkThread.h"
#include "VideoPackThread.h"
#include "VideoCapture.h"


class jrtplib::RTPSession;


class VideoCaptureThread : public WorkThread
{
	DECLARE_DYNCREATE(VideoCaptureThread)
    typedef WorkThread super;
public:
	VideoCaptureThread();
	virtual ~VideoCaptureThread();

    BOOL CreateThread(jrtplib::RTPSession* pSess,
                      VideoFrmQueue* pQueue);

    void SetPreviewWindow(HWND hWnd) { m_hWnd = hWnd; }

// Overrides
public:
    virtual void Start();
	virtual void Stop();
protected:
    virtual void DoJob();

private:
    VideoFrmQueue* m_pQueue;
    HWND m_hWnd;

    jrtplib::RTPSession* m_pSession;
    VideoPackThread m_outThread;

    VideoCapture m_cap;
};

#endif//VIDEO_CAPTURE_THREAD_H