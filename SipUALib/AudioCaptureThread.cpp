#include "stdafx.h"

#include <jrtplib3/rtpsession.h>
#include "AudioCaptureThread.h"

#include "AudioSendSession.h"

#ifdef ENABLE_AUDIO_DENOISE
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#endif//ENABLE_AUDIO_DENOISE
#include "VideoCapture.h"


IMPLEMENT_DYNCREATE(AudioCaptureThread, AudioThread)

AudioCaptureThread::AudioCaptureThread()
{
    m_hWaveIn = NULL;
    m_pWaveHdrs = NULL;
    m_pBuffers = NULL;
    m_pSession = NULL;

    DEBUG_INFO("AudioCaptureThread::AudioCaptureThread() DONE");
}

AudioCaptureThread::~AudioCaptureThread()
{
    DEBUG_INFO("AudioCaptureThread::~AudioCaptureThread() DONE");
}

BOOL AudioCaptureThread::CreateThread(jrtplib::RTPSession* pSession)
{
    m_pSession = pSession;
    m_outThread.CreateThread(pSession);
    return super::CreateThread();
}

BEGIN_MESSAGE_MAP(AudioCaptureThread, AudioCaptureThread::super)
	ON_THREAD_MESSAGE(MM_WIM_OPEN,ON_WIMOPEN)
	ON_THREAD_MESSAGE(MM_WIM_DATA,ON_WIMDATA)
	ON_THREAD_MESSAGE(MM_WIM_CLOSE,ON_WIMCLOSE)
END_MESSAGE_MAP()

void AudioCaptureThread::InitAudio()
{
    InitFormat();

    m_pWaveHdrs = (PWAVEHDR*) malloc(sizeof(PWAVEHDR*) * SOUND_RECORD_BUFFER_COUNT);
    for (int i=0; i < SOUND_RECORD_BUFFER_COUNT; i++)
        m_pWaveHdrs[i] = (WAVEHDR*)malloc(sizeof(WAVEHDR));

    m_pBuffers = (BYTE**) malloc(sizeof(BYTE*) * SOUND_RECORD_BUFFER_COUNT);
}

void AudioCaptureThread::Start()
{
    InitAudio();

    super::Start();

    m_outThread.Start();
}

void AudioCaptureThread::Stop()
{
    DEBUG_INFO("AudioCaptureThread::Stop()......");

    m_outThread.Stop();

    super::Stop();

    ::Sleep(RECORD_BUFFER_SIZE / 2 * 1000 / SAMPLES_PER_SECOND * SOUND_RECORD_BUFFER_COUNT); // wait for skipping incoming samples

    this->UninitAudio();

    this->WaitForComplete();

    DEBUG_INFO("AudioCaptureThread::Stop() DONE");
}

void AudioCaptureThread::DoJob()
{
    DEBUG_INFO("AudioCaptureThread::DoJob()......");

    MMRESULT mmResult = MMSYSERR_NOERROR;

    //TODO: device id selection
    UINT devID = WAVE_MAPPER;
    //VideoCapture cap;
    //if (cap.EnumDevices(false, true) > 0)
        devID = 0;

    mmResult = ::waveInOpen(&m_hWaveIn
        , devID
        , &m_wfx, (DWORD)this->m_nThreadID, NULL, CALLBACK_THREAD);
    checkMMError(mmResult);

    for (int i=0; i < SOUND_RECORD_BUFFER_COUNT; i++)
    {
        m_pBuffers[i] = (BYTE *) malloc(RECORD_BUFFER_SIZE * sizeof(BYTE));

        m_pWaveHdrs[i]->lpData = reinterpret_cast<LPSTR>( m_pBuffers[i] );
        m_pWaveHdrs[i]->dwBufferLength = RECORD_BUFFER_SIZE;
        m_pWaveHdrs[i]->dwBytesRecorded = 0;
        m_pWaveHdrs[i]->dwUser = i;
        m_pWaveHdrs[i]->dwFlags =0;
        m_pWaveHdrs[i]->dwLoops = 1;
        m_pWaveHdrs[i]->lpNext = NULL;
        m_pWaveHdrs[i]->reserved = 0;

        mmResult = ::waveInPrepareHeader(m_hWaveIn, m_pWaveHdrs[i],sizeof(WAVEHDR));
        checkMMError(mmResult);

        mmResult = ::waveInAddBuffer (m_hWaveIn, m_pWaveHdrs[i], sizeof(WAVEHDR));
        checkMMError(mmResult);
    }

    mmResult = ::waveInStart(m_hWaveIn);
    checkMMError(mmResult);

    DEBUG_INFO("AudioCaptureThread::DoJob() DONE");
}

