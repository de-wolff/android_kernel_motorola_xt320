/*
 * $Copyright Broadcom Corporation$
 *
 * Protocol definition to to talk to the LMAC on the dongle across the bus
 *
 * $Id: bcmlmac.h,v 9.11 2008-12-01 22:55:17 nvalji Exp $: bcmlmac.h
 */

#ifndef _bcmlmac_h_
#define _bcmlmac_h_

#ifndef _TYPEDEFS_H_
#include <typedefs.h>
#endif

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>


/* UMAC/WHD->LMAC */
#define BCMLMAC_TXPACKET		0x01
#define BCMLMAC_IOVAR_REQ		0x02
#define BCMLMAC_IOCTL_REQ		0x03

/* LMAC->UMAC/WHD */
#define BCMLMAC_TXPACKET_DONE		0x04
#define BCMLMAC_IOVAR_RESP		0x05
#define BCMLMAC_RXPACKET		0x06
#define BCMLMAC_BCMEVENT		0x07
#define BCMLMAC_IOCTL_RESP		0x08

/* Bidirectional */
#define BCMLMAC_TESTPACKET		0x09

#define BCMLMAC_PROTO_VER	1
#define VALID_BCMLMAC_PROTOVER(ver)	((ver) == BCMLMAC_PROTO_VER)
#define BCMLMAC_PROTO_HDR_LEN	(sizeof(bcmlmac_header_t))


#define TYPE_BCMLMAC_TXPKT(msg_type)		((msg_type) == BCMLMAC_TXPACKET)
#define TYPE_BCMLMAC_IOVAR_REQ(msg_type)	((msg_type) == BCMLMAC_IOVAR_REQ)
#define TYPE_BCMLMAC_IOCTL_REQ(msg_type)	((msg_type) == BCMLMAC_IOCTL_REQ)
#define TYPE_BCMLMAC_TXPKT_DONE(msg_type)	((msg_type) == BCMLMAC_TXPACKET_DONE)
#define TYPE_BCMLMAC_IOVAR_RESP(msg_type)	((msg_type) == BCMLMAC_IOVAR_RESP)
#define TYPE_BCMLMAC_RXPACKET(msg_type)		((msg_type) == BCMLMAC_RXPACKET)
#define TYPE_BCMLMAC_BCMEVENT(msg_type)		((msg_type) == BCMLMAC_BCMEVENT)
#define TYPE_BCMLMAC_IOCTL_RESP(msg_type)	((msg_type) == BCMLMAC_IOCTL_RESP)

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_header {
	uint8 	ver 	: 4;	/* Protocol verison */
	uint8 	msgtype : 4;	/* Message type */
	uint8	crc8;		/* Error detection (optional for debugging) */
	uint16	pkt_len;	/* LMAC protocol packet length, all-inclusive */
} BWL_POST_PACKED_STRUCT bcmlmac_header_t;

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_txpkt_header {
	uint32	packet_id;	/* Packet identifier */
	uint32	max_txrate;	/* max transmit rate */
	uint32	txpower;	/* txpower override */
	uint32	expiry_time;	/* expiry time in TU */
	uint8	tx_queue;	/* queue ID */
	uint8	txrateclass_table_idx; /* index into the table of txrate class table */
	uint8	more;
	uint8	reserved[1];
} BWL_POST_PACKED_STRUCT bcmlmac_txpkt_hdr_t;

#define BCMLMAC_TXPKT_HDRLEN		(sizeof(bcmlmac_txpkt_hdr_t))
#define BCMLMAC_TXPKT_TOT_HDRLEN	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_TXPKT_HDRLEN)

/* LMACHEADER + TXSTATUS1 + TXSTATUS2 +...  */
typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_txpkt_done_header {
	uint32	packet_id;	/* Packet identifier */
	uint32	txstatus;	/* txstatus */
	uint32	txrate;		/* transmit rate */
	uint32	time_to_ack; 	/* time from packet to ack */
	uint32	medium_access_time; /* medium access time */
	uint8	failed_acks;	/* number of failed acks */
	uint8	txq;		/* txq for the current packet */
	uint8	pad[2];
} BWL_POST_PACKED_STRUCT bcmlmac_txpkt_done_hdr_t;

