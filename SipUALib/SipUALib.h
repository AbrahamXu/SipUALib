// SipUALib.h

#pragma once

using namespace System;
using namespace System::Collections::Generic;

class SipClient;

namespace SipUALib {
	public ref class SipUAMgd
	{
    public:
        //delegate Void MyListenerEventHandler(Dictionary<Object^, Object^>^ p);
        delegate Void MyListenerEventHandler(int eT);

    protected:
        MyListenerEventHandler^ OnListenerEvent;

    public:
        event MyListenerEventHandler^ ListenerEvent {
            public:
                void add(MyListenerEventHandler^ _d);
                void remove(MyListenerEventHandler^ _d);
            private:
                void raise(int eT);
        }
        void ListenerInvoke(const int evtType);

        static void _ListenerEvent(const int evtType);
        void* mLsn;
    public:
        SipUAMgd(void);
        virtual ~SipUAMgd(void);

        bool InitSettings(/*long hWndPreview, long hWndRemote*/);
        void SetVideoWindow(long hWndPreview, long hWndRemote);
        void Uninit();

        void SetUserInfo(System::String^ strUserName, System::String^ strPasswd);
        void SetCallee(System::String^ strCalleeName);
        System::String^ GetCallee();

        System::String^ GetCaller();

        void Login();
        void Logout();
        void Dial();
        void Answer();
        void Decline();
        void Hang();

        void EnableEvents(bool enable);

        // log helper
        static void Log(System::String^ strInfo);

    protected:
        !SipUAMgd(void);

    private:
        SipClient *m_impl;
	};
}
