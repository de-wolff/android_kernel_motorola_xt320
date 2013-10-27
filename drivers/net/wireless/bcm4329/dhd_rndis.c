/*
 * DHD Protocol Module for RNDIS
 * Basically selected code segments from usb-rndis.c
 *
 * $Copyright (C) 2005 Broadcom Corporation$
 *
 * $Id: dhd_rndis.c,v 1.17.42.2 2010-12-22 23:47:24 hharte Exp $
 */

#include <linux/module.h>
#include <typedefs.h>
#include <osl.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/random.h>
#include <linux/spinlock.h>
#include <linux/ethtool.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#include <bcm_ndis.h>
#include <rndis.h>

#include <bcmutils.h>
#include <bcmendian.h>

#include <proto/ethernet.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_bus.h>
#include <dhd_proto.h>
#include <dhd_dbg.h>

#define RETRIES 2	/* # of retries to retrieve matching ioctl response  */
/* XXXX ?? 1024 may be for USB incase of SDIO we get request which are
 * higher than 1024. If so should this value should come from bus layer ??
 */
#define IOCTL_BUFFER_LEN (1024 * 10)
#define IOCTL_RNDIS_LEN (IOCTL_BUFFER_LEN - sizeof(RNDIS_MESSAGE))

typedef struct dhd_prot {
	ulong medium;
	uint RequestId;
	RNDIS_MESSAGE msg;
	unsigned buf[IOCTL_RNDIS_LEN];
} dhd_prot_t;

void
dhd_prot_hdrpush(dhd_pub_t *dhd, int ifidx, void *pktbuf)
{
	RNDIS_MESSAGE *msg;
	RNDIS_PACKET *pkt;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* Push RNDIS header */
	PKTPUSH(dhd->osh, pktbuf, RNDIS_MESSAGE_SIZE(RNDIS_PACKET));
	memset(PKTDATA(dhd->osh, pktbuf), 0, RNDIS_MESSAGE_SIZE(RNDIS_PACKET));

	msg = (RNDIS_MESSAGE *)PKTDATA(dhd->osh, pktbuf);
	pkt = (RNDIS_PACKET *)RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(msg);
	put_unaligned(htol32(REMOTE_NDIS_PACKET_MSG), &msg->NdisMessageType);
	put_unaligned(htol32(PKTLEN(dhd->osh, pktbuf)), &msg->MessageLength);
	put_unaligned(htol32(sizeof(RNDIS_PACKET)), &pkt->DataOffset);
	put_unaligned(htol32(PKTLEN(dhd->osh, pktbuf) - RNDIS_MESSAGE_SIZE(RNDIS_PACKET)),
	              &pkt->DataLength);
}

int
dhd_prot_hdrpull(dhd_pub_t *dhd, int *ifidx, void *pktbuf)
{
	RNDIS_MESSAGE *msg = (RNDIS_MESSAGE *) PKTDATA(dhd->osh, pktbuf);
	RNDIS_PACKET *pkt = (RNDIS_PACKET *) RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(msg);
	u32 NdisMessageType, MessageLength, DataOffset, DataLength;

	if (PKTLEN(dhd->osh, pktbuf) < RNDIS_MESSAGE_SIZE(RNDIS_PACKET)) {
		DHD_ERROR(("%s: rx data too short (%d < %d)\n", dhd_ifname(dhd, 0),
		           PKTLEN(dhd->osh, pktbuf), (int)RNDIS_MESSAGE_SIZE(RNDIS_PACKET)));
		return BCME_ERROR;
	}

	/* Check packet */
	NdisMessageType = ltoh32(msg->NdisMessageType);
	MessageLength = ltoh32(msg->MessageLength);
	DataOffset = ltoh32(pkt->DataOffset);
	DataLength = ltoh32(pkt->DataLength);
	if (NdisMessageType != REMOTE_NDIS_PACKET_MSG ||
	    MessageLength > PKTLEN(dhd->osh, pktbuf)) {
		DHD_ERROR(("%s: invalid packet\n", dhd_ifname(dhd, 0)));
		return BCME_ERROR;
	}

	PKTPULL(dhd->osh, pktbuf,  (uintptr)&pkt->DataOffset - (uintptr)msg);
	PKTPULL(dhd->osh, pktbuf, DataOffset);

	/* Force packet length as per RNDIS info */
	PKTSETLEN(dhd->osh, pktbuf, DataLength);

	/* Set a default packet priority */
	PKTSETPRIO(pktbuf, 0);

	return 0;
}

