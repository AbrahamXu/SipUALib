#include "StdAfx.h"
#include "AudioSendSession.h"


AudioSendSession::AudioSendSession(jrtplib::RTPRandom *rnd/* = 0*/,
                                 jrtplib::RTPMemoryManager *mgr/* = 0*/)
                                 : AudioSendSession::super(rnd, mgr)
{
    m_threadCapture.CreateThread(this);
#ifdef ENABLE_AUDIO_AEC
    m_threadAEC.CreateThread();//&m_pkgQueueMic, &m_pkgQueueRef, &m_pkgQueue);
#endif//ENABLE_AUDIO_AEC

    DEBUG_INFO("AudioSendSession::AudioSendSession() DONE");
}

AudioSendSession::~AudioSendSession(void)
{
    DEBUG_INFO("AudioSendSession::~AudioSendSession() DONE");
}