#pragma once
#ifndef SIP_UA_CLIENT_H
#define SIP_UA_CLIENT_H

#include "SipUA.h"
#include "AudioSendSession.h"
#include "AudioReceiveSession.h"
#include "VideoSendSession.h"
#include "VideoReceiveSession.h"

public enum SipEventType {
    SIPUA_REGISTERED = 0
    , SIPUA_RINGING
    , SIPUA_RINGBACK
    , SIPUA_CALLSTARTED
    , SIPUA_CALLENDED
    , SIPUA_CALLCANCELLED
    , SIPUA_UNREGISTERED
    , SIPUA_INITED
    , SIPUA_UNINITED
};

class SipClient : public SipUA
{
    typedef SipUA super;
public:
    SipClient();
    virtual ~SipClient(void);

    bool InitSettings();
    void SetVideoWindow(HWND wndPreview, HWND wndRemote)
    {
        m_wndPreview = wndPreview;
        m_wndRemote = wndRemote;
    }

    void SetUserInfo(CString userid, CString passwd);
    void SetCallee(CString calleeid);
    CString GetCallee();

    CString GetCaller();

    void Login();
    void Logout();
    void Dial();
    void Answer();
    void Hang();

    void EnableEvents(bool enable);
    bool EnableEvents() const;


public:
    class Listener {
    public:
        Listener();
        virtual ~Listener();
        virtual void Callback(const int evtType);
    };
    Listener* mListener;

    void SetListener(Listener* lsn) {
        mListener = lsn;
    }
protected:
    virtual void OnRegistered();
    virtual void OnUnregistered();
    virtual void OnRinging();
    virtual void OnRingback();
    virtual void OnCallCancelled();
    virtual void OnCallStarted();
    virtual void OnCallEnded();
    virtual void OnInited();
    virtual void OnUninited();

private:
    bool m_bEnableEvents;

    std::shared_ptr<AudioSendSession> m_pAudioSender;
    std::shared_ptr<AudioReceiveSession> m_pAudioReceiver;
    std::shared_ptr<VideoSendSession> m_pVideoSender;
    std::shared_ptr<VideoReceiveSession> m_pVideoReceiver;

    HWND m_wndPreview, m_wndRemote;

    bool m_bCaptureAudio;
    bool m_bCaptureVideo;

    // AV info
    CString m_sUserId;
    CString m_sUserPwd;
    CString m_sProxyIp;
    CString m_sProxyPort;
    CString m_sCalleeId;
    CString m_sCallerId; // get from INVITE msg
    CString m_sAudioReceivePort;
    CString m_sAudioSendPort;
    CString m_sVideoReceivePort;
    CString m_sVideoSendPort;
};

#endif//SIP_UA_CLIENT_H