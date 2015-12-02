#include "stdafx.h"

//#include "rtpsession.h"

#include "VideoUnpackThread.h"

//#include "SenderSession.h"
#include "PackageOueue.h"
#include <memory>
#include <jrtplib3/rtptimeutilities.h>
#include "CodecUtils.h"
#include <algorithm>


IMPLEMENT_DYNCREATE(VideoUnpackThread, VideoUnpackThread::super)

VideoUnpackThread::VideoUnpackThread()
{
    m_pQueueRaw = NULL;
    m_pQueuePacked = NULL;

    DEBUG_INFO("VideoUnpackThread::VideoUnpackThread() DONE");
}

VideoUnpackThread::~VideoUnpackThread()
{
    DEBUG_INFO("VideoUnpackThread::~VideoUnpackThread() DONE");
}

BOOL VideoUnpackThread::CreateThread(VideoPkgQueue* pQueueRaw,
                                     VideoFrmQueue* pQueuePacked)
{
    m_pQueueRaw = pQueueRaw;
    m_pQueuePacked = pQueuePacked;

    m_player.CreateThread(m_pQueuePacked);
    return super::CreateThread();
}

BOOL VideoUnpackThread::InitInstance()
{
    super::InitInstance();

    CodecUtils::Init(CODEC_FORMAT);

	return TRUE;
}

void VideoUnpackThread::Start()
{
    m_player.Start();
    super::Start();
}

void VideoUnpackThread::Stop()
{
    DEBUG_INFO("VideoUnpackThread::Stop()......");

    m_player.Stop();

    super::Stop();

    this->WaitForComplete();

    DEBUG_INFO(L"VideoUnpackThread::Stop() DONE");
}

bool VideoUnpackThread::FrameReady(const std::shared_ptr<VideoPkgofSingleFrame>& camList)
{
    VideoPkgType camBuf = camList->begin()->second;
    return camBuf->m_nSubCount == camList->size();
}

void VideoUnpackThread::onPossibleDelay(VideoFramePackages& dictPkg)
{
    double dCurTime = jrtplib::RTPTime::CurrentTime().GetDouble();
    while (IsWorking()
        && dictPkg.size() > 0)
    {
        VideoFramePackages::const_iterator cIt = dictPkg.begin();

        std::shared_ptr<VideoPkgofSingleFrame> pList = cIt->second;
        ASSERT(pList->size() > 0);

        VideoPkgType pVideoPacket = pList->begin()->second;
        if (dCurTime - pVideoPacket->GetReceiveTime() > VIDEO_PLAY_DELAY_PERMITTED)
        {
            SPRINTF_S(dbg_str,
                "%d video packets of frame(#%d) were abandoned.",
                pList->size(), pVideoPacket->m_nFrameIndex);
            DEBUG_INFO(dbg_str);

            dictPkg.erase(cIt);
        }
        else
            break;
    }
}

void VideoUnpackThread::DoJob()
{
    DEBUG_INFO("VideoUnpackThread::DoJob()......");

    VideoPkgQueue* pQueueRaw = m_pQueueRaw;
    VideoFrmQueue* pQueuePacked = m_pQueuePacked;

    VideoFramePackages dictPkg;

    uint32_t nPos;

    int nBufSizeRGB = 1024 * 1024;
    uint8_t* pBufRGB = new UINT8[nBufSizeRGB];
    memset(pBufRGB, 0, nBufSizeRGB);

    int nBufSizeH264 = 1024 * 1024;
    uint8_t* pBufH264 = new UINT8[nBufSizeH264];
    memset(pBufH264, 0, nBufSizeH264);

    uint32_t seqNumber = 0;
    uint32_t extSeqNumber = 0;
    double sysTimeReceived = 0;

    uint32_t iFrame;
    for(; IsWorking(); )
    {
        while (IsWorking() && pQueueRaw->size() > 0)
        {
            VideoPkgQueue::element_type& camBuf = pQueueRaw->front();

            iFrame = camBuf->m_nFrameIndex;
            
            VideoFramePackages::const_iterator cIt = dictPkg.find( iFrame );
            if (cIt == dictPkg.end())
            {
                std::shared_ptr<VideoPkgofSingleFrame> pList = std::shared_ptr<VideoPkgofSingleFrame>(new VideoPkgofSingleFrame);
                dictPkg.insert(std::make_pair(iFrame, pList));
                dictPkg[iFrame]->insert(std::make_pair(camBuf->m_nSubIndex, camBuf));
            }
            else
                dictPkg[iFrame]->insert(std::make_pair(camBuf->m_nSubIndex, camBuf));

            pQueueRaw->pop_front();

            const std::shared_ptr<VideoPkgofSingleFrame>& camList = dictPkg[iFrame];
#ifdef PRINT_AV_INFO
            SPRINTF_S(dbg_str,
                DBG_STR_SIZE,
                "Received frame: %d   subIdx: %d",
                iFrame, camList->size());
            DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO

            if ( FrameReady(camList) )
            {
                nPos = 0;
                for (int i = 0; i < camList->size(); i++)
                {
                    VideoPkgType camBuf = camList->at(i);
                    memcpy(pBufH264 + nPos, camBuf->getBuffer(), camBuf->getSize());
                    nPos += camBuf->getSize();
                    // obtain rtp time info
                    if (i == 0)
                    {
                        seqNumber = camBuf->GetSequenceNumber();
                        extSeqNumber = camBuf->GetExtendedSequenceNumber();
                        sysTimeReceived = camBuf->GetReceiveTime();
                    }
                }

                int src_buf_size = nPos;
                int dst_buf_size = nBufSizeRGB;
                bool bDecoded = false;
                try
                {
                    CodecUtils::CODEC_INFO dec_info;
                    dec_info.codec_fmt = CAPTURE_PIX_FORMAT;

                    dec_info.src_width = getRemoteVideoFormatInfo()->m_nWidth;//H264_WIDTH;
                    dec_info.src_height = getRemoteVideoFormatInfo()->m_nHeight;//H264_HEIGHT;
                    dec_info.src_buf = pBufH264;
                    dec_info.src_buf_size = src_buf_size;

                    dec_info.dst_width = CAPTURE_WIDTH;
                    dec_info.dst_height = CAPTURE_HEIGHT;
                    dec_info.dst_buf = pBufRGB;
                    dec_info.dst_buf_size = &dst_buf_size;

                    CodecUtils::video_decode(dec_info);

                    bDecoded = true;
                }
                catch (CException* e)
                {
                    TCHAR   szError[255];
                    e->GetErrorMessage(szError,255);
                    SPRINTF_S(dbg_str,
                        DBG_STR_SIZE,
                        "Decoding failed with exception: %s"
                        , szError);
                    DEBUG_INFO(dbg_str);
                }
                catch (...)
                {
                    DEBUG_INFO("Decoding failed with UNKNOWN exception.");
                }

                dictPkg.erase(iFrame);

                if (bDecoded)
                {
                    VideoFrmQueue::element_type frmReceived(
                        new VideoFrame(
                        iFrame
                        ,pBufRGB
                        ,dst_buf_size
                        ,seqNumber
                        ,extSeqNumber
                        ,sysTimeReceived
                        ) );
                    pQueuePacked->push_back(frmReceived);
                }
            }
        }
        ::Sleep(1);
    }

    delete[] pBufH264;
    delete[] pBufRGB;

    DEBUG_INFO(L"VideoUnpackThread::DoJob() DONE");
}