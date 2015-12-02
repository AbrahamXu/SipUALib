#include "StdAfx.h"
#include "SipClient.h"

#include "jrtplib3/rtpudpv4transmitter.h"
#include "jrtplib3/rtpipv4address.h"
#include "jrtplib3/rtpsessionparams.h"
#include "SipSettings.h"


using namespace jrtplib;

SipClient::SipClient()
{
    DEBUG_INFO("SipUADemo::SipUADemo...");

    m_bCaptureAudio = false;
    m_bCaptureVideo = false;
    m_wndPreview = NULL;
    m_wndRemote = NULL;

    mListener = new Listener();
    m_bEnableEvents = true; // default true to enable events

    DEBUG_INFO(L"SipUADemo::SipUADemo DONE");
}

SipClient::~SipClient()
{
    DEBUG_INFO("SipUADemo::~SipUADemo...");
    delete mListener;
    mListener = NULL;
    DEBUG_INFO(L"SipUADemo::~SipUADemo DONE");
}

bool SipClient::InitSettings()
{
    // Load settings
    SipSettings cfg(SipSettings::GetCfgFilePath());
    if (!cfg.Load())
        return false;

    TCHAR szTemp[32] = {0};
    DWORD dwSize = 32;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetUserID().c_str(), -1, szTemp, dwSize);
    m_sUserId = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetUserPwd().c_str(), -1, szTemp, dwSize);
    m_sUserPwd = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetProxyIP().c_str(), -1, szTemp, dwSize);
    m_sProxyIp = szTemp;

    _itow_s(cfg.GetProxyPort(), szTemp, 10);
    m_sProxyPort = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetCalleeID().c_str(), -1, szTemp, dwSize);
    m_sCalleeId = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetAudioReceivePort().c_str(), -1, szTemp, dwSize);
    m_sAudioReceivePort = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetAudioSendPort().c_str(), -1, szTemp, dwSize);
    m_sAudioSendPort = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetVideoReceivePort().c_str(), -1, szTemp, dwSize);
    m_sVideoReceivePort = szTemp;

    ::MultiByteToWideChar (CP_ACP, 0, cfg.GetVideoSendPort().c_str(), -1, szTemp, dwSize);
    m_sVideoSendPort = szTemp;

    if ( !super::Init("Sip-client-demo") )
        return false;
    //SetVideoWindow(hWndPreview, hWndRemote);
    return true;
}

void SipClient::SetUserInfo(CString userid, CString passwd)
{
    m_sUserId = userid;
    m_sUserPwd = passwd;

    SPRINTF_S(dbg_str,
        "UserInfo updated: [%s] [%s]"
        , WString2String(m_sUserId).c_str()
        , WString2String(m_sUserPwd).c_str());
    DEBUG_INFO(dbg_str);
}

void SipClient::SetCallee(CString calleeid)
{
    m_sCalleeId = calleeid;

    SPRINTF_S(dbg_str,
        "Callee updated: [%s]"
        , WString2String(m_sCalleeId).c_str());
    DEBUG_INFO(dbg_str);
}

CString SipClient::GetCallee()
{
    return m_sCalleeId;
}

CString SipClient::GetCaller()
{
    return m_sCallerId;
}

void SipClient::Login()
{
    super::Register(
        WString2String(m_sUserId).c_str(),
        WString2String(m_sUserPwd).c_str(),
        WString2String(m_sProxyIp).c_str(),
        WString2String(m_sProxyPort).c_str()
        );
}

void SipClient::Logout()
{
    super::Unregister();
}

void SipClient::Dial()
{
    VideoCapture cap;
    if (cap.EnumDevices() <= 0)
    {
        m_sVideoSendPort.Empty(); // disable video send
    }

    super::Invite(
        WString2String(m_sCalleeId).c_str(),
        WString2String(m_sProxyIp).c_str(),
        WString2String(m_sProxyPort).c_str(),
        WString2String(m_sAudioReceivePort).c_str(),
        WString2String(m_sAudioSendPort).c_str(),
        WString2String(m_sVideoReceivePort).c_str(),
        WString2String(m_sVideoSendPort).c_str(),
        "meeting"
        );

    SPRINTF_S(dbg_str, "Local Audio Port: %s",
        super::GetAudioReceivePort().c_str());
    DEBUG_INFO(dbg_str);
}

