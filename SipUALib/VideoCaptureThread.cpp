#include "stdafx.h"

#include <jrtplib3/rtpsession.h>
#include "VideoCaptureThread.h"

#include "PackageOueue.h"
#include "VideoSendSession.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(VideoCaptureThread, VideoCaptureThread::super)

VideoCaptureThread::VideoCaptureThread()
{
    m_pSession = NULL;
    DEBUG_INFO("VideoCaptureThread::VideoCaptureThread() DONE");
}

VideoCaptureThread::~VideoCaptureThread()
{
    DEBUG_INFO("VideoCaptureThread::~VideoCaptureThread() DONE");
}

BOOL VideoCaptureThread::CreateThread(jrtplib::RTPSession* pSess,
                                      VideoFrmQueue* pQueue)
{
    m_pQueue = pQueue;

    m_outThread.CreateThread(pSess, m_pQueue);
    return super::CreateThread();
}

void VideoCaptureThread::DoJob()
{
    DEBUG_INFO("VideoCaptureThread::DoJob()......");

    //TODO: choose device
    HRESULT hr = m_cap.StartPreview(m_cap.EnumDevices()-1,
        m_hWnd, m_pQueue
        //, vsi
        );
    ASSERT(SUCCEEDED(hr));

    DEBUG_INFO("VideoCaptureThread::DoJob() DONE");
}

void VideoCaptureThread::Start()
{
    DEBUG_INFO("VideoCaptureThread::Start()......");

    CoInitialize(NULL);

    m_outThread.Start();

    super::Start();

    DEBUG_INFO("VideoCaptureThread::Start() DONE");
}

void VideoCaptureThread::Stop()
{
    DEBUG_INFO("VideoCaptureThread::Stop()......");

    super::Stop();

    m_cap.StopPreview();

    m_outThread.Stop();

    this->WaitForComplete();

    CoUninitialize();

    DEBUG_INFO("VideoCaptureThread::Stop() DONE");
}
