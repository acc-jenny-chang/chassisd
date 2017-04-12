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

/* Sample TCP client */
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include "tcp_client.h"

#define DEBUGLFAG 0

#if (DEBUGLFAG == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("tcp_server:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

unsigned char tcp_client_calculate_crc8_checksum(unsigned char crc, unsigned char data);
unsigned char tcp_client_calculate_crc8_checksum_of_buffer(unsigned char *data_buffer_ptr, unsigned long data_buffer_length);


int tcp_client_send_ping_test_request(int sockfd, struct sockaddr_in* servaddr);
int tcp_client_send_power_cycle_request(int sockfd, struct sockaddr_in* servaddr);
int tcp_client_send_hw_information_request(int sockfd, struct sockaddr_in* servaddr);
int tcp_client_send_traffic_test_request(int sockfd, struct sockaddr_in* servaddr, int option);
int tcp_client_send_health_test_request(int sockfd, struct sockaddr_in* servaddr, int option);

int tcp_client_diag_function_search_string(char *file_name, char *delim, char *out_string_buf, int buf_max_len, int string_line, int string_number);

#define MAXIMUM_BUFFER_LENGTH 512
#define MAXIMUM_STRING_LENGTH				(MAXIMUM_BUFFER_LENGTH - 1)

static unsigned int us_card_number;

static uint16_t crc_table[256] = {
	0x0000U, 0x1021U, 0x2042U, 0x3063U, 0x4084U, 0x50a5U, 0x60c6U, 0x70e7U,
	0x8108U, 0x9129U, 0xa14aU, 0xb16bU, 0xc18cU, 0xd1adU, 0xe1ceU, 0xf1efU,
	0x1231U, 0x0210U, 0x3273U, 0x2252U, 0x52b5U, 0x4294U, 0x72f7U, 0x62d6U,
	0x9339U, 0x8318U, 0xb37bU, 0xa35aU, 0xd3bdU, 0xc39cU, 0xf3ffU, 0xe3deU,
	0x2462U, 0x3443U, 0x0420U, 0x1401U, 0x64e6U, 0x74c7U, 0x44a4U, 0x5485U,
	0xa56aU, 0xb54bU, 0x8528U, 0x9509U, 0xe5eeU, 0xf5cfU, 0xc5acU, 0xd58dU,
	0x3653U, 0x2672U, 0x1611U, 0x0630U, 0x76d7U, 0x66f6U, 0x5695U, 0x46b4U,
	0xb75bU, 0xa77aU, 0x9719U, 0x8738U, 0xf7dfU, 0xe7feU, 0xd79dU, 0xc7bcU,
	0x48c4U, 0x58e5U, 0x6886U, 0x78a7U, 0x0840U, 0x1861U, 0x2802U, 0x3823U,
	0xc9ccU, 0xd9edU, 0xe98eU, 0xf9afU, 0x8948U, 0x9969U, 0xa90aU, 0xb92bU,
	0x5af5U, 0x4ad4U, 0x7ab7U, 0x6a96U, 0x1a71U, 0x0a50U, 0x3a33U, 0x2a12U,
	0xdbfdU, 0xcbdcU, 0xfbbfU, 0xeb9eU, 0x9b79U, 0x8b58U, 0xbb3bU, 0xab1aU,
	0x6ca6U, 0x7c87U, 0x4ce4U, 0x5cc5U, 0x2c22U, 0x3c03U, 0x0c60U, 0x1c41U,
	0xedaeU, 0xfd8fU, 0xcdecU, 0xddcdU, 0xad2aU, 0xbd0bU, 0x8d68U, 0x9d49U,
	0x7e97U, 0x6eb6U, 0x5ed5U, 0x4ef4U, 0x3e13U, 0x2e32U, 0x1e51U, 0x0e70U,
	0xff9fU, 0xefbeU, 0xdfddU, 0xcffcU, 0xbf1bU, 0xaf3aU, 0x9f59U, 0x8f78U,
	0x9188U, 0x81a9U, 0xb1caU, 0xa1ebU, 0xd10cU, 0xc12dU, 0xf14eU, 0xe16fU,
	0x1080U, 0x00a1U, 0x30c2U, 0x20e3U, 0x5004U, 0x4025U, 0x7046U, 0x6067U,
	0x83b9U, 0x9398U, 0xa3fbU, 0xb3daU, 0xc33dU, 0xd31cU, 0xe37fU, 0xf35eU,
	0x02b1U, 0x1290U, 0x22f3U, 0x32d2U, 0x4235U, 0x5214U, 0x6277U, 0x7256U,
	0xb5eaU, 0xa5cbU, 0x95a8U, 0x8589U, 0xf56eU, 0xe54fU, 0xd52cU, 0xc50dU,
	0x34e2U, 0x24c3U, 0x14a0U, 0x0481U, 0x7466U, 0x6447U, 0x5424U, 0x4405U,
	0xa7dbU, 0xb7faU, 0x8799U, 0x97b8U, 0xe75fU, 0xf77eU, 0xc71dU, 0xd73cU,
	0x26d3U, 0x36f2U, 0x0691U, 0x16b0U, 0x6657U, 0x7676U, 0x4615U, 0x5634U,
	0xd94cU, 0xc96dU, 0xf90eU, 0xe92fU, 0x99c8U, 0x89e9U, 0xb98aU, 0xa9abU,
	0x5844U, 0x4865U, 0x7806U, 0x6827U, 0x18c0U, 0x08e1U, 0x3882U, 0x28a3U,
	0xcb7dU, 0xdb5cU, 0xeb3fU, 0xfb1eU, 0x8bf9U, 0x9bd8U, 0xabbbU, 0xbb9aU,
	0x4a75U, 0x5a54U, 0x6a37U, 0x7a16U, 0x0af1U, 0x1ad0U, 0x2ab3U, 0x3a92U,
	0xfd2eU, 0xed0fU, 0xdd6cU, 0xcd4dU, 0xbdaaU, 0xad8bU, 0x9de8U, 0x8dc9U,
	0x7c26U, 0x6c07U, 0x5c64U, 0x4c45U, 0x3ca2U, 0x2c83U, 0x1ce0U, 0x0cc1U,
	0xef1fU, 0xff3eU, 0xcf5dU, 0xdf7cU, 0xaf9bU, 0xbfbaU, 0x8fd9U, 0x9ff8U,
	0x6e17U, 0x7e36U, 0x4e55U, 0x5e74U, 0x2e93U, 0x3eb2U, 0x0ed1U, 0x1ef0U
};


