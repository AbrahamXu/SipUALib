#pragma once

#ifndef AUDIO_PLAY_THREAD_H
#define AUDIO_PLAY_THREAD_H


#include "AudioThread.h"
#include "PackageOueue.h"


class AudioPlayThread : public AudioThread
{
    typedef AudioThread super;
	DECLARE_DYNCREATE(AudioPlayThread)
public:
	AudioPlayThread();
    virtual ~AudioPlayThread();

    BOOL CreateThread(PlayPkgQueue* pPkgQueue);

// Operations
public:
    void InitAudio();
    void UninitAudio();

// Overrides
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

    virtual void Start();
    virtual void Stop();
protected:
    virtual void DoJob();

    void onPossibleDelay();

protected:
    afx_msg	void ON_WOMCLOSE(WPARAM wParam, LPARAM lParam);
    afx_msg	void ON_WOMDATA(WPARAM wParam, LPARAM lParam);
    afx_msg	void ON_WOMOPEN(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
    HWAVEOUT m_hWaveOut;
    PWAVEHDR* m_pWaveHdrs;

    PlayPkgQueue* m_pPkgQueue;
    UINT m_nSubPkgPlayed;
};

#endif//AUDIO_PLAY_THREAD_H