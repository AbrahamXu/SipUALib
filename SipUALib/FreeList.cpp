#include "StdAfx.h"

// make sure this compilation unit is initialized first in case someone defides to use a 
// free list within this dll.
#pragma warning( disable : 4073 )	// warning C4073: initializers put in library initialization area
#pragma init_seg(lib)


#include "FreeList.h"

#if !defined( _ENABLE_INLINES )
#include "FreeList.inl"
#endif // _ENABLE_INLINES

#include <hash_map>
#include <crtdbg.h>


// these are initialized to the values that currently work for XP and Vista 32 as well as XP & Vista 64
size_t PAGE_SIZE =	0x00001000;					// size of a page == 4096 bytes for Win32
size_t PAGE_MASK =	0xFFFFF000;					// mask to use to get from a memory address to the beginning of the page
size_t BLOCK_SIZE =	0x00010000;					// minimum block size that can be reserved with MEM_RESERVE without wasting address space
size_t BLOCK_MASK =	0xFFFF0000;					// mask to get from a memory address to the beginning of the reserved block
size_t PAGES_PER_BLOCK = BLOCK_SIZE/PAGE_SIZE;	// number of pages in a block - 16 for XP & Vista 32 & 64
size_t HASH_SHIFT = 16;							// number of bits the PageFreeList hash table should right-shift pointers when computing their hash function


#pragma warning( disable : 4324 )				// warning C4324: 'FreeListDetails::FreeListBlockHeader' : structure was padded due to __declspec(align())


typedef int (*PrintFn)(LPCTSTR format, ...);

#if defined(_NONPROD)

static int DefaultPrintFn(LPCTSTR format, ...)
{
	CString str;
	va_list ptr; va_start(ptr, format);
	str.FormatV(format, ptr);
	va_end(ptr);
	OutputDebugString(static_cast<LPCTSTR>(str));
	return 0;
}
static PrintFn __print = DefaultPrintFn;

#else

inline
int __print(LPCTSTR, ...)
{
	return 0;
}

#endif

static
bool
InitVirtualAllocInfo()
{
	// init the Page info from the system information
	SYSTEM_INFO info={0};
	GetSystemInfo(&info);
	PAGE_SIZE = info.dwPageSize;
	BLOCK_SIZE = info.dwAllocationGranularity; // minimum block size that can be reserved with MEM_RESERVE flag
	PAGES_PER_BLOCK = BLOCK_SIZE/PAGE_SIZE;
	PAGE_MASK = static_cast<size_t>(-(static_cast<INT_PTR>(PAGE_SIZE)));
	BLOCK_MASK = static_cast<size_t>(-(static_cast<INT_PTR>(BLOCK_SIZE)));

	size_t test = BLOCK_MASK;
	size_t nBits = 0;
	while( (0 == (test & 0x01)) )
	{
		++nBits;
		test >>= 1;
	}
	HASH_SHIFT = nBits;

	ASSERT( (PAGE_MASK & (PAGE_SIZE-1)) == 0 );
	ASSERT( (BLOCK_MASK & (BLOCK_SIZE-1)) == 0 );
	return true;
}

const bool bInited = InitVirtualAllocInfo();


void *const &FreeListDebug_FillMemory(void *const &ptr,const size_t &size,const unsigned char &val)
{
	ASSERT(_CrtIsValidPointer(ptr,static_cast<unsigned long int>(size),TRUE));
	for( size_t i=0; i<size; ++i )
		((unsigned char *)ptr)[i] = val;
	//if( val == 0xCB )	// allocating
	//{
	//	// fill the region between the end of the object and the next allocation boundary
	//	// with it's own bit pattern.
	//	for( size_t i=size; i<alloc_size; ++i )
	//		((unsigned char *)ptr)[i] = 0xDE;
	//}
	//else if( val == 0xDD )	// freeing
	//{
	//	// if this assertion fails, then someone has written off the end
	//	// of the object this block of memory was used for
	//	for( size_t i=size; i<alloc_size; ++i )
	//		ASSERT( ((unsigned char *)ptr)[i] == 0xDE );
	//}
	return ptr;
}