int
dhd_prot_attach(dhd_pub_t *dhd)
{
	dhd_prot_t *rndis;

	if (!(rndis = MALLOC(dhd->osh, sizeof(dhd_prot_t)))) {
		DHD_ERROR(("%s: MALLOC of dhd_prot_t failed\n", __FUNCTION__));
		goto fail;
	}
	memset(rndis, 0, sizeof(dhd_prot_t));

	dhd->prot = rndis;
	dhd->maxctl = IOCTL_BUFFER_LEN;
	dhd->hdrlen += RNDIS_MESSAGE_SIZE(RNDIS_PACKET);
	return 0;

fail:
	if (rndis != NULL)
		MFREE(dhd->osh, rndis, sizeof(dhd_prot_t));
	return BCME_NOMEM;
}


/* XXX What if another thread is waiting on the semaphore?  Holding it? */
void
dhd_prot_detach(dhd_pub_t *dhd)
{
	MFREE(dhd->osh, dhd->prot, sizeof(dhd_prot_t));
	dhd->prot = NULL;
}


static int
dhdrndis_cmplt(dhd_pub_t *dhd, u32 NdisMessageType)
{
	int ret = 0;
	dhd_prot_t *prot = dhd->prot;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	do {
		ret = dhd_bus_rxctl(dhd->bus, (uchar*)&prot->msg, IOCTL_BUFFER_LEN);
		if (ret < 0)
			break;
	} while (ltoh32(prot->msg.NdisMessageType) != NdisMessageType);

	return ret;
}

static int
dhdrndis_init(dhd_pub_t *dhd)
{
	dhd_prot_t *prot = dhd->prot;
	RNDIS_MESSAGE *msg = &prot->msg;
	int ret = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* REMOTE_NDIS_INITIALIZE_MSG */
	memset(msg, 0, RNDIS_MESSAGE_SIZE(RNDIS_INITIALIZE_REQUEST));
	msg->NdisMessageType = htol32(REMOTE_NDIS_INITIALIZE_MSG);
	msg->MessageLength = htol32(RNDIS_MESSAGE_SIZE(RNDIS_INITIALIZE_REQUEST));
	msg->Message.InitializeRequest.RequestId = htol32(++prot->RequestId);
	msg->Message.InitializeRequest.MajorVersion = htol32(RNDIS_MAJOR_VERSION);
	msg->Message.InitializeRequest.MinorVersion = htol32(RNDIS_MINOR_VERSION);
	msg->Message.InitializeRequest.MaxTransferSize = htol32(0x4000);
	if ((ret = dhd_bus_txctl(dhd->bus, (uchar*)msg, ltoh32(msg->MessageLength))) < 0)
		goto done;

	/* REMOTE_NDIS_INITIALIZE_CMPLT */
	if ((ret = dhdrndis_cmplt(dhd, REMOTE_NDIS_INITIALIZE_CMPLT)) < 0)
		goto done;
	if (ltoh32(msg->MessageLength) < RNDIS_MESSAGE_SIZE(RNDIS_INITIALIZE_COMPLETE)) {
		DHD_ERROR(("%s: %s: bad message length %d\n", dhd_ifname(dhd, 0),
		           __FUNCTION__, ltoh32(msg->NdisMessageType)));
		ret = -ENODEV;
		goto done;
	}
	if (ltoh32(msg->Message.InitializeComplete.RequestId) != prot->RequestId) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.InitializeComplete.RequestId),
		           prot->RequestId));
		ret = -ENODEV;
		goto done;
	}
	if (ltoh32(msg->Message.InitializeComplete.Status) != RNDIS_STATUS_SUCCESS) {
		DHD_ERROR(("%s: %s: status 0x%x\n", dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.InitializeComplete.Status)));
		ret = -ENODEV;
		goto done;
	}