#define BCMLMAC_TXPKTDONE_HDRLEN	(sizeof(bcmlmac_txpkt_done_hdr_t))
#define BCMLMAC_TXPKTDONE_TOT_HDRLEN(n)	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_TXPKTDONE_HDRLEN * (n))

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_rxpkt_header {
	uint32	rxstatus;	/* rx status */
	uint32	rxrate;		/* received rate */
	uint16	rxchannel;	/* Rx channel */
	uint16	rxband;		/* Rx band */
	uint32	rxflags; 	/* flags */
	uint8	RCPI;		/* RCPI of the received signal */
	uint8	rxalignpad;	/* # additional pad bytes following this struct (2 for QoS pkts) */
	uint8	pad[2]; 	/* Align this struct to 32 bits */
} BWL_POST_PACKED_STRUCT bcmlmac_rxpkt_hdr_t;

#define BCMLMAC_RXPKT_HDRLEN	(sizeof(bcmlmac_rxpkt_hdr_t))
#define BCMLMAC_RXPKT_TOT_HDRLEN (BCMLMAC_PROTO_HDR_LEN + BCMLMAC_RXPKT_HDRLEN)

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_event_header {
	uint32	event_type;
	uint32	event_status;
} BWL_POST_PACKED_STRUCT bcmlmac_event_hdr_t;

#define BCMLMAC_BCMEVENT_HDRLEN	(sizeof(bcmlmac_event_hdr_t))
#define BCMLMAC_BCMEVENT_TOT_HDRLEN	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_BCMEVENT_HDRLEN)
#define VALID_BCMLMAC_BCMEVENT_LEN(len)	((len) >= BCMLMAC_BCMEVENT_TOT_HDRLEN)

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_iovarreq_header {
	uint32	iovar_type;
} BWL_POST_PACKED_STRUCT bcmlmac_iovarreq_hdr_t;

#define BCMLMAC_IOVARREQ_HDRLEN	(sizeof(bcmlmac_iovarreq_hdr_t))
#define BCMLMAC_IOVARREQ_TOT_HDRLEN	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_IOVARREQ_HDRLEN)

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_iovarresp_header {
	int32	bcmerror;
} BWL_POST_PACKED_STRUCT bcmlmac_iovarresp_hdr_t;

#define BCMLMAC_IOVARRESP_HDRLEN	(sizeof(bcmlmac_iovarresp_hdr_t))
#define BCMLMAC_IOVARRESP_TOT_HDRLEN	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_IOVARRESP_HDRLEN)
#define VALID_BCMLMAC_IOVARRESP_LEN(len)        ((len) >= BCMLMAC_IOVARRESP_TOT_HDRLEN)

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_ioctlreq_header {
	uint32	cmd;
} BWL_POST_PACKED_STRUCT bcmlmac_ioctlreq_hdr_t;

#define BCMLMAC_IOCTLREQ_HDRLEN	(sizeof(bcmlmac_ioctlreq_hdr_t))
#define BCMLMAC_IOCTLREQ_TOT_HDRLEN	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_IOCTLREQ_HDRLEN)

typedef BWL_PRE_PACKED_STRUCT struct bcmlmac_ioctlresp_header {
	int32	bcmerror;
} BWL_POST_PACKED_STRUCT bcmlmac_ioctlresp_hdr_t;

#define BCMLMAC_IOCTLRESP_HDRLEN	(sizeof(bcmlmac_ioctlresp_hdr_t))
#define BCMLMAC_IOCTLRESP_TOT_HDRLEN	(BCMLMAC_PROTO_HDR_LEN + BCMLMAC_IOCTLRESP_HDRLEN)

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

#endif	/* !defined(_bcmlmac_h_) */
