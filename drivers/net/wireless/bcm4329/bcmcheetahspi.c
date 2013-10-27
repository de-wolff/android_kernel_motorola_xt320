/*
 * Broadcom SPI over Cheetah USB->SPI Host Controller, low-level hardware driver
 *
 * $ Copyright Broadcom Corporation $
 *
 * $Id: bcmcheetahspi.c,v 1.2 2007-08-28 08:55:13 sridhara Exp $
 */

/* #include <linux/config.h> */
#include <linux/socket.h>
/* #include <linux/kernel.h> */
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <net/sock.h>


/* #include <asm/mach-default/irq_vectors_limits.h> */

#undef ASSERT
#include <typedefs.h>
#include <pcicfg.h>
#include <bcmutils.h>

#define NETLINK_TEST 17

#include <sdio.h>	/* SDIO Specs */
#include <bcmsdbus.h>	/* bcmsdh to/from specific controller APIs */
#include <sdiovar.h>	/* to get msglevel bit values */

#include <linux/sched.h>	/* request_irq() */

#include <bcmsdspi.h>
#include <bcmspi.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0))
#define KERNEL26
#endif


struct sdos_info {
	sdioh_info_t *sd;
	spinlock_t lock;
};

/* Disable device interrupt */
void
sdspi_devintr_off(sdioh_info_t *sd)
{
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_client_ints));
}

/* Enable device interrupt */
void
sdspi_devintr_on(sdioh_info_t *sd)
{
	ASSERT(sd->lockcount == 0);
	sd_trace(("%s: %d\n", __FUNCTION__, sd->use_client_ints));
}

bool
sdspi_start_clock(sdioh_info_t *sd, uint16 new_sd_divisor)
{
	sd_err(("Clock rate not settable from driver.\n"));
	return TRUE;
}

bool
sdspi_controller_highspeed_mode(sdioh_info_t *sd, bool hsmode)
{
	sd_err(("USB-SPI Does not support high-speed mode.\n"));
	return FALSE;
}

bool
check_client_intr(sdioh_info_t *sd, int *is_dev_intr)
{
	ASSERT(sd);
	return TRUE;
}

static struct sock *nl_sk = NULL;

static void nl_data_ready(struct sock *sk, int len)
{
#ifdef KERNEL26
	wake_up_interruptible(sk->sk_sleep);
#else
	wake_up_interruptible(sk->sleep);
#endif /* KERNEL26 */
}

#define MAX_PAYLOAD 1024

bool sdspi_hw_attach(sdioh_info_t *sd)
{
	sd_trace(("%s: enter\n", __FUNCTION__));
	nl_sk = netlink_kernel_create(NETLINK_TEST, nl_data_ready);
	sd_trace(("%s: exit\n", __FUNCTION__));
	return TRUE;
}

bool sdspi_hw_detach(sdioh_info_t *sd)
{
	sd_trace(("%s: enter\n", __FUNCTION__));
#ifdef KERNEL26
	sock_release(nl_sk->sk_socket);
#else
	sock_release(nl_sk->socket);
#endif /* KERNEL26 */
	sd_trace(("%s: exit\n", __FUNCTION__));
	return TRUE;
}


void sdspi_sendrecv(sdioh_info_t *sd, uint8 *msg_out, uint8 *msg_in, int msglen)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	uint32 pid = 7087;
	int err;

	sd_trace(("%s: enter\n", __FUNCTION__));

	skb = alloc_skb(NLMSG_SPACE(msglen), GFP_KERNEL);

	nlh = NLMSG_PUT(skb, 0, 1, 0, NLMSG_SPACE(msglen));
	bcopy(msg_out, NLMSG_DATA(nlh), msglen);

	NETLINK_CB(skb).groups = 0;		/* sender is in group 1<<0 */
	NETLINK_CB(skb).pid = 0;		/* from kernel */
	NETLINK_CB(skb).dst_pid = pid;  /* multicast */
	NETLINK_CB(skb).dst_groups = 0;	/* to mcast group 1<<0 */

	netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);

	/* wait for message coming down from user-space */
	skb = skb_recv_datagram(nl_sk, 0, 0, &err);
	nlh = (struct nlmsghdr *)skb->data;

	bcopy(NLMSG_DATA(nlh), msg_in, msglen);

nlmsg_failure:
	kfree_skb(skb);
}
