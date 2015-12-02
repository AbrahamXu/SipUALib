#include "stdafx.h"

#include <jrtplib3/rtpsession.h>
#include "AudioPlayThread.h"

#include "AudioSendSession.h"
#include "VideoCapture.h"


IMPLEMENT_DYNCREATE(AudioPlayThread, AudioPlayThread::super)

AudioPlayThread::AudioPlayThread()
{
    m_hWaveOut = NULL;
    m_pWaveHdrs = NULL;
    m_pPkgQueue = NULL;
    m_nSubPkgPlayed = 0;

    DEBUG_INFO("AudioPlayThread::AudioPlayThread() DONE");
}

AudioPlayThread::~AudioPlayThread()
{
    DEBUG_INFO("AudioPlayThread::~AudioPlayThread() DONE");
}

BOOL AudioPlayThread::CreateThread(PlayPkgQueue* pPkgQueue)
{
    m_pPkgQueue = pPkgQueue;
    return super::CreateThread();
}

BEGIN_MESSAGE_MAP(AudioPlayThread, AudioPlayThread::super)
	ON_THREAD_MESSAGE(MM_WOM_OPEN,ON_WOMOPEN)
	ON_THREAD_MESSAGE(MM_WOM_DONE,ON_WOMDATA)
	ON_THREAD_MESSAGE(MM_WOM_CLOSE,ON_WOMCLOSE)
END_MESSAGE_MAP()


void AudioPlayThread::InitAudio()
{
    InitFormat();

    m_pWaveHdrs = (PWAVEHDR*) malloc(sizeof(PWAVEHDR*) * SOUND_PLAY_BUFFER_COUNT);
    for (int i=0; i < SOUND_PLAY_BUFFER_COUNT; i++)
        m_pWaveHdrs[i] = (WAVEHDR*)malloc(sizeof(WAVEHDR));

    //TODO: device id selection
    UINT devID = WAVE_MAPPER;
    //VideoCapture cap;
    //if (cap.EnumDevices(false, false) > 0)
        devID = 0;

    MMRESULT mmResult = ::waveOutOpen (&m_hWaveOut
        ,devID
        ,&m_wfx,(DWORD)this->m_nThreadID,NULL,CALLBACK_THREAD);
    checkMMError(mmResult);
	
    for (int i=0; i < SOUND_PLAY_BUFFER_COUNT; i++)
    {
	    m_pWaveHdrs[i]->dwBytesRecorded =0;
        m_pWaveHdrs[i]->dwUser =   i;
        m_pWaveHdrs[i]->dwFlags =  0;
        m_pWaveHdrs[i]->dwLoops =  1;
        m_pWaveHdrs[i]->lpNext =   NULL;
        m_pWaveHdrs[i]->reserved = 0;
    }
}

void AudioPlayThread::ON_WOMOPEN(WPARAM wParam, LPARAM lParam)
{
}

void AudioPlayThread::ON_WOMDATA(WPARAM wParam, LPARAM lParam)
{
}

void AudioPlayThread::ON_WOMCLOSE(WPARAM wParam, LPARAM lParam)
{
}

void AudioPlayThread::Start()
{
    InitAudio();

    super::Start();
}

void AudioPlayThread::Stop()
{
    DEBUG_INFO("AudioPlayThread::Stop()......");

    super::Stop();

    Sleep(RECORD_SEND_BUFFER_SIZE / 2 * 1000 / SAMPLES_PER_SECOND * SOUND_PLAY_BUFFER_COUNT); // wait for incoming data processed or stopped

    this->UninitAudio();

    this->WaitForComplete();

    DEBUG_INFO("AudioPlayThread::Stop() DONE");
}

void AudioPlayThread::UninitAudio()
{
    DEBUG_INFO("AudioPlayThread::UninitAudio()......");

    {
        if ( m_pWaveHdrs )
        {
            MMRESULT mmResult;

            mmResult = ::waveOutReset (m_hWaveOut);
            checkMMError(mmResult);

            mmResult = ::waveOutClose(m_hWaveOut);
            checkMMError(mmResult);
            m_hWaveOut = NULL;

            if ( m_pWaveHdrs )
            {
                for (int i=0; i < SOUND_PLAY_BUFFER_COUNT; i++)
                {
                    PWAVEHDR pWaveHdr = m_pWaveHdrs[i];
                    if (pWaveHdr)
                        free(pWaveHdr);
                }
                free( m_pWaveHdrs );
                m_pWaveHdrs = NULL;
            }
        }
    }

    DEBUG_INFO("AudioPlayThread::UninitAudio() DONE");
}