void SipClient::Answer()
{
    VideoCapture cap;
    if (cap.EnumDevices() <= 0)
    {
        m_sVideoSendPort.Empty(); // disable video send
    }

    super::Answer(
        WString2String(m_sAudioReceivePort).c_str(),
        WString2String(m_sAudioSendPort).c_str(),
        WString2String(m_sVideoReceivePort).c_str(),
        WString2String(m_sVideoSendPort).c_str()
        );
}

void SipClient::Hang()
{
    DEBUG_INFO("SipUADemo::Hang...");
    super::Hang();
    DEBUG_INFO("SipUADemo::Hang DONE");
}

void SipClient::EnableEvents(bool enable)
{
    m_bEnableEvents = enable;
}

bool SipClient::EnableEvents() const
{
    return m_bEnableEvents;
}


void SipClient::OnRegistered()
{
    if ( IsWorking() )
    {
        DEBUG_INFO("SipUADemo::OnRegistered...");
        if (this->EnableEvents())
            mListener->Callback(SIPUA_REGISTERED);
        DEBUG_INFO("SipUADemo::OnRegistered DONE");
    }
}

void SipClient::OnUnregistered()
{
    if ( IsWorking() )
    {
        DEBUG_INFO("SipUADemo::OnUnregistered...");
        if (this->EnableEvents())
            mListener->Callback(SIPUA_UNREGISTERED);
        DEBUG_INFO("SipUADemo::OnUnregistered DONE");
    }
}

void SipClient::OnRinging()
{
    if ( IsWorking() )
    {
        m_sCallerId = String2WString(SipUA::m_strCaller.c_str());

        DEBUG_INFO("SipUADemo::OnRinging...");
        if (this->EnableEvents())
            mListener->Callback(SIPUA_RINGING);
        DEBUG_INFO("SipUADemo::OnRinging DONE");
    }
}

void SipClient::OnRingback()
{
    if ( IsWorking() )
    {
        DEBUG_INFO("SipUADemo::OnRingback...");
        if (this->EnableEvents())
            mListener->Callback(SIPUA_RINGBACK);
        DEBUG_INFO("SipUADemo::OnRingback DONE");
    }
}

