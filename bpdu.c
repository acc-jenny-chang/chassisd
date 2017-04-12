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

#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include  <sys/types.h> 
#include  <sys/times.h> 
#include  <sys/select.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>

#include "bpdu.h"
#include "epoll_loop.h"
#include "chassis.h"

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("bpdu:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

static int socket_fd=0;
static struct epoll_event_handler packet_event;

#if 1
static RSTP_BPDU_T bpdu_packet  = {
  {/* MAC_HEADER_T */
    {0x01, 0x80, 0xc2, 0x00, 0x00, 0x10},   /* dst_mac */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    /* src_mac */
  },
  { /* ETH_HEADER_T */
    {0x00, 0x00},               /* len8023 */
    BPDU_L_SAP, BPDU_L_SAP, LLC_UI      /* dsap, ssap, llc */
  },
  {/* BPDU_HEADER_T */
    {0x00, 0x00},               /* protocol */
    BPDU_VERSION_ID, 0x00           /* version, bpdu_type */
  },
  {
    0x3c,                   /*  flags; */
    {0x10,0x4e,0xe8,0xba,0x70,0xa0,0xfb,0x00},  /*  root_id[8]; */
    {0x00,0x00,0x4e,0x20},          /*  root_path_cost[4]; */
    {0x80,0x00,0x70,0x72,0xcf,0x22,0x22,0x25},  /*  bridge_id[8]; */
    {0x80,0x01},                /*  port_id[2]; */
    {0x01,0x00},                /*  message_age[2]; */
    {0x01,0x00},                /*  max_age[2]; */
    {0x02,0x00},                /*  hello_time[2]; */
    {0x0f,0x00},                /*  forward_delay[2]; */
  },
   {0x00,0x00},                 /*  ver_1_length[2]; */
};
#endif

/* Berkeley Packet filter code to filter out spanning tree packets.
   from tcpdump -dd "ether[0:2]==0x0180 and ether[2:1]==0xc2"
 */
 /*
static struct sock_filter stp_filter2[] = {
{ 0x28, 0, 0, 0x00000000 },
{ 0x15, 0, 3, 0x00000180 },
{ 0x30, 0, 0, 0x00000002 },
{ 0x15, 0, 1, 0x000000c2 },
{ 0x6, 0, 0, 0x0000ffff },
{ 0x6, 0, 0, 0x00000000 },
};
*/

/* Berkeley Packet filter code to filter out spanning tree packets.
   from tcpdump -dd -e -i eth0 ether dst 01:80:c2:00:00:10
 */
static struct sock_filter stp_filter[] = {
{ 0x20, 0, 0, 0x00000002 },
{ 0x15, 0, 3, 0xc2000010 },
{ 0x28, 0, 0, 0x00000000 },
{ 0x15, 0, 1, 0x00000180 },
{ 0x6, 0, 0, 0x0000ffff },
{ 0x6, 0, 0, 0x00000000 },
};

/* Berkeley Packet filter code to filter out spanning tree packets.
   from tcpdump -dd -e -i eth0 ether dst 01:10:b5:00:00:01
 */
static struct sock_filter stp_filter3[] = {
{ 0x20, 0, 0, 0x00000002 },
{ 0x15, 0, 3, 0xb5000001 },
{ 0x28, 0, 0, 0x00000000 },
{ 0x15, 0, 1, 0x00000110 },
{ 0x6, 0, 0, 0x0000ffff },
{ 0x6, 0, 0, 0x00000000 },
};
/* Berkeley Packet filter code to filter out spanning tree packets.
   from tcpdump -dd -e -i eth0 ether dst 01:10:b5:00:00:02
 */
static struct sock_filter stp_filter4[] = {
{ 0x20, 0, 0, 0x00000002 },
{ 0x15, 0, 3, 0xb5000002 },
{ 0x28, 0, 0, 0x00000000 },
{ 0x15, 0, 1, 0x00000110 },
{ 0x6, 0, 0, 0x0000ffff },
{ 0x6, 0, 0, 0x00000000 },
};
/*
static struct sock_filter stp_filter5[] = {
{ 0x20, 0, 0, 0x00000002 },
{ 0x15, 0, 3, 0x33445566 },
{ 0x28, 0, 0, 0x00000000 },
{ 0x15, 0, 1, 0x00001122 },
{ 0x6, 0, 0, 0x0000ffff },
{ 0x6, 0, 0, 0x00000000 },
};
*/

static void bpdu_dump_packet(const unsigned char *buf, int cc)
{
	int i, j;
	for (i = 0; i < cc; i += 16) {
		for (j = 0; j < 16 && i + j < cc; j++)
			DEBUG_PRINT(" %02x", buf[i + j]);
		DEBUG_PRINT("\n\r");
	}
	DEBUG_PRINT("\n\r");
	fflush(stdout);
}



static void bpdu_packet_rcv(uint32_t events, struct epoll_event_handler *h)
{
	int cc;
	unsigned char buf[2048];
	struct sockaddr_ll sl;
	socklen_t salen = sizeof sl;

	cc = recvfrom(h->fd, &buf, sizeof(buf), 0, (struct sockaddr *) &sl, &salen);
	if (cc <= 0) {
		perror("recvfrom failed: %m");
              DEBUG_PRINT("(%s:%d) cc:%d\n\r",__FUNCTION__,__LINE__,cc);
		return;
	}
#ifdef DEBUG_MODE
	DEBUG_PRINT("Receive Src ifindex %d %02x:%02x:%02x:%02x:%02x:%02x\n\r",
	       sl.sll_ifindex, 
	       sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
	       sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);
    if(sl.sll_addr[3]==0x22&&sl.sll_addr[4]==0x22&&(sl.sll_addr[5]==0x39||sl.sll_addr[5]==0x25))
	bpdu_dump_packet(buf, cc);
#endif

	chassis_bpdu_rx_bpdu(sl.sll_ifindex, buf, cc);
}


int bpdu_initial_socket(void)
{
    char vlan_if[256];
    int us_number = chassis_information.us_card_number;
    struct packet_mreq mreq;
    struct ifreq ifopts; /* set promiscuous mode */
    struct sock_fprog prog = {
		.len = sizeof(stp_filter) / sizeof(stp_filter[0]),
		.filter = stp_filter,
    };
    struct sock_fprog prog3 = {
		.len = sizeof(stp_filter3) / sizeof(stp_filter3[0]),
		.filter = stp_filter3,
    };
    struct sock_fprog prog4 = {
		.len = sizeof(stp_filter4) / sizeof(stp_filter4[0]),
		.filter = stp_filter4,
    };
    
    socket_fd=socket(PF_PACKET,SOCK_RAW, htons(ETH_P_ALL));

    if(socket_fd < 0){
        perror("setsockopt");
        return socket_fd;
    }

/* Set interface to promiscuous mode - do we need to do this every time? */
    memset(vlan_if,0,256);
    sprintf(vlan_if,"%s.%d",DEFAULT_IF,chassis_information.internal_tag_vlan);

    DEBUG_PRINT("VLAN interface is %s\r\n",vlan_if);
    strncpy(ifopts.ifr_name, vlan_if,sizeof DEFAULT_IF);
    ioctl(socket_fd, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(socket_fd, SIOCSIFFLAGS, &ifopts);


    mreq.mr_ifindex = if_nametoindex(vlan_if);
    mreq.mr_type = PACKET_MR_PROMISC;
    mreq.mr_alen = 6;

    if (setsockopt(socket_fd,SOL_PACKET,PACKET_ADD_MEMBERSHIP,
         (void*)&mreq,(socklen_t)sizeof(mreq)) < 0)
        return -3;
    if((1<=us_number&&us_number<=16)||(41<=us_number&&us_number<=48)){
	if (setsockopt(socket_fd, SOL_SOCKET, SO_ATTACH_FILTER, &prog3, sizeof(prog)) < 0) 
		perror("setsockopt packet filter failed: %m");
    }
    else{
	if (setsockopt(socket_fd, SOL_SOCKET, SO_ATTACH_FILTER, &prog4, sizeof(prog)) < 0) 
		perror("setsockopt packet filter failed: %m"); 
    }

    if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) < 0)
	perror("fcntl set nonblock failed: %m");

    else {
	packet_event.fd = socket_fd;
	packet_event.handler = bpdu_packet_rcv;

        if (epoll_loop_add_epoll(&packet_event) == 0){
            DEBUG_PRINT("Initial BPDU socket. \n\r");
            return socket_fd;
        }
        else{
            DEBUG_PRINT("Initial BPDU socket Failure. \n\r");
        }
    }

    close(socket_fd);
    return -1;
    
}