unsigned char tcp_client_calculate_crc8_checksum(unsigned char crc, unsigned char data)
{
unsigned char i = 0;


	i = data ^ crc;
	crc = 0;
	if (i & 1)
		crc ^= 0x5e;
	if (i & 2)
		crc ^= 0xbc;
	if (i & 4)
		crc ^= 0x61;
	if (i & 8)
		crc ^= 0xc2;
	if (i & 0x10)
		crc ^= 0x9d;
	if (i & 0x20)
		crc ^= 0x23;
	if (i & 0x40)
		crc ^= 0x46;
	if (i & 0x80)
		crc ^= 0x8c;

	return crc;
}

unsigned char tcp_client_calculate_crc8_checksum_of_buffer(unsigned char *data_buffer_ptr, unsigned long data_buffer_length)
{
unsigned char crc = 0;
unsigned long i = 0;


	for (i = 0, crc = 0; i < data_buffer_length; i++)
		crc = tcp_client_calculate_crc8_checksum(crc, *(data_buffer_ptr + i));

	return crc;
}

int main(int argc, char**argv)
{
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   char sendline[1000]="this is a test!";
   char recvline[1000];
   int operation;
   int ret = -1;
   char buf[10000] = {0};

   if (argc < 3)
   {
      printf("usage:  chassis_agent <IP address> <54|109|110|211|996|212|type>\n");
      exit(1);
   }

   sockfd=socket(AF_INET,SOCK_STREAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(argv[1]);
   servaddr.sin_port=htons(0xFBFB);

   printf("\nTry to connect!\n"); 
   if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))< 0){
       exit(1);
   }
   printf("\nConnected!\n");

	ret = tcp_client_diag_function_search_string(CONFIG_US_CARD_NUMBER, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 1, 1);
	if (ret != 0)
	{
		printf("diag_function_search_string(%d): Error!\r\n", ret);
		fflush(stdout);
		ret = -13;
	}
	us_card_number = (unsigned int) strtoul(buf, NULL, 10);


    operation = atoi(argv[2]);
    switch(operation)
    {
        case 109:
            printf("Send chassis request item:%d to IP:%s\n",PING_TEST_REQUEST-10000,argv[1]);
            tcp_client_send_ping_test_request(sockfd,&servaddr);
            break;
        case 110:
            printf("Send chassis request item:%d to IP:%s\n",POWER_CYCLE_REQUEST-10000,argv[1]);
            tcp_client_send_power_cycle_request(sockfd,&servaddr);
            break;
        case 54:
            printf("Send chassis request item:%d to IP:%s\n",HW_INFORMATION_REQUEST-10000,argv[1]);
            tcp_client_send_hw_information_request(sockfd,&servaddr);
            break;
       case 211:
            printf("Send chassis request item:%d, %d to IP:%s\n",TRAFFIC_REQUEST-10000,atoi(argv[3]),argv[1]);
            tcp_client_send_traffic_test_request(sockfd,&servaddr,atoi(argv[3]));
            break;            
       case 996:
            printf("Send chassis request item:%d to IP:%s\n",HEALTH_REQUEST-10000,argv[1]);
            tcp_client_send_health_test_request(sockfd,&servaddr,0);
            break;
       case 212:
            printf("Send chassis request item:%d, %d %d %d %d %d %d %d to IP:%s\n",ANYTOANY_REQUEST-10000,
                atoi(argv[3]),
                atoi(argv[4]),
                atoi(argv[5]),
                atoi(argv[6]),
                atoi(argv[7]),
                atoi(argv[8]),
                atoi(argv[9]),
                argv[1]);
            tcp_client_send_anytoany_test_request(sockfd,&servaddr,atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]),atoi(argv[7]),atoi(argv[8]),atoi(argv[9]));
            break;
         default:
                printf("\nUnknow command!\n");
        }

   close(sockfd);
   exit(0);
}

