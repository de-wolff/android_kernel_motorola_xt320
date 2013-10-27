/*
 * Copyright 1997 Epigram, Inc.
 *
 * $Id: debug_mem.h,v 2.5 1999-10-20 22:33:29 cast Exp $
 *
 */

#ifndef _DEBUG_MEM_H_

#if defined(DEBUG_MEMORY)

#include <stdlib.h>
#include <stdio.h>

void
debug_mem_print(FILE* fp);

void*
debug_malloc(size_t size, char* file, int line);
void*
debug_calloc(size_t nelem, size_t elsize, char* file, int line);
void*
debug_realloc(void* ptr, size_t size, char* file, int line);
void
debug_free (void *ptr, char* file, int line);
extern void debug_mark(void);
extern int debug_mark_check(int print);
extern void debug_mem_init (void);
extern void verify_alloc_list (void);

/* define macros to override system memory calls */
#define malloc(s)		debug_malloc((s), __FILE__, __LINE__)
#define calloc(c, s)	debug_calloc((c), (s), __FILE__, __LINE__)
#define realloc(p, s)	debug_realloc((p), (s), __FILE__, __LINE__)
#define free(p)			debug_free((p), __FILE__, __LINE__)

#endif /* DEBUG_MEMORY */

#endif /* _DEBUG_MEM_H_ */
