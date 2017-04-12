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
#include <strings.h>
#include  "tcp_server.h"
#include  <sys/types.h> 
#include  <sys/times.h> 
#include  <sys/select.h>
#include  "udp_client.h"
#include  "udp_server.h"
#include  "crc_util.h"

#define DEBUGLFAG 0

#if (DEBUGLFAG == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("tcp_server:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#define PORT 0xFBFB
#define BUFSIZE 2048

int tcp_server_hw_information_init(void);
int tcp_server_do_parsing(uint8_t *data, uint32_t size,struct tcp_packet_command_header* header);
int tcp_server_do_ping_test_request_member(CHASSIS_PING_TEST_T* ping_status);
int tcp_server_do_power_cycle_request_member(CHASSIS_POWER_CYCLE_T* power_status);
int tcp_server_do_hw_information_request_member(CHASSIS_HW_TEST_T* hw_status);
int tcp_server_do_traffic_request_member(uint16_t option, struct tcp_packet_command_traffic_payload* traffic_status);

void tcp_server_do_ping_test_request (int sock,char* buffer);
void tcp_server_do_power_cycle_request (int sock,char* buffer);
void tcp_server_do_hw_information_request (int sock,char* buffer);
void tcp_server_do_traffic_request (int sock,char* buffer);
int tcp_server_do_traffic_test_line_card(uint16_t option, struct tcp_packet_command_traffic_payload* traffic_status);
int tcp_server_do_traffic_test_fabric_card(uint16_t option, struct  tcp_packet_command_traffic_payload* traffic_status);
int tcp_server_do_health_request_member(struct tcp_packet_command_traffic_payload* traffic_status);
void tcp_server_do_health_request (int sock,char* buffer);
void tcp_server_do_anytoany_request (int sock,char* buffer);
int tcp_server_do_anytoany_request_member(struct tcp_packet_command_anytoany_command_parameter* paras, struct tcp_packet_command_traffic_payload* traffic_status);
int tcp_server_do_check_anytoany_packet(char* recvline,unsigned int *result);

void tcp_server_do_error_command (int sock,char* buffer);
int tcp_server_do_send_ping_test_to_client(int usNumber);
int tcp_server_do_send_powe_cycle_to_client(int usNumber);
int tcp_server_do_send_hw_inform_to_client(int usNumber);
int tcp_server_do_send_traffic_to_client(int usNumber,uint16_t option);
int tcp_server_do_check_traffic_packet(char* recvline,unsigned int *result);
int tcp_server_do_check_health_packet(char* recvline,unsigned int *result);
int tcp_server_do_send_health_test_to_client(int usNumber,uint16_t option);
int tcp_server_do_send_anytoany_to_client(int usNumber,struct tcp_packet_command_anytoany_command_parameter* paras, uint16_t type);

int tcp_server_do_send_shutdown_to_client(int usNumber,uint16_t slotid, uint16_t option);
int tcp_server_do_shutdown_to_client(int slotid, int option);

int tcp_server_do_check_udp_client_packet(char* recvline,int type);
int tcp_server_get_chassis_member_count(void);
void tcp_server_do_update_hw_inform(char* recvline);


extern CHASSIS_DATA_T chassis_information;

int tcp_server_hw_information_init(void)
{
#if 0    
    US_HW_DATA_T *hw_inform=NULL;
    int i;
    
    for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
    {
        hw_inform=&us_hw_information[i];
        memset(hw_inform,0,sizeof(struct US_HW_DATA_T));
        hw_inform->us_card_number=1;
        strcpy(hw_inform->bios_vender,"American Megatrends Inc.");
        strcpy(hw_inform->bios_version,"MF1_3A01");
        strcpy(hw_inform->bios_revision,"5.6");
        strcpy(hw_inform->bios_fw_revision,"0.9");
        strcpy(hw_inform->cpu_manufacturer,"Intel(R) Corporation");
        strcpy(hw_inform->cpu_version,"Intel(R) Atom(TM) CPU  C2550  @ 2.40GHz");
        strcpy(hw_inform->cpu_clock,"100 MHz");
        strcpy(hw_inform->cpu_core,"4");
        strcpy(hw_inform->cpu_maxspeed,"2400 MHz");
        strcpy(hw_inform->dimm_size,"8192 MB");
        strcpy(hw_inform->dimm_factor,"SODIMM");
        strcpy(hw_inform->dimm_type,"DDR3");
        strcpy(hw_inform->dimm_speed,"1600 MHz");
        strcpy(hw_inform->dimm_manufacturer,"Micron");
        strcpy(hw_inform->dimm_sn,"25121693");
        strcpy(hw_inform->dimm_pn,"18KSF1G72HZ-1G6E2");
        strcpy(hw_inform->ssd_vender,"INTEL SSDMCEAW120A4");
        strcpy(hw_inform->ssd_sn,"CVDA4233015Q120P");
        strcpy(hw_inform->ssd_fw_version,"DC33");
    }
#else

int ret = 1;
char buf[MAXIMUM_BUFFER_LENGTH] = {0}, buf1[MAXIMUM_BUFFER_LENGTH] = {0};
US_HW_DATA_T us_hw_data;
int us_nmuber = chassis_information.us_card_number;


	sprintf(buf, "dmidecode -t 0 -t 4 -t 17 > %s", DIAG_DEFAULT_INFORMATIN_FILE);
	ret = system(buf);
	if (ret != 0)
	{
		DEBUG_PRINT("dmidecode(%d): Error!\r\n", ret);
		fflush(stdout);
		return -1;
	}
	sprintf(buf, "/usr/local/accton/bin/linuxmcli64 -d | grep -A 3 -e \"Model Number:\" > %s", DIAG_DEFAULT_INFORMATIN_FILE_1);
	ret = system(buf);
	if (ret != 0)
	{
		DEBUG_PRINT("linuxmcli64(%d): Error!\r\n", ret);
		fflush(stdout);
		return -1;
	}

	memset(&us_hw_data, 0, sizeof(us_hw_data));
	ret = tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, ":,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 7, 2);
	strncpy(us_hw_data.bios_vender, buf, sizeof(us_hw_data.bios_vender));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 8, 2);
	strncpy(us_hw_data.bios_version, buf, sizeof(us_hw_data.bios_version));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 27, 3);
	strncpy(us_hw_data.bios_revision, buf, sizeof(us_hw_data.bios_revision));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 28, 3);
	strncpy(us_hw_data.bios_fw_revision, buf, sizeof(us_hw_data.bios_fw_revision));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, ":,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 35, 2);
	strncpy(us_hw_data.cpu_manufacturer, buf, sizeof(us_hw_data.cpu_manufacturer));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, ":,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 67, 2);
	strncpy(us_hw_data.cpu_version, buf, sizeof(us_hw_data.cpu_version));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 69, 3);
	strncpy(us_hw_data.cpu_clock, buf, sizeof(us_hw_data.cpu_clock));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 80, 3);
	strncpy(us_hw_data.cpu_core, buf, sizeof(us_hw_data.cpu_core));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 70, 3);
	strncpy(us_hw_data.cpu_maxspeed, buf, sizeof(us_hw_data.cpu_maxspeed));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 97, 2);
	strncpy(us_hw_data.dimm_size, buf, sizeof(us_hw_data.dimm_size));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 98, 3);
	strncpy(us_hw_data.dimm_factor, buf, sizeof(us_hw_data.dimm_factor));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 102, 2);
	strncpy(us_hw_data.dimm_type, buf, sizeof(us_hw_data.dimm_type));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 104, 2);
	strncpy(us_hw_data.dimm_speed, buf, sizeof(us_hw_data.dimm_speed));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 105, 2);
	strncpy(us_hw_data.dimm_manufacturer, buf, sizeof(us_hw_data.dimm_manufacturer));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 106, 3);
	strncpy(us_hw_data.dimm_sn, buf, sizeof(us_hw_data.dimm_sn));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 108, 3);
	strncpy(us_hw_data.dimm_pn, buf, sizeof(us_hw_data.dimm_pn));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE_1, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 1, 3);
	if (strstr(buf, "INTEL"))
	{
		ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE_1, " :,\r\n", buf1, MAXIMUM_BUFFER_LENGTH, 1, 4);
		sprintf(us_hw_data.ssd_vender, "%s %s", buf, buf1);
	}
	else
		strncpy(us_hw_data.ssd_vender, buf, sizeof(us_hw_data.ssd_vender));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE_1, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 2, 3);
	strncpy(us_hw_data.ssd_sn, buf, sizeof(us_hw_data.ssd_sn));

	ret |= tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE_1, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 4, 3);
	strncpy(us_hw_data.ssd_fw_version, buf, sizeof(us_hw_data.ssd_fw_version));

        us_hw_data.us_card_number = chassis_information.us_card_number;