int tcp_client_send_ping_test_request(int sockfd, struct sockaddr_in* servaddr)
{
    //char sendline[1000]="this is a test!";
    char recvline[10000];
    int n,i;
    struct tcp_packet_ping_command tcp_ping_command;
    struct tcp_packet_ping_reply_command tcp_ping_reply;
    uint8_t *data = (uint8_t *)&tcp_ping_command;
    uint8_t crc1,crc2,crc3;

    tcp_ping_command.header.magic = 0xFBFB;
    tcp_ping_command.header.length = 9;
    tcp_ping_command.header.uSnumber = us_card_number;
    tcp_ping_command.header.commandID = PING_TEST_REQUEST;
    tcp_ping_command.header.externalMode = 0x2;
    tcp_ping_command.payload = 0;
    tcp_ping_command.csum = tcp_client_calculate_crc8_checksum_of_buffer(data + sizeof(tcp_ping_command.header.magic), (sizeof(tcp_ping_command) - sizeof(tcp_ping_command.header.magic)) - sizeof(tcp_ping_command.csum));
   
    sendto(sockfd,&tcp_ping_command,sizeof(struct tcp_packet_ping_command),0,
             (struct sockaddr *)servaddr,sizeof(struct sockaddr_in));
      
    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    if(n>0)
    {
        memcpy(&tcp_ping_reply,recvline,sizeof(struct tcp_packet_ping_reply_command));
        crc1 = tcp_client_calculate_crc8_checksum_of_buffer(recvline + sizeof(tcp_ping_reply.header.magic), (sizeof(tcp_ping_reply) - sizeof(tcp_ping_reply.header.magic)) - sizeof(tcp_ping_reply.csum));
        if(crc1!=tcp_ping_reply.csum){
            printf("crc check sum error, %d:%d\n",crc1,tcp_ping_reply.csum);
        }
        else{
            printf("tcp_ping_reply.header.magic:%d\n",tcp_ping_reply.header.magic);
            printf("tcp_ping_reply.header.length:%d\n",tcp_ping_reply.header.length);
            printf("tcp_ping_reply.header.uSnumber:%d\n",tcp_ping_reply.header.uSnumber);
            printf("tcp_ping_reply.header.commandID:%d\n",tcp_ping_reply.header.commandID);
            printf("tcp_ping_reply.header.externalMode:%d\n",tcp_ping_reply.header.externalMode);      
            printf("tcp_ping_reply.csum:%d\n",tcp_ping_reply.csum);
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                    if(tcp_ping_reply.chassis_payload.test_result[i]==0)
                        printf("us Number %d ping test fail\n",i+1);
                    else
                        printf("us Number %d ping test success\n",i+1);
            }            
        }
    }
    return n;
}