class my_hash_compare : public ::stdext::hash_compare<void *,::std::less<void *> >
{
public:
	size_t operator()(const void * _Keyval)const
	{
		// in the PageFreeList calls to VirtualAlloc align on BLOCK_SIZE boundaries, so 
		// shift the bits down by HASH_SHIFT before masking off to eliminate all the 0's
		return (((size_t)_Keyval)>>HASH_SHIFT);
	}
	bool operator()( void * _Keyval1,  void * _Keyval2) const
	{	// test if _Keyval1 ordered before _Keyval2
		return ::stdext::hash_compare<void *,::std::less<void *> >::operator()(_Keyval1,_Keyval2);
	}

};

namespace FreeListDetails
{
	struct ListEntry
	{
		struct ListEntry *pPrev;
		struct ListEntry *pNext;
	};
	struct ListEntry2 : public ListEntry
	{
		// mimic the layout of the FreeListBlockHeader
		unsigned long int m_nItems;	// just need to make sure we zero this out when we release a block
									// this will insure our heap walking methods work correctly
	};
	__declspec(align(64))
	class FreeListBlockHeaderBase : public ListEntry
	{
	public:
		enum BlockType
		{
			kPageBlock,
			kLargeBlock
		};
	protected:
		const long unsigned int m_nItems;
		const long unsigned int m_nItemSize;
		unsigned long int		m_nInUse;
		BlockType				m_nType;
	public:
		FreeListBlockHeaderBase(const unsigned long nItems, const unsigned long nItemSize, const BlockType &type) : 
		m_nItems(nItems), 
		m_nItemSize(nItemSize),
		m_nInUse(0),
		m_nType(type)
		{}
		const BlockType &Type()const { return m_nType; }
	};

	// This class manages a free list of PAGE_SIZE (4K) pages
	// these pages are used to supply the FreeList with its pages
	class PageFreeList
	{
		friend class FreeListBlockHeader;
		typedef unsigned char byte;
		ListEntry *m_pListHead;
		typedef ::stdext::hash_map<void *,unsigned long int,my_hash_compare> RefCountMapType;
		RefCountMapType m_ref_counts;
		
		void Unlink(byte *const ptr)
		{
			ListEntry *const pEntry = reinterpret_cast<ListEntry *>(ptr);
			if( pEntry == m_pListHead )
			{
				m_pListHead = pEntry->pNext;
			}
			else
			{
				ListEntry *prev = pEntry->pPrev;
				ListEntry *next = pEntry->pNext;
				if(prev)
					prev->pNext = next;
				if(next)
					next->pPrev = prev;
			}
		}

	public:
		static int m_nPageBlocksInUse;
#ifdef _NONPROD
		static int m_nPagesInUse;
#endif
	private:
		static void IncPagesInUse()
		{
#ifdef _NONPROD
			++m_nPagesInUse;
#endif // _NONPROD
		}
		static void DecPagesInUse()
		{
#ifdef _NONPROD
			--m_nPagesInUse;
#endif // _NONPROD
		}
		static void IncPageBlocksInUse()
		{
			++m_nPageBlocksInUse;
		}
		static void DecPageBlocksInUse()
		{
			--m_nPageBlocksInUse;
		}
		
		static void *BlockStart(void *ptr)
		{
			return reinterpret_cast<void *>(reinterpret_cast<UINT_PTR>(ptr) & BLOCK_MASK);
		}

		void AddRef(void *ptr)
		{
			void *block_start = BlockStart(ptr);
			unsigned long int &n = m_ref_counts[block_start];
			++n;
			IncPagesInUse();
			ASSERT(n < 17);
		}

		void Release(void *ptr)
		{
			void *block_start = BlockStart(ptr);
			unsigned long int &n = m_ref_counts[block_start];
			if( (--n == 0) && (m_nPageBlocksInUse > 1) )
			{
				if( reinterpret_cast<FreeListBlockHeaderBase *>(block_start)->Type() == FreeListBlockHeaderBase::kPageBlock )
				{
					byte *pEntry = reinterpret_cast<byte *>(block_start);
					for( int i=0; i<(int)PAGES_PER_BLOCK; ++i )
					{
						Unlink(pEntry);
						pEntry += PAGE_SIZE;
					}
				}
				// all unlinked - now free the 64K block
				FreeBlock(block_start);
			}
			DecPagesInUse();
		}

