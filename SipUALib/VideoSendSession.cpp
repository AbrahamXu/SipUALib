#include "StdAfx.h"
#include "VideoSendSession.h"


VideoSendSession::VideoSendSession(jrtplib::RTPRandom *rnd/* = 0*/,
                             jrtplib::RTPMemoryManager *mgr/* = 0*/)
: VideoSendSession::super(rnd, mgr)
{
    m_threadCapture.CreateThread(this, this->getFrames());

    DEBUG_INFO("VideoSendSession::VideoSendSession() DONE");
}

VideoSendSession::~VideoSendSession()
{
    DEBUG_INFO("VideoSendSession::~VideoSendSession() DONE");
}