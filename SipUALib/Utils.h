#pragma once

#ifndef UTILS_H
#define UTILS_H


#include "Config.h"

void checkRtpError(int rtperr);

void checkMMError(MMRESULT error, bool bAllowStillPlaying=false);

#define DBG_STR_SIZE 4096
extern char dbg_str[DBG_STR_SIZE];
void DEBUG_INFO(const char* InfoStr);
void DEBUG_INFO(const WCHAR* InfoStr);

#ifdef FORMAT_INFO
#define SPRINTF_S sprintf_s
#else
#define SPRINTF_S
#endif//FORMAT_INFO


std::string WString2String(LPCTSTR pwszStr);
CString     String2WString(LPCSTR pszStr);

#ifdef ENABLE_AUDIO_AEC
// access helper for AEC data
RecPkgQueue*    GetMicQueue();
void            SetMicQueue(RecPkgQueue*);
RecPkgQueue*    GetRefQueue();
void            SetRefQueue(RecPkgQueue*);
RecPkgQueue*    GetOutQueue();
void            SetOutQueue(RecPkgQueue*);

bool            GetUseAEC();
void            SetUseAEC(bool);
int             GetTailFilter();
void            SetTailFilter(int);
#endif//ENABLE_AUDIO_AEC


#endif//UTILS_H