#if 0
	if (ret == 0)
	{
	       printf("US Card Number      :%d\r\n", us_hw_data.us_card_number); 
		printf("BIOS Vender           :%s\r\n", us_hw_data.bios_vender);
		printf("BIOS Version          : %s\r\n", us_hw_data.bios_version);
		printf("BIOS Revision         : %s\r\n", us_hw_data.bios_revision);
		printf("BIOS FW Revision      : %s\r\n", us_hw_data.bios_fw_revision);
		printf("CPU Manufacturer      :%s\r\n", us_hw_data.cpu_manufacturer);
		printf("CPU Version           :%s\r\n", us_hw_data.cpu_version);
		printf("CPU Clock             : %s MHz\r\n", us_hw_data.cpu_clock);
		printf("CPU Core              : %s Core\r\n", us_hw_data.cpu_core);
		printf("CPU Max Speed         : %s MHz\r\n", us_hw_data.cpu_maxspeed);
		printf("DIMM Size             : %s MB\r\n", us_hw_data.dimm_size);
		printf("DIMM Factor           : %s\r\n", us_hw_data.dimm_factor);
		printf("DIMM Type             : %s\r\n", us_hw_data.dimm_type);
		printf("DIMM Speed            : %s MHz\r\n", us_hw_data.dimm_speed);
		printf("DIMM Manufacturer     : %s\r\n", us_hw_data.dimm_manufacturer);
		printf("DIMM SN               : %s\r\n", us_hw_data.dimm_sn);
		printf("DIMM PN               : %s\r\n", us_hw_data.dimm_pn);
		printf("SSD Vender            : %s\r\n", us_hw_data.ssd_vender);
		printf("SSD SN                : %s\r\n", us_hw_data.ssd_sn);
		printf("SSD FW Version        : %s\r\n", us_hw_data.ssd_fw_version);
	}
#endif
    if (ret == 0)
    {
        memcpy((us_hw_information+(us_nmuber-1)),&us_hw_data,sizeof(US_HW_DATA_T));
        chassis_information.status |= CHASSIS_HWINFO_READY;
        chassis_information.us_card[chassis_information.us_card_number-1].status |= CHASSIS_HWINFO_READY;
    }
    else{
        us_hw_information[chassis_information.us_card_number-1].us_card_number = chassis_information.us_card_number;
    }
    return ret;
#endif
    return 0;
}



int tcp_server_init(void)
{
    int sockfd;
    struct sockaddr_in serv_addr;
   
    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
   
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        return -1;
    }
   
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
   
    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        return -1;
    }
    listen(sockfd,5);

    tcp_server_hw_information_init();
    return sockfd;

}

int tcp_server_accept(int sockfd)
{
    int newsockfd;
    struct sockaddr_in cli_addr;
    socklen_t  clilen;
    pid_t pid;
    fd_set fds;
    struct timeval timeout={3,0};

    FD_ZERO(&fds); 
    FD_SET(sockfd,&fds);
    
   /* Now start listening for the clients, here
   * process will go in sleep mode and will wait
   * for the incoming connection
   */
    if(select(sockfd+1,&fds,&fds,NULL,&timeout)<=0)
    {
        DEBUG_PRINT("No client connection\n");
        return 0;
    }
   
    clilen = sizeof(cli_addr);
    DEBUG_PRINT("try to accept a connection\n");   
   
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0)
    {
        perror("ERROR on accept");
        return -1;
    }
    DEBUG_PRINT("accept a connection!\n");
    /* Create child process */
    pid = fork();
    if (pid < 0)
    {
        perror("ERROR on fork");
        return -1;
    }
      
    if (pid == 0)
    {
        /* This is the client process */         
        close(sockfd);
        tcp_server_do_processing(newsockfd);
        exit(0);
    }
    else
    {
         close(newsockfd);
    }
    
    return 0;
}

int tcp_server_close(int sock)
{
    if(sock>0)
        close(sock);
    return 0;
}

int  tcp_server_do_parsing(uint8_t *data, uint32_t size,struct tcp_packet_command_header* header)
{
    memcpy(header,data,sizeof(struct tcp_packet_command_header));
    if(header->magic!=0xFBFB)
    {
        return -1;
    }
    return header->commandID;
}

int   tcp_server_do_send_ping_test_to_client(int usNumber)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
     struct udp_packet_command_header sender;


    memset(ipaddr,0,20);
 
    if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status==1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = usNumber;
        sender.type = PING_TEST_REQUEST;
        sender.length=sizeof(int);   
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
       
      	 DEBUG_PRINT("send ping test request to ip:%s\n",ipaddr);
        return udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
    }
    return 0;
}

int  tcp_server_do_send_powe_cycle_to_client(int usNumber)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
     struct udp_packet_command_header sender;


    memset(ipaddr,0,20);
 

    if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status==1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = usNumber;
        sender.type = POWER_CYCLE_REQUEST;
        sender.length=sizeof(int);   
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
        
        DEBUG_PRINT("send power test request to ip:%s\n",ipaddr);
        return udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
    }
    return 0;
}

int  tcp_server_do_send_hw_inform_to_client(int usNumber)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
     struct udp_packet_command_header sender;
     US_HW_DATA_T hw_inform;

    memset(ipaddr,0,20);
    
    if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status==1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = chassis_information.us_card_number;
        sender.type = HW_INFORMATION_REQUEST;
        sender.length=sizeof(US_HW_DATA_T);
        memcpy(&hw_inform,&(us_hw_information[chassis_information.us_card_number-1]),sizeof(US_HW_DATA_T));
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
        memcpy(sendline+sizeof(struct udp_packet_command_header),&hw_inform,sizeof(US_HW_DATA_T));
     
        DEBUG_PRINT("send hwinformation request to ip:%s\n",ipaddr);
        return udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
    }
    return 0;
}

int  tcp_server_do_send_traffic_to_client(int usNumber,uint16_t option)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
     struct udp_packet_command_header sender;

    memset(ipaddr,0,20);

     if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status&=1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = usNumber;
        if(option==TRAFFIC_RESULT_REQUEST)
            sender.type = TRAFFIC_RESULT_REQUEST;
        else sender.type = TRAFFIC_REQUEST;
        sender.length=sizeof(int);
        sprintf(sender.params,"%d",option);
        sender.params_length = strlen(sender.params);
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
        
       DEBUG_PRINT("send traffic test request to ip:%s\n",ipaddr);
        return udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
    }
    return 0;
}

int  tcp_server_do_send_health_test_to_client(int usNumber,uint16_t option)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
     struct udp_packet_command_header sender;

    memset(ipaddr,0,20);

     if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status&=1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = usNumber;
        sender.type = option;
        sender.length=sizeof(int);
        sprintf(sender.params,"%d",option);
        sender.params_length = strlen(sender.params);
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
       
        DEBUG_PRINT("send health test request to ip:%s\n\r",ipaddr);
        return udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
    }
    return 0;
}