	private:
/*
		static int (*m_pAcadNewHandler)( size_t );	// pointer to AutoCAD's new handler function.
		static HANDLE	m_new_handler_mutex;
		static int _my_temp_new_handler( size_t s)
		{
			try
			{
				// Note: we do not check the return value of WaitForSingleObject() here
				// because it is possible for a thread to get in here after we've closed the 
				// m_new_handler_mutex handle. In this case, we no longer need to wait because m_pAcadNewHandler
				// has been assigned, so it is safe to continue execution.
				::WaitForSingleObject(m_new_handler_mutex,INFINITE);
				int nRet = m_pAcadNewHandler ? m_pAcadNewHandler(s) : 0;
				::ReleaseMutex(m_new_handler_mutex);
				return nRet;
			}
			catch(...)
			{
				::ReleaseMutex(m_new_handler_mutex);
				throw;
			}
		}
*/
	public:

		PageFreeList() :
		  m_pListHead(NULL)
		{
/*
			// NOTE: I'm initializing a static member in the constructor because this class is a singleton

			if( NULL == m_pAcadNewHandler )
			{
				// We only need the Mutex for the lifetime of this function
				// Create the mutex with this thread as the initial owner
				// this locks access to m_pAcadNewHandler by other threads
				m_new_handler_mutex = ::CreateMutex(NULL, TRUE, NULL);


				// retrieve the AutoCAD new handler by temporarily installing our new handler
				m_pAcadNewHandler = _set_new_handler(_my_temp_new_handler);
				// now put AutoCADs handler back. - we're done with our temporary _my_temp_new_handler handler
				_set_new_handler(m_pAcadNewHandler);

				// The rest of the code is for cleaning up the mutex as we no longer need it

				// at this point m_pAcadNewHandler has been assigned, so _my_temp_new_handler should function correctly
				// so we can release any threads that were waiting inside of _my_temp_new_handler
				::ReleaseMutex(m_new_handler_mutex);

				// Since m_pAcadNewHandler has been initialized and will not change again, we no longer need to 
				// protect access to it, so we can safely delete the mutex handle
				::CloseHandle(m_new_handler_mutex);
				// and NULL it out as it is no longer valid
				m_new_handler_mutex = NULL;
			}
*/
		}

		void *AllocateBlock(const size_t &blockSize=BLOCK_SIZE)
		{
			void *ptr = VirtualAlloc(NULL,blockSize,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
			if( ptr )
			{
				m_ref_counts[ptr] = 0;
				// increment the count of blocks in use
				IncPageBlocksInUse();
			}
			else
			{
				// call out of memory handler
                ASSERT(0);
			}
			return ptr;
		}

		void
		FreeBlock(void *ptr)
		{
			VirtualFree(ptr,0,MEM_RELEASE);
			m_ref_counts.erase(ptr);
			DecPageBlocksInUse();
		}

		void *
		ll_getFromList()
		{
			ListEntry *ptr = m_pListHead;
			m_pListHead = m_pListHead->pNext;
			if(m_pListHead)
				m_pListHead->pPrev = 0;
			ptr->pNext = 0;
			AddRef(ptr);
			return ptr;
		}

		void
		putOnList(void *ptr)
		{
			ListEntry2 *pEntry = reinterpret_cast<ListEntry2 *>(ptr);
			pEntry->pPrev = 0;
			pEntry->pNext = m_pListHead;
			pEntry->m_nItems = 0;
			if(m_pListHead)
				m_pListHead->pPrev = pEntry;
			m_pListHead = pEntry;
			Release(ptr);
		}
		
		void *getFromList()
		{
			if(NULL != m_pListHead)
				return ll_getFromList();
			else
			{
				// const unsigned long int nRealAllocSize = ((sizeof(T)>=2*sizeof(size_t)) ? sizeof(T) : 2*sizeof(size_t));
				fillList();
				return ll_getFromList();
			}
		}
		
		void fillList()
		{
			// Note: currently (2009) BLOCK_SIZE is 64K and PAGE_SIZE is 4K, so we're allocating 16 pages of memory here
			// allocate a BLOCK_SIZE block of committed virtual memory - this is the granularity for the MEM_RESERVED flag
			// i.e. the address returned by VirtualAlloc with the MEM_RESERVED flag is aligned on a BLOCK_SIZE boundary
			// if we allocate less that BLOCK_SIZE at a time, we end up wasting address space as there will be a gap between reserved memory blocks

			void *ptr = AllocateBlock();
			if(ptr)
			{
				byte *tmp = reinterpret_cast<byte *>(ptr);
				// go through the new block of pages and construct a list of PAGE_SIZE pages
				// Note: each ListEntry struct occupies the head of the page it represents
				ListEntry2 *pHead = reinterpret_cast<ListEntry2 *>(tmp);
				pHead->pPrev = 0;
				pHead->pNext = 0;
				pHead->m_nItems = 0;
				tmp += PAGE_SIZE;
				ListEntry *pTail = pHead;
				for( unsigned long int i=1; i<PAGES_PER_BLOCK; ++i )	// start counter from 1 - already handled the first item
				{
					// add the entry to the tail of the list
					ListEntry2 *pEntry = reinterpret_cast<ListEntry2 *>(tmp);
					// pEntry->pPrev  points at the previous tail
					pEntry->pPrev = pTail;
					// pEntry->pNext will be NULL - pEntry is the new tail
					pEntry->pNext = 0;
					pEntry->m_nItems = 0;
					// update pTail to point at pEntry
					if( pTail )
						pTail->pNext = pEntry;

					// update pTail to be pEntry
					pTail = pEntry;
					// increment to the next item
					tmp += PAGE_SIZE;
				}
				// now, we have created the new linked list and we have both the head and tail
				// of the new list, Now hook it up to the head of the free list 
				_ASSERTE(pTail && !pTail->pNext);
				pTail->pNext = m_pListHead;
				if(m_pListHead)
					m_pListHead->pPrev = pTail;
				m_pListHead = pHead;
				//m_ref_counts[ptr] = 0;
				//// increment the count of blocks in use
				//IncPageBlocksInUse();
			}
		}

		size_t WalkPages();

	};