void AudioCaptureThread::UninitAudio()
{
    DEBUG_INFO("AudioCaptureThread::UninitAudio()......");

    {
        MMRESULT mmResult = ::waveInReset (m_hWaveIn);
        checkMMError(mmResult);

        mmResult = ::waveInClose (m_hWaveIn);
        checkMMError(mmResult);

        m_hWaveIn=NULL;


        if ( m_pBuffers )
        {
            for (int i=0; i < SOUND_RECORD_BUFFER_COUNT; i++)
            {
                BYTE* pBuf = m_pBuffers[i];
                if (pBuf)
                    free(pBuf);
            }
            free( m_pBuffers );
            m_pBuffers = NULL;
        }

        if ( m_pWaveHdrs )
        {
            for (int i=0; i < SOUND_RECORD_BUFFER_COUNT; i++)
            {
                PWAVEHDR pWaveHdr = m_pWaveHdrs[i];
                if (pWaveHdr)
                    free(pWaveHdr);
            }
            free( m_pWaveHdrs );
            m_pWaveHdrs = NULL;
        }
    }

    DEBUG_INFO("AudioCaptureThread::UninitAudio() DONE");
}

void AudioCaptureThread::ON_WIMOPEN(WPARAM wParam, LPARAM lParam)
{
}

void AudioCaptureThread::ON_WIMDATA(WPARAM wParam, LPARAM lParam)
{
    if(IsWorking())
    {
#ifdef ENABLE_AUDIO_DENOISE
        Denoise((BYTE*)((PWAVEHDR)lParam)->lpData, ((PWAVEHDR)lParam)->dwBytesRecorded);
#endif//ENABLE_AUDIO_DENOISE

        static ULONG ulPkgNo = 0;

        MMRESULT mmResult = MMSYSERR_NOERROR;
        PWAVEHDR waveHdr = (PWAVEHDR) lParam;

#ifdef ENABLE_AUDIO_AEC
        VERIFY(::GetMicQueue());
        {
            double dCurTime = jrtplib::RTPTime::CurrentTime().GetDouble();
            RecPkgQueue::element_type bufAEC(new AudioBuffer((BYTE*)waveHdr->lpData, waveHdr->dwBytesRecorded, dCurTime));
            if (IsWorking())
            {
                ::GetMicQueue()->push_back(bufAEC);

#ifdef PRINT_AEC_INFO
                SPRINTF_S(dbg_str,
                    "Audio data: mic recv= %d"
                    "\ttime: %f"
                    , ulPkgNo
                    , dCurTime);
                DEBUG_INFO(dbg_str);
#endif//PRINT_AEC_INFO
            }
        }
#else //ENABLE_AUDIO_AEC

        RecPkgQueue::element_type buf2Send(new AudioBuffer((BYTE*)((PWAVEHDR)lParam)->lpData, ((PWAVEHDR)lParam)->dwBytesRecorded, 0));
        ((AudioSendSession*)m_pSession)->getPackageQueue()->push_back(buf2Send);
#endif//ENABLE_AUDIO_AEC
    
        //mmResult = ::waveInPrepareHeader (hWaveIn, waveHdr, sizeof(WAVEHDR));
        //checkMMError(mmResult, true);
        waveHdr->dwFlags = WHDR_PREPARED;

        mmResult = ::waveInAddBuffer (m_hWaveIn, waveHdr, sizeof(WAVEHDR));
        checkMMError(mmResult, true);

        ++ulPkgNo;
    }
}

void AudioCaptureThread::ON_WIMCLOSE(WPARAM wParam, LPARAM lParam)
{
}

#ifdef ENABLE_AUDIO_DENOISE
void AudioCaptureThread::Denoise(BYTE* lpData, DWORD dwBytes)
{
    DWORD dwProcessed = 0;
    short* in = (short*)lpData;
    int i;
    SpeexPreprocessState *st;
    //int count=0;
    float f;

    st = speex_preprocess_state_init(SAMPLES_PER_RECORD, SAMPLES_PER_SECOND);
    i=1;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &i);
    i=0;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC, &i);
    i=SAMPLES_PER_SECOND;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);
    i=0;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB, &i);
    f=.0;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
    f=.0;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);
    while (dwProcessed < SAMPLES_PER_RECORD)
    {
        int vad;
        //fread(in, sizeof(short), SAMPLES_PER_RECORD, stdin);
        //if (feof(stdin))
        //    break;
        vad = speex_preprocess_run(st, in);
        //fprintf (stderr, "%d\n", vad);
        //fwrite(in, sizeof(short), SAMPLES_PER_RECORD, stdout);
        //count++;
        dwProcessed += SAMPLES_PER_RECORD;
        in += SAMPLES_PER_RECORD;
    }
    speex_preprocess_state_destroy(st);
}
#endif//ENABLE_AUDIO_DENOISE