int  tcp_server_do_send_anytoany_to_client(int usNumber,struct tcp_packet_command_anytoany_command_parameter* paras, uint16_t type)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
    struct udp_packet_command_header sender;

    memset(ipaddr,0,20);
    memset(&sender,0,sizeof(struct udp_packet_command_header));

     if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status&=1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = usNumber;
        sender.type = type;
        sender.length=sizeof(int);

        sprintf(sender.params,"%d %d %2d %d %2d %d %d",paras->select,paras->input_card,paras->input_port,paras->output_card,paras->output_port,paras->src_bp,paras->dest_bp);
        sender.params_length = strlen(sender.params);
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
      
       DEBUG_PRINT("send any to any test request to ip:%s\n",ipaddr);
        return udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
    }
    return 0;
}

int  tcp_server_do_send_shutdown_to_client(int usNumber,uint16_t slotid, uint16_t option)
{
    char ipaddr[20];
    char sendline[BUFFSIZE];
    int i;
    int socket_id;
    
     struct udp_packet_command_header sender;

    memset(ipaddr,0,20);

     if(usNumber>0) i = usNumber-1;
    else return -1;
    /*if chassis member is exist*/
    if(chassis_information.us_card[i].status&=1)
    {
        sprintf(ipaddr,"%d.%d.%d.%d",
            chassis_information.us_card[i].ipaddress[0],
            chassis_information.us_card[i].ipaddress[1],
            chassis_information.us_card[i].ipaddress[2],
            chassis_information.us_card[i].ipaddress[3]);
        sender.us_card_number = usNumber;
        sender.type = CTP_REMOTESHUTDOWN_REQUEST;
        sender.length=sizeof(int);
        sprintf(sender.params,"%d %d",slotid,option);
        sender.params_length = strlen(sender.params);
        memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
        DEBUG_PRINT("send shutdown request to ip:%s\n",ipaddr);
        
        socket_id=udp_client_send(ipaddr,sendline,sizeof(struct udp_packet_command_header)+sender.length);
        if(socket_id==-1) return -1;
    }    
    
    return 0;
}

int tcp_server_do_check_udp_client_packet(char* recvline,int type)
{
    struct udp_packet_command_header receiver;
    unsigned int payload=0;
    
    memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
    
    if(receiver.type==PING_TEST_REPLY||receiver.type==POWER_CYCLE_REPLY)
        memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
    
    if(receiver.type==type&&payload==0) {        
        return receiver.us_card_number;
    }
    else return -1;
}

int tcp_server_do_check_traffic_packet(char* recvline,unsigned int *result)
{
    struct udp_packet_command_header receiver;
    unsigned int payload=0;
    
    memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
    
    if(receiver.type==TRAFFIC_RESULT){
        memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
        *result = payload;
        DEBUG_PRINT("receive %u traffic result reply\n\r ",receiver.us_card_number);
        return receiver.us_card_number;
    }
    else if(receiver.type==TRAFFIC_REPLY)
    {
         *result = RESULT_ACK;
         DEBUG_PRINT("receive %u traffic ack reply\n\r ",receiver.us_card_number);
        return receiver.us_card_number;
    }
    else return -1;
}

int tcp_server_do_check_health_packet(char* recvline,unsigned int *result)
{
    struct udp_packet_command_header receiver;
    unsigned int payload=0;
    
    memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
    
    if(receiver.type==HEALTH_RESULT){
        memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
        *result = payload;
        DEBUG_PRINT("receive %u traffic result reply\n\r ",receiver.us_card_number);
        return receiver.us_card_number;
    }
    else if(receiver.type==HEALTH_REPLY)
    {
         *result = RESULT_ACK;
         DEBUG_PRINT("receive %u traffic ack reply\n\r ",receiver.us_card_number);
        return receiver.us_card_number;
    }
    else return -1;
}

int tcp_server_do_check_anytoany_packet(char* recvline,unsigned int *result)
{
    struct udp_packet_command_header receiver;
    unsigned int payload=0;
    
    memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
    
    if(receiver.type==ANYTOANY_RESULT){
        memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
        *result = payload;
        DEBUG_PRINT("receive %u Any to any result reply\n\r ",receiver.us_card_number);
        return receiver.us_card_number;
    }
    else if(receiver.type==ANYTOANY_REPLY)
    {
         *result = RESULT_ACK;
         DEBUG_PRINT("receive %u Any to any ack reply\n\r ",receiver.us_card_number);
        return receiver.us_card_number;
    }
    else return -1;
}


int tcp_server_get_chassis_member_count(void)
{
    int chassis_count=0,i;
    
    for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
    {
        if(chassis_information.us_card[i].us_card_number>0)
        {
            chassis_count++;
        }
    }
  
    return chassis_count;
}

void tcp_server_do_update_hw_inform(char* recvline)
{
    struct udp_packet_command_header header;
#if(DEBUGLFAG==1)     
    US_HW_DATA_T hw_inform;
#endif

    memcpy(&header,recvline,sizeof(struct udp_packet_command_header));
    if(header.type==HW_INFORMATION_REPLY)
    {
            /*update DB*/
            memcpy(&(us_hw_information[header.us_card_number-1]), recvline+sizeof(struct udp_packet_command_header),sizeof(US_HW_DATA_T));
            chassis_information.us_card[header.us_card_number-1].status |= CHASSIS_HWINFO_READY;
#if(DEBUGLFAG==1)              
            memcpy(&hw_inform,recvline+sizeof(struct udp_packet_command_header),sizeof(US_HW_DATA_T));
            DEBUG_PRINT("us_nmuber:%u,%u\n",header.us_card_number,hw_inform.us_card_number);
#endif
    }
}



int tcp_server_do_ping_test_request_member(CHASSIS_PING_TEST_T* ping_status)
{
    int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
    int maxfd=0;
    int reply_packet_size;
    char recvline[BUFFSIZE];
    int result;
    int chassis_count=0,success_count=0;
    
    memset(ping_status,0,sizeof(CHASSIS_PING_TEST_T));
    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    chassis_count = tcp_server_get_chassis_member_count();
    /*send udp packet  for ping test rquest*/
    do {
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                /* my self*/
                if(i==(chassis_information.us_card_number-1))
                {
                    if(ping_status->test_result[i]!=1)
                    {
                        ping_status->test_result[i]=1;
                        success_count++;
                    }
                    continue;
                }
                else if(ping_status->test_result[i]!=1)
                {
                    socket[i]=tcp_server_do_send_ping_test_to_client(i+1);
                }
            }
    /* wait for receive reply*/
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        sleep(TIME_CHASSIS_PING);
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_CHASSIS_PING);
        if(reply_packet_size>0)
        {
            //printf("(%s:%d) select count %d, socket:%d\n",__FUNCTION__,__LINE__,n,maxfd);
            do{
                    result = tcp_server_do_check_udp_client_packet(recvline,PING_TEST_REPLY);
                    if(result>=0) 
                    {
                        if(ping_status->test_result[result-1]!=1)
                        {
                            ping_status->test_result[result-1]=1;
                            success_count++;
                        }
                    }
                    memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_CHASSIS_PING)>0);
        }
        retry_count++;
    }while(retry_count<COUNT_CHASSIS_PING&&success_count<chassis_count);
     
    DEBUG_PRINT("ping test retry count :%d, success count:%d, chassis count:%d\n",retry_count,success_count,chassis_count);
    return 0;
}

