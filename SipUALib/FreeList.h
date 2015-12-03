#pragma once

#ifndef __FREELIST_H__
#define __FREELIST_H__

#include <memory>
#include <set>
#include "PublicDefs.h"
#define FREELISTAPI

namespace FreeListDetails
{

	// This class wraps the FreeListCriticalSection object up into a class
	// that implements the "construction os resource allocation" design pattern
	// using this class insures the critical section is always released regardless of how
	// we exit a function that uses it
	class 
	FreeList
	{
	protected:
		class FreeListLocker
		{
			// This class wraps a CRITICAL_SECTION and takes care of initialization/destruction
			// of the critical section.
			class FreeListCriticalSection
			{
			private:
				CRITICAL_SECTION m_section;
			public:
				FreeListCriticalSection();
				~FreeListCriticalSection();
				void Lock();
				void Unlock();
			};
		public:
			FreeListLocker();
			~FreeListLocker();
		private:
			// this single static instance of a FreeListCriticalSection object
			FREELISTAPI static FreeListCriticalSection m_section;
		};

	private:
		// struct ListEntry *m_pListHead;
		static ::std::set<const FreeList *> m_pFreeLists;
	protected:
public:
		enum AllocMethodOverride
		{
			kHonorAlignedFlag,
			kForceAllAligned,
			kForceAllPacked
		};
protected:
		static AllocMethodOverride Debug_Alloc_override();
		static struct ListEntry **GetListHeadEntry(const size_t &size);
		static size_t AlignedBlockSize(const size_t &size)
		{
			if( size <= 8 )
				return 8;
			else if( size <= 16 )
				return 16;
			else if( size <= 24 )
				return 24;
			else if( size <= 32 )
				return 32;
			else if( size <= 64 )
				return 64;
			else if( size <= 96 )
				return 96;
			else
				return ( (size + 127) & (~127) );
		}

		FreeList();
		virtual ~FreeList();
		FREELISTAPI static void fillList(const size_t &nObjectSize, struct ListEntry * &pListHead);
		static bool HaveFreeItems(const struct ListEntry * pListHead);
		FREELISTAPI static  void putOnList(void *ptr, struct ListEntry * &pListHead);
		FREELISTAPI static void *ll_getFromList(struct ListEntry * &pListHead);
		static size_t ListLength(const struct ListEntry * pListHead);
		virtual size_t ItemSize()const=0;
		virtual size_t ItemAllocSize()const=0;
		virtual const struct ListEntry *getListHead()const=0;
	public:
		static void WalkFreeLists();
	};

	template <size_t allocSize, bool bAlignedBlock>
	class FreeListT : public FreeList
	{
protected:
		static struct ListEntry * &ListHead()
		{
			static struct ListEntry **pHead = GetListHeadEntry(BlockSize(allocSize));
			return (*pHead);
		}
	typedef unsigned char byte;

	static size_t BlockSize(const size_t &size)
	{
#if defined(_NONPROD)
		AllocMethodOverride ovr = Debug_Alloc_override();
		if( ovr == kForceAllPacked )
			return size;
		else if( ovr == kForceAllAligned )
			return AlignedBlockSize(size);
#endif
		return (bAlignedBlock == true) ? AlignedBlockSize(size) : size;
	}
	void *getFromList();
	virtual size_t ItemSize()const { return allocSize; }
	virtual size_t ItemAllocSize()const { return BlockSize(allocSize); }
	virtual const struct ListEntry *getListHead()const { return ListHead(); }
public:
	FreeListT();
	void *Alloc(const size_t &size);
	void Free(void *ptr,const size_t &size);

	};


    template <bool bAlignedBlock=true>
    class MyFreeList : public FreeList
    {
    protected:
        struct ListEntry * &ListHead() const
        {
            /*static*/ struct ListEntry **pHead = GetListHeadEntry(BlockSize(m_nItemSize));
            return (*pHead);
        }

        typedef unsigned char byte;

        static size_t BlockSize(const size_t &size)
        {
#if defined(_NONPROD)
            AllocMethodOverride ovr = Debug_Alloc_override();
            if( ovr == kForceAllPacked )
                return size;
            else if( ovr == kForceAllAligned )
                return AlignedBlockSize(size);
#endif
            return (bAlignedBlock == true) ? AlignedBlockSize(size) : size;
        }
        void *getFromList();
        virtual size_t ItemSize()const { return m_nItemSize; }
        virtual size_t ItemAllocSize()const { return BlockSize(m_nItemSize); }
        virtual const struct ListEntry *getListHead()const { return ListHead(); }