	// Note: if you add or remove data members from here
	// be sure and update the _FREELIST_PAGE_SIZE macro
	// in the header to reflect the new sizeof(FreeListBlockHeader) 
	// rounded up to the nearest 64 byte boundary
	class FreeListBlockHeader : public FreeListBlockHeaderBase
	{
		// do not add member data items to this class - add data items to FreeListBlockHeaderBase above
		typedef unsigned char byte;

		static PageFreeList thePages;
		static void *GetPage()
		{
			return thePages.getFromList();
		}

		static void ReleasePage(void *ptr)
		{
			thePages.putOnList(ptr);
		}
	public:
		FreeListBlockHeader(const unsigned long nItems,const unsigned long nItemSize, const bool &bSmallBlocks) :
		  FreeListBlockHeaderBase(nItems, nItemSize, bSmallBlocks ? kPageBlock : kLargeBlock)
		{
		}
		~FreeListBlockHeader()
		{
		}

		static void DoWalkPages()
		{
			thePages.WalkPages();
		}

         // Do a quick sanity check on member data to protect
         // against a bad pointer value
         // we expect this method is called inside a __try/__except block
         // which traps access violations
		static bool IsValidHeader(const FreeListBlockHeader *pPage)
		{
			switch(pPage->Type())
			{
				case kPageBlock:
				case kLargeBlock:
					break;
				default:
					return false;
					break;
			}
			if(  pPage->NumItems()*pPage->ItemSize() > (PAGE_SIZE - sizeof(FreeListBlockHeader)) )
				return false;
			return true;
		}

		const unsigned long int & NumItems()const { return m_nItems; }
		const unsigned long int & NumItemsInUse()const { return m_nInUse; }
		const unsigned long int & ItemSize()const { return m_nItemSize; }

