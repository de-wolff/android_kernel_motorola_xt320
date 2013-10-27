#ifdef BCMSDIO
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 BSQUARE Corporation.  All rights reserved.
//
// Module Name:
//
//    SDMem.h  
//
// Abstract:
//
//    Types and variables used for Memory Lists and Memory Tagging    
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////


#ifndef SDMEM_DEFINED
#define SDMEM_DEFINED

    
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    // memory list handle is an opague structure
typedef PVOID SD_MEMORY_LIST_HANDLE;


    //  SDAllocateMemoryWithTag - allocates memory with a tag
    //  Input:  Size - size of the allocation
    //          Tag - tag in the form of 4 ASCII characters to track the allocation
    //  Output:
    //  Return:
    //  Notes:  returns a pointer to the allocated memory
    //          must use SDFreeMemory
    //
PVOID SDAllocateMemoryWithTag(ULONG Size,
                              ULONG Tag );


    //  SDFreeMemory - frees memory allocated with SDAllocateMemoryWithTag
    //  Input:  pMemory - the memory to free
    //  Output:
    //  Return:
    //  Notes:  
    //
VOID SDFreeMemory(PVOID pMemory );


    //  SDDeleteMemhList - delete a memory list
    //      
    //  Input:
    //          hList - memory list to delete
    //        
    //  Output:
    //  Return: 
    //          
    //  Notes:  
    //          
VOID SDDeleteMemList(SD_MEMORY_LIST_HANDLE hList);


    //  SDCreateMemoryList - create a memory list
    //      
    //  Input:  Tag      - Memory Tag
    //          Depth    - threshold of committed memory list size, also initial number of 
    //                     entries
    //          EntrySize - Size of each entry/block
    //          
    //  Output: 
    //  Return: handle to memory list        
    //  Notes:  
    //          Initializes a mem list structure for use in allocating and deallocating
    //          fixed blocks of memory.  This is a mechanism to reduce heap fragmentation.  
    //          Allocations are dynamic (i.e. the list grows to accomodate allocations), 
    //          however, deallocated blocks are recycled rather than returned to the process heap.
    //          
    //          The  depth specifies the maximum number of heap committed allocations.  Additional
    //          allocations beyond the maximum depth are allocated normally, but returned to the heap rather
    //          than re-cycled.
    // 
    //            
    //          Must call SDDeleteMemList to free the list.
    //          Returns Memory hList Object handle, otherwise NULL
    //
SD_MEMORY_LIST_HANDLE SDCreateMemoryList(ULONG Tag,
                                         ULONG Depth,
                                         ULONG EntrySize);


    //  SDFreeToMemList - free a block of memory to the memory list
    //      
    //  Input:  hList - mem list object
    //          pMemory - block of memory to free 
    //  Output: 
    //  Return:
    //          
    //  Notes:  
    //          returns memory block to mem list
    //
VOID SDFreeToMemList(SD_MEMORY_LIST_HANDLE hList,
                     PVOID                 pMemory);


    //  SDAllocateFromMemhList - Allocate a block from the memory list
    //      
    //  Input:
    //          hList - memory list
    //        
    //  Output: 
    //  Return: returns pointer to memory block, NULL if system is out of memory         
    //  Notes:  
    //          returns memory block from mem list
    //
PVOID SDAllocateFromMemList(SD_MEMORY_LIST_HANDLE hList );


    //  SDCheckMemoryTags - Check the memory tags and outputs the results
    //                    to the debugger and a file
    //  Input:  LogFileName - file name to output
    //  Output:
    //  Return: TRUE if the memory tag checking succeeded     
    //  Notes:  
    //         
    //
BOOLEAN SDCheckMemoryTags(PTSTR LogFileName);

    //  SDDumpMemoryTagState - dump the current state of memory tags to the debugger
    //  Input:  
    //  Output:
    //  Notes:  
    //         
    //   
VOID SDDumpMemoryTagState();


    //  SDInitializeMemoryTagging - initialize memory tagging for the module
    //  Input:  Tags - Number of tags
    //          pModuleName - Name of module
    //  Output:
    //  Return: returns TRUE if memory tagging was successfully initialized
    //  Notes:  
    //        This function should be called before a module uses 
    //        SDMemoryAllocateWithTag to set up the tagging list
    //
BOOLEAN SDInitializeMemoryTagging(ULONG Tags, TCHAR *pModuleName); 

    //  SDDeleteMemoryTagging - delete memory tagging
    //  Input: 
    //  Output:
    //  Return:
    //  Notes:  
    //        This function should be called when the module is unloaded
    //
VOID SDDeleteMemoryTagging();


#ifdef __cplusplus
}
#endif //__cplusplus

#endif

#endif
