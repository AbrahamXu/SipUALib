#pragma once

#ifndef __AECCFREELIST_H__
#define __AECCFREELIST_H__

/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright 2003 Autodesk, Inc. All rights reserved.
//
//                     ****  CONFIDENTIAL MATERIAL  ****
//
// The information contained herein is confidential, proprietary to
// Autodesk, Inc., and considered a trade secret.  Use of this information
// by anyone other than authorized employees of Autodesk, Inc. is granted
// only under a written nondisclosure agreement, expressly prescribing the
// the scope and manner of such use.
//
/////////////////////////////////////////////////////////////////////////////
//
// Description:  
//
/////////////////////////////////////////////////////////////////////////////

// See Changelist # 1755

#include <memory>
#include <set>
#include "AeccPublicDefs.h"
//#if !defined(AECCWIN32UTILS_EXPORTS)
//#define AECCFREELISTAPI __declspec(dllimport)
//#else
//#define AECCFREELISTAPI
//#endif
#define AECCFREELISTAPI

namespace AeccDetails
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
			AECCFREELISTAPI static FreeListCriticalSection m_section;
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
		AECCFREELISTAPI static void fillList(const size_t &nObjectSize, struct ListEntry * &pListHead);
		static bool HaveFreeItems(const struct ListEntry * pListHead);
		AECCFREELISTAPI static AECCNOALIAS void putOnList(void *ptr, struct ListEntry * &pListHead);
		AECCFREELISTAPI static AECCNOALIAS AECCRESTRICT void *ll_getFromList(struct ListEntry * &pListHead);
		static AECCNOALIAS size_t ListLength(const struct ListEntry * pListHead);
		virtual AECCNOALIAS size_t ItemSize()const=0;
		virtual AECCNOALIAS size_t ItemAllocSize()const=0;
		virtual AECCNOALIAS AECCRESTRICT const struct ListEntry *getListHead()const=0;
	public:
		static void WalkFreeLists();
	};

	template <size_t allocSize, bool bAlignedBlock>
	class FreeListT : public FreeList
	{
protected:
		static AECCNOALIAS struct ListEntry * &ListHead()
		{
			static struct ListEntry **pHead = GetListHeadEntry(BlockSize(allocSize));
			return (*pHead);
		}
	typedef unsigned char byte;

	static size_t BlockSize(const size_t &size)
	{
#if defined(AECC_NONPROD)
		AllocMethodOverride ovr = Debug_Alloc_override();
		if( ovr == kForceAllPacked )
			return size;
		else if( ovr == kForceAllAligned )
			return AlignedBlockSize(size);
#endif
		return (bAlignedBlock == true) ? AlignedBlockSize(size) : size;
	}
	AECCNOALIAS AECCRESTRICT 
	void *getFromList();
	virtual size_t ItemSize()const { return allocSize; }
	virtual size_t ItemAllocSize()const { return BlockSize(allocSize); }
	virtual AECCNOALIAS AECCRESTRICT const struct ListEntry *getListHead()const { return ListHead(); }
public:
	FreeListT();
	AECCNOALIAS AECCRESTRICT void *Alloc(const size_t &size);
	AECCNOALIAS void Free(void *ptr,const size_t &size);

	};


    template <bool bAlignedBlock=true>
    class MyFreeList : public FreeList
    {
    protected:
        AECCNOALIAS struct ListEntry * &ListHead() const
        {
            /*static*/ struct ListEntry **pHead = GetListHeadEntry(BlockSize(m_nItemSize));
            return (*pHead);
        }

        typedef unsigned char byte;

        static size_t BlockSize(const size_t &size)
        {
#if defined(AECC_NONPROD)
            AllocMethodOverride ovr = Debug_Alloc_override();
            if( ovr == kForceAllPacked )
                return size;
            else if( ovr == kForceAllAligned )
                return AlignedBlockSize(size);
#endif
            return (bAlignedBlock == true) ? AlignedBlockSize(size) : size;
        }
        AECCNOALIAS AECCRESTRICT 
            void *getFromList();
        virtual size_t ItemSize()const { return m_nItemSize; }
        virtual size_t ItemAllocSize()const { return BlockSize(m_nItemSize); }
        virtual AECCNOALIAS AECCRESTRICT const struct ListEntry *getListHead()const { return ListHead(); }

    public:
        MyFreeList(size_t itemSize);
        AECCNOALIAS AECCRESTRICT void *Alloc(const size_t &size);
        AECCNOALIAS void Free(void *ptr,const size_t &size);

    private:
        size_t m_nItemSize;
    };
};