		static FreeListBlockHeader *PageHeader(void *ptr)
		{
			// to get from any pointer in the free list, we simply mask off the low order bits 
			// to get the the begining of the page. 
			return reinterpret_cast<FreeListBlockHeader *>( reinterpret_cast<UINT_PTR>(ptr) & PAGE_MASK );
		}
		static void CreateBlock(const size_t &nItemSize,ListEntry *&listHead)
		{
			const size_t nPageAvailable = PAGE_SIZE - sizeof(FreeListBlockHeader);
			const bool bSmallBlocks = (nItemSize < nPageAvailable/3);
			const size_t bBlockSize = bSmallBlocks ? nPageAvailable : BLOCK_SIZE - sizeof(FreeListBlockHeader);
			// make sure the itemsize is big enough to hold a ListEntry struct
			const size_t nItemAllocSize = (nItemSize >= sizeof(ListEntry) ? nItemSize : sizeof(ListEntry));
			
			// compute the number of items per page
			const size_t nItems = bBlockSize / nItemAllocSize;

			// if this assert fires, then your objects are too large
			// to use this memory manager. 
			ASSERT( nItems > 0 );

			// TODO: We could add another level here where we reserve multiple pages
			// and then just do a commit here. - would probably want to do that if we start 
			// getting free lists of large ( > 512 bytes ) objects
			// allocate a new page
			byte *ptr = reinterpret_cast<byte *>(bSmallBlocks ? GetPage() : thePages.AllocateBlock());
			if(ptr)
			{
#pragma push_macro("new")
#undef new
				::new(ptr) FreeListBlockHeader((unsigned long int)nItems,(unsigned long int)nItemAllocSize, bSmallBlocks);	// make sure the constructor is called
#pragma pop_macro("new")
				// get the first free byte
				byte *const p = ptr + sizeof(FreeListBlockHeader);
				byte *tmp = p;
				// Now, go through the new page and add convert it to a list of items of nItemAllocSize
				ListEntry *pHead = reinterpret_cast<ListEntry *>(tmp);
				pHead->pNext = pHead->pPrev = 0;
				tmp += nItemAllocSize;
				ListEntry *pTail = pHead;
				for( unsigned long int i=1; i<nItems; ++i )	// start counter from 1 - already handled the first item
				{
					// add the entry to the tail of the list
					ListEntry *pEntry = reinterpret_cast<ListEntry *>(tmp);
					// pEntry->pNext will be NULL - pEntry is the new tail
					pEntry->pNext = 0;
					// pEntry->pPrev  points at the previous tail
					pEntry->pPrev = pTail;
					// update pTail to point at pEntry
					if( pTail )
						pTail->pNext = pEntry;

					// update pTail to be pEntry
					pTail = pEntry;
					// increment to the next item
					tmp += nItemAllocSize;
				}
				
				// now, we have created the new linked list and we have both the head and tail
				// of the new list, Now hook it up to the head passed in free list head
				_ASSERTE(pTail && !pTail->pNext);
				pTail->pNext = listHead;
				if(listHead)
					listHead->pPrev = pTail;
				listHead = pHead;
			}
		}
		void AddRef()
		{
			++m_nInUse;
			_ASSERTE(m_nInUse<=m_nItems);
		}
		void Release(ListEntry *&ListHead)
		{
			--m_nInUse;
			_ASSERTE(m_nInUse>=0);
			if(m_nInUse == 0)
			{
				// this page is not in use at all, Need to remove all it's items
				// from their free lists and free the page.

				// get the pointer to the first item on this page
				byte *const p = reinterpret_cast<byte *>(this+1);

				byte *tmp = p;
				// remove the list entries from their free list
				for( unsigned long int i=0; i<m_nItems; ++i,tmp += m_nItemSize )
				{
					ListEntry *pEntry = reinterpret_cast<ListEntry *>(tmp);
					// Note - no NULL pointer check since pEntry is derived from offsetting this - will never be NULL
					if(ListHead == pEntry)
						ListHead = pEntry->pNext;
					if(pEntry->pPrev)
						pEntry->pPrev->pNext = pEntry->pNext;
					if(pEntry->pNext)
						pEntry->pNext->pPrev = pEntry->pPrev;
					pEntry->pNext = pEntry->pPrev = 0;
				}
				const bool bSmallBlock = (Type() == kPageBlock);
				// call the descructor
				this->~FreeListBlockHeader();
				if( bSmallBlock )
				{
					// free this block.
					ReleasePage(this);
				}
				else
				{
					thePages.Release(this);
				}
			}
		}
	};



	// This is the critical section that protects the free list from race conditions
	/*static*/ FreeList::FreeListLocker::FreeListCriticalSection FreeList::FreeListLocker::m_section;

	// This hash table maintains the table of free list headers - each entry represents the head of a free list
	// of memory blocks of a given size
	struct ListEntryTable : public ::stdext::hash_map<size_t, struct ListEntry **>
	{
		ListEntryTable() : ::stdext::hash_map<size_t, struct ListEntry **>() {}
		~ListEntryTable()
		{
			for( __super::iterator it = begin();
				it != end();
				++it )
			{
				struct ListEntry **pEntry = it->second;
				delete pEntry;
			}
		}
	};
	static ListEntryTable glbFreeListListHeadTable;


