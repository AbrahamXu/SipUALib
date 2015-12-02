namespace FreeListDetails
{
	_INLINE
	FreeList::FreeListLocker::FreeListCriticalSection::FreeListCriticalSection()
	{
		// Initialize the Critical section and initialize the spin count
		// on a single processor system, the spin count parameter count is ignored
		// and the sections spin count is set to 0.
		// on a multi-processor, EnterCriticalSection will spin wait for spin count cycles
		// before calling WaitForSingleObject - a kernel call. Here spin count is 1024.
		// this is better on a multi-core processor because crossing from user space to
		// kernel space to call WaitForSingleObject takes thousands of operations
		// with this setup, the only time we should call WaitForSingleObject() is if
		// one thred comes in here while another thread has failed to allocate a block of
		// memory and needs to call VirtualAlloc() to retreive more memory from the system.
		// Setting the high orde bit of the spin count parameter tells the operating
		// system to pre-allocate the event object here. This guarantees that entering and leaving
		// the critical section will not raise an exception.
		::InitializeCriticalSectionAndSpinCount(&m_section, 0x80000400);
	}

	_INLINE
	FreeList::FreeListLocker::FreeListCriticalSection::~FreeListCriticalSection()
	{
		::DeleteCriticalSection(&m_section);
	}

	_INLINE
	void FreeList::FreeListLocker::FreeListCriticalSection::Lock()
	{
		::EnterCriticalSection(&m_section);
	}

	_INLINE
	void FreeList::FreeListLocker::FreeListCriticalSection::Unlock()
	{
		::LeaveCriticalSection(&m_section);
	}

	_INLINE
	FreeList::FreeListLocker::FreeListLocker()
	{
		m_section.Lock();
	}

	_INLINE
	FreeList::FreeListLocker::~FreeListLocker()
	{
		m_section.Unlock();
	}

	_INLINE
	bool FreeList::HaveFreeItems(const ListEntry *m_pListHead)
	{
		return (0 != m_pListHead);
	}
};