/////////////////////////////////////////////////////////////////////////////

template <class T, bool bAlignedBlocks=true>
class AeccFreeList : public AeccDetails::FreeListT<sizeof(T), bAlignedBlocks>
{
public:
	AeccFreeList() : AeccDetails::FreeListT<sizeof(T), bAlignedBlocks>()
	{
	}
//	typedef unsigned char byte;
//
//protected:
//	size_t BlockSize(const size_t &size)const
//	{
//#if defined(AECC_NONPROD)
//		AllocMethodOverride ovr = Debug_Alloc_override();
//		if( ovr == kForceAllPacked )
//			return size;
//		else if( ovr == kForceAllAligned )
//			return AlignedBlockSize(size);
//#endif
//		return (bAlignedBlocks == true) ? AlignedBlockSize(size) : size;
//	}
//	AECCNOALIAS AECCRESTRICT 
//	void *getFromList();
//	virtual size_t ItemSize()const { return sizeof(T); }
//	virtual size_t ItemAllocSize()const { return BlockSize(sizeof(T)); }
//public:
//	AeccFreeList();
//	AECCNOALIAS AECCRESTRICT void *Alloc(const size_t &size);
//	AECCNOALIAS void Free(void *ptr,const size_t &size);
};

/////////////////////////////////////////////////////////////////////////////

#ifdef AECC_NONPROD

#define AECC_DECLARE_FREELIST_EX(className,bAlignedBlock) \
	public:\
	static AeccFreeList<className,bAlignedBlock> *m_theFreeList; \
	AECCNOALIAS AECCRESTRICT void *operator new(size_t size); \
	AECCNOALIAS void operator delete(void *ptr,size_t size); \
	AECCNOALIAS AECCRESTRICT void *operator new(size_t size, const char *,const int ); \
	AECCNOALIAS void operator delete(void *ptr, const char *,const int ); \
	AECCNOALIAS AECCRESTRICT void *operator new(size_t ,void *p) { return p; } \
	AECCNOALIAS void operator delete(void *,void *) { }

#define AECC_IMPLEMENT_FREELIST_EX(className,bAlignedBlock) \
	AeccFreeList< className,bAlignedBlock > *className::m_theFreeList = new AeccFreeList< className,bAlignedBlock >(); \
	AECCNOALIAS AECCRESTRICT void *className::operator new(size_t size) \
	{ \
		return m_theFreeList->Alloc(size); \
	} \
	AECCNOALIAS void className::operator delete(void *ptr,size_t size) \
	{ \
		m_theFreeList->Free(ptr,size); \
	} \
	AECCNOALIAS AECCRESTRICT void *className::operator new(size_t size, const char *,const int ) \
	{ \
		return m_theFreeList->Alloc(size); \
	} \
	AECCNOALIAS void className::operator delete(void *ptr, const char *,const int ) \
	{ \
		m_theFreeList->Free(ptr,sizeof(className)); \
	} \

#else

#define AECC_DECLARE_FREELIST_EX(className,bAlignedBlock) \
	public:\
	static AeccFreeList< className,bAlignedBlock > *m_theFreeList; \
	AECCNOALIAS AECCRESTRICT void *operator new(size_t size); \
	AECCNOALIAS void operator delete(void *ptr,size_t size); \
	AECCNOALIAS AECCRESTRICT void *operator new(size_t /* size*/ ,void *p) { return p; } \
	AECCNOALIAS void operator delete(void * /* p */ ,void * /* pp */ ) { }


#define AECC_IMPLEMENT_FREELIST_EX(className,bAlignedBlock) \
	AeccFreeList< className,bAlignedBlock > *className::m_theFreeList = new AeccFreeList< className,bAlignedBlock >(); \
	AECCNOALIAS AECCRESTRICT void *className::operator new(size_t size) \
	{ \
		return m_theFreeList->Alloc(size); \
	} \
	AECCNOALIAS void className::operator delete(void *ptr,size_t size) \
	{ \
		m_theFreeList->Free(ptr,size); \
	} \

#endif

#if defined(_M_IX86)

#define AECC_DECLARE_FREELIST(className) AECC_DECLARE_FREELIST_EX(className, false)
#define AECC_IMPLEMENT_FREELIST(className) AECC_IMPLEMENT_FREELIST_EX(className,false)

#else

