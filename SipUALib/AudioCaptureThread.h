#pragma once

#ifndef AUDIO_CAPTURE_THREAD_H
#define AUDIO_CAPTURE_THREAD_H


#include "AudioThread.h"

class jrtplib::RTPSession;
#include "AudioPackThread.h"


class AudioCaptureThread : public AudioThread
{
    typedef AudioThread super;
	DECLARE_DYNCREATE(AudioCaptureThread)
public:
	AudioCaptureThread();
	virtual ~AudioCaptureThread();

    BOOL CreateThread(jrtplib::RTPSession* pSession);

// Overrides
public:
    virtual void Start();
    virtual void Stop();
protected:
    virtual void DoJob();

public:
	void InitAudio();
    void UninitAudio();

protected:
    afx_msg	void ON_WIMDATA(WPARAM wParam, LPARAM lParam);
    afx_msg	void ON_WIMOPEN(WPARAM wParam, LPARAM lParam);
    afx_msg	void ON_WIMCLOSE(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

#ifdef ENABLE_AUDIO_DENOISE
    void Denoise(BYTE* lpData, DWORD dwBytes);
#endif//ENABLE_AUDIO_DENOISE

private:
	HWAVEIN m_hWaveIn;
	PWAVEHDR* m_pWaveHdrs;
    BYTE** m_pBuffers;

    jrtplib::RTPSession* m_pSession;
    AudioPackThread m_outThread;
};

#endif//AUDIO_CAPTURE_THREAD_H