int tcp_server_do_power_cycle_request_member(CHASSIS_POWER_CYCLE_T* power_status)
{
    int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
    int maxfd=0;
    int reply_packet_size;
    char recvline[BUFFSIZE];
    int result;
    int chassis_count=0,success_count=0;
    
    memset(power_status,0,sizeof(CHASSIS_POWER_CYCLE_T));
    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    chassis_count = tcp_server_get_chassis_member_count();
    /*send udp packet  for ping test rquest*/
    do {
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                /* my self*/
                if(i==(chassis_information.us_card_number-1))
                {
                    if(power_status->test_result[i]!=1)
                    {
                        power_status->test_result[i]=1;
                        success_count++;
                    }
                    continue;
                }
                else if(power_status->test_result[i]!=1)
                {
                    socket[i]=tcp_server_do_send_powe_cycle_to_client(i+1);
                }
            }
    /* wait for receive reply*/
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        sleep(TIME_CHASSIS_POWER);
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_CHASSIS_POWER);
        if(reply_packet_size>0)
        {
            //printf("(%s:%d) select count %d, socket:%d\n",__FUNCTION__,__LINE__,n,maxfd);
            do{
                    result = tcp_server_do_check_udp_client_packet(recvline,POWER_CYCLE_REPLY);
                    if(result>=0) 
                    {
                        if(power_status->test_result[result-1]!=1)
                        {
                            power_status->test_result[result-1]=1;
                            success_count++;
                        }
                    }
            memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_CHASSIS_POWER)>0);
        }
        retry_count++;
    }while(retry_count<COUNT_CHASSIS_POWER&&success_count<chassis_count);
 
    DEBUG_PRINT("power cycle retry count :%d, success count:%d, chassis count:%d\n",retry_count,success_count,chassis_count);
    return 0;
}

int tcp_server_do_hw_information_request_member(CHASSIS_HW_TEST_T* hw_status)
{
    int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
    int maxfd=0;
    int reply_packet_size;
    char recvline[BUFFSIZE];
    int result;
    int chassis_count=0,success_count=0;
    
    memset(hw_status,0,sizeof(CHASSIS_HW_TEST_T));
    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);
    
    chassis_count = tcp_server_get_chassis_member_count();
    DEBUG_PRINT("chassis count:%d\n",chassis_count);
    /*send udp packet  for ping test rquest*/
    do {
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                /* my self*/
                if(i==(chassis_information.us_card_number-1))
                {
                    if(hw_status->test_result[i]!=1)
                    {
                        hw_status->test_result[i]=1;
                        success_count++;
                    }
                    continue;
                }
                else if(hw_status->test_result[i]!=1)
                {
                    socket[i]=tcp_server_do_send_hw_inform_to_client(i+1);
                }
            }
    /* wait for receive reply*/
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_HW_INFORM);
        if(reply_packet_size>0)
        {
            do {
                    result = tcp_server_do_check_udp_client_packet(recvline,HW_INFORMATION_REPLY);
                    if(result>=0) 
                    {
                        if(hw_status->test_result[result-1]!=1)
                        {
                            hw_status->test_result[result-1]=1;
                            tcp_server_do_update_hw_inform(recvline);
                            success_count++;
                        }
                    }
                memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_HW_INFORM)>0);
        }
        retry_count++;
    }while(retry_count<COUNT_HW_INFORM&&success_count<chassis_count);
 
    DEBUG_PRINT("HW inform retry count :%d, success count:%d, chassis count:%d\n",retry_count,success_count,chassis_count);
    return 0;
}

int tcp_server_do_traffic_test_line_card(uint16_t option, struct tcp_packet_command_traffic_payload* traffic_status)
{
    unsigned int wait_time[]={5,50,50,50,45,50,10,10,10,10};
    unsigned int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER]={0};
    int maxfd=0;
    unsigned int reply_packet_size=0;
    char recvline[BUFFSIZE]={0};
    unsigned int result=0, ret=0;
    int status=0;
    int index=0;
    unsigned int chassis_count=0,success_count=0,ack_count=0;
    unsigned int wait_count = wait_time[option];

    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    chassis_count = 8;
    printf("\n\r");
    do {
            /* try to send request to Line card*/
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(i>=4&&i<=7) continue;
                
                if(traffic_status->result[i] ==RESULT_NOT_READY)
                {
                    socket[i]=tcp_server_do_send_traffic_to_client(i+1, option);
                    traffic_status->result[i] =RESULT_READY;
                }
            }
            /* wait for receive reply*/
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(i>=4&&i<=7) continue;
                if(socket[i]>maxfd) maxfd=socket[i];
            }
            sleep(TIME_TRAFFIC);
        
            reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
            if(reply_packet_size>0)
            {
                do{
                    index = tcp_server_do_check_traffic_packet(recvline,&result);
                    if(index>=0) 
                    {
                        if(traffic_status->result[index-1]!=result)
                        {
                            traffic_status->result[index-1]=result;
                            if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                                success_count++;
                            }
                            else if(result==RESULT_ACK){
                                ack_count++;
                            }
                        }
                    }
                    memset(recvline,0,BUFFSIZE);
                }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
            }
            retry_count++;
            printf(".");
    }while(retry_count<wait_count&&success_count<chassis_count);
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
   if(success_count<chassis_count)
   {
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(i>=4&&i<=7) continue;
            if(traffic_status->result[i] ==RESULT_ACK)
            {
                    socket[i]=tcp_server_do_send_traffic_to_client(i+1, TRAFFIC_RESULT_REQUEST);
            }
        }
        
        /* wait for receive reply*/
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(i>=4&&i<=7) continue;
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        sleep(TIME_TRAFFIC);
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
        if(reply_packet_size>0)
        {
            do{
                index = tcp_server_do_check_traffic_packet(recvline,&result);
                if(index>=0) 
                {
                    if(traffic_status->result[index-1]!=result)
                    {
                        traffic_status->result[index-1]=result;
                        if(result==RESULT_FAILED) ret =1; 
                        if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                            traffic_status->result[index-1]=result;
                            success_count++;
                         }
                    }
                }
                memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
        }        
    }
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if((ret==1) || success_count<8) status=1;
    return status;

}

int tcp_server_do_traffic_test_fabric_card(uint16_t option, struct tcp_packet_command_traffic_payload* traffic_status)
{
    unsigned int wait_time[]={5,50,50,50,45,20,10,10,10,40};
    unsigned int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER]={0};
    int maxfd=0;
    unsigned int reply_packet_size=0;
    char recvline[BUFFSIZE]={0};
    unsigned int result=0, ret=0;
    int status=0;
    int index=0;
    unsigned int chassis_count=0,success_count=0,ack_count=0;
    unsigned int wait_count = wait_time[option];//= COUNT_TRAFFIC;

    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    chassis_count = 4;
    printf("\n\r");
    do {
            /* try to send request to Fabric card*/
            for(i=4;i<8;i++)
            {
                if(traffic_status->result[i] ==RESULT_NOT_READY)
                {
                    socket[i]=tcp_server_do_send_traffic_to_client(i+1, option);
                    traffic_status->result[i] =RESULT_READY;
                }
            }
            /* wait for receive reply*/
            for(i=4;i<8;i++)
            {
                if(socket[i]>maxfd) maxfd=socket[i];
            }
            sleep(TIME_TRAFFIC);
        
            reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
            if(reply_packet_size>0)
            {
                do{
                    index = tcp_server_do_check_traffic_packet(recvline,&result);
                    if(index>=0) 
                    {
                        if(traffic_status->result[index-1]!=result)
                        {
                            traffic_status->result[index-1]=result;
                            if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                                success_count++;
                            }
                            else if(result==RESULT_ACK){
                                ack_count++;
                            }
                        }
                    }
                    memset(recvline,0,BUFFSIZE);
                }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
            }
            retry_count++;
            printf(".");
    }while(retry_count<wait_count&&success_count<chassis_count);
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if(success_count<chassis_count)
    {
        for(i=4;i<8;i++)
        {
            if(traffic_status->result[i] ==RESULT_ACK)
            {
                    socket[i]=tcp_server_do_send_traffic_to_client(i+1, TRAFFIC_RESULT_REQUEST);
            }
        }
        /* wait for receive reply*/
        for(i=4;i<8;i++)
        {
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        sleep(TIME_TRAFFIC);
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
        if(reply_packet_size>0)
        {
            do{
                index = tcp_server_do_check_traffic_packet(recvline,&result);
                if(index>=0) 
                {
                    if(traffic_status->result[index-1]!=result)
                    {
                        traffic_status->result[index-1]=result;
                        if(result==RESULT_FAILED) ret =1;                        
                        if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                            success_count++;
                        }
                    }
                }
                memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
        }        
    }
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if((ret==1) || success_count<4) status=1;
    return status;
}

