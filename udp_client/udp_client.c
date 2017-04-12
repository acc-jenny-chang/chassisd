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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>

#define ONLP 0
#define VLAN_INTERNAL_INTERFACE 1
#define THERMAL_PLAN 1

#define VERSION_STRING "chassis-0.0.0.9R0B"

#define MAXIMUM_MICRO_SERVER_CARD_NUMBER 48

#define PORT 0xFBFB
#define BUFFSIZE 10000

#define PING_TEST_REQUEST         10109
#define PING_TEST_REPLY           20109
#define POWER_CYCLE_REQUEST       10110
#define POWER_CYCLE_REPLY         20110
#define HW_INFORMATION_REQUEST    10054
#define HW_INFORMATION_REPLY      20054
#define TRAFFIC_REQUEST    10211
#define TRAFFIC_RESULT_REQUEST    11211
#define TRAFFIC_REPLY      20211
#define TRAFFIC_RESULT      30211

#define CTP_INFORMATION_REQUEST    10001
#define CTP_INFORMATION_REPLY      20001

#define CTP_REMOTESHUTDOWN_REQUEST    10002
#define CTP_REMOTESHUTDOWN_REPLY      20002

#define HEALTH_REQUEST    10996
#define HEALTH_RESULT_REQUEST    11996
#define HEALTH_REPLY      20996
#define HEALTH_RESULT      30996

#define ANYTOANY_REQUEST    10212
#define ANYTOANY_RESULT_REQUEST    11212
#define ANYTOANY_REPLY      20212
#define ANYTOANY_RESULT      30212

#define RESULT_SUCCESS      4   
#define RESULT_FAILED         3 
#define RESULT_ACK               2
#define RESULT_READY  1
#define RESULT_NOT_READY  0


#define MY_CARD_NUMBER 2

#define MAXIMUM_BUFFER_LENGTH 512
#define MAXIMUM_STRING_LENGTH				(MAXIMUM_BUFFER_LENGTH - 1)

#if (ONLP>0)
#define CONFIG_US_CARD_NUMBER "/usr/sbin/us_card_number"
#else
#define CONFIG_US_CARD_NUMBER "/usr/local/accton/parameter/us_card_number"
#endif

static unsigned int us_card_number;

typedef struct {
       unsigned int us_card_number;
	char bios_vender[32];
	char bios_version[16];
	char bios_revision[4];
	char bios_fw_revision[4];
	char cpu_manufacturer[32];
	char cpu_version[64];
	char cpu_clock[8];
	char cpu_core[4];
	char cpu_maxspeed[8];
	char dimm_size[8];
	char dimm_factor[8];
	char dimm_type[8];
	char dimm_speed[8];
	char dimm_manufacturer[16];
	char dimm_sn[16];
	char dimm_pn[32];
	char ssd_vender[32];
	char ssd_sn[32];
	char ssd_fw_version[8];
} __attribute__((__packed__)) US_HW_DATA_T;

#define NR16(p) (p[0] << 8 | p[1])
#define NR32(p) ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3])

typedef enum {
    STATE_BLOCK=0,
    STATE_LISTENING,
    STATE_LEARNING,
    STATE_FORWARDING,
    STATE_DISABLE=255,
}chassis_ctpm_state;

typedef struct timevalues_t {
  unsigned short MessageAge;
  unsigned short MaxAge;
  unsigned short ForwardDelay;
  unsigned short HelloTime;
} TIMEVALUES_T;

typedef enum {
  CTP_DISABLED,
  CTP_ENABLED,
} UID_CTP_MODE_T;

typedef struct bridge_id
{
  unsigned short    prio;
  unsigned char     addr[6];
} BRIDGE_ID;

typedef unsigned short  PORT_ID;

typedef struct prio_vector_t {
  BRIDGE_ID root_bridge;
  unsigned long root_path_cost;
  BRIDGE_ID design_bridge;
  PORT_ID   design_port;
  PORT_ID   bridge_port;
} PRIO_VECTOR_T;

