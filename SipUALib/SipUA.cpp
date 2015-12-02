#include "StdAfx.h"
#include "SipUA.h"
#include <string>
#include <winsock2.h>
#include <osip2/osip.h>



using namespace std;

static std::string g_UAState[] = {
    string("STATUS_UNKNOWN"),
    string("STATUS_REGUNREG_TRYING"),
    string("STATUS_REGUNREG_FAIL"),
    string("STATUS_REGISTERED"),
    string("STATUS_UNREGISTERED"),
    string("STATUS_INVITED"),
    string("STATUS_ANSWERED"),
    string("STATUS_RINGED"),
    string("STATUS_CALL_PROCEEDING"),
    string("STATUS_RINGBACKED"),
    string("STATUS_CALLACKED"),
    string("STATUS_CALLCANCELLED"),
    string("STATUS_CALLENDED"),
    string("STATUS_CALL_MESSAGE_ANSWERED"),
};


SipUA::SipUA()
{
    m_bRejectIncomingCallIfInConference = true;
    m_bInit             = false;
    m_nRegisterID       = 0;
    m_nCallID           = 0;
    m_nDialogID         = 0;
    m_nTransactionID    = 0;
    m_hEventHandler = NULL;
    m_eStatus = STATUS_UNKNOWN;
}

SipUA::~SipUA(void)
{
    Uninit();
}

bool SipUA::Init(const char* ua_name, int transport/*=IPPROTO_UDP*/, int port/*=0*/)
{
    int ret = 0;

    ret = eXosip_init();
    if(0 != ret)
    {
        DEBUG_INFO("Couldn't initialize eXosip!\n");
        return false;
    }

    eXosip_set_user_agent(ua_name);

    ret = eXosip_listen_addr(transport, NULL, port, AF_INET, 0);
    if(0 != ret)
    {
        DEBUG_INFO("Fatal Error: couldn't initialize transport layer!\n");
        DebugBreak();
        eXosip_quit ();
        return false;
    }

    unsigned threadID = 0;
    //m_hEventHandler = (HANDLE)::_beginthreadex(NULL, 0, SipUA::WorkProc, this, 0, &threadID);
    m_hEventHandler = (HANDLE)::_beginthread(SipUA::WorkProc, 0, this);

    // Wait for sip agent to get ready
    while (!this->m_bInit)
        ::Sleep(10);

    this->OnInited();

    return true;
}
/*
int MY_eXosip_transaction_find(int tid, osip_transaction_t ** transaction)
{
    int pos = 0;

    *transaction = NULL;
    while (!osip_list_eol(&eXosip.j_transactions, pos)) {
        osip_transaction_t *tr;

        tr = (osip_transaction_t *) osip_list_get(&eXosip.j_transactions, pos);
        if (tr->transactionid == tid) {
            *transaction = tr;
            return OSIP_SUCCESS;
        }
        pos++;
    }
    return OSIP_NOTFOUND;
}
*/