void AudioPlayThread::DoJob()
{
    DEBUG_INFO("AudioPlayThread::DoJob()......");

    MMRESULT mmResult = MMSYSERR_NOERROR;
    static ULONG ulPkgNo = 0;

    while (IsWorking()
        && m_pPkgQueue->size() < SOUND_PLAY_BUFFER_COUNT)
        jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 1));

    // play packets
    {
        for (int i=0; IsWorking() && i < SOUND_PLAY_BUFFER_COUNT; i++)
        {
            PlayPkgQueue::const_element_type& pBuf = m_pPkgQueue->at(i);
            //printf("\nPlaying packet(#%d) size: %d\n", ulPkgNo++, pBuf->getSize());

            m_pWaveHdrs[i]->dwBufferLength = pBuf->getSize();
            m_pWaveHdrs[i]->lpData         = (LPSTR) pBuf->getBuffer();

            mmResult = ::waveOutPrepareHeader(m_hWaveOut, m_pWaveHdrs[i], sizeof(WAVEHDR));
            checkMMError(mmResult);

            mmResult = ::waveOutWrite (m_hWaveOut, m_pWaveHdrs[i], sizeof(WAVEHDR));
            checkMMError(mmResult);
        }
    }

    DEBUG_INFO("AudioPlayThread::DoJob() DONE");
}


BOOL AudioPlayThread::PreTranslateMessage(MSG* pMsg) 
{
    if(!IsWorking() || pMsg->message != WOM_DONE )
    	return super::PreTranslateMessage(pMsg);
    else
    {
        MMRESULT mmResult = 0;
        static ULONG ulPkgNo = 0;

        WAVEHDR* waveHdr = (WAVEHDR*) pMsg->lParam;
        //int nBufferIndex = waveHdr->dwUser;

        //mmResult = waveOutUnprepareHeader(m_hWaveOut, waveHdr, sizeof(WAVEHDR));
        //checkMMError(mmResult);

#ifdef ENABLE_AUDIO_AEC
        if (::GetUseAEC())
        {
            VERIFY(::GetRefQueue());

            // save played data for later AEC use
            double dCurTime = jrtplib::RTPTime::CurrentTime().GetDouble();
            RecPkgQueue::element_type bufAEC(new AudioBuffer((BYTE*)waveHdr->lpData, waveHdr->dwBufferLength, dCurTime));
            if (IsWorking())
            {
                ::GetRefQueue()->push_back(bufAEC);

#ifdef PRINT_AEC_INFO
                SPRINTF_S(dbg_str,
                    "Audio data: ref recv= %d"
                    "\ttime: %f"
                    , ulPkgNo
                    , dCurTime);
                DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO
            }
        }
        else
        {
            VERIFY(NULL == ::GetRefQueue());
        }
#endif//ENABLE_AUDIO_AEC

        m_pPkgQueue->pop_front();

        m_nSubPkgPlayed++;
#ifdef REMOVE_AUDIO_DELAY
        if ( 0 == (m_nSubPkgPlayed % RECORD_SEND_BUFFER_SENTOUT_PER_SECOND) )
            this->onPossibleDelay();
#endif//REMOVE_AUDIO_DELAY


        while (IsWorking() && m_pPkgQueue->size() < SOUND_PLAY_BUFFER_COUNT)
            jrtplib::RTPTime::Wait(jrtplib::RTPTime(0, 1));

        if (!IsWorking())
            return super::PreTranslateMessage(pMsg);

        // play 1st packet
        {
            PlayPkgQueue::reference pBuf = m_pPkgQueue->at(SOUND_PLAY_BUFFER_COUNT-1);

            if (!IsWorking())
                return super::PreTranslateMessage(pMsg);
            //printf("\nPlaying packet(#%d) size: %d\n", ulPkgNo, pBuf->getSize());

            waveHdr->dwBufferLength = pBuf->getSize();
            waveHdr->lpData         = (LPSTR) pBuf->getBuffer();

            //mmResult = ::waveOutPrepareHeader (m_hWaveOut,waveHdr,sizeof(WAVEHDR));
            //checkMMError(mmResult, true);

            mmResult = ::waveOutWrite (m_hWaveOut,waveHdr,sizeof(WAVEHDR));
            checkMMError(mmResult, true);

            ulPkgNo++;
        }
        return TRUE;
    }
}

void AudioPlayThread::onPossibleDelay()
{
    double dCurTime = jrtplib::RTPTime::CurrentTime().GetDouble();
    while (IsWorking()
        && m_pPkgQueue->size() > SOUND_PLAY_BUFFER_COUNT)
    {
        PlayPkgQueue::reference pBuf = m_pPkgQueue->front();
        if (dCurTime - pBuf->GetReceiveTime() > AUDIO_PLAY_DELAY_PERMITTED)
        {
            printf("\nAbandoning packet(#%d) size: %d\n", pBuf, pBuf->getSize());
            m_pPkgQueue->pop_front();
        }
        else
            break;
    }
}