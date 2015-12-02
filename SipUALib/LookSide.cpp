#include "stdafx.h"
#include "LookSide.h"

LookSide::LookSide()
{
    m_pEnd			= &m_end;
    m_pHead			= m_pEnd;
    
    m_bOpen			= FALSE;
}

LookSide::~LookSide()
{
    Close();
}

BOOL LookSide::Open(int nSize, int nMaxCount)
{
    if(!m_bOpen)
    {
        InitializeCriticalSection(&m_cs);
        
        m_nSize		= nSize;				
        m_nMaxCount	= nMaxCount;
        m_nCount	= 0;
        m_bOpen	 	= TRUE;
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void LookSide::Close()
{
    DeleteCriticalSection(&m_cs);	
}

char* LookSide::New()
{
    char		**ppNext;
    char		*pReturn;
    
    EnterCriticalSection(&m_cs);
    if(m_pHead == m_pEnd)
    {
        pReturn = new char[m_nSize+sizeof(char *)];
        ppNext	= (char **)&pReturn[m_nSize];
        *ppNext	= NULL;
    }
    else
    {
        pReturn = m_pHead;
        ppNext	= (char **)&pReturn[m_nSize];
        m_pHead = *ppNext;
        *ppNext	= NULL;
    }
    LeaveCriticalSection(&m_cs);		
    
    return pReturn;	
}

void LookSide::Delete(char* pData)
{
    char			**ppNext;
    
    EnterCriticalSection(&m_cs);
    ppNext	= (char **)&pData[m_nSize];
    if((*ppNext) == NULL)
    {
        if(m_nCount < m_nMaxCount)
        {
            *ppNext 	= m_pHead;
            m_pHead		= pData;
            
            m_nCount++;
        }
        else
        {
            delete []pData;
        }
    }	
    LeaveCriticalSection(&m_cs);		
}