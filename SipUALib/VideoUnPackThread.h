#pragma once

#ifndef VIDEO_UNPACK_THREAD_H
#define VIDEO_UNPACK_THREAD_H


#include "WorkThread.h"
#include "PackageOueue.h"
#include "VideoPlayThread.h"

class VideoUnpackThread : public WorkThread
{
	DECLARE_DYNCREATE(VideoUnpackThread)
    typedef WorkThread super;
public:
	VideoUnpackThread();
	virtual ~VideoUnpackThread();

    BOOL CreateThread(VideoPkgQueue* pQueueRaw,
                      VideoFrmQueue* pQueuePacked);

    void SetRemoteWindow(HWND hWnd) { m_player.SetRemoteWindow(hWnd); }

// Overrides
public:
	virtual BOOL InitInstance();

    virtual void Start();
    virtual void Stop();
protected:
    virtual void DoJob();

protected:
    bool FrameReady(const std::shared_ptr<VideoPkgofSingleFrame>& camList);
    void onPossibleDelay(VideoFramePackages& dictPkg);

private:
    VideoPkgQueue* m_pQueueRaw;
    VideoFrmQueue* m_pQueuePacked;

    VideoPlayThread m_player;
};

#endif//VIDEO_UNPACK_THREAD_H