//unsigned int SipUA::WorkProc(void* pArg)
void SipUA::WorkProc(void* pArg)
{
    //TODO:
    //CoInitialize(NULL);

    DWORD dwId = ::GetCurrentThreadId();
    SPRINTF_S(dbg_str,
        "SipUA::WorkProc thread id: %d",
        dwId);
    DEBUG_INFO(dbg_str);

    SipUA* pThis = (SipUA*) pArg;

	DEBUG_INFO("Monitor for ua started!");

	eXosip_event_t*     uac_e   = NULL;
	osip_message_t*     ack     = NULL;

	//eXosip_automatic_action();

    pThis->m_bInit = true;
    pThis->Start();

    while(pThis->IsWorking())
	{
        eXosip_lock ();
        //uac_e = eXosip_event_get();
        uac_e = eXosip_event_wait (0, 50);
        eXosip_unlock ();


        if (uac_e == NULL)
		{
			continue;
		}

        if(NULL != uac_e->response)
		{
			SPRINTF_S(dbg_str, "%d %s", uac_e->response->status_code, uac_e->response->reason_phrase);
			DEBUG_INFO(dbg_str);
		}
		else
		{
			DEBUG_INFO("<No Msg Available.>");
			continue;
		}

        // eXosip_default_action may got OSIP_WRONG_STATE (last transaction not done yet)
        // when retrying REGISTER, so try multiple times here until succeed.
        int ret = OSIP_SUCCESS;

        int nRetryTimes = 0;
        eXosip_lock ();
        do {
            ret = eXosip_default_action(uac_e);
            nRetryTimes++;
            Sleep(50);
        } while(ret < 0 && nRetryTimes < 10);
        eXosip_unlock ();

        if (ret < 0) //!= OSIP_SUCCESS)
        {
            SPRINTF_S(dbg_str, "eXosip_default_action failed with error code: %d", ret);
            DEBUG_INFO(dbg_str);
        }

        switch (uac_e->type)
		{
		case EXOSIP_REGISTRATION_FAILURE:
			{
                SPRINTF_S(dbg_str,
                    "Received: [%s] [status]=%d"
                    "\n    at UA state: %s"
                    , "REGISTRATION_FAILURE"
                    , uac_e->response->status_code
                    , g_UAState[pThis->m_eStatus].c_str());
                DEBUG_INFO(dbg_str);

                // code for testing only
                if (405 == uac_e->response->status_code)
                {
                    DEBUG_INFO("========== Following methods allowed: ");
                    osip_list_t& allowList = uac_e->response->allows;
                    __node_t* nd = NULL;
                    char* alwText;
                    nd=allowList.node;
                    for (int i=0; i < allowList.nb_elt; ++i)
                    {
                        osip_allow_to_str(reinterpret_cast<const osip_content_length_t *>(nd->element), &alwText);
                        nd = nd->next;
                        SPRINTF_S(dbg_str, "allow method[%d]: %s", i, alwText);
                        DEBUG_INFO(dbg_str);
                    }
                    //TODO: any further action?
                }
                else if (401 == uac_e->response->status_code
                    || 407 == uac_e->response->status_code)
                {
                    // fist time register/un-register failure, let eXosip_automatic_action help to retry
                    if (STATUS_UNKNOWN == pThis->m_eStatus
                        || STATUS_REGISTERED == pThis->m_eStatus)
                    {
                        pThis->m_eStatus = STATUS_REGUNREG_TRYING;
                    }
                    else if (STATUS_REGUNREG_TRYING == pThis->m_eStatus)
                    {
                        pThis->m_eStatus = STATUS_REGUNREG_FAIL;
                        DEBUG_INFO("Failed to register/unregister.");
                        ASSERT(0); // failed even with default action, no trying anymore
                    }
                }
			}
			break;
			case EXOSIP_REGISTRATION_SUCCESS:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    at UA state: %s"
                        , "REGISTRATION_SUCCESS"
                        , uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str());
                    DEBUG_INFO(dbg_str);

				    if(0 < uac_e->response->contacts.nb_elt)
				    {
                        pThis->m_eStatus = STATUS_REGISTERED;
                        DEBUG_INFO("Registered successfully.");
                        pThis->OnRegistered();
				    }
				    else
				    {
                        pThis->m_eStatus = STATUS_UNREGISTERED;
                        DEBUG_INFO("Unregistered successfully.");
                        pThis->OnUnregistered();
				    }
                }
				break;

            case EXOSIP_CALL_RINGING:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    [call_id]=%d [dialog_id]=%d"
                        "\n    at UA state: %s"
                        , "CALL_RINGING"
                        , uac_e->response->status_code
                        , uac_e->cid, uac_e->did
                        , g_UAState[pThis->m_eStatus].c_str());

                    pThis->m_nCallID = uac_e->cid;
                    pThis->m_nDialogID = uac_e->did;

                    pThis->m_eStatus = STATUS_RINGBACKED;
                    pThis->OnRingback();
                }
                break;

            case EXOSIP_CALL_INVITE:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    from [displayname]: %s    [url]: %s"
                        "\n    at UA state: %s"
                        "\n    [transaction_id]=%d [call_id]=%d [dialog_id]=%d"
                        "\n    Current cid=%d"
                        , "CALL_INVITE", uac_e->response->status_code
                        , uac_e->request->from->displayname
                        , uac_e->request->from->url->username
                        , g_UAState[pThis->m_eStatus].c_str()
                        , uac_e->tid, uac_e->cid, uac_e->did
                        , pThis->m_nCallID);
                    DEBUG_INFO(dbg_str);

                    if ( pThis->m_bRejectIncomingCallIfInConference
                         && (pThis->m_nCallID != 0
                             && pThis->m_nCallID != uac_e->cid) )
                    {
                        DEBUG_INFO("Sent: [Answer-486] automatically as reply for CALL_INVITE."
                            "We rejected this INVITE as in an conference now.");
                        eXosip_lock ();
                        eXosip_call_send_answer (uac_e->tid, 486, NULL);
                        eXosip_unlock ();
                    }
                    else
                    {
                        // remember who's calling us
                        pThis->m_strCaller = uac_e->request->from->url->username;//uac_e->request->req_uri->username;

                        pThis->m_eStatus = STATUS_INVITED;

                        pThis->GetPeerInfo(uac_e);

                        pThis->m_nTransactionID = uac_e->tid;
                        pThis->m_nCallID = uac_e->cid;
                        pThis->m_nDialogID = uac_e->did;
                        eXosip_lock ();
                        eXosip_call_send_answer (uac_e->tid, 180, NULL);
                        eXosip_unlock ();

                        SPRINTF_S(dbg_str,
                            "Sent: [Answer-180] automatically as reply for %s"
                            "\n    [transaction_id]=%d [call_id]=%d [dialog_id]=%d"
                            "\n    at UA state: %s"
                            , "CALL_INVITE"
                            , uac_e->tid, uac_e->cid, uac_e->did
                            , g_UAState[pThis->m_eStatus].c_str());

                        pThis->m_eStatus = STATUS_RINGED;
                        pThis->OnRinging();
                    }
                }
                break;
            case EXOSIP_CALL_ACK:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    at UA state: %s"
                        "\n    Current cid=%d"
                        , "CALL_ACK", uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str()
                        , pThis->m_nCallID);
                    DEBUG_INFO(dbg_str);

                    pThis->m_eStatus = STATUS_CALLACKED;
                    pThis->OnCallStarted();
                }
                break;
            case EXOSIP_CALL_CLOSED:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    at UA state: %s"
                        "\n    [transaction_id]=%d [call_id]=%d [dialog_id]=%d"
                        "\n    Current cid=%d"
                        , "CALL_CLOSED", uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str()
                        , uac_e->tid, uac_e->cid, uac_e->did
                        , pThis->m_nCallID);
                    DEBUG_INFO(dbg_str);

                    if ( uac_e->response->status_code == 200 )
                    {
                        pThis->m_eStatus = STATUS_CALLENDED;
                        pThis->OnCallEnded();
                        pThis->m_eStatus = STATUS_REGISTERED;
                        pThis->m_nCallID = 0;
                    }
                    else
                    {
                        // skip calls we rejected
                        if (pThis->m_nCallID != uac_e->cid)
                        {
                            DEBUG_INFO("Reject: [CALL_CLOSED] automatically as in an conference now.");
                        }
                        else
                        {
                            pThis->m_eStatus = STATUS_CALLCANCELLED;
                            pThis->OnCallCancelled();
                            pThis->m_nCallID = 0;
                            pThis->m_eStatus = STATUS_REGISTERED;
                        }
                    }
                }
				break;

            case EXOSIP_CALL_PROCEEDING:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    at UA state: %s"
                        "\n    Current cid=%d"
                        , "CALL_PROCEEDING", uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str()
                        , pThis->m_nCallID);
                    DEBUG_INFO(dbg_str);

                    pThis->m_eStatus = STATUS_CALL_PROCEEDING;
                }
                break;

            case EXOSIP_CALL_REQUESTFAILURE:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    at UA state: %s"
                        "\n    Current cid=%d"
                        , "CALL_REQUESTFAILURE", uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str()
                        , pThis->m_nCallID);
                    DEBUG_INFO(dbg_str);

                    // skip calls we rejected
                    if (pThis->m_nCallID != 0
                        && pThis->m_nCallID != uac_e->cid)
                    {
                        DEBUG_INFO("Reject: [CALL_REQUESTFAILURE] automatically as in an conference now.");
                    }
                    else
                    {
                        if (401 == uac_e->response->status_code
                            || 407 == uac_e->response->status_code)
                        {// wait for default action to solve it
                        }
                        else
                        {
                            switch(pThis->m_eStatus)
                            {
                            case STATUS_RINGBACKED:
                                {// hang within a call
                                    //if(487 == uac_e->response->status_code
                                    //    || 408 == uac_e->response->status_code)
                                    {
                                        pThis->m_eStatus = STATUS_CALLCANCELLED;
                                        pThis->OnCallCancelled();
                                        pThis->m_nCallID = 0;
                                        pThis->m_eStatus = STATUS_REGISTERED;
                                    }
                                }
                                break;
                            case STATUS_CALL_PROCEEDING:
                                {// Initializing a call but failed
                                    pThis->m_eStatus = STATUS_CALLCANCELLED;
                                    pThis->OnCallCancelled();
                                    pThis->m_nCallID = 0;
                                    pThis->m_eStatus = STATUS_REGISTERED;
                                }
                                break;
                            }
                        }
                    }
                }
                break;

			case EXOSIP_CALL_ANSWERED:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    at UA state: %s"
                        "\n    [transaction_id]=%d [call_id]=%d [dialog_id]=%d"
                        "\n    Current cid=%d"
                        , "CALL_ANSWERED", uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str()
                        , uac_e->tid, uac_e->cid, uac_e->did
                        , pThis->m_nCallID);
                    DEBUG_INFO(dbg_str);

                    if (200 == uac_e->response->status_code)
                    {
                        pThis->GetPeerInfo(uac_e);

                        pThis->m_nCallID = uac_e->cid;
                        pThis->m_nDialogID = uac_e->did;

                        pThis->m_eStatus = STATUS_ANSWERED;
                        pThis->OnCallStarted();

                        eXosip_call_build_ack(uac_e->did, &ack);
                        eXosip_call_send_ack(uac_e->did, ack);

                        SPRINTF_S(dbg_str,
                            "Sent: [Ack] automatically as reply for %s"
                            "\n    [call_id]=%d [dialog_id]=%d"
                            "\n    at UA state: %s"
                            , "CALL_ANSWERED"
                            , uac_e->cid, uac_e->did
                            , g_UAState[pThis->m_eStatus].c_str());
                    }
                    else
                    {
                        pThis->m_eStatus = STATUS_REGISTERED;
                    }
                }
				break;
            case EXOSIP_CALL_MESSAGE_ANSWERED:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    within call: [call_id]=%d [dialog_id]=%d"
                        "\n    at UA state: %s"
                        , "CALL_MESSAGE_ANSWERED", uac_e->response->status_code
                        , uac_e->cid, uac_e->did
                        , g_UAState[pThis->m_eStatus].c_str());
                    DEBUG_INFO(dbg_str);

                    if ( pThis->m_eStatus == STATUS_ANSWERED
                        || pThis->m_eStatus == STATUS_CALLACKED)
                    { // This case should be a reply for previously sent bye
                        pThis->m_eStatus = STATUS_CALLENDED;
                        pThis->OnCallEnded();
                        pThis->m_eStatus = STATUS_REGISTERED;
                        pThis->m_nCallID = 0;
                    }
                }
                break;
            case EXOSIP_CALL_CANCELLED:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    within call: [call_id]=%d [dialog_id]=%d"
                        "\n    at UA state: %s"
                        , "CALL_CANCELLED", uac_e->response->status_code
                        , uac_e->cid, uac_e->did
                        , g_UAState[pThis->m_eStatus].c_str());
                    DEBUG_INFO(dbg_str);

                    pThis->m_eStatus = STATUS_CALLCANCELLED;
                    pThis->OnCallCancelled();
                    pThis->m_nCallID = 0;
                    pThis->m_eStatus = STATUS_REGISTERED;
                }
                break;

            case EXOSIP_CALL_GLOBALFAILURE:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [%s] [status]=%d"
                        "\n    within call: [call_id]=%d [dialog_id]=%d"
                        "\n    at UA state: %s"
                        , "CALL_GLOBALFAILURE", uac_e->response->status_code
                        , uac_e->cid, uac_e->did
                        , g_UAState[pThis->m_eStatus].c_str());
                    DEBUG_INFO(dbg_str);

                    pThis->m_eStatus = STATUS_CALLCANCELLED;
                    pThis->OnCallCancelled();
                    pThis->m_nCallID = 0;
                    pThis->m_eStatus = STATUS_REGISTERED;
                }
                break;

            default:
                {
                    SPRINTF_S(dbg_str,
                        "Received: [<%d(Unknown Message)>] [status]=%d"
                        "\n    at UA state: %s"
                        , uac_e->type, uac_e->response->status_code
                        , g_UAState[pThis->m_eStatus].c_str());
                    DEBUG_INFO(dbg_str);
                }
				break;
		}

        eXosip_event_free (uac_e);
	}

    DEBUG_INFO("Monitor for ua stopped!");
}

