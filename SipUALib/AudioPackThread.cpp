#include "stdafx.h"

#include <jrtplib3/rtpsession.h>

#include "AudioPackThread.h"

#include "AudioSendSession.h"
#include "PackageOueue.h"
#include <memory>
#include <jrtplib3/rtptimeutilities.h>


IMPLEMENT_DYNCREATE(AudioPackThread, AudioPackThread::super)

AudioPackThread::AudioPackThread()
{
    m_pSession = NULL;

    DEBUG_INFO("AudioPackThread::AudioPackThread() DONE");
}

AudioPackThread::~AudioPackThread()
{
    DEBUG_INFO("AudioPackThread::~AudioPackThread() DONE");
}

BOOL AudioPackThread::CreateThread(jrtplib::RTPSession* pSession)
{
    m_pSession = pSession;
    return super::CreateThread();
}

void AudioPackThread::Start()
{
    super::Start();
}

void AudioPackThread::Stop()
{
    DEBUG_INFO("AudioPackThread::Stop()......");

    super::Stop();

    this->WaitForComplete();

    DEBUG_INFO("AudioPackThread::Stop() DONE");
}

void AudioPackThread::DoJob()
{
    DEBUG_INFO("AudioPackThread::DoJob()......");

    static ULONG ulPkgNo = 1;
    AudioSendSession* pSess = dynamic_cast<AudioSendSession*>(m_pSession);

    RecPkgQueue* pQueue = pSess->getPackageQueue();
    for(; IsWorking() ;)
    {
        while (IsWorking()
            && pQueue->size())
        {
            RecPkgQueue::const_reference& pBuf = pQueue->front();

            const int nSizePerSubPackage = RECORD_SEND_BUFFER_SIZE;

            int nPkgSize = pBuf->getSize();
            int nPackets = nPkgSize / nSizePerSubPackage;
            if (nPackets * nSizePerSubPackage < nPkgSize)
                nPackets++;

            int nSizeLeft = nPkgSize;
            int nPos2Send = 0;
            int nSize2Send = 0;

            for (int i=0; IsWorking() && i < nPackets; i++)
            {
#ifdef PRINT_AV_INFO
                SPRINTF_S(dbg_str, "Sending packet(#%d) %d/%d", ulPkgNo, i+1, nPackets);
                DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO

                if ( nSizeLeft > RECORD_SEND_BUFFER_SIZE )
                {
                    nSize2Send = nSizePerSubPackage;
                    nSizeLeft -= nSizePerSubPackage;
                }
                else
                {
                    nSize2Send = nSizeLeft;// last package
                }

                if ( !IsWorking() )
                    break;

                int status = pSess->SendPacket(pBuf->getBuffer() + nPos2Send,
                    nSize2Send,
                    PAYLOAD_TYPE_AUDIO,
                    MARK_FLAG_AUDIO,
                    TIMESTAMP_TO_INC);
                checkRtpError(status);

                nPos2Send += nSizePerSubPackage;
            }

            if ( !IsWorking() )
                break;
            pQueue->pop_front();
            ulPkgNo++;
        }
        jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 1));
    }

    DEBUG_INFO("AudioPackThread::DoJob() DONE");
}