#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include <jrtplib3/rtperrors.h>

void checkRtpError(int rtperr)
{
    if (rtperr)
    {
        SPRINTF_S(dbg_str, "Rtp Error:\n%s",
            jrtplib::RTPGetErrorString(rtperr)
            );
        DEBUG_INFO(dbg_str);

        DebugBreak();
    }
}

void checkMMError(MMRESULT error, bool bAllowStillPlaying)
{
    if (error != MMSYSERR_NOERROR)
    {
        if (!bAllowStillPlaying
            || (bAllowStillPlaying && error != WAVERR_STILLPLAYING))
        {
            TCHAR err_msg[256] = {0};
            waveInGetErrorText(error, err_msg, 255);

            SPRINTF_S(dbg_str, "MMERROR:\n%s",
                err_msg
                );
            DEBUG_INFO(dbg_str);

            DebugBreak();
        }
    }
}

std::string WString2String(LPCTSTR pwszStr)
{
    char szTemp[256] = {0};
    DWORD dwSize = 256;

    dwSize = ::WideCharToMultiByte(CP_ACP, NULL, pwszStr, -1, szTemp, dwSize, NULL, FALSE);
    ASSERT(dwSize > 0);

    return std::string(szTemp);
}

CString String2WString(LPCSTR pszStr)
{
    TCHAR szTemp[256] = {0};
    DWORD dwSize = 256;
    ::MultiByteToWideChar (CP_ACP, 0, pszStr, -1, szTemp, dwSize);
    return CString(szTemp);
}

extern char dbg_str[DBG_STR_SIZE] = {0};

#ifdef LOG_DBG_INFO
class MyLog {
public:
    MyLog() {

        // check map components ready
        CString strModulePath;
        TCHAR* szModuleFileName = strModulePath.GetBuffer(MAX_PATH);
        ::GetModuleFileName(NULL, szModuleFileName, MAX_PATH);
        strModulePath.ReleaseBuffer();
        strModulePath.MakeLower();
        strModulePath.Replace(L"\\", L"/");

        CString strLogFilePath;
        strLogFilePath = strModulePath.Mid(0, 1 + strModulePath.ReverseFind(L'/'));
        strLogFilePath += L"SipLog.txt";

        fp = fopen(WString2String(strLogFilePath).c_str(), "a");
        ASSERT(fp);
        if (fp)
        {
            // log header
            void FileLog(const char* szText);
            FileLog("================ File Logging Started ================");
        }

    }
    ~MyLog() {
        // log tail
        void FileLog(const char* szText);
        FileLog("================ File Logging Ended ================");

        fflush(fp);
        fclose(fp);
        fp = NULL;
    }
    void Log(const char* szText)
    {
        if (fp)
        {
            fwrite(szText, strlen(szText), 1, fp);
        }
    }
    void Flush()
    {
        fflush(fp);
    }
    FILE* fp;
};

void FileLog(const char* szText)
{
    static MyLog log;

    static char szHeader[64] = {0};
    time_t rawtime;
    time ( &rawtime );
    sprintf_s (szHeader, "\r\n%s[Thread id: %d]    "
        , ctime(&rawtime)
        , ::GetCurrentThreadId());
    log.Log(szHeader);
    log.Log(szText);
    log.Flush();
}
#endif//LOG_DBG_INFO

void DEBUG_INFO(const char* InfoStr)
{
#ifdef PRINT_DBG_INFO
    std::cout << InfoStr << std::endl;
    ::OutputDebugString(L"\n");
    ::OutputDebugString(String2WString(InfoStr));
#endif//PRINT_DBG_INFO

#ifdef LOG_DBG_INFO
    FileLog(InfoStr);
#endif//LOG_DBG_INFO
}

void DEBUG_INFO(const WCHAR* InfoStr)
{
#ifdef PRINT_DBG_INFO
    std::cout << InfoStr << std::endl;
    ::OutputDebugString(L"\n");
    ::OutputDebugString(InfoStr);
#endif//PRINT_DBG_INFO

#ifdef LOG_DBG_INFO
    FileLog(WString2String(InfoStr).c_str());
#endif//LOG_DBG_INFO
}


#ifdef ENABLE_AUDIO_AEC
RecPkgQueue* g_pMic = NULL;
RecPkgQueue* g_pRef = NULL;
RecPkgQueue* g_pOut = NULL;

bool g_bUseAEC = false;
int g_nTailFilter = 1024;
// access helper for AEC data
RecPkgQueue* GetMicQueue()
{
    return g_pMic;
}

void SetMicQueue(RecPkgQueue* q)
{
    g_pMic = q;
}

RecPkgQueue* GetRefQueue()
{
    return g_pRef;
}
void SetRefQueue(RecPkgQueue* q)
{
    g_pRef = q;
}
RecPkgQueue* GetOutQueue()
{
    return g_pOut;
}
void SetOutQueue(RecPkgQueue* q)
{
    g_pOut = q;
}

bool GetUseAEC()
{
    return g_bUseAEC;
}
void SetUseAEC(bool b)
{
    g_bUseAEC = b;
}

int GetTailFilter()
{
    return g_nTailFilter;
}
void SetTailFilter(int tail)
{
    g_nTailFilter = tail;
}

#endif//ENABLE_AUDIO_AEC
