#include "StdAfx.h"
#ifdef ENABLE_AUDIO_AEC
#include "EchoCanceller.h"


EchoCanceller::EchoCanceller()
{
    st = NULL;
    den = NULL;
}

EchoCanceller::~EchoCanceller()
{
    ASSERT(NULL == st);
    ASSERT(NULL == den);
}

void EchoCanceller::Init()
{
    int sampleRate = 8000;
    int nTailFilter = ::GetTailFilter();
//#ifdef PRINT_AEC_INFO
    SPRINTF_S(dbg_str,
        "AEC Init: TailFilter: %d"
        , nTailFilter);
    DEBUG_INFO(dbg_str);
//#endif//PRINT_AEC_INFO

    st = speex_echo_state_init(128, nTailFilter);
    den = speex_preprocess_state_init(128, sampleRate);
    speex_echo_ctl(st, SPEEX_ECHO_SET_SAMPLING_RATE, &sampleRate);
    speex_preprocess_ctl(den, SPEEX_PREPROCESS_SET_ECHO_STATE, st);
}

void EchoCanceller::DoAEC(BYTE* ref_buf, BYTE* echo_buf, BYTE* e_buf)
{
    speex_echo_cancellation(st, (spx_int16_t*)ref_buf, (spx_int16_t*)echo_buf, (spx_int16_t*)e_buf);
    speex_preprocess_run(den, (spx_int16_t*)e_buf);
}

void EchoCanceller::Uninit()
{
    speex_echo_state_destroy(st);
    st = NULL;
    speex_preprocess_state_destroy(den);
    den = NULL;
}

#endif//ENABLE_AUDIO_AEC