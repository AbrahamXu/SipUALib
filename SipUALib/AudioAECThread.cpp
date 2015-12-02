//#ifdef ENABLE_AUDIO_AEC

#include "stdafx.h"

#include "AudioAECThread.h"

#include "PackageOueue.h"
#include <memory>
#include <jrtplib3/rtptimeutilities.h>
#include "EchoCanceller.h"


IMPLEMENT_DYNCREATE(AudioAECThread, AudioAECThread::super)

AudioAECThread::AudioAECThread()
{
    DEBUG_INFO("AudioAECThread::AudioAECThread() DONE");
}

AudioAECThread::~AudioAECThread()
{
    DEBUG_INFO("AudioAECThread::~AudioAECThread() DONE");
}

BOOL AudioAECThread::CreateThread()
{
    return super::CreateThread();
}

void AudioAECThread::Start()
{
    DEBUG_INFO("AudioAECThread::Start()......");

    super::Start();

    DEBUG_INFO("AudioAECThread::Start() DONE");
}

void AudioAECThread::Stop()
{
    DEBUG_INFO("AudioAECThread::Stop()......");

    super::Stop();

    this->WaitForComplete();

    DEBUG_INFO("AudioAECThread::Stop() DONE");
}

#define AEC_TIME_OFFSET 0.0 // 300 ms, reverberation time for a small room
#define MAX_AEC_TIME_INTERVAL (128.0/8000)// * 3/4) // max time interval between record data and play data, 0.0125 for 200 B (100 samples)

void AudioAECThread::DoJob()
{
    DEBUG_INFO("AudioAECThread::DoJob()......");

#ifdef ENABLE_AUDIO_AEC

    RecPkgQueue* pQueueMic = ::GetMicQueue();
    RecPkgQueue* pQueueRef = ::GetRefQueue();
    RecPkgQueue* pQueueOut = ::GetOutQueue();

    AECQueue aecQueue(pQueueMic, pQueueRef);

    EchoCanceller aec;
    bool bUseAEC = ::GetUseAEC();// enable AEC on recorded data ?
    BYTE* pAecOut = NULL;

    if (bUseAEC)
    {
        aec.Init();
        pAecOut = new BYTE[RECORD_SEND_BUFFER_SIZE]; //TODO: customization needed

        DEBUG_INFO(L"AEC EN-ABLED for audio data");
    }
    else
        DEBUG_INFO(L"AEC DIS-ABLED for audio data");

    int nSentMiced = 0;
    double dLastMatchedMic = 0,
           dLastMatchedRef = 0;
    bool bMatchedBefore = false;

    for(; IsWorking(); )
    {
        while (IsWorking() && pQueueMic->size())
        {
            if (bUseAEC)
            {
                if ( pQueueRef->size() == 0 )
                {
                    //TODO: no play data, if never did AEC, record data is ok to send out
                    //      otherwise, reserve for sync & AEC
                    if (!bMatchedBefore)
                    {
#ifdef PRINT_AEC_INFO
                        SPRINTF_S(dbg_str,
                            "Audio data: mic sent= %d"
                            , nSentMiced);
                        DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO
                        ++nSentMiced;

                        RecPkgQueue::const_reference& pBufMic = pQueueMic->front();
                        pQueueOut->push_back(pBufMic);
                        pQueueMic->pop_front();
                    }
                }
                else
                {
                    int idxMic = 0, idxRef = 0;
                    bool bMatch = aecQueue.FindMatch(AEC_TIME_OFFSET, MAX_AEC_TIME_INTERVAL, idxMic, idxRef);
                    if (bMatch)
                    {
                        bMatchedBefore = true;

                        while (idxMic > 0)
                        {
                            pQueueMic->pop_front();
                            --idxMic;
                        }
                        while (idxRef > 1) // TODO: reserve 1 play for later match
                        {
                            pQueueRef->pop_front();
                            --idxRef;
                        }

                        RecPkgQueue::const_reference& pBufMic = pQueueMic->front();
                        RecPkgQueue::reference& pBufRef = pQueueRef->front();
                        if (idxRef == 1)
                            pBufRef = pQueueRef->at(1);

                        const int nSizeMic = pBufMic->getSize();
                        const int nSizeRef = pBufRef->getSize();
                        ASSERT(nSizeMic == nSizeRef);

                        aec.DoAEC(pBufMic->getBuffer(), pBufRef->getBuffer(), pAecOut);
                        RecPkgQueue::element_type buf2Send(new AudioBuffer(pAecOut, RECORD_SEND_BUFFER_SIZE, 0));//TODO: customization needed
                        pQueueOut->push_back(buf2Send);

                        pQueueMic->pop_front();
                    }
                    else
                    {
                        // if can't found matched audio data, reserve it for later use
#ifdef PRINT_AEC_INFO
                        SPRINTF_S(dbg_str,
                            "Audio data match failed, ref queue = %d"
                            , pQueueRef->size());
                        DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO
                    }
                    jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 1));
                }
            }
            else // bUseAEC == false
            {
                RecPkgQueue::const_reference& pBufMic = pQueueMic->front();
                pQueueOut->push_back(pBufMic);
                pQueueMic->pop_front();
            }

            jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 1));
        }
        jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 1));
    }

    if (bUseAEC)
    {
        aec.Uninit();
        delete[] pAecOut;
    }

#else//ENABLE_AUDIO_AEC
    ASSERT(0); // use this thread while ENABLE_AUDIO_AEC disabled?
#endif//ENABLE_AUDIO_AEC

    DEBUG_INFO("AudioAECThread::DoJob() DONE");
}