done:
	return ret;
}

static int
dhdrndis_query_oid(dhd_pub_t *dhd, uint oid, void *buf, uint len)
{
	dhd_prot_t *prot = dhd->prot;
	RNDIS_MESSAGE *msg = &prot->msg;
	void *info;
	int ret = 0, retries = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* XXX Restrict buffer length to IOCTL_BUFFER_LEN */
	if ((RNDIS_MESSAGE_SIZE(RNDIS_QUERY_REQUEST) + len) > IOCTL_BUFFER_LEN) {
		DHD_ERROR(("%s: %s: bad length\n", dhd_ifname(dhd, 0), __FUNCTION__));
		return -EINVAL;
	}

	/* REMOTE_NDIS_QUERY_MSG */
	memset(msg, 0, RNDIS_MESSAGE_SIZE(RNDIS_QUERY_REQUEST));
	msg->NdisMessageType = htol32(REMOTE_NDIS_QUERY_MSG);
	msg->MessageLength = htol32(RNDIS_MESSAGE_SIZE(RNDIS_QUERY_REQUEST) + len);

	msg->Message.QueryRequest.RequestId = htol32(++prot->RequestId);
	msg->Message.QueryRequest.Oid = htol32(oid);
	if (buf) {
		msg->Message.QueryRequest.InformationBufferLength = htol32(len);
		msg->Message.QueryRequest.InformationBufferOffset =
		    htol32(sizeof(RNDIS_QUERY_REQUEST));
		memcpy((void *)((uintptr)msg + RNDIS_MESSAGE_SIZE(RNDIS_QUERY_REQUEST)), buf, len);
	}
	if ((ret = dhd_bus_txctl(dhd->bus, (uchar*)msg, ltoh32(msg->MessageLength))) < 0)
		goto done;

retry:
	/* REMOTE_NDIS_QUERY_CMPLT */
	if ((ret = dhdrndis_cmplt(dhd, REMOTE_NDIS_QUERY_CMPLT)) < 0)
		goto done;
	if (ltoh32(msg->MessageLength) < RNDIS_MESSAGE_SIZE(RNDIS_QUERY_COMPLETE)) {
		DHD_ERROR(("%s: %s: bad message length %d\n", dhd_ifname(dhd, 0),
		           __FUNCTION__, ltoh32(msg->MessageLength)));
		ret = -EINVAL;
		goto done;
	}
	if (ltoh32(msg->Message.QueryComplete.RequestId) < prot->RequestId &&
	    ++retries < RETRIES)
		goto retry;
	if (ltoh32(msg->Message.QueryComplete.RequestId) != prot->RequestId) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.QueryComplete.RequestId), prot->RequestId));
		ret = -EINVAL;
		goto done;
	}
	if (ltoh32(msg->Message.QueryComplete.Status) != RNDIS_STATUS_SUCCESS) {
		DHD_ERROR(("%s: %s: status 0x%x\n", dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.QueryComplete.Status)));
		ret = -EINVAL;
		goto done;
	}

	/* Check info buffer */
	info = (void *)((uintptr)&msg->Message.QueryComplete.RequestId +
	                ltoh32(msg->Message.QueryComplete.InformationBufferOffset));
	if (len > ltoh32(msg->Message.QueryComplete.InformationBufferLength))
		len = ltoh32(msg->Message.QueryComplete.InformationBufferLength);
	if (((uintptr)info + len) > ((uintptr)msg + ltoh32(msg->MessageLength))) {
		DHD_ERROR(("%s: %s: bad message length %d\n", dhd_ifname(dhd, 0),
		           __FUNCTION__, ltoh32(msg->MessageLength)));
		ret = -EINVAL;
		goto done;
	}

	/* Copy info buffer */
	if (buf)
		memcpy(buf, info, len);

done:
	return ret;
}

