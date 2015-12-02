#include "StdAfx.h"


#include "AudioReceiveSession.h"

#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsourcedata.h>


AudioReceiveSession::AudioReceiveSession(jrtplib::RTPRandom *rnd/* = 0*/,
                                 jrtplib::RTPMemoryManager *mgr/* = 0*/)
: AudioReceiveSession::super(rnd, mgr)
{
    m_playThread.CreateThread(
        getPackageQueue() );

    DEBUG_INFO("AudioReceiveSession::AudioReceiveSession() DONE");
}

AudioReceiveSession::~AudioReceiveSession(void)
{
    DEBUG_INFO("AudioReceiveSession::~AudioReceiveSession() DONE");
}

void AudioReceiveSession::ProcessRTPPacket(const jrtplib::RTPSourceData &srcdat,const jrtplib::RTPPacket &rtppack)
{
    {
        PlayPkgQueue::element_type pBuf(new PlayBuffer((BYTE*)rtppack.GetPayloadData(),
                                                       rtppack.GetPayloadLength(),
                                                       rtppack.GetSequenceNumber(),
                                                       rtppack.GetExtendedSequenceNumber(),
                                                       rtppack.GetReceiveTime().GetDouble()));
        getPackageQueue()->push_back(pBuf);
    }
}