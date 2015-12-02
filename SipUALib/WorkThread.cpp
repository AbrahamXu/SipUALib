#include "StdAfx.h"
#include "WorkThread.h"


IMPLEMENT_DYNCREATE(WorkThread, WorkThread::super)

WorkThread::WorkThread()
{
    m_hJobDone = NULL;
}

WorkThread::~WorkThread()
{
}

BOOL WorkThread::CreateThread()
{
    return super::CreateThread(CREATE_SUSPENDED);
}

BOOL WorkThread::InitInstance()
{
    return TRUE;
}

void WorkThread::PreThreadRun()
{
    m_hJobDone = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    VERIFY(m_hJobDone != NULL);

    MSG msg;
    memset(&msg, 0, sizeof(msg));
    ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

void WorkThread::PostThreadRun()
{
    VERIFY(::SetEvent(m_hJobDone));
}

void WorkThread::DoJob()
{
}

int WorkThread::DefaultThreadRun()
{
    int nRetVal = super::Run();
    return nRetVal;
}

int WorkThread::Run()
{
    this->PreThreadRun();
    this->DoJob();
    this->PostThreadRun();
    int nRetVal = this->DefaultThreadRun();
    return nRetVal;
}

void WorkThread::WaitForJobDone(DWORD dwMilliseconds)
{
    DWORD dwWait = ::WaitForSingleObject(this->m_hJobDone, dwMilliseconds);
    ASSERT(dwWait == WAIT_OBJECT_0);
}

void WorkThread::WaitForThreadDone(DWORD dwMilliseconds)
{
    ASSERT(this->m_bAutoDelete);
    this->m_bAutoDelete = FALSE;

    BOOL bPost = ::PostThreadMessage(this->m_nThreadID, WM_QUIT, 0, 0); // wait for super::Run()
    ASSERT(bPost);
    DWORD dwWait = ::WaitForSingleObject(this->m_hThread, dwMilliseconds);
    ASSERT(dwWait == WAIT_OBJECT_0);
}

void WorkThread::Start()
{
    CWorker::Start();
    this->ResumeThread();
}

void WorkThread::WaitForComplete(DWORD dwMilliseconds)
{
    this->WaitForJobDone(dwMilliseconds);
    this->WaitForThreadDone(dwMilliseconds);
}