/*******************************************************************************
 * $Id: queue.h,v 1.6 1998-06-19 23:11:48 stafford Exp $
 * queue.h - burst buffer definition
 ******************************************************************************/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "typedefs.h"

/* type definition */
typedef struct {
    uint32 bufsize;  /* total bufsize */
    uint32 head;     /* index to head of queue */
    uint32 tail;     /* index to tail of queue */
    uint8  *pbuffer; /* pointer to buffer */
} queue_t;

/* prototypes */
void queue_init(queue_t *, uint32);
void queue_set(queue_t *, uint32, void *);
uint queue_is_empty(queue_t *);
uint queue_is_full(queue_t *);
uint queue_full_bytes(queue_t *);
uint queue_empty_bytes(queue_t *);
uint queue_get(queue_t *, uint32, uint8 *);
uint queue_put(queue_t *, uint32, uint8 *);

static INLINE void
queue_init_copy(
    queue_t * pqueue_copy,
    queue_t * pqueue_orig,
    void * pbuf
)
{
    pqueue_copy->bufsize = pqueue_orig->bufsize;
    pqueue_copy->head = pqueue_orig->head;
    pqueue_copy->tail = pqueue_orig->tail;
    pqueue_copy->pbuffer = (uint8*)pbuf;
}

static INLINE void
queue_flush_put(
    queue_t * pqueue_orig,
    queue_t * pqueue_copy
)
{
    pqueue_orig->tail = pqueue_copy->tail;
}

static INLINE void
queue_flush_get(
    queue_t * pqueue_orig,
    queue_t * pqueue_copy
)
{
    pqueue_orig->head = pqueue_copy->head;
}

#endif /* _QUEUE_H_ */
