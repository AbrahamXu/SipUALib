#pragma once

#include <string>
#include <eXosip2/eXosip.h>
#include "Worker.h"

class SipUA : public CWorker
{
public:
    enum UAStatus { // changes to this need also update g_UAState in SipUA.cpp
        STATUS_UNKNOWN = 0,
        STATUS_REGUNREG_TRYING,
        STATUS_REGUNREG_FAIL,
        STATUS_REGISTERED,
        STATUS_UNREGISTERED,
        STATUS_INVITED,
        STATUS_ANSWERED,
        STATUS_RINGED,
        STATUS_CALL_PROCEEDING,
        STATUS_RINGBACKED,
        STATUS_CALLACKED,
        STATUS_CALLCANCELLED,
        STATUS_CALLENDED,
        STATUS_CALL_MESSAGE_ANSWERED,
    };
public:
    SipUA();
    virtual ~SipUA(void);

    virtual bool Init(const char* ua_name, int transport=IPPROTO_UDP, int port=0);
    virtual void Uninit();

    void Register(const char* userid,
                  const char* password,
                  const char* proxy_ip,
                  const char* proxy_port,
                  int expires=SIPUA_REG_EXPIRE);
    void Unregister();
    void Invite(const char* szCalleeId,
                const char* szCalleeHost,
                const char* szCalleePort,
                const char* audio_receive_port,
                const char* audio_send_port,
                const char* video_receive_port,
                const char* video_send_port,
                const char* szSubject);
    void Answer(const char* audio_receive_port,
        const char* audio_send_port,
        const char* video_receive_port,
        const char* video_send_port);

    void Decline();
    void Hang();

    //bool IsWorking() const { return m_bWorking; }

    std::string GetAudioReceivePort() const { return m_strAudioReceivePort; }
    //std::string GetAudioSendPort() const { return m_strAudioSendPort; }
    std::string GetRemoteAudioIP() const { return m_strPeerAudioIp; }
    std::string GetRemoteAudioPort() const { return m_strPeerAudioPort; }

    std::string GetVideoReceivePort() const { return m_strVideoReceivePort; }
    //std::string GetVideoSendPort() const { return m_strVideoSendPort; }
    std::string GetRemoteVideoIP() const { return m_strPeerVideoIp; }
    std::string GetRemoteVideoPort() const { return m_strPeerVideoPort; }

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

    void GetPeerInfo(eXosip_event_t* je);
    //void SipUA::OnCallRinging(eXosip_event_t* je);

protected:
    //static unsigned __stdcall WorkProc(void*);
    static void WorkProc(void*);

    void GetAuthenticateInfo(osip_message_t* msg);
    void PutAuthenticateInfo(osip_message_t* msg);
    void GetProxyAuthenticateInfo(osip_message_t* msg);
    void PutProxyAuthenticateInfo(osip_message_t* msg);

private:
    void Register(int expires);

public:
    UAStatus m_eStatus;

private:
    bool m_bInit;
    //bool m_bWorking;
    int m_nRegisterID;
    int m_nCallID;
    int m_nDialogID;
    int m_nTransactionID;
    std::string m_strUserId;
    std::string m_strPwd;
    std::string m_strProxyIp;
    std::string m_strProxyPort;

    std::string m_strWWWAuthenticate;
    std::string m_strProxyAuthenticate;

    HANDLE m_hEventHandler;

    std::string m_strAudioReceivePort;
    std::string m_strAudioSendPort; // base port to send
    std::string m_strPeerAudioIp;
    std::string m_strPeerAudioPort; // adr to recv
    std::string m_strVideoReceivePort;
    std::string m_strVideoSendPort;
    std::string m_strPeerVideoIp;
    std::string m_strPeerVideoPort;

protected:
    std::string m_strCaller; // get from INVITE msg
    bool m_bRejectIncomingCallIfInConference;
};