void SipClient::OnCallStarted()
{
    if ( IsWorking() )
    {
        DEBUG_INFO("SipUADemo::OnCallStarted...");

#ifdef WIN32
        WSADATA dat;
        int iWSRet = WSAStartup(MAKEWORD(2,2),&dat);
        ASSERT(iWSRet == 0);
#endif // WIN32

        {
            VideoCapture cap;
            if (cap.EnumDevices(false) > 0)
                m_bCaptureAudio = true;

            VIDEOSAMPLEINFO vsiLocal;
            VIDEOFORMATINFO vfiRemote;
            int nVideoDevices = cap.EnumDevices();
            HRESULT hr = E_FAIL;
            if (nVideoDevices > 0)
                hr = cap.GetPreviewInfo(nVideoDevices-1, vsiLocal);

            SPRINTF_S(dbg_str,
                "Video device detecting: count:%d, HRES:%X"
                , nVideoDevices
                , hr);
            DEBUG_INFO(dbg_str);

            //if ( cap.EnumDevices() > 0
            //    && SUCCEEDED(cap.GetPreviewInfo(cap.EnumDevices()-1, vsiLocal)) )
            if ( SUCCEEDED(hr) )
            {
                m_bCaptureVideo = true;

                setVideoSampleInfo(vsiLocal);
                //TODO: support codecoder selection (for h264, etc.)
                vfiRemote = VIDEOFORMATINFO(
                    CODEC_FORMAT,
                    H264_WIDTH,
                    H264_HEIGHT,
                    vsiLocal.m_AvgTimePerFrame);
                setRemoteVideoFormatInfo(vfiRemote);
            }
            else
            {
                //TODO: support codecoder selection (for h264, etc.)
                vfiRemote = VIDEOFORMATINFO(
                    CODEC_FORMAT,
                    H264_WIDTH,
                    H264_HEIGHT,
                    FRAMES_PER_SECOND);
                setRemoteVideoFormatInfo(vfiRemote);
            }
        }

#ifdef RTP_AUDIO_SENDRECV
        //Sender
        {
            if (m_bCaptureAudio)
            {
                SPRINTF_S(dbg_str,
                    "Send Audio to [IP]=%s, [Port]=%s"
                    //", [BasePort]=%s"
                    , GetRemoteAudioIP().c_str()
                    , GetRemoteAudioPort().c_str()
                    //, GetAudioSendPort().c_str()
                    );
                DEBUG_INFO(dbg_str);

                uint16_t portbase,destport;
                uint32_t destip;
                std::string ipstr;
                int status;

                //ASSERT(GetAudioSendPort().length() > 0);
                portbase = 6666 + rand() % 6666;//atoi(GetAudioSendPort().c_str());
                if (portbase % 2 == 1)
                    portbase += 1;

                // destination IP address
                ipstr = GetRemoteAudioIP();
                ASSERT(ipstr.length() > 0);
                destip = inet_addr(ipstr.c_str());
                if (destip == INADDR_NONE)
                {
                    DEBUG_INFO("Bad IP address specified");
                    DebugBreak();
                }

                // The inet_addr function returns a value in network byte order, but
                // we need the IP address in host byte order, so we use a call to ntohl
                destip = ntohl(destip);

                // destination port
                ASSERT(GetRemoteAudioPort().length() > 0);
                destport = atoi(GetRemoteAudioPort().c_str());

                // Now, we'll create a RTP session, set the destination, send some
                // packets and poll for incoming data.

                RTPUDPv4TransmissionParams transparams;
                RTPSessionParams sessparams;

                // IMPORTANT: The local timestamp unit MUST be set, otherwise
                //            RTCP Sender Report info will be calculated wrong
                // In this case, we'll be sending 10 samples each second, so we'll
                // put the timestamp unit to (1.0/10.0)
                //sessparams.SetOwnTimestampUnit(1.0/8000.0);		
                sessparams.SetOwnTimestampUnit(TIMESTAMP_INC_UNIT);

                //sessparams.SetAcceptOwnPackets(true);
                transparams.SetPortbase(portbase);
                m_pAudioSender = std::shared_ptr<AudioSendSession>( new AudioSendSession() );
                status = m_pAudioSender->Create(sessparams,&transparams);	
                checkRtpError(status);

                RTPIPv4Address addr(destip,destport);

                status = m_pAudioSender->AddDestination(addr);
                checkRtpError(status);

#ifdef ENABLE_AUDIO_AEC
                ::SetMicQueue(m_pAudioSender->getMicQueue());
                ::SetOutQueue(m_pAudioSender->getPackageQueue());
                if (::GetUseAEC())
                {
                    ::SetRefQueue(m_pAudioSender->getRefQueue());
                }
                else
                {
                    ::SetRefQueue(NULL);
                }
#endif//ENABLE_AUDIO_AEC

                m_pAudioSender->Start();
            }
            else
            {
#ifdef ENABLE_AUDIO_AEC
                if (::GetUseAEC())
                {
                    ::SetUseAEC(false);
                    DEBUG_INFO(L"AEC DIS-ABLED: no device for recording.");
                }
                ::SetMicQueue(m_pAudioSender->getMicQueue());
                ::SetOutQueue(m_pAudioSender->getPackageQueue());
                ::SetRefQueue(NULL);
#endif//ENABLE_AUDIO_AEC
            }
        }
        //Receiver
        {
            SPRINTF_S(dbg_str, "Receive Audio At [BasePort]=%s",
                GetAudioReceivePort().c_str());
            DEBUG_INFO(dbg_str);

            // Setup receiver session
            uint16_t portbase;
            int status;
            RTPUDPv4TransmissionParams transparams;
            RTPSessionParams sessparams;

            portbase = atoi(GetAudioReceivePort().c_str());
            //SPRINTF_S(dbg_str, "Port Base: %d",
            //    portbase);
            //DEBUG_INFO(dbg_str);

            //TODO:
            portbase = atoi(GetAudioReceivePort().c_str());

            // IMPORTANT: The local timestamp unit MUST be set, otherwise
            //            RTCP Sender Report info will be calculated wrong
            // In this case, we'll be just use 8000 samples per second.
            //sessparams.SetOwnTimestampUnit(1.0/8000.0);		
            sessparams.SetOwnTimestampUnit(TIMESTAMP_INC_UNIT);

            transparams.SetPortbase(portbase);
            m_pAudioReceiver = std::shared_ptr<AudioReceiveSession>( new AudioReceiveSession() );
            status = m_pAudioReceiver->Create(sessparams, &transparams);	
            checkRtpError(status);

            m_pAudioReceiver->Start();
        }
#endif//RTP_AUDIO_SENDRECV


#ifdef RTP_VIDEO_SENDER
        //Sender
        {
            if (m_bCaptureVideo)
            {
                SPRINTF_S(dbg_str,
                    "Send Video to [IP]=%s, [Port]=%s"
                    //", [BasePort]=%s"
                    , GetRemoteVideoIP().c_str()
                    , GetRemoteVideoPort().c_str()
                    //, GetVideoSendPort().c_str()
                    );
                DEBUG_INFO(dbg_str);

                uint16_t portbase,destport;
                uint32_t destip;
                std::string ipstr;
                int status;

                //TODO:
                portbase = 8888 + rand() % 8888;//atoi(GetVideoReceivePort().c_str());
                if (portbase % 2 == 1)
                    portbase += 1;

                // destination IP address
                //ASSERT(GetRemoteIP().length() > 0);
                //TODO:
                //ipstr = "127.0.0.1";//GetRemoteIP();
                //ipstr = "10.148.206.29";
                //ipstr = "10.148.206.93";
                ipstr = GetRemoteVideoIP();
                destip = inet_addr(ipstr.c_str());
                if (destip == INADDR_NONE)
                {
                    DEBUG_INFO("Bad IP address specified");
                    DebugBreak();
                }

                // The inet_addr function returns a value in network byte order, but
                // we need the IP address in host byte order, so we use a call to ntohl
                destip = ntohl(destip);

                // destination port
                //ASSERT(GetRemotePort().length() > 0);
                //TODO:
                destport = atoi(GetRemoteVideoPort().c_str());

                // Now, we'll create a RTP session, set the destination, send some
                // packets and poll for incoming data.

                RTPUDPv4TransmissionParams transparams;
                RTPSessionParams sessparams;

                // IMPORTANT: The local timestamp unit MUST be set, otherwise
                //            RTCP Sender Report info will be calculated wrong
                // In this case, we'll be sending 10 samples each second, so we'll
                // put the timestamp unit to (1.0/10.0)
                //sessparams.SetOwnTimestampUnit(1.0/8000.0);		
                sessparams.SetOwnTimestampUnit(TIMESTAMP_INC_UNIT);

                //sessparams.SetAcceptOwnPackets(true);
                transparams.SetPortbase(portbase);
                m_pVideoSender = std::shared_ptr<VideoSendSession>( new VideoSendSession() );
                status = m_pVideoSender->Create(sessparams,&transparams);
                checkRtpError(status);

                RTPIPv4Address addr(destip,destport);

                status = m_pVideoSender->AddDestination(addr);
                checkRtpError(status);

                //VIDEOSAMPLEINFO vsiLocal;
                //if ( SUCCEEDED(cap.GetPreviewInfo(cap.EnumDevices()-1, vsiLocal)) )
                //{
                //    setVideoSampleInfo(vsiLocal);
                //    //TODO: support codecoder selection (for h264, etc.)
                //    vfiRemote = VIDEOFORMATINFO(
                //        CODEC_FORMAT,
                //        H264_WIDTH,
                //        H264_HEIGHT,
                //        vsiLocal.m_AvgTimePerFrame);
                //    setRemoteVideoFormatInfo(vfiRemote);
                    ASSERT(m_wndPreview != NULL);
                    m_pVideoSender->SetPreviewWindow(m_wndPreview);
                    m_pVideoSender->Start();
                //}
            }
        }
#endif//RTP_VIDEO_SENDER
#ifdef RTP_VIDEO_RECEIVER
        //Receiver
        {
            SPRINTF_S(dbg_str,
                "Receive Video At [BasePort]=%s"
                , GetVideoReceivePort().c_str());
            DEBUG_INFO(dbg_str);

            // Setup receiver session
            uint16_t portbase;
            int status;
            RTPUDPv4TransmissionParams transparams;
            RTPSessionParams sessparams;

            //TODO:
            portbase = atoi(GetVideoReceivePort().c_str());
            //SPRINTF_S(dbg_str, "Port Base: %d",
            //    portbase);
            //DEBUG_INFO(dbg_str);

            // IMPORTANT: The local timestamp unit MUST be set, otherwise
            //            RTCP Sender Report info will be calculated wrong
            // In this case, we'll be just use 8000 samples per second.
            //sessparams.SetOwnTimestampUnit(1.0/8000.0);		
            sessparams.SetOwnTimestampUnit(TIMESTAMP_INC_UNIT);

            transparams.SetPortbase(portbase);
            m_pVideoReceiver = std::shared_ptr<VideoReceiveSession>( new VideoReceiveSession() );
            status = m_pVideoReceiver->Create(sessparams, &transparams);	
            checkRtpError(status);

            //VideoCapture cap;
            //if (cap.EnumDevices() > 0 )
            //{
            //    // set remote video format already
            //}
            //else
            //{
            //    //TODO: support codecoder selection (for h264, etc.)
            //    vfiRemote = VIDEOFORMATINFO(
            //        CODEC_FORMAT,
            //        H264_WIDTH,
            //        H264_HEIGHT,
            //        FRAMES_PER_SECOND);
            //    setRemoteVideoFormatInfo(vfiRemote);
            //}
            ASSERT(m_wndRemote != NULL);
            m_pVideoReceiver->SetRemoteWindow(m_wndRemote);
            m_pVideoReceiver->Start();
        }
#endif//RTP_VIDEO_RECEIVER

        if (this->EnableEvents())
            mListener->Callback(SIPUA_CALLSTARTED);

        DEBUG_INFO("SipUADemo::OnCallStarted DONE");
   }
}