	// static
	struct ListEntry **FreeList::GetListHeadEntry(const size_t &size)
	{

		struct ListEntry **pRetVal = NULL;
		ListEntryTable::iterator it = glbFreeListListHeadTable.find(size);
		if( it == glbFreeListListHeadTable.end() )
		{
			pRetVal = new struct ListEntry *;
			*pRetVal = NULL;
			glbFreeListListHeadTable[size] = pRetVal;
		}
		else
			pRetVal = it->second;
		return pRetVal;
	}



	static FreeList::AllocMethodOverride s_glbAllocOverride = FreeList::kHonorAlignedFlag;
	// static
	FreeList::AllocMethodOverride FreeList::Debug_Alloc_override()
	{
		static bool bInited = false;
		if( !bInited )
		{
			CString str(GetCommandLine());
			if( str.Find(_T("-ForceAlignedAllocs")) > -1 )
				s_glbAllocOverride = FreeList::kForceAllAligned;
			else if( str.Find(_T("-ForcePackedAllocs")) > -1 )
				s_glbAllocOverride = FreeList::kForceAllPacked;
			else
				s_glbAllocOverride = FreeList::kHonorAlignedFlag;
			bInited = true;
		}
		return s_glbAllocOverride;
	}


	// static
	::std::set<const FreeList *> FreeList::m_pFreeLists;

	FreeList::FreeList() // : m_pListHead(0)
	{
		m_pFreeLists.insert(this);
	}
	FreeList::~FreeList()
	{
		m_pFreeLists.erase(this);
	}


	size_t FreeList::ListLength(const ListEntry *m_pListHead)
	{
		size_t nSize = 0;
		for( const ListEntry *pNode = m_pListHead;
			pNode != NULL;
			pNode = pNode->pNext )
		{
			++nSize;
		}
		return nSize;
	}

	//static 
	void FreeList::WalkFreeLists()
	{
		__print(_T("Dumping information about the currently active free lists by block size.\n"));

		__print(_T("There are currently %Iu active free lists in the product.\n"), glbFreeListListHeadTable.size() );
		for( ListEntryTable::const_iterator it = glbFreeListListHeadTable.begin();
			it != glbFreeListListHeadTable.end();
			++it )
		{
			const size_t &size = it->first;
			const ListEntry *pList = *(it->second);
			__print(_T("FreeList<%Iu> has %Iu blocks in the free list for a total of %Iu bytes.\n"), size, ListLength(pList), ListLength(pList)*size );
		}


		__print(_T("Dumping info for type specific free lists. Item size is the size of the \nstruct/class being managed by the free list.\n"));
		__print(_T("allocation size is the size of the block actually allocated for each item.\n"));
		__print(_T("Note: since structs/classes that are the same size share a free list, this dump\n"));
		__print(_T("will be longer than the number of active free lists dumped above."));
		for( ::std::set<const FreeList *>::const_iterator it = m_pFreeLists.begin();
			it != m_pFreeLists.end();
			++it )
		{
			const FreeList *pFreeList = *it;
			ASSERT(pFreeList);
			if( pFreeList )
			{
				CString sClassName(typeid(*pFreeList).name());
				const size_t itemSize = pFreeList->ItemSize();
				const size_t blockSize = pFreeList->ItemAllocSize();

				__print(_T("Free List %s: Item size is %Iu, allocation size is %Iu.\n"), static_cast<LPCTSTR>(sClassName), itemSize, blockSize);
			}
		}
	}

#ifdef _NONPROD
	int PageFreeList::m_nPagesInUse = 0;
#endif
	int PageFreeList::m_nPageBlocksInUse = 0;
//	int (*PageFreeList::m_pAcadNewHandler)( size_t ) = NULL;
//	HANDLE PageFreeList::m_new_handler_mutex = NULL;

	PageFreeList FreeListBlockHeader::thePages;