int tcp_server_do_traffic_request_member(uint16_t option, struct tcp_packet_command_traffic_payload* traffic_status)
{
    int result=0;

    /*send udp packet  for traffic test rquest*/
    switch(option)
    {
        case 0 : 
            result = tcp_server_do_traffic_test_line_card(option,traffic_status);
            result |=tcp_server_do_traffic_test_fabric_card(option,traffic_status);
            break;
        case 1:
        case 2:
        case 3:
            result = tcp_server_do_traffic_test_line_card(option,traffic_status);
            break;
        case 4:
            result = tcp_server_do_traffic_test_fabric_card(option,traffic_status);
            break;
        case 5:
            if((result = tcp_server_do_traffic_test_fabric_card(option,traffic_status))!=0)
            {
                DEBUG_PRINT("Burn in test in Fabric card failure:%d",result);
                return result;
            }
            result |= tcp_server_do_traffic_test_line_card(option,traffic_status);
            break;
        case 6:
        case 7:
        case 8:
            result = tcp_server_do_traffic_test_fabric_card(option,traffic_status);
            result |= tcp_server_do_traffic_test_line_card(option,traffic_status);
            break;
        case 9:
             if((result = tcp_server_do_traffic_test_line_card(option,traffic_status))!=0) 
             {
                 DEBUG_PRINT("Burn in test in Line card failure:%d",result);
                 return result;
             }
             result |= tcp_server_do_traffic_test_fabric_card(option,traffic_status);
            default:
                break;
    }

    return result;
}

int tcp_server_do_health_request_member(struct tcp_packet_command_traffic_payload* traffic_status)
{
    //unsigned int wait_time[]={5,50,50,50,45,50,10,10,10,40};
    unsigned int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER]={0};
    int maxfd=0;
    unsigned int reply_packet_size=0;
    char recvline[BUFFSIZE]={0};
    unsigned int result=0, ret=0;
    int status=0;
    int index=0;
    unsigned int chassis_count=0,success_count=0,ack_count=0;
    unsigned int wait_count = COUNT_HEALTH;

    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    chassis_count = MAXIMUM_MICRO_SERVER_CARD_NUMBER;
    printf("\n\r");
    do {
            /* try to send request to Fabric card*/
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(traffic_status->result[i] ==RESULT_NOT_READY)
                {
                    socket[i]=tcp_server_do_send_health_test_to_client(i+1,HEALTH_REQUEST);
                    traffic_status->result[i] =RESULT_READY;
                }
            }
            /* wait for receive reply*/
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(socket[i]>maxfd) maxfd=socket[i];
            }
            sleep(TIME_TRAFFIC);
        
            reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
            if(reply_packet_size>0)
            {
                //printf("(%s:%d) select count %d, socket:%d\n",__FUNCTION__,__LINE__,n,maxfd);
                do{
                    index = tcp_server_do_check_health_packet(recvline,&result);
                    if(index>=0) 
                    {
                        if(traffic_status->result[index-1]!=result)
                        {
                            traffic_status->result[index-1]=result;
                            if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                                success_count++;
                            }
                            else if(result==RESULT_ACK){
                                ack_count++;
                            }
                        }
                    }
                    memset(recvline,0,BUFFSIZE);
                }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
            }
            retry_count++;
            printf(".");
    }while(retry_count<wait_count&&success_count<chassis_count);
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if(success_count<chassis_count)
    {
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(traffic_status->result[i] ==RESULT_ACK)
            {
                    socket[i]=tcp_server_do_send_health_test_to_client(i+1, HEALTH_RESULT_REQUEST);
            }
        }
        /* wait for receive reply*/
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        sleep(TIME_TRAFFIC);
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
        if(reply_packet_size>0)
        {
            do{
                index = tcp_server_do_check_health_packet(recvline,&result);
                if(index>=0) 
                {
                    if(traffic_status->result[index-1]!=result)
                    {
                        traffic_status->result[index-1]=result;
                        if(result==RESULT_FAILED) ret =1;                        
                        if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                            success_count++;
                            //printf("%d:traffic result:%d! \n\r",index,result);
                        }
                    }
                }
                memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
        }        
    }
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if((ret==1) || success_count<chassis_count) status=1;
    return status;
}

int tcp_server_do_anytoany_request_member(struct tcp_packet_command_anytoany_command_parameter* paras, struct tcp_packet_command_traffic_payload* traffic_status)
{
    //unsigned int wait_time[]={5,50,50,50,45,50,10,10,10,40};
    unsigned int retry_count =0;
    unsigned int i;
    int socket[MAXIMUM_MICRO_SERVER_CARD_NUMBER]={0};
    int maxfd=0;
    unsigned int reply_packet_size=0;
    char recvline[BUFFSIZE]={0};
    unsigned int result=0, ret=0;
    int status=0;
    int index=0;
    unsigned int chassis_count=0,success_count=0,ack_count=0;
    unsigned int wait_count = 10;

    memset(socket,0,sizeof(int)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    chassis_count = MAXIMUM_MICRO_SERVER_CARD_NUMBER;
    printf("\n\r");
    do {
            /* try to send request to all card*/
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(traffic_status->result[i] ==RESULT_NOT_READY)
                {
                    socket[i]=tcp_server_do_send_anytoany_to_client(i+1,paras,ANYTOANY_REQUEST);
                    traffic_status->result[i] =RESULT_READY;
                }
            }
            /* wait for receive reply*/
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(socket[i]>maxfd) maxfd=socket[i];
            }
            sleep(TIME_TRAFFIC);
        
            reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
            if(reply_packet_size>0)
            {
                do{
                    index = tcp_server_do_check_anytoany_packet(recvline,&result);
                    if(index>=0) 
                    {
                        if(traffic_status->result[index-1]!=result)
                        {
                            traffic_status->result[index-1]=result;
                            if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                                success_count++;
                            }
                            else if(result==RESULT_ACK){
                                ack_count++;
                            }
                        }
                    }
                    memset(recvline,0,BUFFSIZE);
                }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
            }
            retry_count++;
            printf(".");
    }while(retry_count<wait_count&&success_count<chassis_count);
    DEBUG_PRINT("Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if(success_count<chassis_count)
    {
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(traffic_status->result[i] ==RESULT_ACK)
            {
                    socket[i]=tcp_server_do_send_anytoany_to_client(i+1, paras, ANYTOANY_RESULT_REQUEST);
            }
        }
        /* wait for receive reply*/
        for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
        {
            if(socket[i]>maxfd) maxfd=socket[i];
        }
        sleep(TIME_TRAFFIC);
        
        reply_packet_size = udp_client_recv(maxfd,recvline,TIME_TRAFFIC);
        if(reply_packet_size>0)
        {
            do{
                index = tcp_server_do_check_anytoany_packet(recvline,&result);
                if(index>=0) 
                {
                    if(traffic_status->result[index-1]!=result)
                    {
                        traffic_status->result[index-1]=result;
                        if(result==RESULT_FAILED) ret =1;                        
                        if(result==RESULT_SUCCESS||result==RESULT_FAILED){
                            success_count++;
                            //printf("%d:traffic result:%d! \n\r",index,result);
                        }
                    }
                }
                memset(recvline,0,BUFFSIZE);
            }while(udp_client_recv(maxfd,recvline,TIME_TRAFFIC)>0);
        }        
    }
    DEBUG_PRINT("chassis daemon : Traffic retry count :%d, ack count:%d, success count:%d, chassis count:%d\n",retry_count,ack_count,success_count,chassis_count);
    printf("\n\r");
    if((ret==1) || success_count<chassis_count) status=1;
    return status;
}