void SipClient::OnCallEnded()
{
    DEBUG_INFO(L"Try SipUADemo::OnCallEnded...");
    if ( IsWorking() )
    {
        DEBUG_INFO(L"SipUADemo::OnCallEnded...");

        if ( m_bCaptureAudio )
        {
            m_pAudioSender->Stop();
            m_pAudioSender->Destroy();
            m_pAudioSender = NULL;
        }

        m_pAudioReceiver->Stop();
        m_pAudioReceiver->Destroy();
        m_pAudioReceiver = NULL;

        if ( m_bCaptureVideo )
        {
            m_pVideoSender->Stop();
            m_pVideoSender->Destroy();
            m_pVideoSender = NULL;
        }

        m_pVideoReceiver->Stop();
        m_pVideoReceiver->Destroy();
        m_pVideoReceiver = NULL;

#ifdef WIN32
        int iWSRet = WSACleanup();
        ASSERT(iWSRet == 0);
#endif // WIN32

        DEBUG_INFO(L"SipUADemo::OnCallEnded DONE");

        if (this->EnableEvents())
            mListener->Callback(SIPUA_CALLENDED);
    }
    else
        DEBUG_INFO(L"SipUADemo::OnCallEnded Not Working");
}

void SipClient::OnCallCancelled()
{
    if ( IsWorking() )
    {
        DEBUG_INFO(L"Try SipUADemo::OnCallCancelled...");
        if (this->EnableEvents())
            mListener->Callback(SIPUA_CALLCANCELLED);
    }
    else
        DEBUG_INFO(L"SipUADemo::OnCallCancelled Not Working");

    DEBUG_INFO(L"Try SipUADemo::OnCallCancelled DONE");
}

void SipClient::OnInited()
{
    DEBUG_INFO(L"Try SipUADemo::OnInited...");
    if (this->EnableEvents())
        mListener->Callback(SIPUA_INITED);
    DEBUG_INFO(L"Try SipUADemo::OnInited DONE");
}

void SipClient::OnUninited()
{
    DEBUG_INFO(L"Try SipUADemo::OnUninited...");
    if (this->EnableEvents())
        mListener->Callback(SIPUA_UNINITED);
    DEBUG_INFO(L"Try SipUADemo::OnUninited DONE");
}


SipClient::Listener::Listener() {
}

SipClient::Listener::~Listener() {
}

void SipClient::Listener::Callback(const int evtType) {
}
