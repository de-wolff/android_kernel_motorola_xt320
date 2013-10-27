/*******************************************************************************
 * $Id: read_param.h,v 10.9 2008-09-22 19:58:25 akira Exp $
 * read_param.h - function to read in test paramaters
 ******************************************************************************/

#ifndef _READ_PARAM_H_
#define _READ_PARAM_H_

#include <stdio.h>
#include <stdlib.h>

extern char argv0[];

/* string parameters have max length of 256 chars */
typedef char string_t[256];

#define READ_SCALAR(pfile, fname, type, name, var)                             \
do {                                                                           \
    int sizeof_var = read_param(pfile, type, name, sizeof(var), &var);         \
    if (sizeof_var != sizeof(var)) {                                           \
        fprintf(stderr, "%s: error reading '%s' from %s\n", argv0,name,fname); \
        exit(1);                                                               \
    }                                                                          \
    rewind(pfile);                                                             \
} while (0)

#define READ_VECTOR(pfile, fname, type, name, vector_size, pvector)            \
do {                                                                           \
    int sizeof_vector = read_param(pfile, type, name, vector_size, pvector);   \
    if (sizeof_vector != (int)vector_size) {                                   \
        fprintf(stderr, "%s: error reading '%s' from %s\n", argv0,name,fname); \
        exit(1);                                                               \
    }                                                                          \
    rewind(pfile);                                                             \
} while (0)


#define	READ_BITFIELD(pfile, fname, name, var)                                 \
do  {                                                                          \
    uint32 tmp;                                                                  \
	READ_SCALAR(pfile, fname, "uint32", name, tmp);                              \
	var = tmp;                                                                 \
} while (0)

int read_param(FILE *, const char *, const char *, int size, void *);

typedef struct
	{
	char* type;
	size_t element_size;
	int vector;
	char* name;
	size_t var;
	size_t vector_size_var;
	}params_def_t;

void
read_params_def(
    FILE *pfile,
    const char* param_fname,
    const char* source_file,
    const params_def_t* params_def,
    int params_def_size,
    void* params_struct );

#define STYPE(type,ptype,name) #ptype,sizeof(ptype),FALSE,#name,offsetof(type,name),0
#define VTYPE(type,ptype,name) #ptype "[]",sizeof(ptype),TRUE,#name,offsetof(type,name),offsetof(type,name##_length)

#endif /* _READ_PARAM_H_ */