int tcp_client_send_power_cycle_request(int sockfd, struct sockaddr_in* servaddr)
{
    //char sendline[1000]="this is a test!";
    char recvline[10000];
    int n,i;
    struct tcp_packet_ping_command tcp_power_command;
    struct tcp_packet_power_reply_command tcp_power_reply;
    uint8_t *data = (uint8_t *)&tcp_power_command;
    uint8_t crc1,crc2,crc3;

    tcp_power_command.header.magic = 0xFBFB;
    tcp_power_command.header.length = 9;
    tcp_power_command.header.uSnumber = us_card_number;
    tcp_power_command.header.commandID = POWER_CYCLE_REQUEST;
    tcp_power_command.header.externalMode = 0x2;
    tcp_power_command.payload = 0;
    tcp_power_command.csum = tcp_client_calculate_crc8_checksum_of_buffer(data + sizeof(tcp_power_command.header.magic), (sizeof(tcp_power_command) - sizeof(tcp_power_command.header.magic)) - sizeof(tcp_power_command.csum));

    sendto(sockfd,&tcp_power_command,sizeof(struct tcp_packet_ping_command),0,
             (struct sockaddr *)servaddr,sizeof(struct sockaddr_in));
      
    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    if(n>0)
    {

        memcpy(&tcp_power_reply,recvline,sizeof(struct tcp_packet_power_reply_command));
        crc1 = tcp_client_calculate_crc8_checksum_of_buffer(recvline + sizeof(tcp_power_reply.header.magic), (sizeof(tcp_power_reply) - sizeof(tcp_power_reply.header.magic)) - sizeof(tcp_power_reply.csum));        
        if(crc1!=tcp_power_reply.csum){
            printf("crc check sum error\n");
        }
        else{
            printf("tcp_power_reply.header.magic:%d\n",tcp_power_reply.header.magic);
            printf("tcp_power_reply.header.length:%d\n",tcp_power_reply.header.length);
            printf("tcp_power_reply.header.uSnumber:%d\n",tcp_power_reply.header.uSnumber);
            printf("tcp_power_reply.header.commandID:%d\n",tcp_power_reply.header.commandID);
            printf("tcp_power_reply.header.externalMode:%d\n",tcp_power_reply.header.externalMode);            
            printf("tcp_power_reply.csum:%d\n",tcp_power_reply.csum);
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                    if(tcp_power_reply.chassis_payload.test_result[i]==0)
                        printf("us Number %d power cycle  fail\n",i+1);
                    else
                        printf("us Number %d power cycle success\n",i+1);
            }
            
        }
    }
    return n;
}