typedef struct ctpm_t {
  PORT_ID       port;

  /* The only "per bridge" state machine */
  chassis_ctpm_state         state;   /* the Port Role Selection State machione: 17.22 */

  /* variables */
  /*PROTOCOL_VERSION_T    ForceVersion;*/   /* 17.12, 17.16.1 */
  BRIDGE_ID             BrId;           /* 17.17.2 */
  TIMEVALUES_T          BrTimes;        /* 17.17.4 */
  PORT_ID               rootPortId;     /* 17.17.5 */
  PORT_ID               rootLCPortId;     
  PRIO_VECTOR_T         rootPrio;       /* 17.17.6 */
  TIMEVALUES_T          rootTimes;      /* 17.17.7 */
  
  int                   vlan_id;        /* let's say: tag */
  char*                 name;           /* name of the VLAN, maily for debugging */
  UID_CTP_MODE_T        admin_state;    /* CTP_DISABLED or CTP_ENABLED; type see in UiD */

  unsigned long         timeSince_Topo_Change; /* 14.8.1.1.3.b */
  unsigned long         Topo_Change_Count;     /* 14.8.1.1.3.c */
  unsigned char         Topo_Change;           /* 14.8.1.1.3.d */

  char chassis_exist[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
} CTPM_T;

struct udp_packet_command_header{
    unsigned int us_card_number;
    uint16_t type;
    uint16_t length;
    char params[256];
    uint16_t params_length;    
}__attribute__((packed));

int udp_client_dump_ctpm(CTPM_T* this);

int udp_client_hw_information_init(US_HW_DATA_T* hw_inform)
{
    hw_inform->us_card_number=us_card_number;
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

int main(int argc, char**argv)
{
    int sockfd,n;
    struct sockaddr_in servaddr,cliaddr;
    char sendline[BUFFSIZE];
    char recvline[BUFFSIZE];
    struct udp_packet_command_header sender,receiver;
    int operation;
    int ret = -1;
    char buf[10000] = {0};
    US_HW_DATA_T hw_inform,us_hw_data;
    int payload;
    char params[256];
    int i, param_length;
    CTPM_T ctp_inform;

    printf("Chassis Version = (%s)\n\r",VERSION_STRING);
    printf("Chassis Managment for Chassis utility\n\r");
     
    if (argc < 3)
    {
        printf("usage:  chass_client <IP address> <ID> <params>\n");
        exit(1);
    }

    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr(argv[1]);
    servaddr.sin_port=htons(PORT);

    memset(sendline,0,BUFFSIZE);
    memset(params,0,256);
	
    ret = udp_client_diag_function_search_string(CONFIG_US_CARD_NUMBER, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 1, 1);
    if (ret != 0)
    {
        printf("diag_function_search_string(%d): Error!\r\n", ret);
        fflush(stdout);
        ret = -13;
    }
    us_card_number = (unsigned int) strtoul(buf, NULL, 10);
	
    sender.us_card_number = us_card_number;
    operation = atoi(argv[2]);

    switch(operation)
    {
        case 1:
            sender.type = CTP_INFORMATION_REQUEST;
            sender.length=sizeof(int);   
            memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));            
            break;
        case 2:
            sender.type = CTP_REMOTESHUTDOWN_REQUEST;
            sender.length=sizeof(int);

            /* parse parameter*/
            if(argc>3) {
                for(i=3;i<argc;i++){
                    strcat(params,argv[i]);
                    strcat(params," ");
                }
                param_length = strlen(params);
                memcpy(sender.params,params,param_length);
                sender.params_length = param_length;
                printf("(%d:%s)\n\r",sender.params_length,sender.params);
            }

            memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));            
            break;
        case 109:
            sender.type = PING_TEST_REQUEST;
            sender.length=sizeof(int);   
            memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
            break;
        case 110:
            sender.type = POWER_CYCLE_REQUEST;
            sender.length=sizeof(int);   
            memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
            break;
        case 54:
            sender.type = HW_INFORMATION_REQUEST;
            sender.length=sizeof(US_HW_DATA_T);
            udp_client_hw_information_init(&hw_inform); 
            memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
            memcpy(sendline+sizeof(struct udp_packet_command_header),&hw_inform,sizeof(US_HW_DATA_T));
            break;
         default:
            if(operation<=10000)
            {
                printf("option must bigger than 10001\n\r");
                return 1;
            }
 #if 1
                sender.type = operation;
                sender.length=sizeof(int);
                /* parse parameter*/
                if(argc>3) {
                    for(i=3;i<argc;i++){
                        strcat(params,argv[i]);
                        strcat(params," ");
                    }
                    param_length = strlen(params);
                    memcpy(sender.params,params,param_length);
                    sender.params_length = param_length;
                    printf("(%d:%s)\n\r",sender.params_length,sender.params);
                }
                memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
            break;
 #else
            sender.type = operation+10000;
            sender.length=sizeof(int);   
            memcpy(sendline,&sender,sizeof(struct udp_packet_command_header));
            break;		 	
            printf("\nUnknow command!\n");
