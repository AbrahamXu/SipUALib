#pragma once

#ifndef WORK_THREAD_H
#define WORK_THREAD_H

#include "Worker.h"

class WorkThread : public CWinThread, public CWorker
{
    DECLARE_DYNCREATE(WorkThread)
    typedef CWinThread super;

public:
    WorkThread(void);
    virtual ~WorkThread(void);

    BOOL CreateThread();

    // Overrides
public:
    virtual BOOL InitInstance();
    virtual void Start();
private:
    virtual int Run();

protected:
    virtual void PreThreadRun();
    virtual void DoJob();
    virtual void PostThreadRun();
    virtual int  DefaultThreadRun();

    void WaitForComplete(DWORD dwMilliseconds = INFINITE);

private:
    void WaitForJobDone(DWORD dwMilliseconds);
    void WaitForThreadDone(DWORD dwMilliseconds);

private:
    HANDLE m_hJobDone;
};

#endif//WORK_THREAD_H