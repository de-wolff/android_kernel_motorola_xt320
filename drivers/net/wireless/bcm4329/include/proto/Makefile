#
# include/proto/Makefile
#
# Copyright 2006, Broadcom, Inc.
#
# $Id: Makefile,v 9.2 2006-03-06 23:35:11 mzhu Exp $
#

# build etags
#ifdef BCMINTERNAL
TAGFILES := $(shell find . -maxdepth 1 -regex '.*/[^/.]+\.[ch]' -printf "%f ")

TAGS: $(TAGFILES)
	etags -r "/BCMINITFN(\([^(]+\))/" $(TAGFILES)
#endif
