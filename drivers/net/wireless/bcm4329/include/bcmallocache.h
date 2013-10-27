/*
 * bcmallocache.h
 * Header file for a working-set malloc cache
 * $Copyright Open Broadcom Corporation$
 * $Id: bcmallocache.h,v 13.4 2007-07-02 02:55:16 pmoutarl Exp $
 */

#ifndef _bcm_alloc_cache_h
#define	_bcm_alloc_cache_h

typedef struct cache_s bcmcache_t;

extern int bcmcache_count_elems(bcmcache_t *cachep);
extern bcmcache_t* bcmcache_create(osl_t *osh, char *name, uint size);
extern void bcmcache_destroy(bcmcache_t *cachep);
extern void *bcmcache_alloc(bcmcache_t *cachep);
extern void bcmcache_free(bcmcache_t *cachep, void *objp);
extern void bcmcache_reclaim(bcmcache_t *cachep);

#ifdef BCMDBG
extern void bcmcache_info(bcmcache_t *cachep, char *buf);
#endif /* BCMDBG */

#endif	/* _bcm_alloc_cache_h */