void print_sip_msg(osip_message_t * sip)
{
    size_t msg_len = 0;
    char* szSipInfo = NULL;
    if ( OSIP_SUCCESS != osip_message_to_str(sip, &szSipInfo, &msg_len) )
    {
        DEBUG_INFO("Failed to print sip msg!");
    }
    else
    {
        SPRINTF_S(dbg_str,
            "Received a sip message:\n%s",
            szSipInfo);
        DEBUG_INFO(dbg_str);
        osip_free(szSipInfo);
    }
}

void PrintSDPMsgFields(sdp_message_t* sdp)
{
    SPRINTF_S(dbg_str, "SDP message fields:\n"
        "[o_username]=%s\n"
        "[o_sess_id]=%s\n"
        "[o_nettype]=%s\n"
        "[o_addrtype]=%s\n"
        "[o_addr]=%s\n"
        "[s_name]=%s\n"
        "[i_info]=%s\n"
        "[u_uri]=%s\n"
        "[sdp_connection.c_nettype]=%s\n"
        "[sdp_connection.c_addrtype]=%s\n"
        "[sdp_connection.c_addr]=%s\n"
        "[sdp_connection.c_addr_multicast_ttl]=%s\n"
        "[sdp_connection.c_addr_multicast_int]=%s\n"
        ,
        sdp->o_username,
        sdp->o_sess_id,
        sdp->o_nettype,
        sdp->o_addrtype,
        sdp->o_addr,
        sdp->s_name,
        sdp->i_info,
        sdp->u_uri,
        sdp->c_connection->c_nettype,
        sdp->c_connection->c_addrtype,
        sdp->c_connection->c_addr,
        sdp->c_connection->c_addr_multicast_ttl,
        sdp->c_connection->c_addr_multicast_int
        );
    DEBUG_INFO(dbg_str);
}

