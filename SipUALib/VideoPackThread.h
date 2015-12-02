#pragma once

#ifndef VIDEO_PACK_THREAD_H
#define VIDEO_PACK_THREAD_H


#include "WorkThread.h"

namespace jrtplib
{
    class RTPSession;
}
#include "PackageOueue.h"

class VideoPackThread : public WorkThread
{
	DECLARE_DYNCREATE(VideoPackThread)
    typedef WorkThread super;
public:
	VideoPackThread();
	virtual ~VideoPackThread();

    BOOL CreateThread(jrtplib::RTPSession* pSession,
                      VideoFrmQueue* pQueueOut);

// Overrides
public:
    virtual void Start();
    virtual void Stop();
protected:
    virtual void DoJob();

private:
    jrtplib::RTPSession* m_pSession;
    VideoFrmQueue* m_pQueueFrames;
};

#endif//VIDEO_PACK_THREAD_H