void tcp_server_do_ping_test_request (int sock,char* buffer)
{
    struct tcp_packet_ping_command tcp_ping_request;
    struct tcp_packet_ping_reply_command tcp_ping_reply;
    CHASSIS_PING_TEST_T ping_status;
    char* crcptr=(char*)&tcp_ping_reply;
    int errIndex;
    int n;
    uint8_t crc1;

    memcpy(&tcp_ping_request,buffer,sizeof(struct tcp_packet_ping_command));
    memset(&tcp_ping_reply,0,sizeof(struct tcp_packet_ping_command));
    memset(&ping_status,0,sizeof(CHASSIS_PING_TEST_T));

    /*crc check sum*/
    crc1 = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)buffer+sizeof(tcp_ping_request.header.magic),(sizeof(tcp_ping_request) - sizeof(tcp_ping_request.header.magic)) - sizeof(tcp_ping_request.csum));
    if(crc1!=tcp_ping_request.csum)
    {
        perror("ERROR check sum");
        return;
    }
        
    errIndex=tcp_server_do_ping_test_request_member(&ping_status);

    tcp_ping_reply.header.magic = 0xFBFB;
    tcp_ping_reply.header.length = 7+sizeof(CHASSIS_PING_TEST_T);
    tcp_ping_reply.header.uSnumber = tcp_ping_request.header.uSnumber;
    tcp_ping_reply.header.commandID = PING_TEST_REPLY;
    tcp_ping_reply.header.externalMode = tcp_ping_request.header.externalMode;
    memcpy(&(tcp_ping_reply.chassis_payload),&ping_status,sizeof(CHASSIS_PING_TEST_T));
    tcp_ping_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr + sizeof(tcp_ping_reply.header.magic), (sizeof(tcp_ping_reply) - sizeof(tcp_ping_reply.header.magic)) - sizeof(tcp_ping_reply.csum));
    n = write(sock,&tcp_ping_reply,sizeof(struct tcp_packet_ping_reply_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
 
    DEBUG_PRINT("receive a ping test request! :%d\n",errIndex);
}

void tcp_server_do_power_cycle_request (int sock,char* buffer)
{
    struct tcp_packet_power_command tcp_power_request;
    struct tcp_packet_power_reply_command tcp_power_reply;
    CHASSIS_POWER_CYCLE_T power_status;
    char* crcptr=(char*)(&tcp_power_reply);
    int errIndex;
    int n;
    uint8_t crc1;

    memcpy(&tcp_power_request,buffer,sizeof(struct tcp_packet_power_command));
    memset(&tcp_power_reply,0,sizeof(struct tcp_packet_power_command));
    memset(&power_status,0,sizeof(CHASSIS_POWER_CYCLE_T));

    /*crc check sum*/
    crc1 = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)buffer+sizeof(tcp_power_request.header.magic),(sizeof(tcp_power_request) - sizeof(tcp_power_request.header.magic)) - sizeof(tcp_power_request.csum));
    if(crc1!=tcp_power_request.csum)
    {
        perror("ERROR check sum");
        return;
    }
        
    errIndex=tcp_server_do_power_cycle_request_member(&power_status);

    tcp_power_reply.header.magic = 0xFBFB;
    tcp_power_reply.header.length = 7 +sizeof(CHASSIS_POWER_CYCLE_T);
    tcp_power_reply.header.uSnumber = tcp_power_request.header.uSnumber;
    tcp_power_reply.header.commandID = POWER_CYCLE_REPLY;
    tcp_power_reply.header.externalMode = tcp_power_request.header.externalMode;
    memcpy(&(tcp_power_reply.chassis_payload),&power_status,sizeof(CHASSIS_POWER_CYCLE_T));
    tcp_power_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr+sizeof(tcp_power_reply.header.magic), (sizeof(tcp_power_reply) - sizeof(tcp_power_reply.header.magic)) - sizeof(tcp_power_reply.csum));

    n = write(sock,&tcp_power_reply,sizeof(struct tcp_packet_power_reply_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
 
    DEBUG_PRINT("receive a power cycle request! :%d\n",errIndex);
}


void tcp_server_do_hw_information_request (int sock,char* buffer)
{
    struct tcp_packet_hwinforequest_command tcp_hwinform_request;
    struct tcp_packet_hwinforeplay_command tcp_hwinform_reply;
    CHASSIS_HW_TEST_T hw_status;
    char* crcptr=(char*)(&tcp_hwinform_reply);
    int errIndex;
    int n;
    int usNumber;
    uint8_t crc1;

    memcpy(&tcp_hwinform_request,buffer,sizeof(struct tcp_packet_hwinforequest_command));
    memset(&tcp_hwinform_reply,0,sizeof(struct tcp_packet_hwinforeplay_command));
    memset(&hw_status,0,sizeof(CHASSIS_HW_TEST_T));

    /*crc check sum*/
    crc1 = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)buffer+sizeof(tcp_hwinform_request.header.magic),(sizeof(tcp_hwinform_request) - sizeof(tcp_hwinform_request.header.magic)) - sizeof(tcp_hwinform_request.csum));
    if(crc1!=tcp_hwinform_request.csum)
    {
        perror("ERROR check sum\n");
        return;
    }
    /*update hardware information*/
    usNumber = tcp_hwinform_request.header.uSnumber;
    if(usNumber<1||usNumber>12) 
    {
        perror("ERROR uSnumber\n");
        return;
    }
    //memcpy(&us_hw_information+usNumber-1,&tcp_hwinform_request.payload,sizeof(US_HW_DATA_T));
#if(DEBUGLFAG==1)
    printf("us_card_number:%d\n",usNumber);
    printf("bios_vender:%s\n",tcp_hwinform_request.payload.bios_vender);
    printf("bios_version:%s\n",tcp_hwinform_request.payload.bios_version);
    printf("bios_revision:%s\n",tcp_hwinform_request.payload.bios_revision);
    printf("bios_fw_revision:%s\n",tcp_hwinform_request.payload.bios_fw_revision);
    printf("cpu_manufacturer:%s\n",tcp_hwinform_request.payload.cpu_manufacturer);
    printf("cpu_version:%s\n",tcp_hwinform_request.payload.cpu_version);
    printf("cpu_clock:%s\n",tcp_hwinform_request.payload.cpu_clock);
    printf("cpu_core:%s\n",tcp_hwinform_request.payload.cpu_core);
    printf("cpu_maxspeed:%s\n",tcp_hwinform_request.payload.cpu_maxspeed);
    printf("dimm_size:%s\n",tcp_hwinform_request.payload.dimm_size);
    printf("dimm_factor:%s\n",tcp_hwinform_request.payload.dimm_factor);
    printf("dimm_type:%s\n",tcp_hwinform_request.payload.dimm_type);
    printf("dimm_speed:%s\n",tcp_hwinform_request.payload.dimm_speed);
    printf("dimm_manufacturer:%s\n",tcp_hwinform_request.payload.dimm_manufacturer);
    printf("dimm_sn:%s\n",tcp_hwinform_request.payload.dimm_sn);
    printf("dimm_pn:%s\n",tcp_hwinform_request.payload.dimm_pn);
    printf("ssd_vender:%s\n",tcp_hwinform_request.payload.ssd_vender);
    printf("ssd_sn:%s\n",tcp_hwinform_request.payload.ssd_sn);
    printf("ssd_fw_version:%s\n",tcp_hwinform_request.payload.ssd_fw_version);
