/*
 * Copyright 1998 Epigram, Inc.
 *
 * $Id: ntsyslog.h,v 8.1 1998-11-09 22:40:41 stafford Exp $
 *
 */

#ifndef _NTSYSLOG_H_
#define _NTSYSLOG_H_

HANDLE
open_event_log(void);

void
close_event_log(void);

HANDLE
get_event_log(void);

void
set_event_source(char* source_name);

#endif /* _NTSYSLOG_H_ */
