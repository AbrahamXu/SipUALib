#ifdef ENABLE_AUDIO_AEC

#pragma once

#ifndef ECHO_CANCELLER_H
#define ECHO_CANCELLER_H


#include "speex/speex_echo.h"
#include "speex/speex_preprocess.h"

class EchoCanceller
{
public:
    EchoCanceller(void);
    virtual ~EchoCanceller(void);

    void Init();
    void DoAEC(BYTE* ref_buf, BYTE* echo_buf, BYTE* e_buf);
    void Uninit();

private:
    SpeexEchoState *st;
    SpeexPreprocessState *den;
};

#endif//ECHO_CANCELLER_H

#endif//ENABLE_AUDIO_AEC