static int
dhdrndis_set_oid(dhd_pub_t *dhd, uint oid, void *buf, uint len)
{
	dhd_prot_t *prot = dhd->prot;
	RNDIS_MESSAGE *msg = &prot->msg;
	int ret = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* XXX Restrict buffer length to IOCTL_BUFFER_LEN */
	if ((RNDIS_MESSAGE_SIZE(RNDIS_SET_REQUEST) + len) > IOCTL_BUFFER_LEN) {
		DHD_ERROR(("%s: %s: bad length\n", dhd_ifname(dhd, 0), __FUNCTION__));
		return -EINVAL;
	}

	/* REMOTE_NDIS_SET_MSG */
	memset(msg, 0, RNDIS_MESSAGE_SIZE(RNDIS_SET_REQUEST));
	msg->NdisMessageType = htol32(REMOTE_NDIS_SET_MSG);
	msg->MessageLength = htol32(RNDIS_MESSAGE_SIZE(RNDIS_SET_REQUEST) + len);
	msg->Message.SetRequest.RequestId = htol32(++prot->RequestId);
	msg->Message.SetRequest.Oid = htol32(oid);
	msg->Message.SetRequest.InformationBufferLength = len;
	msg->Message.SetRequest.InformationBufferOffset = sizeof(RNDIS_SET_REQUEST);
	memcpy((void *)((uintptr) msg + RNDIS_MESSAGE_SIZE(RNDIS_SET_REQUEST)), buf, len);
	if ((ret = dhd_bus_txctl(dhd->bus, (uchar*)msg, ltoh32(msg->MessageLength))) < 0)
		goto done;

	/* REMOTE_NDIS_SET_CMPLT */
	if ((ret = dhdrndis_cmplt(dhd, REMOTE_NDIS_SET_CMPLT)) < 0)
		goto done;
	if (ltoh32(msg->MessageLength) < RNDIS_MESSAGE_SIZE(RNDIS_SET_COMPLETE)) {
		DHD_ERROR(("%s: %s: bad message length %d\n", dhd_ifname(dhd, 0),
		           __FUNCTION__, ltoh32(msg->MessageLength)));
		ret = -EINVAL;
		goto done;
	}
	if (ltoh32(msg->Message.SetComplete.RequestId) != prot->RequestId) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.SetComplete.RequestId), prot->RequestId));
		ret = -EINVAL;
		goto done;
	}
	if (ltoh32(msg->Message.SetComplete.Status) != RNDIS_STATUS_SUCCESS) {
		DHD_ERROR(("%s: %s: status 0x%x\n", dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.SetComplete.Status)));
		ret = -EINVAL;
		goto done;
	}

done:
	return ret;
}

int
dhd_prot_ioctl(dhd_pub_t *dhd, int ifidx, wl_ioctl_t * ioc, void * buf, int len)
{
	int ret = -1;
	uint8 action;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	ASSERT(len <= IOCTL_RNDIS_LEN);

	if (len > IOCTL_RNDIS_LEN)
		return ret;

#if defined(BCMINTERNAL) && defined(DONGLEOVERLAYS)
	action = ioc->action & WL_IOCTL_ACTION_SET;
#else
	action = ioc->set;
#endif /* defined(BCMINTERNAL) && defined(DONGLEOVERLAYS) */
	if (action & WL_IOCTL_ACTION_SET)
		ret = dhdrndis_set_oid(dhd, WL_OID_BASE + ioc->cmd, buf, len);
	else
		ret = dhdrndis_query_oid(dhd, WL_OID_BASE + ioc->cmd, buf, len);

	/* Too many programs assume ioctl() returns 0 on success */
	if (ret >= 0)
		ret = 0;

	return ret;
}

void
dhd_prot_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	bcm_bprintf(strbuf, "Protocol RNDIS: reqid %d\n", dhdp->prot->RequestId);
}

int
dhd_prot_iovar_op(dhd_pub_t *dhdp, const char *name,
                  void *params, int plen, void *arg, int len, bool set)
{
	return BCME_UNSUPPORTED;
}