void SipUA::GetPeerInfo(eXosip_event_t* je)
{
    sdp_message_t* remote_sdp = eXosip_get_remote_sdp (je->did);
    if (remote_sdp)
    {
        char* szSdpInfo = NULL;

        int ret = sdp_message_to_str(remote_sdp, &szSdpInfo);
        if (0 > ret)
        {
            DEBUG_INFO("Failed to parse sdp message");
        }
        else
        {
            SPRINTF_S(dbg_str, "Received a sdp message:\n%s", szSdpInfo);
            DEBUG_INFO(dbg_str);
            osip_free(szSdpInfo);

            PrintSDPMsgFields(remote_sdp);

            sdp_media_t* audio_media = eXosip_get_audio_media(remote_sdp);
            if (audio_media)
            {
                SPRINTF_S(dbg_str, "Audio media: \n"
                    "[port]=%s", audio_media->m_port);
                    DEBUG_INFO(dbg_str);
                m_strPeerAudioPort = audio_media->m_port;
            }
            sdp_connection_t* audio_conn = eXosip_get_audio_connection(remote_sdp);
            if (audio_conn)
            {
                SPRINTF_S(dbg_str, "Audio connection: \n"
                    "[ip]=%s", audio_conn->c_addr);
                DEBUG_INFO(dbg_str);
                m_strPeerAudioIp = audio_conn->c_addr;
            }

            sdp_media_t* video_media = eXosip_get_video_media(remote_sdp);
            if (video_media)
            {
                SPRINTF_S(dbg_str, "video media: \n"
                    "[port]=%s", video_media->m_port);
                DEBUG_INFO(dbg_str);
                m_strPeerVideoPort = video_media->m_port;
            }
            sdp_connection_t* video_conn = eXosip_get_video_connection(remote_sdp);
            if (video_conn)
            {
                SPRINTF_S(dbg_str, "Video connection: \n"
                    "[ip]=%s", video_conn->c_addr);
                DEBUG_INFO(dbg_str);
                m_strPeerVideoIp = video_conn->c_addr;
            }

            //TODO: parse attributes
            //int pos = 0;
            //while (!osip_list_eol ( &(remote_sdp->a_attributes), pos))
            //{
            //    sdp_attribute_t *at;

            //    at = (sdp_attribute_t *) osip_list_get ( &remote_sdp->a_attributes, pos);
            //    cout << "\n\t" << at->a_att_field 
            //        << " : " << at->a_att_value << endl;

            //    pos ++;
            //}

        }
    }
    else
    {
        DEBUG_INFO("No sdp message");
        DebugBreak();
    }
}