#define AECC_DECLARE_FREELIST(className) AECC_DECLARE_FREELIST_EX(className, true)
#define AECC_IMPLEMENT_FREELIST(className) AECC_IMPLEMENT_FREELIST_EX(className,true)

#endif

/////////////////////////////////////////////////////////////////////////////


template <size_t allocSize, bool bAlignedBlocks>
AECCNOALIAS AECCRESTRICT 
void *AeccDetails::FreeListT<allocSize, bAlignedBlocks>::getFromList()
{
	struct ListEntry * &pListHead = ListHead();
	if(HaveFreeItems(pListHead))
		return ll_getFromList(pListHead);
	else
	{
		// const unsigned long int nRealAllocSize = ((sizeof(T)>=2*sizeof(size_t)) ? sizeof(T) : 2*sizeof(size_t));
		// classes that want tightly packed blocks at the expense of access performance use false for the bAlignedBlocks template parameter
		AeccDetails::FreeList::fillList( BlockSize(allocSize), pListHead );
		return ll_getFromList(pListHead);
	}
}

template <bool bAlignedBlocks>
AECCNOALIAS AECCRESTRICT 
void *AeccDetails::MyFreeList<bAlignedBlocks>::getFromList()
{
    struct ListEntry * &pListHead = ListHead();
    if(HaveFreeItems(pListHead))
        return ll_getFromList(pListHead);
    else
    {
        // const unsigned long int nRealAllocSize = ((sizeof(T)>=2*sizeof(size_t)) ? sizeof(T) : 2*sizeof(size_t));
        // classes that want tightly packed blocks at the expense of access performance use false for the bAlignedBlocks template parameter
        AeccDetails::FreeList::fillList( BlockSize(m_nItemSize), pListHead );
        return ll_getFromList(pListHead);
    }
}

/////////////////////////////////////////////////////////////////////////////

#ifdef AECC_NONPROD
extern
void *const &
AeccFreeListDebug_FillMemory(void *const &ptr,const size_t &size,const unsigned char &val);
#endif // AECC_NONPROD

/////////////////////////////////////////////////////////////////////////////

__forceinline
AECCNOALIAS 
void *const &FillNewMemory(void *const &ptr,const size_t &size)
{
#ifdef AECC_NONPROD
	return AeccFreeListDebug_FillMemory(ptr,size,0xCB);
#else
	UNREFERENCED_PARAMETER(size);
	return ptr;
#endif
}

/////////////////////////////////////////////////////////////////////////////

template <size_t allocSize, bool bAlignedBlocks>
AECCNOALIAS AECCRESTRICT 
void *AeccDetails::FreeListT<allocSize,bAlignedBlocks>::Alloc(const size_t &size)
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
AECCNOALIAS AECCRESTRICT 
void *AeccDetails::MyFreeList<bAlignedBlocks>::Alloc(const size_t &size)
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
AECCNOALIAS 
void *const &FillOldMemory(void *const &ptr,const size_t &size)
{
#ifdef AECC_NONPROD
	return AeccFreeListDebug_FillMemory(ptr,size,0xDD);
#else
	UNREFERENCED_PARAMETER(size);
	return ptr;
#endif
}

/////////////////////////////////////////////////////////////////////////////

template <size_t allocSize, bool bAlignedBlocks>
AECCNOALIAS
void AeccDetails::FreeListT<allocSize, bAlignedBlocks>::Free(void *ptr,const size_t &size)
{
	FreeListLocker lock;

	if(size == allocSize)
		putOnList(FillOldMemory(ptr, BlockSize(size)), ListHead() );
	else
		::delete [] reinterpret_cast<unsigned __int8 *>(ptr);
}

template <bool bAlignedBlocks>
AECCNOALIAS
void AeccDetails::MyFreeList<bAlignedBlocks>::Free(void *ptr,const size_t &size)
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
AeccDetails::FreeListT<allocSize, bAlignedBlocks>::FreeListT() :
AeccDetails::FreeList()
{
}

template <bool bAlignedBlocks>
AeccDetails::MyFreeList<bAlignedBlocks>::MyFreeList(size_t nItemSize)
    :AeccDetails::FreeList()
    , m_nItemSize(nItemSize)
{
}

/////////////////////////////////////////////////////////////////////////////

#include "AeccPublicDefs.h"
#if defined( AECC_ENABLE_INLINES )
#include "AeccFreeList.inl"
#endif // AECC_ENABLE_INLINES

/////////////////////////////////////////////////////////////////////////////

#endif  // __AECCFREELIST_H__