#ifdef BCMINTERNAL
static int
dhdrndis_test(dhd_pub_t *dhd, int type)
{
	dhd_prot_t *prot = dhd->prot;
	RNDIS_MESSAGE *msg = &prot->msg;
	NDIS_802_11_TEST test;
	void *resp;
	int ret = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	test.Length = sizeof(NDIS_802_11_TEST);
	test.Type = type;
	if (type == 1) {
		test.AuthenticationEvent.Status.StatusType =
		    Ndis802_11StatusType_Authentication;
		test.AuthenticationEvent.Request[0].Length =
		    sizeof(NDIS_802_11_AUTHENTICATION_REQUEST);
		memcpy(&test.AuthenticationEvent.Request[0].Bssid,
		       dhd->mac.octet, ETH_ALEN);
		test.AuthenticationEvent.Request[0].Flags = (ULONG) 0xdeadbeef;
	} else if (type == 2) {
		test.RssiTrigger = (NDIS_802_11_RSSI) 0xdeadbeef;
	} else
		return -EINVAL;

	if ((ret = dhdrndis_set_oid(dhd, OID_802_11_TEST, &test, sizeof(test)) < 0))
		return ret;

	/* REMOTE_NDIS_INDICATE_STATUS_MSG */
	if ((ret = dhdrndis_cmplt(dhd, REMOTE_NDIS_INDICATE_STATUS_MSG)) < 0)
		goto done;
	if (ltoh32(msg->MessageLength) < RNDIS_MESSAGE_SIZE(RNDIS_INDICATE_STATUS)) {
		DHD_ERROR(("%s: %s: bad message length %d\n", dhd_ifname(dhd, 0),
		           __FUNCTION__, ltoh32(msg->NdisMessageType)));
		ret = -EINVAL;
		goto done;
	}
	if (ltoh32(msg->Message.IndicateStatus.Status) !=
	    RNDIS_STATUS_MEDIA_SPECIFIC_INDICATION) {
		DHD_ERROR(("%s: %s: status 0x%x\n", dhd_ifname(dhd, 0), __FUNCTION__,
		           ltoh32(msg->Message.IndicateStatus.Status)));
		ret = -EINVAL;
		goto done;
	}

	/* Check response buffer */
	resp = (void *)((uintptr) &msg->Message.IndicateStatus +
	                ltoh32(msg->Message.IndicateStatus.StatusBufferOffset));
	if (type == 1) {
		if (memcmp(&test.AuthenticationEvent, resp, sizeof(test.AuthenticationEvent)))
			ret = -EINVAL;
	} else if (type == 2) {
		if (memcmp(&test.RssiTrigger, resp, sizeof(test.RssiTrigger)))
			ret = -EINVAL;
	}

	if (ret < 0)
		DHD_ERROR(("%s: %s: test %d failed\n", dhd_ifname(dhd, 0),
		           __FUNCTION__, type));

done:
	return ret;
}
#endif /* BCMINTERNAL */

void
dhd_prot_dstats(dhd_pub_t *dhd)
{
	/* Use only stats obtained from dongle */
	dhdrndis_query_oid(dhd, RNDIS_OID_GEN_XMIT_OK,
	                   &dhd->dstats.tx_packets, sizeof(ulong));
	dhdrndis_query_oid(dhd, RNDIS_OID_GEN_RCV_OK,
	                   &dhd->dstats.rx_packets, sizeof(ulong));
	dhdrndis_query_oid(dhd, RNDIS_OID_GEN_XMIT_ERROR,
	                   &dhd->dstats.tx_errors, sizeof(ulong));
	dhdrndis_query_oid(dhd, RNDIS_OID_GEN_RCV_ERROR,
	                   &dhd->dstats.rx_errors, sizeof(ulong));
	dhdrndis_query_oid(dhd, RNDIS_OID_GEN_RCV_NO_BUFFER,
	                   &dhd->dstats.rx_dropped, sizeof(ulong));
	dhdrndis_query_oid(dhd, RNDIS_OID_GEN_MULTICAST_FRAMES_RCV,
	                   &dhd->dstats.multicast, sizeof(ulong));

	dhd->dstats.tx_errors += dhd->tx_errors;
	dhd->dstats.rx_errors += dhd->rx_errors;
	dhd->dstats.rx_dropped += dhd->rx_dropped;
}

