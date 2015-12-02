#include "stdafx.h"

#include <jrtplib3/rtpsession.h>
#include "VideoPackThread.h"

#include "VideoSendSession.h"
#include "PackageOueue.h"
#include <memory>
#include "CodecUtils.h"


IMPLEMENT_DYNCREATE(VideoPackThread, VideoPackThread::super)

VideoPackThread::VideoPackThread()
{
    m_pSession = NULL;
    m_pQueueFrames = NULL;

    DEBUG_INFO("VideoPackThread::VideoPackThread() DONE");
}

VideoPackThread::~VideoPackThread()
{
    DEBUG_INFO("VideoPackThread::~VideoPackThread() DONE");
}

BOOL VideoPackThread::CreateThread(jrtplib::RTPSession* pSession,
                                VideoFrmQueue* pQueueOut)
{
    m_pSession = pSession;
    m_pQueueFrames = pQueueOut;
    return super::CreateThread();
}

void VideoPackThread::Start()
{
    DEBUG_INFO("VideoPackThread::Start()......");

    CodecUtils::Init(CODEC_FORMAT);

    super::Start();

    DEBUG_INFO("VideoPackThread::Start() DONE");
}

void VideoPackThread::Stop()
{
    DEBUG_INFO("VideoPackThread::Stop()......");

    super::Stop();

    this->WaitForComplete();

    DEBUG_INFO("VideoPackThread::Stop() DONE");
}

void VideoPackThread::DoJob()
{
    DEBUG_INFO("VideoPackThread::DoJob()......");

    VideoSendSession* pSess = dynamic_cast<VideoSendSession*>(m_pSession);

    VideoFrmQueue* pQueueFrames = m_pQueueFrames;

    int nBufSizeTemp = 1024 * 1024;
    uint8_t* pBufTemp = new UINT8[nBufSizeTemp]; // buf to hold encoded frame
    memset(pBufTemp, 0, nBufSizeTemp);

    std::auto_ptr<uint8_t> pBufSend(// buf for send sub-package
        new uint8_t[
            sizeof(VIDEOPACKAGEHEADER)
        + VIDEO_SEND_BUFFER_SIZE] );

    for(; IsWorking(); )
    {
        while (IsWorking() && pQueueFrames->size())
        {
            static char szInfo[512] = {0};
#ifdef PRINT_AV_INFO
            sprintf_s(szInfo,
                "\nRaw QueueOut: %d"
                , pQueueFrames->size()
                );
            DEBUG_INFO(szInfo);
#endif//PRINT_AV_INFO

            VideoFrmType frame = pQueueFrames->front();

            CodecUtils::CODEC_INFO enc_info;
            int enc_buf_size = nBufSizeTemp;

            bool bEncoded = false;
            try
            {
                enc_info.codec_fmt = CAPTURE_PIX_FORMAT;

                enc_info.src_width = CAPTURE_WIDTH;
                enc_info.src_height = CAPTURE_HEIGHT;
                enc_info.src_buf = frame->getBuffer();
                enc_info.src_buf_size = frame->getSize();

                enc_info.dst_width = getRemoteVideoFormatInfo()->m_nWidth;//H264_WIDTH;
                enc_info.dst_height = getRemoteVideoFormatInfo()->m_nHeight;//H264_HEIGHT;
                enc_info.dst_buf = pBufTemp;
                enc_info.dst_buf_size = &enc_buf_size;

                CodecUtils::video_encode(enc_info);

                bEncoded = true;
            }
            catch (CException* e)
            {
                TCHAR   szError[255];
                e->GetErrorMessage(szError,255);
                SPRINTF_S(dbg_str,
                    DBG_STR_SIZE,
                    "Encoding failed with exception: %s"
                    , szError);
                DEBUG_INFO(dbg_str);
            }
            catch (...)
            {
                DEBUG_INFO("Encoding failed with UNKNOWN exception.");
            }

            if (bEncoded)
            {
                const int nSizePerSubPackage = VIDEO_SEND_BUFFER_SIZE;

                int nPkgSize = *(enc_info.dst_buf_size);
                int nPackets = nPkgSize / nSizePerSubPackage;
                if (nPackets * nSizePerSubPackage < nPkgSize)
                    nPackets++;

                int nSizeLeft = nPkgSize;
                int nPos2Send = 0;
                int nSize2Send = 0;

                VIDEOPACKAGEHEADER vph(
                    frame->m_nFrameIndex
                    ,nPackets
                    ,nPackets);

                for (int i=0; IsWorking() && i < nPackets; i++)
                {
                    vph.m_nSubIndex = i;

#ifdef PRINT_AV_INFO
                    SPRINTF_S(dbg_str,
                        "\nSending video packet(#%d) %d/%d\n"
                        , vph.m_nFrameIndex
                        , vph.m_nSubIndex + 1
                        , nPackets);
                    DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO

                    if ( nSizeLeft > VIDEO_SEND_BUFFER_SIZE )
                    {
                        nSize2Send = nSizePerSubPackage;
                        nSizeLeft -= nSizePerSubPackage;
                    }
                    else
                    {
                        nSize2Send = nSizeLeft;// last package
                    }
                    memcpy(pBufSend.get(),
                        &vph,
                        sizeof(vph));

                    memcpy(pBufSend.get() + sizeof(vph),
                        enc_info.dst_buf + nPos2Send,
                        nSize2Send);

                    nPos2Send += nSize2Send;

                    if ( !IsWorking() )
                        break;

                    int status = pSess->SendPacket(
                        pBufSend.get(),
                        sizeof(vph) + nSize2Send,
                        PAYLOAD_TYPE_VIDEO,
                        MARK_FLAG_VIDEO,
                        TIMESTAMP_TO_INC);
                    checkRtpError(status);
                }
            }

            if ( !IsWorking() )
                break;
            pQueueFrames->pop_front();
        }
        jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 10));
    }

    delete[] pBufTemp;

    DEBUG_INFO("VideoPackThread::DoJob() DONE");
}