void SipUA::GetAuthenticateInfo(osip_message_t *reg) 
{
    if (m_strWWWAuthenticate.length() > 0)
        return;

    osip_www_authenticate_t* auth;
    char* szAuth = NULL;

    int ret = osip_message_get_www_authenticate(reg, 0, &auth);
    if (0 > ret)
    {
        DEBUG_INFO("Failed to get WWW-Authenticate header.");
        return;
    }

    ret = osip_www_authenticate_to_str(auth, &szAuth);
    if (0 > ret)
    {
        DEBUG_INFO("Failed to get a string representation of an WWW-Authenticate element.");
        return;
    }

    m_strWWWAuthenticate = szAuth;
    osip_free(szAuth);
}

void SipUA::PutAuthenticateInfo(osip_message_t* msg)
{
    if (m_strWWWAuthenticate.length())
    {
        osip_message_replace_header(msg, WWW_AUTHENTICATE, m_strWWWAuthenticate.c_str());
    }
}

void SipUA::GetProxyAuthenticateInfo(osip_message_t *reg) 
{
    if (m_strProxyAuthenticate.length() > 0)
    {
        DEBUG_INFO("Proxy-Authenticate already existing while trying to get Proxy Authenticate header.");
        return;
    }

    osip_proxy_authenticate_t* auth;
    char* szAuth = NULL;

    int ret = osip_message_get_proxy_authenticate(reg, 0, &auth);
    if (0 > ret)
    {
        DEBUG_INFO("Failed to get Proxy Authenticate header.");
        return;
    }

    ret = osip_proxy_authenticate_to_str(auth, &szAuth);
    if (0 > ret)
    {
        DEBUG_INFO("Failed to get a string representation of an Authorization element.");
        return;
    }

    m_strProxyAuthenticate = szAuth;
    osip_free(szAuth);
}