int tcp_client_hw_information_init(US_HW_INFORMATION_T* hw_inform)
{
    hw_inform->us_card_number=2;
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

int tcp_client_send_hw_information_request(int sockfd, struct sockaddr_in* servaddr)
{
    //char sendline[1000]="this is a test!";
    char recvline[10000];
    int n,i;
    struct tcp_packet_hwinforequest_command tcp_hw_command;
    struct tcp_packet_hwinforeplay_command tcp_hw_reply;
    uint8_t *data = (uint8_t *)&tcp_hw_command;
    uint8_t crc1,crc2,crc3;

    tcp_hw_command.header.magic = 0xFBFB;
    tcp_hw_command.header.length = 9;
    tcp_hw_command.header.uSnumber = us_card_number;
    tcp_hw_command.header.commandID = HW_INFORMATION_REQUEST;
    tcp_hw_command.header.externalMode = 0x2;
    tcp_client_hw_information_init(&tcp_hw_command.payload);
    tcp_hw_command.csum = tcp_client_calculate_crc8_checksum_of_buffer(data + sizeof(tcp_hw_command.header.magic), (sizeof(tcp_hw_command) - sizeof(tcp_hw_command.header.magic)) - sizeof(tcp_hw_command.csum));

    sendto(sockfd,&tcp_hw_command,sizeof(struct tcp_packet_hwinforequest_command),0,
             (struct sockaddr *)servaddr,sizeof(struct sockaddr_in));
      
    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    if(n>0)
    {
        memcpy(&tcp_hw_reply,recvline,sizeof(struct tcp_packet_hwinforeplay_command));
        crc1 = tcp_client_calculate_crc8_checksum_of_buffer(recvline + sizeof(tcp_hw_reply.header.magic), (sizeof(tcp_hw_reply) - sizeof(tcp_hw_reply.header.magic)) - sizeof(tcp_hw_reply.csum));
        if(crc1!=tcp_hw_reply.csum){
            printf("crc check sum error\n");
        }
        else{
            printf("tcp_hw_reply.header.magic:%d\n",tcp_hw_reply.header.magic);
            printf("tcp_hw_reply.header.length:%d\n",tcp_hw_reply.header.length);
            printf("tcp_hw_reply.header.uSnumber:%d\n",tcp_hw_reply.header.uSnumber);
            printf("tcp_hw_reply.header.commandID:%d\n",tcp_hw_reply.header.commandID);
            printf("tcp_hw_reply.header.externalMode:%d\n",tcp_hw_reply.header.externalMode);
            printf("tcp_hw_reply.csum:%d\n",tcp_hw_reply.csum);
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                    if(tcp_hw_reply.payload[i].us_card_number==0)
                        printf("us Number %d hw information  fail\n",i+1);
                    else
                    {
                        printf("us Number %d hw information success\n",i+1);

    printf("us_card_number:%d\n",tcp_hw_reply.payload[i].us_card_number);
    printf("bios_vender:%s\n",tcp_hw_reply.payload[i].bios_vender);
    printf("bios_version:%s\n",tcp_hw_reply.payload[i].bios_version);
    printf("bios_revision:%s\n",tcp_hw_reply.payload[i].bios_revision);
    printf("bios_fw_revision:%s\n",tcp_hw_reply.payload[i].bios_fw_revision);
    printf("cpu_manufacturer:%s\n",tcp_hw_reply.payload[i].cpu_manufacturer);
    printf("cpu_version:%s\n",tcp_hw_reply.payload[i].cpu_version);
    printf("cpu_clock:%s\n",tcp_hw_reply.payload[i].cpu_clock);
    printf("cpu_core:%s\n",tcp_hw_reply.payload[i].cpu_core);
    printf("cpu_maxspeed:%s\n",tcp_hw_reply.payload[i].cpu_maxspeed);
    printf("dimm_size:%s\n",tcp_hw_reply.payload[i].dimm_size);
    printf("dimm_factor:%s\n",tcp_hw_reply.payload[i].dimm_factor);
    printf("dimm_type:%s\n",tcp_hw_reply.payload[i].dimm_type);
    printf("dimm_speed:%s\n",tcp_hw_reply.payload[i].dimm_speed);
    printf("dimm_manufacturer:%s\n",tcp_hw_reply.payload[i].dimm_manufacturer);
    printf("dimm_sn:%s\n",tcp_hw_reply.payload[i].dimm_sn);
    printf("dimm_pn:%s\n",tcp_hw_reply.payload[i].dimm_pn);
    printf("ssd_vender:%s\n",tcp_hw_reply.payload[i].ssd_vender);
    printf("ssd_sn:%s\n",tcp_hw_reply.payload[i].ssd_sn);
    printf("ssd_fw_version:%s\n",tcp_hw_reply.payload[i].ssd_fw_version);
                      
                    }
            }            
        }
    }
    return n;
}

