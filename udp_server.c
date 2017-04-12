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

#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "udp_server.h"
#include  <sys/types.h> 
#include  <sys/times.h> 
#include  <sys/select.h>

#include "epoll_loop.h"
#include "shutdown_mgr.h"

#define DEBUGLFAG 0

#if (DEBUGLFAG == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("udp_server:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif


#define PORT 0xFBFB
#define BUFSIZE 2048

static struct epoll_event_handler packet_event;

extern CHASSIS_DATA_T chassis_information;
extern US_HW_DATA_T us_hw_information[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
extern int  tcp_server_doShutdownToClinet(int  slotid, int option);

static uint16_t last_payload;

int udp_server_do_system_command(unsigned int command, char* params);
int udp_server_do_shutdown_mgr_shutdown_linecard_local(unsigned int command, char* params);


int udp_server_do_parse_udp(char* packet, int length);
static void upd_server_packet_recev(uint32_t events, struct epoll_event_handler *h);

int udp_server_do_system_command(unsigned int command, char* params)
{
#if 1
    char buffer[MAXIMUM_BUFFER_LENGTH]= {0};
    char command_buf[MAXIMUM_BUFFER_LENGTH]= {0};
    char buf[MAXIMUM_BUFFER_LENGTH] = {0};
    FILE *filep = NULL;
    FILE *pp = NULL;
    char in_string_buf[MAXIMUM_BUFFER_LENGTH] = {0};
#define PREFIX_1 "cd /usr/local/accton/bin/; export PATH=$PATH:/usr/local/accton/bin ;"

    /*Keep log in local card*/
    
		if (system("mkdir -p /usr/local/accton/log") != 0)
			return 1;
		if((pp = popen("date +%Y%m%d%H%M%S", "r")) == NULL) /* EX: 20140521133509 */
			return 1;
		if (fgets(buf, sizeof(buf), pp) == NULL)
		{
			pclose(pp);
			fflush(stdout);
			return 1;
		}
		pclose(pp);
		buf[14] = buf[15] = 0;
		snprintf(command_buf, MAXIMUM_STRING_LENGTH, "%s%u-%s.log", "/usr/local/accton/log/chassis_test_log-", command, buf);
    /**/   
    if(params==NULL)
    {
        sprintf(buffer,PREFIX_1"/usr/local/accton/bin/diag_main -m %d -e 0x80 > %s",command,command_buf);
    }
    else if(strlen(params)== 0){
        sprintf(buffer,PREFIX_1"/usr/local/accton/bin/diag_main -m %d -e 0x80 > %s",command,command_buf);
    }
    else{
        sprintf(buffer,PREFIX_1"/usr/local/accton/bin/diag_main -m %d -s %s -e 0x80 > %s",command,params,command_buf);
    }    
    
    sprintf(buffer,"/usr/local/accton/bin/diag_main -m %d -e 0x80 ",command);
    DEBUG_PRINT("%s\n",buffer);

    if (system(buffer) != 0)
    {
        printf("chassis execute diag_main fail!\n\r");
        return 4;
    }
    if ((filep = fopen(command_buf, "r")) == NULL)
    {
        printf("fopen(%s): Error!\r\n", command_buf);
        return 4;
    }
    
    while (fgets(in_string_buf, MAXIMUM_STRING_LENGTH, filep) != NULL)
    {
        if(strstr(in_string_buf, " FAILED")!=0)
        {
            DEBUG_PRINT("chassis execute FAILED!");
            return 1;
        }
        if(strstr(in_string_buf, "Utility doesn't support this mode ID")!=0)
        {
            DEBUG_PRINT("chassis execute not supoort!");
            return 2;
        }
    }
    fclose(filep);
    DEBUG_PRINT("chassis execute success!\n\r");
#endif
    return 0;
}

int udp_server_do_shutdown_mgr_shutdown_linecard_local(unsigned int command, char* params)
{
    int lc_slot_id =2;
    int option = 0;
    const char s[2] = " ";
    char *token;

    DEBUG_PRINT("command:%d, params:%s\n\r",command,params);

    /* get the first token */
    token = strtok(params, s);
    if(token!=NULL)
        lc_slot_id = atoi(token);

    token = strtok(NULL, s);
    if(token!=NULL)
        option = atoi(token);

    DEBUG_PRINT("slot:%d, option:%d\n\r",lc_slot_id,option);

    switch (chassis_information.us_card_number)
    {
        case 33:
        case 35:
        case 41:
        case 43:
            if(lc_slot_id>3)
                return tcp_server_do_shutdown_to_client(lc_slot_id, option);
            else
                return shutdown_mgr_power_linecard_local(lc_slot_id,option);            
            break;
        case 37:
        case 39:
        case 45:
        case 47:
            if(lc_slot_id<4)
                return tcp_server_do_shutdown_to_client(lc_slot_id, option);
            else
                return shutdown_mgr_power_linecard_local(lc_slot_id,option);            
            break;
            break;
        default:
            return -1;
    }
       
    return 0;
}


int udp_server_do_parse_udp(char* packet, int length)
{
    struct udp_packet_command_header header;
    
    memcpy(&header,packet,sizeof(struct udp_packet_command_header));
    switch(header.type)
    {
        case PING_TEST_REQUEST:
            /*update DB*/
            chassis_information.us_card[header.us_card_number-1].status |= CHASSIS_PING_READY;
            break;
        case POWER_CYCLE_REQUEST:
            /*update DB*/
            chassis_information.us_card[header.us_card_number-1].status |= CHASSIS_POWER_READY;
            break;        
        case HW_INFORMATION_REQUEST:
            /*update DB*/
            memcpy(&(us_hw_information[header.us_card_number-1]), packet+sizeof(struct udp_packet_command_header),sizeof(US_HW_DATA_T));
            chassis_information.us_card[header.us_card_number-1].status |= CHASSIS_HWINFO_READY;
            break;
        default:
            DEBUG_PRINT("(%s:%d) receive udp request type:%d\n",__FUNCTION__,__LINE__,header.type);
            return header.type;
    }
    DEBUG_PRINT("(%s:%d) receive udp request type:%d\n",__FUNCTION__,__LINE__,header.type);
    return header.type;
}

int
upd_server_init(void)
{
	struct sockaddr_in myaddr;      /* our address */
        int fd;                         /* our socket */

        /* create a UDP socket */
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("cannot create socket\n\r");
                return 0;
        }

        /* bind the socket to any valid IP address and a specific port */
        bzero(&myaddr,sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        myaddr.sin_port = htons(PORT);

        if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
                perror("bind failed");
                return 0;
        }

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		perror("fcntl set nonblock failed: %m");

	else {
		packet_event.fd = fd;
		packet_event.handler = upd_server_packet_recev;

		if (epoll_loop_add_epoll(&packet_event) == 0){
                  printf("Initial UDP Server socket. \n\r");
			return fd;
	        }
            else{
                printf("Initial UDP Server Failure. \n\r");
            }
	}

	close(fd);
	return -1;

}
static void upd_server_packet_recev(uint32_t events, struct epoll_event_handler *h)
{
    upd_server_recev(h->fd);
}