void SipUA::PutProxyAuthenticateInfo(osip_message_t* msg)
{
    if (m_strProxyAuthenticate.length())
    {
        osip_message_replace_header(msg, PROXY_AUTHENTICATE, m_strProxyAuthenticate.c_str());
    }
}

void SipUA::Register(const char* userid,
                     const char* password,
                     const char* proxy_ip,
                     const char* proxy_port,
                     int expires)
{
    m_strUserId = userid;
    m_strPwd = password;
    m_strProxyIp = proxy_ip;
    m_strProxyPort = proxy_port;

    Register(expires);
}

void SipUA::Register(int expires)
{
    osip_message_t *reg;
    int ret;

    char sip_uri[100] = {0};
    char proxy[100] = {0};

    sprintf_s(sip_uri, "%s <sip:%s@%s:%s>", m_strUserId.c_str(), m_strUserId.c_str(), m_strProxyIp.c_str(), m_strProxyPort.c_str());
    sprintf_s(proxy, "sip:%s:%s", m_strProxyIp.c_str(), m_strProxyPort.c_str());

    if (m_nRegisterID == 0)
    {
        eXosip_clear_authentication_info();
        eXosip_add_authentication_info(m_strUserId.c_str(), m_strUserId.c_str(), m_strPwd.c_str(), "MD5", NULL);
    }

    if (m_nRegisterID == 0)
    {
        eXosip_lock();
        m_nRegisterID = eXosip_register_build_initial_register(sip_uri, proxy, NULL, expires, &reg);
        eXosip_unlock();

        if(0 > m_nRegisterID)
        {
            DEBUG_INFO("Register init Failed!");
            osip_message_free(reg);
        }
    }
    else
    {
        eXosip_lock();
        ret = eXosip_register_build_register(m_nRegisterID, expires, &reg);
        eXosip_unlock();

        if(0 > ret)
        {
            SPRINTF_S(dbg_str,
                "Building register failed! Error Code: [%d]."
                , ret);
            DEBUG_INFO(dbg_str);
            osip_message_free(reg);
        }
    }

    PutAuthenticateInfo(reg);
    PutProxyAuthenticateInfo(reg);

    eXosip_lock ();
    ret = eXosip_register_send_register(m_nRegisterID, reg);
    eXosip_unlock ();
    if(0 != ret)
    {
        SPRINTF_S(dbg_str,
            "Sending register failed! Error Code: [%d]."
            , ret);
        DEBUG_INFO(dbg_str);
    }
    //else
    //    DEBUG_INFO("Sending register succeed.");
}