	size_t PageFreeList::WalkPages()
	{

		size_t sTotalSpaceAllocated = 0;
		size_t sTotalPagesInUse = 0;
		size_t sTotalPagesOnPageFreeList = 0;

		::std::map<size_t, ::std::list<const FreeListBlockHeader *> > thePages;

		// m_ref_counts maintains a map of allocated 64K 
		// blocks of memory to ref counts for the block
		for( RefCountMapType::const_iterator it = m_ref_counts.begin();
			it != m_ref_counts.end();
			++it )
		{
			sTotalSpaceAllocated += BLOCK_SIZE;
			// each page has a FreeListBlockHeader struct at its beginning
			byte *ptr = reinterpret_cast<byte *>(it->first);
			const unsigned long int &nRefCount = it->second;
			sTotalPagesInUse += nRefCount;
			if( reinterpret_cast<const FreeListBlockHeader *>(ptr)->Type() == FreeListBlockHeader::kPageBlock )
			{
				sTotalPagesOnPageFreeList += (PAGES_PER_BLOCK - nRefCount);
				for( size_t i=0; i<PAGES_PER_BLOCK; ++i )
				{
					const FreeListBlockHeader *pHeader = reinterpret_cast<const FreeListBlockHeader *>(ptr + i*PAGE_SIZE);
					UNREFERENCED_PARAMETER(pHeader);
					if (pHeader && pHeader->ItemSize() > 0 )
					{
						thePages[pHeader->ItemSize()].push_back(pHeader);
					}
					// pHeader->WalkBlock();
				}
			}
			else
			{
				const FreeListBlockHeader *pHeader = reinterpret_cast<const FreeListBlockHeader *>(ptr);
				sTotalPagesOnPageFreeList += PAGES_PER_BLOCK;
				if( pHeader->ItemSize() > 0 )
					thePages[pHeader->ItemSize()].push_back(pHeader);
			}
		}

		__print(_T(".\n"));
		__print(_T("FreeList<T>: Currently a total of %Iu pages have been allocated by VirtualAlloc() in %Iu 64k blocks corresponding to %Iu bytes\n"), sTotalSpaceAllocated/PAGE_SIZE, sTotalSpaceAllocated/BLOCK_SIZE, sTotalSpaceAllocated);
		__print(_T("FreeList<T>: of these pages a total of %Iu pages are in use by the FreeList<T> allocators while %Iu unused pages are on the PageFreeList list\n"), sTotalPagesInUse, sTotalPagesOnPageFreeList);
		__print(_T(".\n"));

		size_t nTotalPagesInUse = 0;
		size_t nTotalSpaceInUse = 0;
		size_t nTotalSpaceFree = 0;

		// Now walk the page headers and report on usage
		for( ::std::map<size_t, ::std::list<const FreeListBlockHeader *> >::const_iterator it = thePages.begin();
			it != thePages.end();
			++it )
		{
			const ::std::list<const FreeListBlockHeader *> &theList = it->second;
			::std::list<const FreeListBlockHeader *>::const_iterator jt = theList.begin();
			if( jt != theList.end() )
			{
				const FreeListBlockHeader *pHeader = *jt;
				if( pHeader && pHeader->NumItems() > 0 )
				{
					const size_t nPages = theList.size();
					nTotalPagesInUse += nPages;
					const size_t itemSize = pHeader->ItemSize();
					unsigned long int nInUse = 0;
					size_t nItems = 0;
					size_t nFreeItems = 0;
					for( jt = theList.begin(); jt != theList.end(); ++jt )
					{
						pHeader = *jt;
						if( pHeader )
						{
							nInUse += pHeader->NumItemsInUse();
							nItems += pHeader->NumItems();
							nFreeItems += (nItems - nInUse);
						}
					}
					nTotalSpaceInUse += nInUse*itemSize;
					nTotalSpaceFree += nFreeItems*itemSize;
					const bool bIsPageBlockList = (pHeader->Type() == FreeListBlockHeader::kPageBlock);
					DWORD dwBlockAllocSize = (DWORD)( bIsPageBlockList ? PAGE_SIZE : BLOCK_SIZE);
					DWORD dwPageAvailableSpace = (DWORD)(dwBlockAllocSize - sizeof(FreeListBlockHeader));
					DWORD dwItemsPerPage = (DWORD)(dwPageAvailableSpace/itemSize);
					DWORD dwWastedPageSpace = (DWORD)(dwPageAvailableSpace - (dwItemsPerPage*itemSize));
					// DWORD dwTotalOverheadPerPage = (DWORD)(dwWastedPageSpace + sizeof(FreeListBlockHeader));
					if( bIsPageBlockList )
					{
						__print(_T("FreeList<T>: Have %d pages devoted to block size %d with %d blocks per page and %d bytes wasted at the end of the page. "), 
							nPages, 
							(DWORD)itemSize, 
							dwItemsPerPage, 
							dwWastedPageSpace );
					}
					else
					{
						__print(_T("FreeList<T>: Have a 16 page allocation in a Large Block Free List devoted to block size %d with %d blocks in the allocation and %d wasted space. "),
							(DWORD)itemSize, 
							dwItemsPerPage, 
							dwWastedPageSpace );
					}
					__print(_T("%d blocks are currently in use and %d blocks are available on the free list\n"), nInUse, (DWORD)nFreeItems);
				}
			}
		}
		__print(_T(".\n"));
		__print(_T("Total Pages allocated by FreeList<T> %d\n"), static_cast<DWORD>(nTotalPagesInUse));
		__print(_T("Total Space allocated by FreeList<T> currently in use %d bytes\n"), static_cast<DWORD>(nTotalSpaceInUse));
		__print(_T("Total Space allocated by FreeList<T> currently not in use %d bytes\n"), static_cast<DWORD>(nTotalSpaceFree));
		__print(_T(".\n"));

		return m_ref_counts.size();
	}