#endif                
        }
      sendto(sockfd,sendline,sizeof(struct udp_packet_command_header)+sender.length,0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      n=recvfrom(sockfd,recvline,BUFFSIZE,0,NULL,NULL);
      if(n>0)
      {
            memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
            switch(receiver.type)
            {
                case PING_TEST_REPLY:
                    memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                    printf("receive ping test reply, usNumber:%d, error:%d\n",receiver.us_card_number,payload);
                    break;
                case POWER_CYCLE_REPLY:
                    memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                    printf("receive power cycle test reply, usNumber:%d,error:%d\n",receiver.us_card_number,payload);
                    break;
                case HW_INFORMATION_REPLY:
                    printf("receive HW information reply\n");
#if 1
	{
	       memcpy(&ctp_inform,recvline+sizeof(struct udp_packet_command_header),sizeof(CTPM_T));
	       printf("US Card Number        : %d\r\n", us_hw_data.us_card_number); 
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
                    break;
                case CTP_INFORMATION_REPLY:
{
                    memcpy(&ctp_inform,recvline+sizeof(struct udp_packet_command_header),sizeof(US_HW_DATA_T));
                    udp_client_dump_ctpm(&ctp_inform);
}
                    break;
                case TRAFFIC_REPLY:
                    memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                    printf("receive traffic test ack reply, usNumber:%d, error:%d\n",receiver.us_card_number,(payload==RESULT_ACK)?0:1);
                    if(payload==RESULT_ACK)
                    {
                        n=recvfrom(sockfd,recvline,BUFFSIZE,0,NULL,NULL);
                        if(n>0)
                        {
                            memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
                            if (receiver.type==TRAFFIC_RESULT)
                            {
                                memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                                printf("receive traffic test result reply, usNumber:%d, error:%d\n",receiver.us_card_number,(payload==RESULT_SUCCESS)?0:1);
                            }
                            else 
                            {
                                memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                                printf("receive unknown reply, usNumber:%d, error:%d\n",receiver.us_card_number,payload);
                            }
                        }
                    }
                    break;
                case CTP_REMOTESHUTDOWN_REPLY:
                    memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                    printf("receive remote shutdown reply, usNumber:%d,error:%d\n",receiver.us_card_number,payload);
                    break;
                case ANYTOANY_REPLY:
                    memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                    printf("receive any to any test ack reply, usNumber:%d, error:%d\n",receiver.us_card_number,(payload==RESULT_ACK)?0:1);
                    if(payload==RESULT_ACK)
                    {
                        n=recvfrom(sockfd,recvline,BUFFSIZE,0,NULL,NULL);
                        if(n>0)
                        {
                            memcpy(&receiver,recvline,sizeof(struct udp_packet_command_header));
                            if (receiver.type==ANYTOANY_RESULT)
                            {
                                memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                                printf("receive any to any test result reply, usNumber:%d, error:%d\n",receiver.us_card_number,(payload==RESULT_SUCCESS)?0:1);
                            }
                            else 
                            {
                                memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                                printf("receive unknown reply, usNumber:%d, error:%d\n",receiver.us_card_number,payload);
                            }
                        }
                    }
                    break;
		case 255:
                    printf("response unknow command!\n");
                    break;
                default:
 /*others*/
                    memcpy(&payload,recvline+sizeof(struct udp_packet_command_header),sizeof(2));
                    printf("receive request item:%d test reply, usNumber:%d, error:%d\n",receiver.type,receiver.us_card_number,payload);
                    //printf("response unknow!\n");
                    break;
            }
      }
      else{
        printf("no response !\n");
      }
   close(sockfd);
   exit(0);
}

int udp_client_diag_function_search_string(char *file_name, char *delim, char *out_string_buf, int buf_max_len, int string_line, int string_number)
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

int udp_client_id_to_str(int id, char* card)
{
    int ret =0;    
    int index = id/2+1;

    if(index>=1&& index<=16)
        sprintf(card,"LC%d",index);
    else
        sprintf(card,"FC%d",(index-16));


    return ret;
}

int udp_client_dump_ctpm(CTPM_T* this)
{
    CTPM_T* ctpm = this;
    char* state[]={"BLOCK","LISTENING","STATE_LEARNING","STATE_FORWARDING","STATE_DISABLE"};
    char* admin_state[]={"DISABLED","ENABLED"};
    char card[10];
    int i,j;

    printf("CTPM Information:\n\r");
    printf("port :%d\n\r",ctpm->port);    
    memset(card,0,10);
    udp_client_id_to_str((htons(ctpm->BrId.prio)&0x00FF),card);
    printf("BRIDGE_ID : %s %02x:%02x:%02x:%02x:%02x:%02x\n\r",
        card,
        ctpm->BrId.addr[0],
        ctpm->BrId.addr[1],
        ctpm->BrId.addr[2],
        ctpm->BrId.addr[3],
        ctpm->BrId.addr[4],
        ctpm->BrId.addr[5]);
    printf("BrTimes :\n\r");
    printf("              MessageAge :%d\n\r",ctpm->BrTimes.MessageAge);
    printf("              MaxAge :%d\n\r",ctpm->BrTimes.MaxAge);
    printf("              ForwardDelay :%d\n\r",ctpm->BrTimes.ForwardDelay);
    printf("              HelloTime :%d\n\r",ctpm->BrTimes.HelloTime);
    printf("rootPortId :%d\n\r",ctpm->rootPortId);
    printf("rootLCPortId :%d\n\r",ctpm->rootLCPortId);
    printf("rootPrio :\n\r");
    memset(card,0,10);
    udp_client_id_to_str((htons(ctpm->rootPrio.root_bridge.prio)&0x00FF),card);  
    printf("              root_bridge : %s %02x:%02x:%02x:%02x:%02x:%02x\n\r",
        card,
        ctpm->rootPrio.root_bridge.addr[0],
        ctpm->rootPrio.root_bridge.addr[1],
        ctpm->rootPrio.root_bridge.addr[2],
        ctpm->rootPrio.root_bridge.addr[3],
        ctpm->rootPrio.root_bridge.addr[4],
        ctpm->rootPrio.root_bridge.addr[5]);
    printf("              root_path_cost :%lu\n\r",ctpm->rootPrio.root_path_cost);
    memset(card,0,10);
    udp_client_id_to_str((htons(ctpm->rootPrio.design_bridge.prio)&0x00FF),card);  
    printf("              design_bridge : %s %02x:%02x:%02x:%02x:%02x:%02x\n\r",
        card,
        ctpm->rootPrio.design_bridge.addr[0],
        ctpm->rootPrio.design_bridge.addr[1],
        ctpm->rootPrio.design_bridge.addr[2],
        ctpm->rootPrio.design_bridge.addr[3],
        ctpm->rootPrio.design_bridge.addr[4],
        ctpm->rootPrio.design_bridge.addr[5]);
    printf("              design_port :%d\n\r",ctpm->rootPrio.design_port);
    printf("              bridge_port :%d\n\r",ctpm->rootPrio.bridge_port);
    printf("rootTimes :\n\r");
    printf("              MessageAge :%d\n\r",ctpm->rootTimes.MessageAge);
    printf("              MaxAge :%d\n\r",ctpm->rootTimes.MaxAge);
    printf("              ForwardDelay :%d\n\r",ctpm->rootTimes.ForwardDelay);
    printf("              HelloTime :%d\n\r",ctpm->rootTimes.HelloTime);
    printf("vlan_id :\n\r");
    printf("name :\n\r");
    if(0<=ctpm->admin_state&&ctpm->admin_state<=1)
        printf("admin state :%s\n\r",admin_state[ctpm->admin_state]);
    else
        printf("admin state :Unknown\n\r");
    printf("timeSince_Topo_Change :%lu\n\r",ctpm->timeSince_Topo_Change);
    printf("Topo_Change_Count :%lu\n\r",ctpm->Topo_Change_Count);
    printf("Topo_Change %d:\n\r",ctpm->Topo_Change);
    if(0<=ctpm->state&&ctpm->state<=3)
        printf("state :%s\n\r",state[ctpm->state]);
    else
        printf("state :%s\n\r",state[4]);
   
    printf("Chassis Information:\n\r");
    for(i=0,j=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i=i+2,j++)
    {
        if(i==0) {printf("LC:%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d%3d\n\r   ",j+1,j+2,j+3,j+4,j+5,j+6,j+7,j+8,j+9,j+10,j+11,j+12,j+13,j+14,j+15,j+16); }
        if(i==32) {j=0;printf("FC:%3d%3d%3d%3d%3d%3d%3d%3d\n\r   ",j+1,j+2,j+3,j+4,j+5,j+6,j+7,j+8); }
        if(ctpm->chassis_exist[i]>=1) printf("  Y");
        else if(ctpm->admin_state==1)printf("  N");
        else printf("  -");      
        if(i%32==30) printf("\n");
    }
    printf("\nEND\n\r");
    return 0;
}