void SipUA::Invite(const char* szCalleeId,
                   const char* szCalleeHost,
                   const char* szCalleePort,
                   const char* audio_receive_port,
                   const char* audio_send_port,
                   const char* video_receive_port,
                   const char* video_send_port,
                   const char* szSubject)
{
    osip_message_t *inv = NULL;

    m_strAudioReceivePort = audio_receive_port;
    m_strAudioSendPort = audio_send_port;
    m_strVideoReceivePort = video_receive_port;
    m_strVideoSendPort = video_send_port;

    char caller_uri[100] = {0};
    char callee_uri[100] = {0};

    sprintf_s(caller_uri, "%s <sip:%s@%s:%s>", m_strUserId.c_str(), m_strUserId.c_str(), m_strProxyIp.c_str(), m_strProxyPort.c_str());
    sprintf_s(callee_uri, "%s <sip:%s@%s:%s>", szCalleeId, szCalleeId, szCalleeHost, szCalleePort);

    char szTemp[4096];

    int ret = eXosip_call_build_initial_invite(&inv, callee_uri, caller_uri, NULL, szSubject);
    if(OSIP_SUCCESS != ret)
    {
        DEBUG_INFO("Initial INVITE failed!");
        osip_message_free(inv);
    }

    char localip[128];

    eXosip_guess_localip (AF_INET, localip, 128);
    // Transport Information in SDP media (-m) field, by default,
    // SHOULD be the remote address and remote port to which data is sent.
    //
    // session header
    std::string strSDPMsg;
    _snprintf_s(szTemp, 4096,
        "v=0\r\n"
        "o=%s 0 0 IN IP4 %s\r\n"
        "s=conversation\r\n"
        "c=IN IP4 %s\r\n"
        "t=0 0\r\n"
        , m_strUserId.c_str()
        , localip
        , localip
        );
    strSDPMsg += szTemp;

    // audio
    if (m_strAudioReceivePort.length() > 0)
    {
        _snprintf_s(szTemp, 4096,
            "m=audio %s RTP/AVP 0 8\r\n"
            "a=rtpmap:0 PCMU/8000\r\n"
            "a=rtpmap:8 PCMA/8000\r\n"
            , m_strAudioReceivePort.c_str()
            );
        strSDPMsg += szTemp;

        std::string strTransType;
        if (m_strAudioSendPort.length() > 0)
            strTransType = "a=sendrecv\r\n";
        else
            strTransType = "a=recvonly\r\n";
        strSDPMsg += strTransType;
    }

    // video
    if (m_strVideoReceivePort.length() > 0)
    {
        _snprintf_s(szTemp, 4096,
            "m=video %s RTP/AVP 99\r\n"
            "a=rtpmap:96 H264/90000\r\n"
            , m_strVideoReceivePort.c_str()
            );
        strSDPMsg += szTemp;

        std::string strTransType;
        if (m_strVideoSendPort.length() > 0)
            strTransType = "a=sendrecv\r\n";
        else
            strTransType = "a=recvonly\r\n";
        strSDPMsg += strTransType;
    }

    osip_message_set_body(inv, strSDPMsg.c_str(), strSDPMsg.length());
    osip_message_set_content_type(inv, "application/sdp");

    PutAuthenticateInfo(inv);
    PutProxyAuthenticateInfo(inv);

    eXosip_lock ();
    eXosip_call_send_initial_invite(inv);
    eXosip_unlock ();
}

void SipUA::Answer(const char* audio_receive_port,
                   const char* audio_send_port,
                   const char* video_receive_port,
                   const char* video_send_port)
{
    m_strAudioReceivePort = audio_receive_port;
    m_strAudioSendPort = audio_send_port;
    m_strVideoReceivePort = video_receive_port;
    m_strVideoSendPort = video_send_port;

    osip_message_t *ans = NULL;
    eXosip_lock ();
    int ret = eXosip_call_build_answer (m_nTransactionID, 200, &ans);
    eXosip_unlock ();

    if(0 != ret)
    {
        osip_message_free(ans);
        DEBUG_INFO("Build answer failed !");
        eXosip_lock ();
        eXosip_call_send_answer(m_nTransactionID, 400, NULL);
        eXosip_unlock ();
    }
    else
    {
        char szTemp[4096];
        char localip[128];
        eXosip_guess_localip (AF_INET, localip, 128);
        std::string strSDPMsg;

        // session header
        _snprintf_s(szTemp, 4096,
            "v=0\r\n"
            "o=%s 0 0 IN IP4 %s\r\n"
            "s=conversation\r\n"
            "c=IN IP4 %s\r\n"
            "t=0 0\r\n"
            , m_strUserId.c_str()
            , localip
            , localip
            );
        strSDPMsg += szTemp;

        // audio
        if (m_strAudioReceivePort.length() > 0)
        {
            _snprintf_s(szTemp, 4096,
                "m=audio %s RTP/AVP 0 8\r\n"
                "a=rtpmap:0 PCMU/8000\r\n"
                "a=rtpmap:8 PCMA/8000\r\n"
                , m_strAudioReceivePort.c_str()
                );
            strSDPMsg += szTemp;

            std::string strTransType;
            if (m_strAudioSendPort.length() > 0)
                strTransType = "a=sendrecv\r\n";
            else
                strTransType = "a=recvonly\r\n";
            strSDPMsg += strTransType;
        }

        // video
        if (m_strVideoReceivePort.length() > 0)
        {
            _snprintf_s(szTemp, 4096,
                "m=video %s RTP/AVP 99\r\n"
                "a=rtpmap:96 H264/90000\r\n"
                , m_strVideoReceivePort.c_str()
                );
            strSDPMsg += szTemp;

            std::string strTransType;
            if (m_strVideoSendPort.length() > 0)
                strTransType = "a=sendrecv\r\n";
            else
                strTransType = "a=recvonly\r\n";
            strSDPMsg += strTransType;
        }

        osip_message_set_body(ans, strSDPMsg.c_str(), strSDPMsg.length());
        osip_message_set_content_type(ans, "application/sdp");

        PutAuthenticateInfo(ans);
        PutProxyAuthenticateInfo(ans);

        eXosip_lock ();
        eXosip_call_send_answer (m_nTransactionID, 200, ans);
        eXosip_unlock ();
    }
}


