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

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include "udp_client.h"
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include "chassis.h"

#define PORT 0xFBFB

static int socket_fd=0;

#define DEBUGLFAG 0

#if (DEBUGLFAG == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("udp_client:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

int udp_client_initial_socket(void)
{
    socket_fd=socket(AF_INET,SOCK_DGRAM,0);

    return 0;
}

int udp_client_close_socket(void)
{
    close(socket_fd);
    socket_fd =0;
    return 0;
}


int udp_client_send(char*ipaddr,char* sendline,int size)
{
   int sockfd;
   struct sockaddr_in servaddr;

   if (ipaddr == NULL)
   {
      printf("usage:  udpcli <IP address>\n");
      return -1;
   }
   sockfd = socket_fd;

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(ipaddr);
   servaddr.sin_port=htons(PORT);

      sendto(sockfd,sendline,size,0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
   
   return sockfd;
}

int udp_client_select(int sockfd)
{
    int n;
    fd_set fds;
    struct timeval timeout={TIME_CHASSIS_PING,0};
    
    FD_ZERO(&fds); 
    FD_SET(sockfd,&fds);
    
    if((n=select(sockfd+1,&fds,NULL,NULL,&timeout))<0)
    {
        DEBUG_PRINT("No udp reply packet\n");
        return 0;
    }

   return n;
}

int udp_client_recv(int sockfd,char* recvline,int time)
{
    int n=0;
    struct timeval timeout={time,0};
    fd_set fds;
    
    FD_ZERO(&fds); 
    FD_SET(sockfd,&fds);   
    if((n=select(sockfd+1,&fds,NULL,NULL,&timeout))<0)
    {
       DEBUG_PRINT("No udp reply packet\n");
        return 0;
    }

    if(n>0){
        DEBUG_PRINT("Recevie %d udp client packet!\n\r",n);
        bzero(recvline,10000);
        n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
        
    }

    DEBUG_PRINT("(%s:%d) n:%d\n",__FUNCTION__,__LINE__,n);
    return n;
}

int udp_client_close(int sockfd)
{
    close(sockfd);
    sockfd =0;
    return 0;
}
