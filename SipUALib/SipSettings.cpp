#include "StdAfx.h"
#include "SipSettings.h"



const CString SipSettings::sm_strUserID("UserID:");
const CString SipSettings::sm_strUserPwd("UserPwd:");
const CString SipSettings::sm_strProxyIP("ProxyIP:");
const CString SipSettings::sm_strProxyPort("ProxyPort:");
const CString SipSettings::sm_strCalleeID("CalleeID:");
const CString SipSettings::sm_strAudioReceivePort("AudioReceivePort:");
const CString SipSettings::sm_strAudioSendPort("AudioSendPort:");
const CString SipSettings::sm_strVideoReceivePort("VideoReceivePort:");
const CString SipSettings::sm_strVideoSendPort("VideoSendPort:");

#ifdef ENABLE_AUDIO_AEC
const CString SipSettings::sm_strUseAEC("UseAEC:");
const CString SipSettings::sm_strTailFilter("TailFilter:");
#endif//ENABLE_AUDIO_AEC

SipSettings::SipSettings(LPCTSTR lpszCfgFilePath)
{
    m_strCfgFilePath = lpszCfgFilePath;
}

SipSettings::~SipSettings(void)
{
}

CString SipSettings::GetCfgFilePath()
{
    CString strModulePath;
    TCHAR* szModuleFileName = strModulePath.GetBuffer(MAX_PATH);
    ::GetModuleFileName(NULL, szModuleFileName, MAX_PATH);
    strModulePath.ReleaseBuffer();
    strModulePath.Replace('\\', '/');
    strModulePath = strModulePath.Mid(0, strModulePath.ReverseFind('/') + 1);
    strModulePath += L"Setting.cfg";

    return strModulePath;
}

bool SipSettings::Load()
{
    CStdioFile file;

    if ( !file.Open(m_strCfgFilePath.c_str(), CFile::modeRead) )
        return false;

    CString strLine, strValue;
    CHAR szTemp[32] = {0};
    int nSize = 0;

    int nCommentPos = -1;
    while (file.ReadString(strLine))
    {
        strLine.Trim();
        nCommentPos = strLine.Find(L"//"); // skip comments
        if (nCommentPos >= 0)
            strLine = strLine.Mid(0, nCommentPos);

        if (strLine.IsEmpty())
            continue;

        strValue = ParseItem((LPCTSTR)strLine, sm_strUserID);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strUserID = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strUserPwd);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strUserPwd = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strProxyIP);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strProxyIP = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strProxyPort);
        if (!strValue.IsEmpty())
        {
            m_iProxyPort = _wtoi(strValue);
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strCalleeID);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strCalleeID = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strAudioReceivePort);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strAudioReceivePort = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strAudioSendPort);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strAudioSendPort = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strVideoReceivePort);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strVideoReceivePort = szTemp;
            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strVideoSendPort);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_strVideoSendPort = szTemp;
            continue;
        }

#ifdef ENABLE_AUDIO_AEC
        strValue = ParseItem((LPCTSTR)strLine, sm_strUseAEC);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_bAECEnabled = atoi(szTemp) ? true : false;
            //TODO: move AEC enable setting to somewhere else
            ::SetUseAEC(m_bAECEnabled);

            DEBUG_INFO( "[Config] UseAEC updated: " + strValue);

            continue;
        }

        strValue = ParseItem((LPCTSTR)strLine, sm_strTailFilter);
        if (!strValue.IsEmpty())
        {
            nSize = ::WideCharToMultiByte(CP_ACP, NULL, strValue, strValue.GetLength(), szTemp, 32, NULL, FALSE);
            ASSERT(nSize > 0);
            szTemp[nSize] = '\0';
            m_iTailFilter = atoi(szTemp);
            //TODO: move AEC tail setting to somewhere else
            ::SetTailFilter(m_iTailFilter);

            DEBUG_INFO( "[Config] TailFilter updated: " + strValue);

            continue;
        }
#endif//ENABLE_AUDIO_AEC
    }
    file.Close();

    return true;
}

CString SipSettings::ParseItem(LPCTSTR szLine, const CString& strItem)
{
    CString strLine(szLine);
    CString strValue;
    int nPos = strLine.Find(strItem);
    if (nPos >= 0)
    {
        strValue = strLine.Mid(nPos + strItem.GetLength());
    }
    return strValue;
}

std::string SipSettings::GetUserID() const
{
    return m_strUserID;
}

std::string SipSettings::GetUserPwd() const
{
    return m_strUserPwd;
}

std::string SipSettings::GetProxyIP() const
{
    return m_strProxyIP;
}

uint16_t SipSettings::GetProxyPort() const
{
    return m_iProxyPort;
}

std::string SipSettings::GetCalleeID() const
{
    return m_strCalleeID;
}

std::string SipSettings::GetAudioReceivePort() const
{
    return m_strAudioReceivePort;
}

std::string SipSettings::GetAudioSendPort() const
{
    return m_strAudioSendPort;
}

std::string SipSettings::GetVideoReceivePort() const
{
    return m_strVideoReceivePort;
}

std::string SipSettings::GetVideoSendPort() const
{
    return m_strVideoSendPort;
}