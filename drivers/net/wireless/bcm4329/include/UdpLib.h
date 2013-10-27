/* UdpLib helper header
 * $Copyright Open Broadcom Corporation$
 * $Id: UdpLib.h,v 1.4 2007-11-19 02:27:38 kenlo Exp $
*/
#ifndef _UDPLIB_
#define _UDPLIB_

int udp_open();
int udp_bind(int fd, int portno);
void udp_close(int fd);
int udp_write(int fd, char * buf, int len, struct sockaddr_in * to);
int udp_read(int fd, char * buf, int len, struct sockaddr_in * from);
int udp_read_timed(int fd, char * buf, int len,
                   struct sockaddr_in * from, int timeout);
#endif /* _UDPLIB_ */
