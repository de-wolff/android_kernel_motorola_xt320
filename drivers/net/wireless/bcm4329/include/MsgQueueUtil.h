/*
 * MsgQueueUtil.h
 * Windows CE message queue utilities
 *
 *
 * $Copyright (C) 2005-2006 Broadcom Corporation$
 *
 * $Id: MsgQueueUtil.h,v 13.4 2009-08-20 23:22:03 rtrask Exp $
 *
 */

#ifndef __MsgQueueUtil_H
#define __MsgQueueUtil_H

#include <typedefs.h>
#include <windows.h>
#include <MsgQueue.h>
#define BCMSDD_PRI_QUEUE	"BCMSDD_PRI_QUEUE"
#define BCMSDD_VIF_QUEUE		"BCMSDD_VIF_QUEUE"
HANDLE MsgQueueCreate(char *stringIn, bool rw);
void MsgQueueDelete(HANDLE qhandle);

BOOL MsgQueueSend(HANDLE hMsgQueue, LPVOID pMsgData, DWORD dwMsgLen);
BOOL MsgQueueRead(HANDLE hMsgQueue, LPVOID pMsgData, LPDWORD pdwBytesRead);
BOOL MsgQueueFlush(HANDLE qhandle);
BOOL MsgQueueGetInfo(HANDLE hMsgQueue, LPMSGQUEUEINFO pMsgQueueInfo);

#endif	/* __MsgQueueUtil_H */
