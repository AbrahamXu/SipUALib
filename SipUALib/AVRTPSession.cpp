#include "StdAfx.h"
#include "AVRTPSession.h"

#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpsourcedata.h>

#include "BufferManager.h"

jrtplib::RTPMemoryManager* GetRTPMemMgr()
{
    return ::GetMemMgr();
}


AVRTPSession::AVRTPSession(jrtplib::RTPRandom *rnd/* = 0*/,
                           jrtplib::RTPMemoryManager *mgr/* = 0*/)
: AVRTPSession::super(rnd, mgr)
{
}

AVRTPSession::~AVRTPSession(void)
{
}

int AVRTPSession::SendPacket(const void *data,size_t len,
    uint8_t pt,bool mark,uint32_t timestampinc)
{
    if (IsWorking())
        return super::SendPacket(data, len, pt, mark, timestampinc);
    else
        return 0;
}


void AVRTPSession::OnRTPPacket(jrtplib::RTPPacket *pack,
                               const jrtplib::RTPTime &receivetime,
                               const jrtplib::RTPAddress *senderaddress)
{
}

void AVRTPSession::OnRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack,
                                        const jrtplib::RTPTime &receivetime,
                                        const jrtplib::RTPAddress *senderaddress)
{
}

void AVRTPSession::OnNewSource(jrtplib::RTPSourceData *srcdat)
{
}

void AVRTPSession::OnRemoveSource(jrtplib::RTPSourceData *srcdat)
{
}
void AVRTPSession::OnTimeout(jrtplib::RTPSourceData *srcdat)
{
}

void AVRTPSession::OnBYETimeout(jrtplib::RTPSourceData *srcdat)
{
}

void AVRTPSession::OnAPPPacket(jrtplib::RTCPAPPPacket *apppacket,
                               const jrtplib::RTPTime &receivetime,
                               const jrtplib::RTPAddress *senderaddress)
{
}

void AVRTPSession::OnUnknownPacketType(jrtplib::RTCPPacket *rtcppack,
                                       const jrtplib::RTPTime &receivetime,
                                       const jrtplib::RTPAddress *senderaddress)
{
}

void AVRTPSession::OnUnknownPacketFormat(jrtplib::RTCPPacket *rtcppack,
                                         const jrtplib::RTPTime &receivetime,
                                         const jrtplib::RTPAddress *senderaddress)
{
}

void AVRTPSession::OnNoteTimeout(jrtplib::RTPSourceData *srcdat)
{
}

void AVRTPSession::OnBYEPacket(jrtplib::RTPSourceData *srcdat)
{
}

void AVRTPSession::OnSendRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack)
{
}

#ifdef RTP_SUPPORT_THREAD
void AVRTPSession::OnPollThreadError(int errcode)
{
}

void AVRTPSession::OnPollThreadStep()
{
    if (IsWorking())
    {
        BeginDataAccess();

        // check incoming packets
        if (GotoFirstSourceWithData())
        {
            do
            {
                jrtplib::RTPPacket *pack;
                jrtplib::RTPSourceData *srcdat;

                srcdat = GetCurrentSourceInfo();

                while ((pack = GetNextPacket()) != NULL)
                {
                    if ( IsWorking() )
                    {
#ifdef PRINT_AV_INFO
                        SPRINTF_S(dbg_str, DBG_STR_SIZE,
                            "Got packet %d from SSRC %d",
                            pack->GetExtendedSequenceNumber(),
                            srcdat->GetSSRC()
                            );
                        DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO

                        ProcessRTPPacket(*srcdat,*pack);
                    }
                    else
                    {
#ifdef PRINT_AV_INFO
                        SPRINTF_S(dbg_str, "Got&Discard packet %d from SSRC %d",
                            pack->GetExtendedSequenceNumber(),
                            srcdat->GetSSRC()
                            );
                        DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO
                    }

                    DeletePacket(pack);
                }
            } while (GotoNextSourceWithData());
        }

        EndDataAccess();
    }
}

void AVRTPSession::OnPollThreadStart(bool &stop)
{
}

void AVRTPSession::OnPollThreadStop()
{
}
#endif // RTP_SUPPORT_THREAD

void AVRTPSession::ProcessRTPPacket(const jrtplib::RTPSourceData &srcdat,const jrtplib::RTPPacket &rtppack)
{
}