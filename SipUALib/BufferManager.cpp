#include "StdAfx.h"
#include "BufferManager.h"

#include "LookSide.h"
#include "AeccFreeList.h"


BufferManager g_freeDict;
BufferManager* GetMemMgr()
{
    return &g_freeDict;
}


BufferManager::BufferManager()
{
}

BufferManager::~BufferManager()
{
    Clear();
}

void* BufferManager::Alloc(const size_t &size)
{
    m_cs.Lock();

    void* pBufNew = NULL;

    if (size > 512)
    {
        LookSide* pList = NULL;
        LookSideDict::iterator it;
        if (m_alloc.size() <= 0
            || (it = m_alloc.find(size)) == m_alloc.end())
        {
            pList = new LookSide();
            BOOL bOpened = pList->Open(size, 128);
            ASSERT(bOpened);
            m_alloc.insert(std::make_pair(size, pList));
        }
        else
        {
            pList = it->second;
        }

        pBufNew = pList->New();
    }
    else
    {
        AeccDetails::FreeList* pList = NULL;
        FreeDict::iterator it;
        if (m_mgr.size() <= 0
            || (it = m_mgr.find(size)) == m_mgr.end())
        {
            pList = new AeccDetails::MyFreeList<true>(size);
            m_mgr.insert(std::make_pair(size, pList));
        }
        else
        {
            pList = it->second;
        }

        AeccDetails::MyFreeList<true>* pMyList = 
            static_cast<AeccDetails::MyFreeList<true>*>( pList );

        pBufNew = pMyList->Alloc(size);
    }

    m_cs.Unlock();

    return pBufNew;
}

void BufferManager::Free(void *ptr,const size_t &size)
{
    m_cs.Lock();

    if (size > 512)
    {
        LookSide* pList = NULL;
        LookSideDict::iterator it;
        ASSERT(m_alloc.size() > 0);
        it = m_alloc.find(size);
        ASSERT(it != m_alloc.end());
        if (it != m_alloc.end())
        {
            pList = it->second;
        }

        pList->Delete((char*)ptr);
    }
    else
    {
        AeccDetails::FreeList* pList = NULL;
        FreeDict::iterator it;
        ASSERT(m_mgr.size() > 0);
        it = m_mgr.find(size);
        ASSERT(it != m_mgr.end());
        if (it != m_mgr.end())
        {
            pList = it->second;
        }

        AeccDetails::MyFreeList<true>* pMyList = 
            static_cast<AeccDetails::MyFreeList<true>*>( pList );
        pMyList->Free(ptr, size);
    }

    m_cs.Unlock();
}

void* BufferManager::AllocateBuffer(size_t numbytes, int memtype)
{
    UNREFERENCED_PARAMETER(memtype);
    return this->Alloc(numbytes);
}

void BufferManager::FreeBuffer(void *buffer)
{
    return this->Free(buffer, -1); // -1 means we don't know the size, but we want to free whole block
}

void BufferManager::Clear()
{
    m_cs.Lock();
    m_mgr.clear();
    m_cs.Unlock();
}