    public:
        MyFreeList(size_t itemSize);
        void *Alloc(const size_t &size);
        void Free(void *ptr,const size_t &size);

    private:
        size_t m_nItemSize;
    };
};

/////////////////////////////////////////////////////////////////////////////

template <class T, bool bAlignedBlocks=true>
class FreeList : public FreeListDetails::FreeListT<sizeof(T), bAlignedBlocks>
{
public:
	FreeList() : FreeListDetails::FreeListT<sizeof(T), bAlignedBlocks>()
	{
	}
//	typedef unsigned char byte;
//
//protected:
//	size_t BlockSize(const size_t &size)const
//	{
//#if defined(_NONPROD)
//		AllocMethodOverride ovr = Debug_Alloc_override();
//		if( ovr == kForceAllPacked )
//			return size;
//		else if( ovr == kForceAllAligned )
//			return AlignedBlockSize(size);
//#endif
//		return (bAlignedBlocks == true) ? AlignedBlockSize(size) : size;
//	}
//	void *getFromList();
//	virtual size_t ItemSize()const { return sizeof(T); }
//	virtual size_t ItemAllocSize()const { return BlockSize(sizeof(T)); }
//public:
//	FreeList();
//	void *Alloc(const size_t &size);
//	void Free(void *ptr,const size_t &size);
};

/////////////////////////////////////////////////////////////////////////////

#ifdef _NONPROD

#define _DECLARE_FREELIST_EX(className,bAlignedBlock) \
	public:\
	static FreeList<className,bAlignedBlock> *m_theFreeList; \
	void *operator new(size_t size); \
	void operator delete(void *ptr,size_t size); \
	void *operator new(size_t size, const char *,const int ); \
	void operator delete(void *ptr, const char *,const int ); \
	void *operator new(size_t ,void *p) { return p; } \
	void operator delete(void *,void *) { }

#define _IMPLEMENT_FREELIST_EX(className,bAlignedBlock) \
	FreeList< className,bAlignedBlock > *className::m_theFreeList = new FreeList< className,bAlignedBlock >(); \
	void *className::operator new(size_t size) \
	{ \
		return m_theFreeList->Alloc(size); \
	} \
	void className::operator delete(void *ptr,size_t size) \
	{ \
		m_theFreeList->Free(ptr,size); \
	} \
	void *className::operator new(size_t size, const char *,const int ) \
	{ \
		return m_theFreeList->Alloc(size); \
	} \
	void className::operator delete(void *ptr, const char *,const int ) \
	{ \
		m_theFreeList->Free(ptr,sizeof(className)); \
	} \

#else

#define _DECLARE_FREELIST_EX(className,bAlignedBlock) \
	public:\
	static FreeList< className,bAlignedBlock > *m_theFreeList; \
	void *operator new(size_t size); \
	void operator delete(void *ptr,size_t size); \
	void *operator new(size_t /* size*/ ,void *p) { return p; } \
	void operator delete(void * /* p */ ,void * /* pp */ ) { }


#define _IMPLEMENT_FREELIST_EX(className,bAlignedBlock) \
	FreeList< className,bAlignedBlock > *className::m_theFreeList = new FreeList< className,bAlignedBlock >(); \
	void *className::operator new(size_t size) \
	{ \
		return m_theFreeList->Alloc(size); \
	} \
	void className::operator delete(void *ptr,size_t size) \
	{ \
		m_theFreeList->Free(ptr,size); \
	} \

#endif

#if defined(_M_IX86)

#define _DECLARE_FREELIST(className) _DECLARE_FREELIST_EX(className, false)
#define _IMPLEMENT_FREELIST(className) _IMPLEMENT_FREELIST_EX(className,false)

#else

#define _DECLARE_FREELIST(className) _DECLARE_FREELIST_EX(className, true)
#define _IMPLEMENT_FREELIST(className) _IMPLEMENT_FREELIST_EX(className,true)

#endif

/////////////////////////////////////////////////////////////////////////////


template <size_t allocSize, bool bAlignedBlocks>
void *FreeListDetails::FreeListT<allocSize, bAlignedBlocks>::getFromList()
{
	struct ListEntry * &pListHead = ListHead();
	if(HaveFreeItems(pListHead))
		return ll_getFromList(pListHead);
	else
	{
		// const unsigned long int nRealAllocSize = ((sizeof(T)>=2*sizeof(size_t)) ? sizeof(T) : 2*sizeof(size_t));
		// classes that want tightly packed blocks at the expense of access performance use false for the bAlignedBlocks template parameter
		FreeListDetails::FreeList::fillList( BlockSize(allocSize), pListHead );
		return ll_getFromList(pListHead);
	}
}