int bpdu_close_socket(void)
{
    close(socket_fd);
    socket_fd =0;
    return 0;
}



int bpdu_client_send(char*mac,char* sendline,int size)
{
    int us_number = chassis_information.us_card_number;
    int bytes_sent=0;
    int sockfd;
    struct sockaddr_ll servaddr;
    unsigned char src_addr[6];
    char vlan_if[256];

    if (mac == NULL)
    {
      printf("usage:  udpcli <IP address>\n\r");
      return -1;
    }

    sockfd = socket_fd;

    memset(&servaddr, 0, sizeof(struct sockaddr_ll));
    servaddr.sll_family = AF_PACKET;
    servaddr.sll_protocol = htons(ETH_P_802_2);
    memset(vlan_if,0,256);
    sprintf(vlan_if,"%s.%d",DEFAULT_IF,chassis_information.internal_tag_vlan);
    servaddr.sll_ifindex = if_nametoindex(vlan_if);
    servaddr.sll_halen = ETH_ALEN;
    memcpy(servaddr.sll_addr,mac,sizeof(servaddr.sll_addr));

    /*Initial BPDU*/
       
    netif_utils_get_hwaddr(vlan_if, src_addr);
    memcpy(bpdu_packet.mac.src_mac, src_addr, 6);
    memcpy(bpdu_packet.mac.dst_mac, mac, 6);
    memcpy(bpdu_packet.body.root_id+2, src_addr, 6);
    if(chassis_information.type==FABRIC_CARD_TYPE)
        bpdu_packet.body.root_id[1] = us_number;
    else 
        bpdu_packet.body.root_id[1] = 0;
    memcpy(bpdu_packet.body.bridge_id+2, src_addr, 6);
    if(chassis_information.type==LINE_CARD_TYPE)
        bpdu_packet.body.bridge_id[1] = us_number;
    else
        bpdu_packet.body.bridge_id[1] = 0;
    bpdu_packet.body.port_id[1]=us_number;
    size = sizeof(RSTP_BPDU_T);
    bpdu_packet.eth.len8023[1] =0x26;
    if(size<64) size =64;

        bytes_sent=sendto(sockfd,sendline,size,0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
        
    DEBUG_PRINT("Send BPDU n:%d \n", bytes_sent);
    
    return bytes_sent;
}

int bpdu_client_select(int sockfd)
{
    int n;
    fd_set fds;
    struct timeval timeout={TIME_CHASSIS_PING,0};
    
    FD_ZERO(&fds); 
    FD_SET(sockfd,&fds);
    
    if((n=select(sockfd+1,&fds,NULL,NULL,&timeout))<0)
    {
        DEBUG_PRINT("No bpdu reply packet\n");
        return 0;
    }

   return n;
}

int bpdu_client_recv(int sockfd,char* recvline,int time)
{
    int n=0;
    int saddr_size , data_size=0;
    struct sockaddr_ll saddr;
    /* Header structures */
    struct ether_header *eh = (struct ether_header *) recvline;
    
    struct timeval timeout={time,0};
    fd_set fds;
    unsigned char sll_addr_1[8]={0x1,0x80,0xc2,0x0,0x0,0x22};
    unsigned char sll_addr_2[8]={0x1,0x80,0xc2,0x0,0x0,0x11};


    sockfd = socket_fd;
    FD_ZERO(&fds); 
    FD_SET(sockfd,&fds);   

    if((n=select(sockfd+1,&fds,NULL,NULL,&timeout))<=0)
    {
        DEBUG_PRINT("No reply packet\n");
        return 0;
    }
    DEBUG_PRINT("BPDU select: n =%d\n\r",n);
    saddr_size = sizeof (struct sockaddr_ll);

    data_size = read(sockfd, recvline, 10000);
    if (data_size < 0) {
	if (errno == EAGAIN)
	    return 0;
	perror("rawsock_output: read failed");
	return -1;
    }
    DEBUG_PRINT("BPDU select: data_size =%d\n\r",data_size);
    if(n>0){
        DEBUG_PRINT("Recevie %d udp client packet!\n\r",n);
        bzero(recvline,10000);
        data_size=recvfrom(sockfd,recvline,10000,0,(struct sockaddr *) &saddr , (socklen_t*)&saddr_size);
        
    }
	/* Check the packet is for me */
	if (eh->ether_dhost[0] == sll_addr_1[0] &&
			eh->ether_dhost[1] == sll_addr_1[1] &&
			eh->ether_dhost[2] == sll_addr_1[2] &&
			eh->ether_dhost[3] == sll_addr_1[3]&&
			eh->ether_dhost[4] == sll_addr_1[4] &&
			(eh->ether_dhost[5] == sll_addr_1[5]||eh->ether_dhost[5] == sll_addr_2[5])) {
		DEBUG_PRINT("Correct destination MAC address\n\r");
	}
	DEBUG_PRINT("Transmit Dst index %d %02x:%02x:%02x:%02x:%02x:%02x\n\r",
	       saddr.sll_ifindex, 
	       saddr.sll_addr[0], saddr.sll_addr[1], saddr.sll_addr[2],
	       saddr.sll_addr[3], saddr.sll_addr[4], saddr.sll_addr[5]);

	bpdu_dump_packet((unsigned char *)recvline, data_size);

    DEBUG_PRINT("(%s:%d) n:%d,size:%d\n",__FUNCTION__,__LINE__,n,data_size);
    return data_size;
}

int bpdu_client_close(int sockfd)
{
    close(sockfd);
    sockfd =0;
    return 0;
}


