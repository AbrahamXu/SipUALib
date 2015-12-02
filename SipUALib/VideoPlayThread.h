#pragma once

#ifndef VIDEO_PLAY_THREAD_H
#define VIDEO_PLAY_THREAD_H

#include "WorkThread.h"
#include "PackageOueue.h"
#include "PushSource.h"

class VideoPlayThread : public WorkThread
{
    DECLARE_DYNCREATE(VideoPlayThread)
    typedef WorkThread super;
public:
    VideoPlayThread();
    virtual ~VideoPlayThread();

    BOOL CreateThread(VideoFrmQueue* pQueue);

    void SetRemoteWindow(HWND hWnd) { m_hWnd = hWnd; }

// Overrides
public:
    virtual void Stop();
protected:
    virtual void DoJob();

protected:
    HRESULT Play(VideoFrmQueue* pQueue, HWND hWnd);
    HRESULT SelectAndRender(CSource *pSourceFilter, IFilterGraph **pFG);
    HRESULT PlayBufferWait(IFilterGraph *pFGh, HWND Wnd);

protected:
    HWND m_hWnd;
    VideoFrmQueue* m_pQueue;
    CUnknown* m_renderer;
};

#endif//VIDEO_PLAY_THREAD_H