int
dhd_prot_init(dhd_pub_t *dhd)
{
	int ret = 0;
	ulong ul;
	dhd_prot_t *prot = dhd->prot;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* Startup RNDIS */
	dhdrndis_init(dhd);

	/* Get the device MAC address */
	ret = dhdrndis_query_oid(dhd,
		RNDIS_OID_802_3_CURRENT_ADDRESS, dhd->mac.octet, ETHER_ADDR_LEN);
	if (ret < 0)
		goto done;
	DHD_INFO(("%s: mac  %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", dhd_ifname(dhd, 0),
	          dhd->mac.octet[0], dhd->mac.octet[1], dhd->mac.octet[2],
	          dhd->mac.octet[3], dhd->mac.octet[4], dhd->mac.octet[5]));

	/* Get the device frame limit (but don't do anything with it??) */
	ret = dhdrndis_query_oid(dhd, RNDIS_OID_GEN_MAXIMUM_FRAME_SIZE, &ul, sizeof(ul));
	if (ret < 0)
		goto done;
	DHD_INFO(("%s: mtu %lu\n", dhd_ifname(dhd, 0), ul));

	/* Get and save the driver version number */
	ret = dhdrndis_query_oid(dhd, RNDIS_OID_GEN_VENDOR_DRIVER_VERSION, &ul, sizeof(ul));
	if (ret < 0)
		goto done;
	DHD_INFO(("%s: drv version %lu\n", dhd_ifname(dhd, 0), ul));
	dhd->drv_version = ul;

	/* Get and save the physical medium type */
	if (dhdrndis_query_oid(dhd, OID_GEN_PHYSICAL_MEDIUM, &ul, sizeof(ulong)) >= 0) {
		dhd->prot->medium = ul;
		DHD_INFO(("%s: medium %s\n", dhd_ifname(dhd, 0),
		          (ul == NdisPhysicalMediumWirelessLan) ? "wireless" : "unspecified"));
		dhd->iswl = (ul == NdisPhysicalMediumWirelessLan);
	} else
		prot->medium = NdisPhysicalMediumUnspecified;
#ifdef BCMINTERNAL
	if (ul == NdisPhysicalMediumWirelessLan) {
		dhdrndis_test(dhd, 1);
		dhdrndis_test(dhd, 2);
	}
#endif /* BCMINTERNAL */

	/* XXX Support wireless configuration */

	ul = htol32(NDIS_PACKET_TYPE_DIRECTED |
	                 NDIS_PACKET_TYPE_MULTICAST |
	                 NDIS_PACKET_TYPE_BROADCAST);
	dhdrndis_set_oid(dhd, RNDIS_OID_GEN_CURRENT_PACKET_FILTER, &ul, sizeof(ul));
	DHD_INFO(("%s: filter %lu\n", dhd_ifname(dhd, 0), ul));

done:
	return ret;
}

void
dhd_prot_stop(dhd_pub_t *dhd)
{
	dhd_prot_t *prot = dhd->prot;
	RNDIS_MESSAGE *msg = &prot->msg;
	int ret;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* REMOTE_NDIS_HALT_MSG */
	memset(msg, 0, RNDIS_MESSAGE_SIZE(RNDIS_HALT_REQUEST));
	msg->NdisMessageType = htol32(REMOTE_NDIS_HALT_MSG);
	msg->MessageLength = htol32(RNDIS_MESSAGE_SIZE(RNDIS_HALT_REQUEST));
	msg->Message.HaltRequest.RequestId = htol32(++prot->RequestId);
	ret = dhd_bus_txctl(dhd->bus, (uchar*)msg, ltoh32(msg->MessageLength));
	if (ret != 0)
		DHD_ERROR(("%s: %s: dhd_bus_txctl failed with status %d\n",
		           dhd_ifname(dhd, 0), __FUNCTION__, ret));

	/* ~NOTE~ We may not care about the response, but if we don't wait
	 * then we don't know the request went out beause it's async...
	 */
}
