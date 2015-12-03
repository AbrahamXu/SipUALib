#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H


#include <jrtplib3/rtpmemorymanager.h>

class FreeListDetails::FreeList;
class LookSide;


class BufferManager : public jrtplib::RTPMemoryManager
{
private:
    typedef std::map<size_t, FreeListDetails::FreeList*> FreeDict;
    typedef std::map<size_t, LookSide*> LookSideDict;
public:
    BufferManager();
    virtual ~BufferManager();

    void *Alloc(const size_t &size);
    void Free(void *ptr,const size_t &size);

    // from RTPMemoryManager
    virtual void *AllocateBuffer(size_t numbytes, int memtype);
    virtual void FreeBuffer(void *buffer);

    void Clear();

private:
    FreeDict m_mgr; // for small mem blocks
    LookSideDict m_alloc;
    mutable CCriticalSection m_cs;
};


extern BufferManager* GetMemMgr();


#endif//BUFFER_MANAGER_H