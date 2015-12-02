#pragma once

#ifndef SIP_SETTINGS_H
#define SIP_SETTINGS_H


class SipSettings
{
public:
    SipSettings(LPCTSTR lpszCfgFilePath);
    virtual ~SipSettings(void);

public:
    static CString GetCfgFilePath();

    bool Load();

    std::string GetUserID() const;
    std::string GetUserPwd() const;
    std::string GetProxyIP() const;
    uint16_t GetProxyPort() const;
    std::string GetCalleeID() const;
    std::string GetAudioReceivePort() const;
    std::string GetAudioSendPort() const;
    std::string GetVideoReceivePort() const;
    std::string GetVideoSendPort() const;

private:
    CString ParseItem(LPCTSTR szLine, const CString& strItem);

private:
    std::wstring m_strCfgFilePath;

    static const CString sm_strUserID;
    static const CString sm_strUserPwd;
    static const CString sm_strProxyIP;
    static const CString sm_strProxyPort;
    static const CString sm_strCalleeID;
    static const CString sm_strAudioReceivePort;
    static const CString sm_strAudioSendPort;
    static const CString sm_strVideoReceivePort;
    static const CString sm_strVideoSendPort;
#ifdef ENABLE_AUDIO_AEC
    static const CString sm_strUseAEC;
    static const CString sm_strTailFilter;
#endif//ENABLE_AUDIO_AEC

    std::string m_strUserID;
    std::string m_strUserPwd;
    std::string m_strProxyIP;
    uint16_t    m_iProxyPort;
    std::string m_strCalleeID;
    std::string m_strAudioReceivePort;
    std::string m_strAudioSendPort;
    std::string m_strVideoReceivePort;
    std::string m_strVideoSendPort;

#ifdef ENABLE_AUDIO_AEC
    bool        m_bAECEnabled;
    int         m_iTailFilter;
#endif//ENABLE_AUDIO_AEC
};

#endif//SIP_SETTINGS_H