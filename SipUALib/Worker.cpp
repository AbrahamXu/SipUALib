#include "StdAfx.h"
#include "Worker.h"


CWorker::CWorker(void)
{
    m_bWorking = false;
}


CWorker::~CWorker(void)
{
    ASSERT(!m_bWorking);
}