void SipUA::Decline()
{
    osip_message_t *ans = NULL;
    eXosip_lock ();
    int ret = eXosip_call_build_answer (m_nTransactionID, 603, &ans);
    eXosip_unlock ();

    if(0 != ret)
    {
        osip_message_free(ans);
        DEBUG_INFO("Build decline failed !");
    }
    else
    {
        PutAuthenticateInfo(ans);
        PutProxyAuthenticateInfo(ans);

        eXosip_lock ();
        ret = eXosip_call_send_answer (m_nTransactionID, 603, ans);
        eXosip_unlock ();

        if(OSIP_SUCCESS != ret)
        {
            DEBUG_INFO("Send decline failed!");
        }
    }
}

void SipUA::Hang()
{
    DEBUG_INFO("SipUA::Hang...");

    int ret;
    ret = eXosip_call_terminate(m_nCallID, m_nDialogID);
    if(OSIP_SUCCESS != ret)
    {
        DEBUG_INFO("Call terminate failed!");
    }

    DEBUG_INFO("SipUA::Hang DONE");
}

void SipUA::Unregister()
{
    Register(0);
    m_nRegisterID = 0;
}

void SipUA::Uninit()
{
    if (m_bInit)
    {
        DEBUG_INFO("SipUA::Uninit ....");

        DWORD dwId = ::GetCurrentThreadId();
        SPRINTF_S(dbg_str,
            "SipUA::Uninit thread id: %d",
            dwId);
        DEBUG_INFO(dbg_str);

        // caller should logout before un-initialization

        // wait for STATUS_REGUNREG_TRYING complete its job
        // as there might be some REGISTER/UNREGISTER msg in processing.
        int nTryLogoutTimes = 0;
        while (STATUS_UNREGISTERED != this->m_eStatus
            && nTryLogoutTimes < 40)
        {// test STATUS_UNREGISTERED here to make sure we peacefully logout.
            SPRINTF_S(dbg_str,
                "SipUA::Uninit waiting STATUS_REGUNREG_TRYING to complete..."
                "\n    at UA state: %s"
                , g_UAState[this->m_eStatus].c_str());
            DEBUG_INFO(dbg_str);
            ::Sleep(50);
            ++nTryLogoutTimes;
        }
        ASSERT(nTryLogoutTimes < 40); // 2 seconds pass while still not logout??

        this->Stop();

        DWORD dwWait = ::WaitForSingleObject(m_hEventHandler, INFINITE);

        switch(dwWait)
        {
        case WAIT_ABANDONED:
            DEBUG_INFO("Wait result for monitor: WAIT_ABANDONED.");
            break;
        case WAIT_OBJECT_0:
            DEBUG_INFO("Wait result for monitor: WAIT_OBJECT_0.");
            break;
        case WAIT_TIMEOUT:
            DEBUG_INFO("Wait result for monitor: WAIT_TIMEOUT.");
            break;
        case WAIT_FAILED:
            DEBUG_INFO("Wait result for monitor: WAIT_FAILED.");
            break;
        default:
            DEBUG_INFO("Wait result for monitor: UNKNOWN.");
            break;
        }

        eXosip_quit();
        m_bInit = false;

        this->OnUninited();

        DEBUG_INFO("SipUA::Uninit DONE");
    }
}

void SipUA::OnRegistered()
{
    DEBUG_INFO("SipUA::OnRegistered");
}

void SipUA::OnUnregistered()
{
    DEBUG_INFO("SipUA::OnUnregistered");
}

void SipUA::OnRinging()
{
    DEBUG_INFO("SipUA::OnRinging");
}

void SipUA::OnRingback()
{
    DEBUG_INFO("SipUA::OnRingback");
}

void SipUA::OnCallStarted()
{
    DEBUG_INFO("SipUA::OnCallStarted");
}

void SipUA::OnCallEnded()
{
    DEBUG_INFO("SipUA::OnCallEnded");
}

void SipUA::OnCallCancelled()
{
    DEBUG_INFO("SipUA::OnCallCancelled");
}

void SipUA::OnInited()
{
    DEBUG_INFO("SipUA::OnInited");
}

void SipUA::OnUninited()
{
    DEBUG_INFO("SipUA::OnUninited");
}