#endif    
    errIndex=tcp_server_do_hw_information_request_member(&hw_status);

    tcp_hwinform_reply.header.magic = 0xFBFB;
    tcp_hwinform_reply.header.length = 7+sizeof(US_HW_DATA_T)*MAXIMUM_MICRO_SERVER_CARD_NUMBER;
    tcp_hwinform_reply.header.uSnumber = tcp_hwinform_request.header.uSnumber;
    tcp_hwinform_reply.header.commandID = HW_INFORMATION_REPLY;
    tcp_hwinform_reply.header.externalMode = tcp_hwinform_request.header.externalMode;
    memcpy(tcp_hwinform_reply.us_hw_information, us_hw_information,sizeof(US_HW_DATA_T)*MAXIMUM_MICRO_SERVER_CARD_NUMBER);
    tcp_hwinform_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr+sizeof(tcp_hwinform_reply.header.magic), (sizeof(tcp_hwinform_reply) - sizeof(tcp_hwinform_reply.header.magic)) - sizeof(tcp_hwinform_reply.csum));

#if(DEBUGLFAG==1)
	{
	    for(n=0;n<MAXIMUM_MICRO_SERVER_CARD_NUMBER;n++){
	       printf("US Card Number      :%d\r\n", us_hw_information[n].us_card_number); 
		printf("BIOS Vender           :%s\r\n", us_hw_information[n].bios_vender);
		printf("BIOS Version          : %s\r\n", us_hw_information[n].bios_version);
		printf("BIOS Revision         : %s\r\n", us_hw_information[n].bios_revision);
		printf("BIOS FW Revision      : %s\r\n", us_hw_information[n].bios_fw_revision);
		printf("CPU Manufacturer      :%s\r\n", us_hw_information[n].cpu_manufacturer);
		printf("CPU Version           :%s\r\n", us_hw_information[n].cpu_version);
		printf("CPU Clock             : %s MHz\r\n", us_hw_information[n].cpu_clock);
		printf("CPU Core              : %s Core\r\n", us_hw_information[n].cpu_core);
		printf("CPU Max Speed         : %s MHz\r\n", us_hw_information[n].cpu_maxspeed);
		printf("DIMM Size             : %s MB\r\n", us_hw_information[n].dimm_size);
		printf("DIMM Factor           : %s\r\n", us_hw_information[n].dimm_factor);
		printf("DIMM Type             : %s\r\n", us_hw_information[n].dimm_type);
		printf("DIMM Speed            : %s MHz\r\n", us_hw_information[n].dimm_speed);
		printf("DIMM Manufacturer     : %s\r\n", us_hw_information[n].dimm_manufacturer);
		printf("DIMM SN               : %s\r\n", us_hw_information[n].dimm_sn);
		printf("DIMM PN               : %s\r\n", us_hw_information[n].dimm_pn);
		printf("SSD Vender            : %s\r\n", us_hw_information[n].ssd_vender);
		printf("SSD SN                : %s\r\n", us_hw_information[n].ssd_sn);
		printf("SSD FW Version        : %s\r\n", us_hw_information[n].ssd_fw_version);
	        }
	}

