#include "StdAfx.h"

#include "VideoReceiveSession.h"

#include <iostream>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsourcedata.h>


VideoReceiveSession::VideoReceiveSession(jrtplib::RTPRandom *rnd/* = 0*/,
                                 jrtplib::RTPMemoryManager *mgr/* = 0*/)
: VideoReceiveSession::super(rnd, mgr)
{
    m_unpacker.CreateThread(//hWnd,
        getRawPkgQueue(), getFrameQueue());

    DEBUG_INFO("VideoReceiveSession::VideoReceiveSession() DONE");
}

VideoReceiveSession::~VideoReceiveSession(void)
{
    DEBUG_INFO("VideoReceiveSession::~VideoReceiveSession() DONE");
}

void VideoReceiveSession::ProcessRTPPacket(const jrtplib::RTPSourceData &srcdat,const jrtplib::RTPPacket &rtppack)
{
    {
        VIDEOPACKAGEHEADER* vph = (VIDEOPACKAGEHEADER*) rtppack.GetPayloadData();

        VideoPkgQueue::element_type pBuf(
            new VideoPackage(vph->m_nFrameIndex
                         ,vph->m_nSubIndex
                         ,vph->m_nSubCount
                         ,(BYTE*)rtppack.GetPayloadData() + sizeof(VIDEOPACKAGEHEADER)
                         ,rtppack.GetPayloadLength() - sizeof(VIDEOPACKAGEHEADER)
                         ,rtppack.GetSequenceNumber()
                         ,rtppack.GetExtendedSequenceNumber()
                         ,rtppack.GetReceiveTime().GetDouble()
                         ));

        getRawPkgQueue()->push_back(pBuf);

#ifdef PRINT_AV_INFO
        SPRINTF_S(dbg_str,
            DBG_STR_SIZE,
            "Received package of frame: %d   subIdx: %d   subCnt:%d",
            vph->m_nFrameIndex, vph->m_nSubIndex, vph->m_nSubCount);
        DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO
    }
}