int tcp_client_send_traffic_test_request(int sockfd, struct sockaddr_in* servaddr, int option)
{
    //char sendline[1000]="this is a test!";
    char recvline[10000];
    int n,i;
    struct tcp_packet_traffic_command tcp_traffic_command;
    struct tcp_packet_traffic_command tcp_traffic_reply;
    uint8_t *data = (uint8_t *)&tcp_traffic_command;
    uint8_t crc1,crc2,crc3;

    tcp_traffic_command.header.magic = 0xFBFB;
    tcp_traffic_command.header.length = 9;
    tcp_traffic_command.header.uSnumber = us_card_number;
    tcp_traffic_command.header.commandID = TRAFFIC_REQUEST;
    tcp_traffic_command.header.externalMode = 0x2;
    tcp_traffic_command.payload = option+1;
    tcp_traffic_command.csum = tcp_client_calculate_crc8_checksum_of_buffer(data + sizeof(tcp_traffic_command.header.magic), (sizeof(tcp_traffic_command) - sizeof(tcp_traffic_command.header.magic)) - sizeof(tcp_traffic_command.csum));

       
        printf("tcp_traffic_command.header.magic:%d\n",tcp_traffic_command.header.magic);
        printf("tcp_traffic_command.header.length:%d\n",tcp_traffic_command.header.length);
        printf("tcp_traffic_command.header.uSnumber:%d\n",tcp_traffic_command.header.uSnumber);
        printf("tcp_traffic_command.header.commandID:%d\n",tcp_traffic_command.header.commandID);
        printf("tcp_traffic_command.header.externalMode:%d\n",tcp_traffic_command.header.externalMode);
        printf("tcp_traffic_command.payload:%d\n",tcp_traffic_command.payload);
        printf("tcp_traffic_command.csum:%d\n",tcp_traffic_command.csum);
   
    sendto(sockfd,&tcp_traffic_command,sizeof(struct tcp_packet_traffic_command),0,
             (struct sockaddr *)servaddr,sizeof(struct sockaddr_in));
      
    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    if(n>0)
    {
        memcpy(&tcp_traffic_reply,recvline,sizeof(struct tcp_packet_traffic_command));
        crc1 = tcp_client_calculate_crc8_checksum_of_buffer(recvline + sizeof(tcp_traffic_reply.header.magic), (sizeof(tcp_traffic_reply) - sizeof(tcp_traffic_reply.header.magic)) - sizeof(tcp_traffic_reply.csum));
        if(crc1!=tcp_traffic_reply.csum){
            printf("crc check sum error, %d:%d\n",crc1,tcp_traffic_reply.csum);
        }
        else{
            printf("tcp_traffic_reply.header.magic:%d\n",tcp_traffic_reply.header.magic);
            printf("tcp_traffic_reply.header.length:%d\n",tcp_traffic_reply.header.length);
            printf("tcp_traffic_reply.header.uSnumber:%d\n",tcp_traffic_reply.header.uSnumber);
            printf("tcp_traffic_reply.header.commandID:%d\n",tcp_traffic_reply.header.commandID);
            printf("tcp_traffic_reply.header.externalMode:%d\n",tcp_traffic_reply.header.externalMode);      
            printf("tcp_traffic_reply.csum:%d\n",tcp_traffic_reply.csum);
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                    if(tcp_traffic_reply.chassis_payload.result[i]!=4)
                        printf("us Number %d traffic test fail(%d)\n",i+1,tcp_traffic_reply.chassis_payload.result[i]);
                    else
                        printf("us Number %d traffic test success(%d)\n",i+1,tcp_traffic_reply.chassis_payload.result[i]);
            }            
        }
    }
    return n;
}