#endif

    n = write(sock,&tcp_hwinform_reply,sizeof(struct tcp_packet_hwinforeplay_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
 
    DEBUG_PRINT("receive a hw information request! :%d\n",errIndex);
}

void tcp_server_do_traffic_request (int sock,char* buffer)
{
    struct tcp_packet_traffic_command tcp_traffic_request;
    struct tcp_packet_traffic_command tcp_traffic_reply;
    struct tcp_packet_command_traffic_payload traffic_status;
    char* crcptr=(char*)(&tcp_traffic_reply);
    int errIndex;
    int n;
    uint8_t crc1;

    memcpy(&tcp_traffic_request,buffer,sizeof(struct tcp_packet_power_command));
    memset(&tcp_traffic_reply,0,sizeof(struct tcp_packet_power_command));
    memset(&traffic_status,0,sizeof(struct tcp_packet_command_traffic_payload));

    /*crc check sum*/
    crc1 = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)buffer+sizeof(tcp_traffic_request.header.magic),(sizeof(tcp_traffic_request) - sizeof(tcp_traffic_request.header.magic)) - sizeof(tcp_traffic_request.csum));
    if(crc1!=tcp_traffic_request.csum)
    {
        perror("ERROR check sum");
        return;
    }
        
    errIndex=tcp_server_do_traffic_request_member(tcp_traffic_request.payload,&traffic_status);

    tcp_traffic_reply.header.magic = 0xFBFB;
    tcp_traffic_reply.header.length = 7 +sizeof(struct tcp_packet_command_traffic_payload);
    tcp_traffic_reply.header.uSnumber = tcp_traffic_request.header.uSnumber;
    tcp_traffic_reply.header.commandID = TRAFFIC_RESULT;
    tcp_traffic_reply.header.externalMode = tcp_traffic_reply.header.externalMode;
    memcpy(&(tcp_traffic_reply.chassis_payload),&traffic_status,sizeof(struct tcp_packet_command_traffic_payload));
    tcp_traffic_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr+sizeof(tcp_traffic_reply.header.magic), (sizeof(tcp_traffic_reply) - sizeof(tcp_traffic_reply.header.magic)) - sizeof(tcp_traffic_reply.csum));

    n = write(sock,&tcp_traffic_reply,sizeof(struct tcp_packet_power_reply_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
    
    DEBUG_PRINT("receive a traffic  request! :%d\n",errIndex);
}

void tcp_server_do_health_request (int sock,char* buffer)
{
    struct tcp_packet_traffic_command tcp_traffic_request;
    struct tcp_packet_traffic_command tcp_traffic_reply;
    struct tcp_packet_command_traffic_payload traffic_status;
    char* crcptr=(char*)(&tcp_traffic_reply);
    int errIndex;
    int n;
    uint8_t crc1;

    memcpy(&tcp_traffic_request,buffer,sizeof(struct tcp_packet_power_command));
    memset(&tcp_traffic_reply,0,sizeof(struct tcp_packet_power_command));
    memset(&traffic_status,0,sizeof(struct tcp_packet_command_traffic_payload));

    /*crc check sum*/
    crc1 = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)buffer+sizeof(tcp_traffic_request.header.magic),(sizeof(tcp_traffic_request) - sizeof(tcp_traffic_request.header.magic)) - sizeof(tcp_traffic_request.csum));
    if(crc1!=tcp_traffic_request.csum)
    {
        perror("ERROR check sum");
        return;
    }
        
    errIndex=tcp_server_do_health_request_member(&traffic_status);

    tcp_traffic_reply.header.magic = 0xFBFB;
    tcp_traffic_reply.header.length = 7 +sizeof(struct tcp_packet_command_traffic_payload);
    tcp_traffic_reply.header.uSnumber = tcp_traffic_request.header.uSnumber;
    tcp_traffic_reply.header.commandID = HEALTH_RESULT;
    tcp_traffic_reply.header.externalMode = tcp_traffic_reply.header.externalMode;
    memcpy(&(tcp_traffic_reply.chassis_payload),&traffic_status,sizeof(struct tcp_packet_command_traffic_payload));
    tcp_traffic_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr+sizeof(tcp_traffic_reply.header.magic), (sizeof(tcp_traffic_reply) - sizeof(tcp_traffic_reply.header.magic)) - sizeof(tcp_traffic_reply.csum));

    n = write(sock,&tcp_traffic_reply,sizeof(struct tcp_packet_power_reply_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
    
    DEBUG_PRINT("receive a traffic  request! :%d\n",errIndex);
}

void tcp_server_do_anytoany_request (int sock,char* buffer)
{
    struct tcp_packet_traffic_command tcp_traffic_request;
    struct tcp_packet_traffic_command tcp_traffic_reply;
    struct tcp_packet_command_traffic_payload traffic_status;
    char* crcptr=(char*)(&tcp_traffic_reply);
    int errIndex;
    int n;
    uint8_t crc1;

    memcpy(&tcp_traffic_request,buffer,sizeof(struct tcp_packet_power_command));
    memset(&tcp_traffic_reply,0,sizeof(struct tcp_packet_power_command));
    memset(&traffic_status,0,sizeof(struct tcp_packet_command_traffic_payload));

    /*crc check sum*/
    crc1 = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)buffer+sizeof(tcp_traffic_request.header.magic),(sizeof(tcp_traffic_request) - sizeof(tcp_traffic_request.header.magic)) - sizeof(tcp_traffic_request.csum));
    if(crc1!=tcp_traffic_request.csum)
    {
        perror("ERROR check sum");
        return;
    }
        
    errIndex=tcp_server_do_anytoany_request_member(&tcp_traffic_request.paras,&traffic_status);

    tcp_traffic_reply.header.magic = 0xFBFB;
    tcp_traffic_reply.header.length = 7 +sizeof(struct tcp_packet_command_traffic_payload);
    tcp_traffic_reply.header.uSnumber = tcp_traffic_request.header.uSnumber;
    tcp_traffic_reply.header.commandID = ANYTOANY_RESULT;
    tcp_traffic_reply.header.externalMode = tcp_traffic_reply.header.externalMode;
    memcpy(&(tcp_traffic_reply.chassis_payload),&traffic_status,sizeof(struct tcp_packet_command_traffic_payload));
    tcp_traffic_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr+sizeof(tcp_traffic_reply.header.magic), (sizeof(tcp_traffic_reply) - sizeof(tcp_traffic_reply.header.magic)) - sizeof(tcp_traffic_reply.csum));

    n = write(sock,&tcp_traffic_reply,sizeof(struct tcp_packet_power_reply_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
    
    DEBUG_PRINT("receive a traffic  request! :%d\n",errIndex);
}

void tcp_server_do_error_command (int sock,char* buffer)
{
    struct tcp_packet_ping_command tcp_ping_request;
    struct tcp_packet_ping_command tcp_ping_reply;
    int n;
    char* crcptr=(char*)(&tcp_ping_reply);

    memcpy(&tcp_ping_request,buffer,sizeof(struct tcp_packet_ping_command));
    memset(&tcp_ping_reply,0,sizeof(struct tcp_packet_ping_command));
    
    tcp_ping_reply.header.magic = 0xFBFB;
    tcp_ping_reply.header.length = 9;
    tcp_ping_reply.header.uSnumber = tcp_ping_request.header.uSnumber;
    tcp_ping_reply.header.commandID = ERROR_COMMAND;
    tcp_ping_reply.header.externalMode = tcp_ping_request.header.externalMode;
    tcp_ping_reply.payload = 0;
    tcp_ping_reply.csum = crc_util_calculate_crc8_checksum_of_buffer((unsigned char*)crcptr+sizeof(tcp_ping_reply.header.magic), (sizeof(tcp_ping_reply) - sizeof(tcp_ping_reply.header.magic)) - sizeof(tcp_ping_reply.csum));

    n = write(sock,&tcp_ping_reply,sizeof(struct tcp_packet_ping_command));   
    if (n < 0)
    {
        perror("ERROR writing to socket");
    }
}

void tcp_server_do_processing (int sock)
{
    int n;
    char buffer[BUFSIZE];
    struct tcp_packet_command_header header;
    int commandID;
   
    bzero(buffer,BUFSIZE);
   
    n = read(sock,buffer,BUFSIZE-1);
   
    if (n < 0)
    {
        perror("ERROR reading from socket\n\r");
        return;
    }
    if((commandID=tcp_server_do_parsing((uint8_t *)buffer,(uint32_t)BUFSIZE,&header))<0)
    {
        perror("ERROR reading data\n\r");
        return;
    }

    switch(commandID)
    {
        case PING_TEST_REQUEST:
            tcp_server_do_ping_test_request(sock,buffer);
            break;
        case POWER_CYCLE_REQUEST:
            tcp_server_do_power_cycle_request(sock,buffer);
            break;
        case HW_INFORMATION_REQUEST:
            tcp_server_do_hw_information_request(sock,buffer);
            break;
        case TRAFFIC_REQUEST:
            tcp_server_do_traffic_request(sock,buffer);
            break;
        case HEALTH_REQUEST:
            printf("chassis daemon : do health request\n\r");
            tcp_server_do_health_request(sock,buffer);
            break;
        case ANYTOANY_REQUEST:
            printf("chassis daemon : do any to any request\n\r");
            tcp_server_do_anytoany_request(sock,buffer);
            break;
        default:
            tcp_server_do_error_command(sock,buffer);
            DEBUG_PRINT("ERROR tcp command %d\n",commandID);
            return;
    }

}


int tcp_server_diag_function_search_string(char *file_name, char *delim, char *out_string_buf, int buf_max_len, int string_line, int string_number)
{
FILE *filep = NULL;
char in_string_buf[MAXIMUM_BUFFER_LENGTH] = {0}, *ptr = NULL;
int line = 1, number = 1;


	if (file_name == NULL || delim == NULL || out_string_buf == NULL || buf_max_len != MAXIMUM_BUFFER_LENGTH)
		return -1;
	if (string_line < 1 || string_number < 1)
		return -2;

	memset(out_string_buf, 0, buf_max_len);
	if ((filep = fopen(file_name, "r")) == NULL)
	{
		DEBUG_PRINT("fopen(%s): Error!\r\n", file_name);
		return -3;
	}
	while (fgets(in_string_buf, MAXIMUM_STRING_LENGTH, filep) != NULL)
	{
		ptr = strtok(in_string_buf, delim);
		do {
			if (line == string_line && number == string_number)
			{
				strncpy(out_string_buf, ptr, MAXIMUM_STRING_LENGTH);
				pclose(filep);
				return 0;
			}
			number++;
		} while ((ptr = strtok(NULL, delim)) != NULL);
		memset(in_string_buf, 0, buf_max_len);
		line++;
		number = 1;
	}
	pclose(filep);

	return -4;
}

int tcp_server_do_shutdown_to_client(int slotid, int option)
{
    int usNumber=0,i;
    int my_number = chassis_information.us_card_number;

    /*char status[2][MAXIMUM_BUFFER_LENGTH]={"OFF","ON"};*/
    
    for (i=0;i<4;i++)
    {
        if(my_number==chassis_information.local_Master[i])
        {
            switch(i)
            {
                case 0:usNumber = chassis_information.local_Master[1];break;
                case 1:usNumber = chassis_information.local_Master[0];break;
                case 2:usNumber = chassis_information.local_Master[3];break;
                case 3:usNumber = chassis_information.local_Master[2];break;
                default:                
                    return -1;
             }
            break;
        }
    }    

    if(usNumber==0)
    {
        DEBUG_PRINT("No another Master FC, can't set LC:%d %s\r\n", slotid,status[option]);
        return -1;
    }
    return tcp_server_do_send_shutdown_to_client(usNumber, slotid, option);
    
    //return 0;
}


