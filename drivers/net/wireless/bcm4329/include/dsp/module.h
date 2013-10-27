/*******************************************************************************
 * $Id: module.h,v 1.12 1999-01-12 01:22:10 khp Exp $
 * module.h - dsp module type definition
 ******************************************************************************/

#ifndef _DSP_MODULE_H_
#define _DSP_MODULE_H_

#include "typedefs.h"

#if RESOURCE_MANAGER
/* 
 * A module consists of 4 methods: reqs, init, run, and exit.  The
 * resource manager calls reqs & init in response to an on event with
 * arguments that accompany the event.  The run method is called from
 * the whiff dispatch loop with 3 pointers, the first to the state
 * memory, the second to the on chip input buffer and the third to the
 * on chip output buffer.  In practise, the run_method field points to
 * the source of the code which must be relocated into DSP memory.  The
 * exit method is called in response to an off event with only a
 * pointer to state memory.  The complete set of runnable modules in
 * any given system is contained in the module_table[], the index into
 * which is the module identifier.  The base size of dmem is also kept
 * in the module entry to allow claiming the first chunk of dmem
 * before callling the init method.  
 */
#endif /* RESOURCE_MANAGER */

#define MODULE_MAX_NINPUTS  4
#define MODULE_MAX_NOUTPUTS 4

/* parameterized module resource requirements */
typedef struct {
    uint32 imem;     /* instruction memory size, measured in bytes */
    uint32 dmem;     /* data memory size, measured in bytes */
    uint32 nrelocs;  /* number of relocatable pointers required */
    uint16 ninputs;  /* number of inputs */
    uint16 noutputs; /* number of outputs */
    uint32 ibuf[MODULE_MAX_NINPUTS];  /* input buf sizes, measured in bytes */
    uint32 obuf[MODULE_MAX_NOUTPUTS]; /* output buf sizes, measured in bytes */
    uint32 cycles;   /* processor cycle count */
} reqs_t;

typedef void (*reqs_method_t)(reqs_t *, void *);
typedef void (*init_method_t)(void *, void *);
typedef void (*set_method_t)(void *, void *);
typedef void (*run_method_t) (void *, uint8 *, uint32 *, uint8 *, uint32 *);
typedef void (*exit_method_t)(void *, void *);

typedef struct {
    uint32        dmem_base_size; /* used to claim first chunk of dmem */
    reqs_method_t reqs_method;    /* requirements method, called at on time */
    init_method_t init_method;    /* init method, called at on time */
    set_method_t  set_method;     /* set method, called to change params */
    run_method_t  run_method;     /* source run method, reloc'd to DSP space */
    exit_method_t exit_method;    /* exit method, called at off time */
} module_t;

#endif /* _DSP_MODULE_H_ */
