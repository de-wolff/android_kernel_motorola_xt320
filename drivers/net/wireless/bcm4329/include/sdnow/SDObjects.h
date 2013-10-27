#ifdef BCMSDIO
//////////////////////////////////////////////////////////////////
// Copyright (c) 2002 BSQUARE Corporation.  All rights reserved.
//
// Module Name:
//
//    SDObjects.h   
//
// Abstract:
//
//    SD Object class definitions
//
// Notes:
//
//////////////////////////////////////////////////////////////////


#include "SDCardDDK.h"
 
#ifndef _OBJECT_DEFINED
#define _OBJECT_DEFINED

    // forward declaration
typedef struct _OBJECT_ENTRY *POBJECT_ENTRY;
  
    // object list entry
typedef struct _OBJECT_ENTRY {
    POBJECT_ENTRY pNext;        // next
    UCHAR         Data[1];      // start of entry data
}OBJECT_ENTRY;

    // class definitions for the object list
class CSDObjectList
{
public:

        // override operator new and delete to use memory tagging
    PVOID operator new(size_t Size)
        {   return SDAllocateMemoryWithTag(Size,
                                           'SOBJ'); }
    VOID operator delete(PVOID pMemory)
        {          if (pMemory != NULL) SDFreeMemory(pMemory); }

    ~CSDObjectList();

    CSDObjectList(ULONG Tag) {
        m_Tag = Tag;
        InitializeCriticalSection(&m_ListCritSection);
        m_pList = NULL;
        m_CommitCount = 0;
        m_Depth = 0;
        m_ObjectSize = 0;
    }

    BOOL CreateList(ULONG ObjectSize,
                    ULONG ListDepth);

    POBJECT_ENTRY CreateEntry();

    VOID DeleteEntry(POBJECT_ENTRY pEntry);

    PVOID GetObject();

    VOID FreeObject(PVOID pObject);

    virtual BOOLEAN OnObjectCreate(PVOID pObject) {
        return TRUE;
    }

    virtual VOID OnObjectDelete(PVOID pObject) {

    }

protected:
    ULONG                   m_Tag;                     // Tag
    ULONG                   m_Depth;                   // Depth
    ULONG                   m_ObjectSize;              // size of each object 
    ULONG                   m_CommitCount;             // number of entries committed in memory so far
    CRITICAL_SECTION        m_ListCritSection;         // list critical section
    POBJECT_ENTRY           m_pList;                   // list of objects
};

#endif

#endif
