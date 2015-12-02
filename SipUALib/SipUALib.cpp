// This is the main DLL file.

#include "stdafx.h"

#include "SipUALib.h"
#include "SipClient.h"


using namespace SipUALib;

static gcroot<SipUAMgd^> gCli = nullptr;

static gcroot<SipUAMgd^> &getCli(void) {
    return gCli;
}

class __Listener : public SipClient::Listener {

public:
    __Listener() {
    }
    virtual ~__Listener() {
    }

    virtual void Callback(const int eT) {
        getCli()->_ListenerEvent(eT);
    }

};

SipUAMgd::SipUAMgd(void)
{
    DEBUG_INFO("SipUAMgd::SipUAMgd...");

    m_impl = NULL;

    DEBUG_INFO(L"SipUAMgd::SipUAMgd DONE");
}

SipUAMgd::~SipUAMgd(void)
{
    DEBUG_INFO("SipUAMgd::~SipUAMgd...");

    if (m_impl || mLsn)
        this->Uninit();

    ASSERT(m_impl == NULL);
    ASSERT(mLsn == NULL);

    DEBUG_INFO(L"SipUAMgd::~SipUAMgd DONE");
}

SipUAMgd::!SipUAMgd(void)
{
    DEBUG_INFO("SipUAMgd::!SipUAMgd...");

    if (m_impl || mLsn)
    this->Uninit();

    ASSERT(m_impl == NULL);
    ASSERT(mLsn == NULL);

    DEBUG_INFO(L"SipUAMgd::!SipUAMgd DONE");
}

bool SipUAMgd::InitSettings(/*long hWndPreview, long hWndRemote*/)
{
    ASSERT(m_impl == NULL);
    ASSERT(mLsn == NULL);

    // Instantiate the native C++ class CSimpleObject.
    m_impl = new SipClient();
    gCli = this;

    mLsn = new __Listener();
    m_impl->SetListener((__Listener*)mLsn);

    return m_impl->InitSettings();//(HWND)hWndPreview, (HWND)hWndRemote);
}

void SipUAMgd::SetVideoWindow(long hWndPreview, long hWndRemote)
{
    return m_impl->SetVideoWindow((HWND)hWndPreview, (HWND)hWndRemote);
}

void SipUAMgd::Uninit()
{
    m_impl->Uninit();

    m_impl->SetListener(NULL);
    delete (__Listener*)mLsn;
    mLsn = NULL;
    delete m_impl;
    m_impl = NULL;
}

#include <msclr\marshal.h>
#include <msclr\marshal_cppstd.h>

std::wstring
String2WString(System::String^ str)
{
    return str != nullptr ? msclr::interop::marshal_as<std::wstring>(str) : std::wstring(_T(""));
}
System::String^
WString2String(const std::wstring& str)
{
    return msclr::interop::marshal_as<System::String ^>(str);
}

void SipUAMgd::Log(System::String^ strInfo)
{
    DEBUG_INFO(String2WString(strInfo).c_str());
}

void SipUAMgd::SetUserInfo(System::String^ strUserName, System::String^ strPasswd)
{
    //::OutputDebugString(L"\nSetUserInfo: ");
    //::OutputDebugString(String2WString(strUserName).c_str());
    //::OutputDebugString(String2WString(strPasswd).c_str());
    m_impl->SetUserInfo(String2WString(strUserName).c_str(),
        String2WString(strPasswd).c_str());
}

void SipUAMgd::SetCallee(System::String^ strCalleeName)
{
    m_impl->SetCallee(String2WString(strCalleeName).c_str());
}

System::String^ SipUAMgd::GetCallee()
{
    std::wstring strCallee(m_impl->GetCallee());
    return WString2String(strCallee);
}

System::String^ SipUAMgd::GetCaller()
{
    std::wstring strCaller(m_impl->GetCaller());
    return WString2String(strCaller);
}

void SipUAMgd::Login()
{
    m_impl->Login();
}

void SipUAMgd::Logout()
{
    m_impl->Logout();
}

void SipUAMgd::Dial()
{
    m_impl->Dial();
}

void SipUAMgd::Answer()
{
    m_impl->Answer();
}

void SipUAMgd::Decline()
{
    m_impl->Decline();
}

void SipUAMgd::Hang()
{
    m_impl->Hang();
}

void SipUAMgd::EnableEvents(bool enable)
{
    m_impl->EnableEvents(enable);
}


void SipUAMgd::ListenerEvent::add(MyListenerEventHandler^ _d) {
    OnListenerEvent += _d;
}

void SipUAMgd::ListenerEvent::remove(MyListenerEventHandler^ _d) {
    OnListenerEvent -= _d;
}

void SipUAMgd::ListenerEvent::raise(int eT) {
    if(OnListenerEvent)
        OnListenerEvent->Invoke(eT);
}

Void SipUAMgd::ListenerInvoke(const int eT)
{
    ListenerEvent(eT);
}

void SipUAMgd::_ListenerEvent(const int eT)
{
    if(!getCli()) return;
    getCli()->ListenerInvoke(eT);
}