int
upd_server_recev(int fd)
{
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
    int recvlen;                    /* # bytes received */
    char buf[BUFSIZE]={0};     /* receive buffer */
    char mesg[BUFSIZE]="this is udp server response";
    char params[256];
    fd_set fds;
    struct timeval timeout={3,0};
    int n;
    int i,type,length;
    uint16_t payload;
    struct udp_packet_command_header header;
    struct udp_packet_command_header* header_ptr; 

    FD_ZERO(&fds); 
    FD_SET(fd,&fds);
    
    memset(params,256,0);
    memset(&header,0,sizeof(struct udp_packet_command_header));
    
    if((n=select(fd+1,&fds,NULL,NULL,&timeout))<0)
    {
        return 0;
    }
    /* now loop, receiving data and printing what we received */
    if(n>0){
        ;//printf("Recevie %d udp server packet!\n\r",n);
    }
    for (i=0;i<n;i++) 
    {
        if(FD_ISSET(fd, &fds)){
            recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
            if (recvlen > 0) {
                type=udp_server_do_parse_udp(buf,recvlen);
                switch(type)
                {
                    case PING_TEST_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = PING_TEST_REPLY;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        if((payload=udp_server_do_system_command(PING_TEST_REQUEST,NULL))!=0) {
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                            msg_lprintf("(%s:%d)\n\r",__FUNCTION__,__LINE__);
                        }
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        break;
                    case POWER_CYCLE_REQUEST:
                    /* reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = POWER_CYCLE_REPLY;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        if((payload=udp_server_do_system_command(POWER_CYCLE_REQUEST,NULL))!=0){ 
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t)); 
                            msg_lprintf("(%s:%d)\n\r",__FUNCTION__,__LINE__);
                        }
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        break;        
                    case HW_INFORMATION_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = HW_INFORMATION_REPLY;
                        header.length = sizeof(US_HW_DATA_T);
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&(us_hw_information[header.us_card_number-1]),sizeof(US_HW_DATA_T));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        break;
                    case CTP_INFORMATION_REQUEST:
                        memset(mesg,0,BUFSIZE);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = CTP_INFORMATION_REPLY;
                        header.length = sizeof(CTPM_T);
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&chassis_information.ctpm,sizeof(CTPM_T));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        DEBUG_PRINT("chassis master:%d:%d:%d:%d\n\r",
                            chassis_information.local_Master[0],
                            chassis_information.local_Master[1],
                            chassis_information.local_Master[2],
                            chassis_information.local_Master[3]);
                        break;
                   case CTP_REMOTESHUTDOWN_REQUEST:
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = CTP_REMOTESHUTDOWN_REPLY;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));

                        header_ptr = (struct udp_packet_command_header*)buf;
                        if(header_ptr->params_length>0) 
                        {
                            memcpy(params,header_ptr->params,header_ptr->params_length);                            
                        }
                        if((payload=udp_server_do_shutdown_mgr_shutdown_linecard_local(type,params))!=0) {
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        }
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        break;
                    case TRAFFIC_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = TRAFFIC_REPLY;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        payload  = RESULT_ACK;
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        //printf("reply traffic ack\n\r");
#if 1
                        header_ptr = (struct udp_packet_command_header*)buf;
                        if(header_ptr->params_length>0) 
                        {
                            memcpy(params,header_ptr->params,header_ptr->params_length);                            
                        }
                        header.type = TRAFFIC_RESULT;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        payload=udp_server_do_system_command(type,params);
                        last_payload = payload;
                            if(payload==0) payload =RESULT_SUCCESS;
                            else payload = RESULT_FAILED;
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        //printf("reply traffic result(%d:%d)\n\r",last_payload,payload);
#endif                        
                        break;
                    case TRAFFIC_RESULT_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = TRAFFIC_RESULT;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        if(last_payload==0) payload =RESULT_SUCCESS;
                        else payload = RESULT_FAILED;
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        //printf("reply traffic result(%d:%d)\n\r",last_payload,payload);                       
                        break;
                    case HEALTH_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = HEALTH_REPLY;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        payload  = RESULT_ACK;
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        //printf("reply traffic ack\n\r");
#if 1
                        header_ptr = (struct udp_packet_command_header*)buf;
                        if(header_ptr->params_length>0) 
                        {
                            memcpy(params,header_ptr->params,header_ptr->params_length);                            
                        }
                        header.type = HEALTH_RESULT;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        payload=udp_server_do_system_command(type,params);
                        last_payload = payload;
                            if(payload==0) payload =RESULT_SUCCESS;
                            else payload = RESULT_FAILED;
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        //printf("reply traffic result(%d:%d)\n\r",last_payload,payload);
#endif                        
                        break;
                    case HEALTH_RESULT_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = HEALTH_RESULT;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        if(last_payload==0) payload =RESULT_SUCCESS;
                        else payload = RESULT_FAILED;
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        DEBUG_PRINT("reply traffic result(%d:%d)\n\r",last_payload,payload);                       
                        break;
                    case ANYTOANY_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = ANYTOANY_REPLY;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        payload  = RESULT_ACK;
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        DEBUG_PRINT("reply any to any ack\n\r");

                        header_ptr = (struct udp_packet_command_header*)buf;
                        if(header_ptr->params_length>0) 
                        {
                            memcpy(params,header_ptr->params,header_ptr->params_length);
                            params[header_ptr->params_length]=0;
                            DEBUG_PRINT("params:%s, %s\n\r", params,header_ptr->params);
                        }
                        header.type = ANYTOANY_RESULT;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        payload=udp_server_do_system_command(type,params);
                        last_payload = payload;
                            if(payload==0) payload =RESULT_SUCCESS;
                            else payload = RESULT_FAILED;
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        DEBUG_PRINT("reply any to any result(%d:%d)\n\r",last_payload,payload);
                        
                        break;
                    case ANYTOANY_RESULT_REQUEST:
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = ANYTOANY_RESULT;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));
                        if(last_payload==0) payload =RESULT_SUCCESS;
                        else payload = RESULT_FAILED;
                        memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                        DEBUG_PRINT("reply any to any result(%d:%d)\n\r",last_payload,payload);                       
                        break;
                    default:
                        /*other only return result, success or not*/
                        /*reply*/
                        memset(mesg,0,BUFSIZE);
                        memset(params,0,256);
                        header.us_card_number = chassis_information.us_card_number;
                        header.type = type;
                        header.length = 2;
                        length= sizeof(struct udp_packet_command_header) + header.length;
                        memcpy(mesg,&header,sizeof(struct udp_packet_command_header));

                        header_ptr = (struct udp_packet_command_header*)buf;
                        if(header_ptr->params_length>0) 
                        {
                            memcpy(params,header_ptr->params,header_ptr->params_length);                            
                        }
                        if((payload=udp_server_do_system_command(type,params))!=0) {
                            memcpy(mesg+sizeof(struct udp_packet_command_header),&payload,sizeof(uint16_t));
                        }
                        sendto(fd,mesg,length,0,(struct sockaddr *)&remaddr,sizeof(remaddr));
                     
                        continue;
                }
            }
        }
    }
    /* never exits */
    return 0;
}

int
upd_server_close(int fd)
{
    if(fd>0)
        close(fd);
    return 0;
}