template <bool bAlignedBlocks>
void *FreeListDetails::MyFreeList<bAlignedBlocks>::getFromList()
{
    struct ListEntry * &pListHead = ListHead();
    if(HaveFreeItems(pListHead))
        return ll_getFromList(pListHead);
    else
    {
        // const unsigned long int nRealAllocSize = ((sizeof(T)>=2*sizeof(size_t)) ? sizeof(T) : 2*sizeof(size_t));
        // classes that want tightly packed blocks at the expense of access performance use false for the bAlignedBlocks template parameter
        FreeListDetails::FreeList::fillList( BlockSize(m_nItemSize), pListHead );
        return ll_getFromList(pListHead);
    }
}

/////////////////////////////////////////////////////////////////////////////

#ifdef _NONPROD
extern
void *const &
FreeListDebug_FillMemory(void *const &ptr,const size_t &size,const unsigned char &val);
#endif // _NONPROD

/////////////////////////////////////////////////////////////////////////////

__forceinline

void *const &FillNewMemory(void *const &ptr,const size_t &size)
{
#ifdef _NONPROD
	return FreeListDebug_FillMemory(ptr,size,0xCB);
#else
	UNREFERENCED_PARAMETER(size);
	return ptr;
#endif
}

/////////////////////////////////////////////////////////////////////////////

template <size_t allocSize, bool bAlignedBlocks>
void *FreeListDetails::FreeListT<allocSize,bAlignedBlocks>::Alloc(const size_t &size)
{
	FreeListLocker lock;

	if(size == allocSize)
		return FillNewMemory( getFromList(),BlockSize(size) );
	else
    {
        ASSERT(0); // should never called
		return ::new unsigned __int8[size];
    }
}

template <bool bAlignedBlocks>
void *FreeListDetails::MyFreeList<bAlignedBlocks>::Alloc(const size_t &size)
{
    FreeListLocker lock;

    if(size == m_nItemSize)
        return FillNewMemory( getFromList(),BlockSize(size) );
    else
    {
        ASSERT(0); // should never called
        return ::new unsigned __int8[size];
    }
}

/////////////////////////////////////////////////////////////////////////////

__forceinline

void *const &FillOldMemory(void *const &ptr,const size_t &size)
{
#ifdef _NONPROD
	return FreeListDebug_FillMemory(ptr,size,0xDD);
#else
	UNREFERENCED_PARAMETER(size);
	return ptr;
#endif
}

/////////////////////////////////////////////////////////////////////////////

template <size_t allocSize, bool bAlignedBlocks>
void FreeListDetails::FreeListT<allocSize, bAlignedBlocks>::Free(void *ptr,const size_t &size)
{
	FreeListLocker lock;

	if(size == allocSize)
		putOnList(FillOldMemory(ptr, BlockSize(size)), ListHead() );
	else
		::delete [] reinterpret_cast<unsigned __int8 *>(ptr);
}

template <bool bAlignedBlocks>
void FreeListDetails::MyFreeList<bAlignedBlocks>::Free(void *ptr,const size_t &size)
{
    FreeListLocker lock;

    if(size == m_nItemSize
        || -1 == size) // -1 means we don't know the size, but we want to free whole block
        putOnList(FillOldMemory(ptr, BlockSize(m_nItemSize)), ListHead() );

    else
        ::delete [] reinterpret_cast<unsigned __int8 *>(ptr);
}

#undef FillNewMemory
#undef FillOldMemory

/////////////////////////////////////////////////////////////////////////////

template <size_t allocSize, bool bAlignedBlocks>
FreeListDetails::FreeListT<allocSize, bAlignedBlocks>::FreeListT() :
FreeListDetails::FreeList()
{
}

template <bool bAlignedBlocks>
FreeListDetails::MyFreeList<bAlignedBlocks>::MyFreeList(size_t nItemSize)
    :FreeListDetails::FreeList()
    , m_nItemSize(nItemSize)
{
}

/////////////////////////////////////////////////////////////////////////////

#include "PublicDefs.h"
#if defined( _ENABLE_INLINES )
#include "FreeList.inl"
#endif // _ENABLE_INLINES

/////////////////////////////////////////////////////////////////////////////

#endif  // __FreeList_H__