int tcp_client_send_health_test_request(int sockfd, struct sockaddr_in* servaddr, int option)
{
    char recvline[10000];
    int n,i;
    struct tcp_packet_traffic_command tcp_traffic_command;
    struct tcp_packet_traffic_command tcp_traffic_reply;
    uint8_t *data = (uint8_t *)&tcp_traffic_command;
    uint8_t crc1,crc2,crc3;

    tcp_traffic_command.header.magic = 0xFBFB;
    tcp_traffic_command.header.length = 9;
    tcp_traffic_command.header.uSnumber = us_card_number;
    tcp_traffic_command.header.commandID = HEALTH_REQUEST;
    tcp_traffic_command.header.externalMode = 0x2;
    tcp_traffic_command.payload = option+1;
    tcp_traffic_command.csum = tcp_client_calculate_crc8_checksum_of_buffer(data + sizeof(tcp_traffic_command.header.magic), (sizeof(tcp_traffic_command) - sizeof(tcp_traffic_command.header.magic)) - sizeof(tcp_traffic_command.csum));
 
        printf("============================= TX PACKET ==============================\r\n");
        printf("tcp_traffic_command.header.magic:%d\n",tcp_traffic_command.header.magic);
        printf("tcp_traffic_command.header.length:%d\n",tcp_traffic_command.header.length);
        printf("tcp_traffic_command.header.uSnumber:%d\n",tcp_traffic_command.header.uSnumber);
        printf("tcp_traffic_command.header.commandID:%d\n",tcp_traffic_command.header.commandID);
        printf("tcp_traffic_command.header.externalMode:%d\n",tcp_traffic_command.header.externalMode);
        printf("tcp_traffic_command.payload:%d\n",tcp_traffic_command.payload);
        printf("tcp_traffic_command.csum:%d\n",tcp_traffic_command.csum);
        printf("======================================================================\r\n");
  
    sendto(sockfd,&tcp_traffic_command,sizeof(struct tcp_packet_traffic_command),0,
             (struct sockaddr *)servaddr,sizeof(struct sockaddr_in));
      
    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    if(n>0)
    {
        memcpy(&tcp_traffic_reply,recvline,sizeof(struct tcp_packet_traffic_command));
        crc1 = tcp_client_calculate_crc8_checksum_of_buffer(recvline + sizeof(tcp_traffic_reply.header.magic), (sizeof(tcp_traffic_reply) - sizeof(tcp_traffic_reply.header.magic)) - sizeof(tcp_traffic_reply.csum));
        if(crc1!=tcp_traffic_reply.csum){
            printf("crc check sum error, %d:%d\n",crc1,tcp_traffic_reply.csum);
        }
        else{        
            printf("============================= RX PACKET ==============================\r\n");
            printf("tcp_traffic_reply.header.magic:%d\n",tcp_traffic_reply.header.magic);
            printf("tcp_traffic_reply.header.length:%d\n",tcp_traffic_reply.header.length);
            printf("tcp_traffic_reply.header.uSnumber:%d\n",tcp_traffic_reply.header.uSnumber);
            printf("tcp_traffic_reply.header.commandID:%d\n",tcp_traffic_reply.header.commandID);
            printf("tcp_traffic_reply.header.externalMode:%d\n",tcp_traffic_reply.header.externalMode);      
            printf("tcp_traffic_reply.csum:%d\n",tcp_traffic_reply.csum);
            printf("======================================================================\r\n");
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                if(tcp_traffic_reply.chassis_payload.result[i]==0)
                    printf("us Number %2d traffic test fail\n",i+1);
                else
                    printf("us Number %2d traffic test success\n",i+1);
            }            
        }
    }
    return n;
}