	void
	FreeList::putOnList(void *ptr, ListEntry *&m_pListHead)
	{
        // protect ourselves against a bad pointer passed in
        // by the time we get here destructors have been called
        // so we shouldn't have to worry about exceptions thrown from 
        // AutoCAD code. We do want to trap access violations here and continue on
        // an access violation here means either ptr was never allocated by this memory
        // manager or (ListEntry *)ptr points to something that's been corrupted.
        // in either case, the worst outcome of trapping an exception here is we leak a page of memory
        // trapping and continuing on is a reasonable thing to do and makes this allocator behave more
        // like a system allocator.
		__try
		{

			ListEntry *pHead = m_pListHead;

			reinterpret_cast<ListEntry *>(ptr)->pNext = pHead;
			reinterpret_cast<ListEntry *>(ptr)->pPrev = 0;
			if(pHead)
				pHead->pPrev = reinterpret_cast<ListEntry *>(ptr);
			pHead = reinterpret_cast<ListEntry *>(ptr);

			FreeListBlockHeader *pPageHeader = FreeListBlockHeader::PageHeader(ptr);

			if( FreeListBlockHeader::IsValidHeader(pPageHeader) )
            {
				pPageHeader->Release(pHead);
			    // don't update m_pListHead until we successfully get through the above code.
			    // that way if a bad ptr is passed in, we'll except out without currupting the current free list
			    m_pListHead = pHead;
            }
		}
		__except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
		}
	}

	void *
	FreeList::ll_getFromList(ListEntry *&m_pListHead)
	{
		ListEntry *ptr = m_pListHead;
		m_pListHead = m_pListHead->pNext;
		if(m_pListHead)
			m_pListHead->pPrev = 0;
		ptr->pNext = 0;
		FreeListBlockHeader::PageHeader(ptr)->AddRef();
		return ptr;
	}


//	::std::list<FreeListBlockHeader *>FreeListBlockHeader::m_pTheBlocks;

	void
	FreeList::fillList(const size_t &nItemSize, ListEntry *&m_pListHead)
	{
		FreeListBlockHeader::CreateBlock(nItemSize,m_pListHead);
	}
};
#ifdef _NONPROD

extern "C"
{


	__declspec(dllexport)
	void SetOutputFunction(PrintFn fcn)
	{
		if( fcn )
			__print = fcn;
		else
			__print = DefaultPrintFn;
	}

	__declspec(dllexport)
	int
	FreeListReportPagesInUse()
	{
		return FreeListDetails::PageFreeList::m_nPagesInUse;
	}
	__declspec(dllexport)
	int
	FreeListReportPageBlocksInUse()
	{
		return FreeListDetails::PageFreeList::m_nPageBlocksInUse;
	}
	__declspec(dllexport)
	void
	FreeListWalkFreeLists()
	{
		FreeListDetails::FreeListBlockHeader::DoWalkPages();
		FreeListDetails::FreeList::WalkFreeLists();
	}
}
#endif // _NONPROD
