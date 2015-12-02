#include "StdAfx.h"
#include <jrtplib3/rtpsession.h>
#include "AudioThread.h"


IMPLEMENT_DYNCREATE(AudioThread, AudioThread::super)

AudioThread::AudioThread()
{
}

AudioThread::~AudioThread()
{
}

void AudioThread::InitFormat()
{
    memset(&m_wfx, 0, sizeof(m_wfx));
    m_wfx.cbSize            = 0;
    m_wfx.wFormatTag        = WAVE_FORMAT_PCM;
    m_wfx.nChannels         = 1; // mono
    m_wfx.nSamplesPerSec    = SAMPLES_PER_SECOND;
    m_wfx.wBitsPerSample    = BITS_PER_SAMPLE;
    m_wfx.nBlockAlign       = m_wfx.nChannels * m_wfx.wBitsPerSample / 8;
    m_wfx.nAvgBytesPerSec   = AVERAGE_BYTES_PER_SECOND;
}