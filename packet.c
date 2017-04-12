/*************************************************************
 *       Copyright 2016 Accton Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 ************************************************************/

#include "packet.h"
#include "epoll_loop.h"
#include "netif_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>

#include "log.h"

static struct epoll_event_handler packet_event;

#define DEBUGLFAG 0

#if (DEBUGLFAG == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("tcp_server:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#ifdef PACKET_DEBUG
static void packet_dump_packet(const unsigned char *buf, int cc)
{
	int i, j;
	for (i = 0; i < cc; i += 16) {
		for (j = 0; j < 16 && i + j < cc; j++)
			printf(" %02x", buf[i + j]);
		printf("\n");
	}
	printf("\n");
	fflush(stdout);
}
#endif

/*
 * To send/receive Spanning Tree packets we use PF_PACKET because
 * it allows the filtering we want but gives raw data
 */
void packet_send(int ifindex, const unsigned char *data, int len)
{
	int l;
	struct sockaddr_ll sl = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_802_2),
		.sll_ifindex = ifindex,
		.sll_halen = ETH_ALEN,
	};

	memcpy(&sl.sll_addr, data, ETH_ALEN);

#ifdef PACKET_DEBUG
	printf("Transmit Dst index %d %02x:%02x:%02x:%02x:%02x:%02x\n",
	       sl.sll_ifindex, 
	       sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
	       sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);

	packet_dump_packet(data, len);
#endif

	l = sendto(packet_event.fd, data, len, 0, 
		   (struct sockaddr *) &sl, sizeof(sl));

	if (l < 0) {
		if (errno != EWOULDBLOCK)
			ERROR("send failed: %m");
	} else if (l != len)
		ERROR("short write in sendto: %d instead of %d", l, len);
}

static void packet_rcv(uint32_t events, struct epoll_event_handler *h)
{
	int cc;
	unsigned char buf[2048];
	struct sockaddr_ll sl;
	socklen_t salen = sizeof sl;

	cc = recvfrom(h->fd, &buf, sizeof(buf), 0, (struct sockaddr *) &sl, &salen);
	if (cc <= 0) {
		ERROR("recvfrom failed: %m");
        printf("(%s:%d) cc:%d\n",__FUNCTION__,__LINE__,cc);
		return;
	}
printf("(%s:%d) cc:%d\n",__FUNCTION__,__LINE__,cc);
#ifdef PACKET_DEBUG
	printf("Receive Src ifindex %d %02x:%02x:%02x:%02x:%02x:%02x\n",
	       sl.sll_ifindex, 
	       sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
	       sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);

	packet_dump_packet(buf, cc);
#endif

	//bridge_bpdu_rcv(sl.sll_ifindex, buf, cc);
}

/* Berkeley Packet filter code to filter out spanning tree packets.
   from tcpdump -dd stp
 */
static struct sock_filter stp_filter[] = {
	{ 0x28, 0, 0, 0x0000000c },
	{ 0x25, 3, 0, 0x000005dc },
	{ 0x30, 0, 0, 0x0000000e },
	{ 0x15, 0, 1, 0x00000042 },
	{ 0x6, 0, 0, 0x00000060 },
	{ 0x6, 0, 0, 0x00000000 },
};

/*
 * Open up a raw packet socket to catch all 802.2 packets.
 * and install a packet filter to only see STP (SAP 42)
 *
 * Since any bridged devices are already in promiscious mode
 * no need to add multicast address.
 */
int packet_sock_init(void)
{
	int s;
	struct sock_fprog prog = {
		.len = sizeof(stp_filter) / sizeof(stp_filter[0]),
		.filter = stp_filter,
	};

	s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_802_2));
	if (s < 0) {
		ERROR("socket failed: %m");
		return -1;
	}

	if (setsockopt(s, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog)) < 0) 
		ERROR("setsockopt packet filter failed: %m");

	else if (fcntl(s, F_SETFL, O_NONBLOCK) < 0)
		ERROR("fcntl set nonblock failed: %m");

	else {
		packet_event.fd = s;
		packet_event.handler = packet_rcv;

		if (epoll_loop_add_epoll(&packet_event) == 0)
			return 0;
	}

	close(s);
	return -1;
}




