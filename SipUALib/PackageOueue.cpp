#include "stdafx.h"
#include "PackageOueue.h"

#include "BufferManager.h"



#pragma push_macro("new")
#undef new
AECC_IMPLEMENT_FREELIST(BaseBuffer);
#pragma pop_macro("new")

BaseBuffer::BaseBuffer(BYTE* pBuf, int size)
{
    m_nSize = size;
    //m_pBuf = new BYTE[m_nSize];
    m_pBuf = (BYTE*) GetMemMgr()->Alloc(size);
    memcpy(m_pBuf, pBuf, size);
}

BaseBuffer::BaseBuffer(const BaseBuffer& other)
{
    m_nSize = other.m_nSize;
    //m_pBuf = new BYTE[m_nSize];
    m_pBuf = (BYTE*) GetMemMgr()->Alloc(m_nSize);
    memcpy(m_pBuf, other.m_pBuf, other.m_nSize);
}

BaseBuffer::~BaseBuffer()
{
    //delete[] m_pBuf;
    GetMemMgr()->Free(m_pBuf, m_nSize);
}

#pragma push_macro("new")
#undef new
AECC_IMPLEMENT_FREELIST(AudioBuffer);
#pragma pop_macro("new")

AudioBuffer::AudioBuffer(BYTE* pBuf, int size, double sysTimeRecordedOrPlayed)
    : BaseBuffer(pBuf, size)
    , AECInfo(sysTimeRecordedOrPlayed)
{
}

AudioBuffer::AudioBuffer(const AudioBuffer& other)
    : BaseBuffer(other.m_pBuf, other.m_nSize)
    , AECInfo(other.m_sysTimeRecordedOrPlayed)
{
}

AudioBuffer::~AudioBuffer()
{
}


#pragma push_macro("new")
#undef new
AECC_IMPLEMENT_FREELIST(PlayBuffer);
#pragma pop_macro("new")

PlayBuffer::PlayBuffer(BYTE* pBuf, int size, uint32_t seqNumber, uint32_t extSeqNumber, double sysTimeReceived)
    : BaseBuffer(pBuf, size),
    RTPPackageInfo(seqNumber, extSeqNumber, sysTimeReceived)
{
}

PlayBuffer::PlayBuffer(const PlayBuffer& other)
    : BaseBuffer(other.m_pBuf, other.m_nSize),
    RTPPackageInfo(other.m_seqNumber, other.m_extSeqNumber, other.m_sysTimeReceived)
{
}

PlayBuffer::~PlayBuffer()
{
}


#pragma push_macro("new")
#undef new
AECC_IMPLEMENT_FREELIST(VideoFrame);
#pragma pop_macro("new")


#pragma push_macro("new")
#undef new
AECC_IMPLEMENT_FREELIST(VideoPackage);
#pragma pop_macro("new")


//TODO: move to somewhere like videosetting/videocfg
VIDEOSAMPLEINFO g_localVInfo;
VIDEOFORMATINFO g_remoteVInfo;

const VIDEOSAMPLEINFO* getVideoSampleInfo()
{
    return &g_localVInfo;
}
void setVideoSampleInfo(const VIDEOSAMPLEINFO& val)
{
    g_localVInfo = val;
}
const VIDEOFORMATINFO* getRemoteVideoFormatInfo()
{
    return &g_remoteVInfo;
}
void setRemoteVideoFormatInfo(const VIDEOFORMATINFO& val)
{
    g_remoteVInfo = val;
}


#ifdef ENABLE_AUDIO_AEC
bool AECQueue::FindMatch(double offset, double maxTimeInterval, int& idxMic, int& idxRef)
{
    //const double dMaxInterval = 10000;
    //double minInterval = dMaxInterval;
    double dMic = 0;
    double dRef = 0;

#ifdef PRINT_AEC_INFO
    SPRINTF_S(dbg_str,
        "Audio data match: mic queue=%d\tref queue=%d"
        "           offset: %.4f interval: %.4f"
        , m_pQueueMic->size(), m_pQueueRef->size()
        , offset, maxTimeInterval);
    DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO

    bool bMatch = false;
    bool bMicBehindOfPlay = false;
    m_cs.Lock();

    for (size_t i=0; !bMatch && i < m_pQueueMic->size(); ++i)
    {
        //#ifdef PRINT_AEC_INFO
        //            SPRINTF_S(dbg_str,
        //                "Audio data match: mic index=%d"
        //                , i);
        //            DEBUG_INFO(dbg_str);
        //#endif//PRINT_AEC_INFO
        bMicBehindOfPlay = false;
        RecPkgQueue::const_reference& pBufMic = m_pQueueMic->at(i);

        dMic = pBufMic->GetRecordedOrPlayedTime();

        for (size_t j=0; !bMatch && j < m_pQueueRef->size(); ++j)
        //size_t j=0;
        {
    //#ifdef PRINT_AEC_INFO
    //        SPRINTF_S(dbg_str,
    //            "Audio data match: ref index=%d"
    //            , j);
    //        DEBUG_INFO(dbg_str);
    //#endif//PRINT_AEC_INFO

            RecPkgQueue::const_reference& pBufRef = m_pQueueRef->at(j);

            dRef = pBufRef->GetRecordedOrPlayedTime();

#ifdef PRINT_AEC_INFO
            SPRINTF_S(dbg_str,
                "Audio data time of mic[%d]: %f\tref[%d]: %f"
                , i, dMic
                , j, dRef);
            DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO

            //double interval = dMic - dRef/* - maxTimeInterval*/;
            double interval = (dMic - offset) - dRef/* - maxTimeInterval*/;

            if (interval < 0)
            { // mic now ahead of this play
                if (bMicBehindOfPlay) // last play ahead of this mic
                {
                    idxMic = i;
                    idxRef = j - 1;
                    bMatch = true;
                    break;
                }
                //else
                //{
                //    if (interval * (-1) < maxTimeInterval / 4)
                //    {
                //        idxMic = i;
                //        idxRef = j;
                //        bMatch = true;
                //        break;
                //    }
                //    else
                //    {// mic too much ahead of rec, try another mic
                //        break;
                //    }
                //}
            }
            else if (interval > 0)
            { // mic now behind of this play
                bMicBehindOfPlay = true;

                if (interval < maxTimeInterval * 1.5)
                {
                    idxMic = i;
                    idxRef = j;
                    bMatch = true;
                    break;
                }
            }
        }
    }

    if (bMatch)
    {
#ifdef PRINT_AEC_INFO
        SPRINTF_S(dbg_str,
            "Audio data matched: "
            "\tat mic: %d"
            "\tat ref: %d"
            , idxMic
            , idxRef);
        DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO
    }
    else
    {
#ifdef PRINT_AEC_INFO
        //DEBUG_INFO("Audio data NO MATCH found!");
#endif//PRINT_AEC_INFO
    }

    m_cs.Unlock();
    return bMatch;
}
#endif//ENABLE_AUDIO_AEC