int tcp_client_send_anytoany_test_request(int sockfd, struct sockaddr_in* servaddr, int select, int input_card
    , int input_port, int output_card, int output_port, int src_bp, int dest_bp)
{
    //char sendline[1000]="this is a test!";
    char recvline[10000];
    int n,i;
    struct tcp_packet_traffic_command tcp_traffic_command;
    struct tcp_packet_traffic_command tcp_traffic_reply;
    uint8_t *data = (uint8_t *)&tcp_traffic_command;
    uint8_t crc1,crc2,crc3;

    tcp_traffic_command.header.magic = 0xFBFB;
    tcp_traffic_command.header.length = 9;
    tcp_traffic_command.header.uSnumber = us_card_number;
    tcp_traffic_command.header.commandID = ANYTOANY_REQUEST;
    tcp_traffic_command.header.externalMode = 0x2;
    tcp_traffic_command.paras.select = select;
    tcp_traffic_command.paras.input_card = input_card;
    tcp_traffic_command.paras.input_port = input_port;
    tcp_traffic_command.paras.output_card = output_card;
    tcp_traffic_command.paras.output_port = output_port;
    tcp_traffic_command.paras.src_bp = src_bp;
    tcp_traffic_command.paras.dest_bp = dest_bp;

    tcp_traffic_command.csum = tcp_client_calculate_crc8_checksum_of_buffer(data + sizeof(tcp_traffic_command.header.magic), (sizeof(tcp_traffic_command) - sizeof(tcp_traffic_command.header.magic)) - sizeof(tcp_traffic_command.csum));
      
        printf("tcp_traffic_command.header.magic:%d\n",tcp_traffic_command.header.magic);
        printf("tcp_traffic_command.header.length:%d\n",tcp_traffic_command.header.length);
        printf("tcp_traffic_command.header.uSnumber:%d\n",tcp_traffic_command.header.uSnumber);
        printf("tcp_traffic_command.header.commandID:%d\n",tcp_traffic_command.header.commandID);
        printf("tcp_traffic_command.header.externalMode:%d\n",tcp_traffic_command.header.externalMode);
        printf("tcp_traffic_command.payload:%d\n",tcp_traffic_command.payload);
        printf("tcp_traffic_command.csum:%d\n",tcp_traffic_command.csum);
   
    sendto(sockfd,&tcp_traffic_command,sizeof(struct tcp_packet_traffic_command),0,
             (struct sockaddr *)servaddr,sizeof(struct sockaddr_in));
      
    n=recvfrom(sockfd,recvline,10000,0,NULL,NULL);
    if(n>0)
    {
        memcpy(&tcp_traffic_reply,recvline,sizeof(struct tcp_packet_traffic_command));
        crc1 = tcp_client_calculate_crc8_checksum_of_buffer(recvline + sizeof(tcp_traffic_reply.header.magic), (sizeof(tcp_traffic_reply) - sizeof(tcp_traffic_reply.header.magic)) - sizeof(tcp_traffic_reply.csum));
        if(crc1!=tcp_traffic_reply.csum){
            printf("crc check sum error, %d:%d\n",crc1,tcp_traffic_reply.csum);
        }
        else{
            printf("tcp_traffic_reply.header.magic:%d\n",tcp_traffic_reply.header.magic);
            printf("tcp_traffic_reply.header.length:%d\n",tcp_traffic_reply.header.length);
            printf("tcp_traffic_reply.header.uSnumber:%d\n",tcp_traffic_reply.header.uSnumber);
            printf("tcp_traffic_reply.header.commandID:%d\n",tcp_traffic_reply.header.commandID);
            printf("tcp_traffic_reply.header.externalMode:%d\n",tcp_traffic_reply.header.externalMode);      
            printf("tcp_traffic_reply.csum:%d\n",tcp_traffic_reply.csum);
            for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
            {
                    if(tcp_traffic_reply.chassis_payload.result[i]!=4)
                        printf("us Number %d any to any test fail(%d)\n",i+1,tcp_traffic_reply.chassis_payload.result[i]);
                    else
                        printf("us Number %d any to any  test success(%d)\n",i+1,tcp_traffic_reply.chassis_payload.result[i]);
            }            
        }
    }
    return n;
}

int tcp_client_diag_function_search_string(char *file_name, char *delim, char *out_string_buf, int buf_max_len, int string_line, int string_number)
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
		printf("fopen(%s): Error!\r\n", file_name);
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

