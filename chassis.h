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

#ifndef CHASSIS_H
#define CHASSIS_H


#include "chassis_bpdu.h"

#define VERSION_STRING "chassis-0.0.0.10R0B"

#define BACKPLANE_VERSION 1

#define SOCKET_PATH "/var/run/chassis-client"


#define FIRST_LC_SLOT_ID 1
#define SECOND_LC_SLOT_ID 17

#define LOG_FILE "/var/log/chassis"

#define PID_FILE "/var/run/chassis.pid"


/*
#define LC1_MULTICAST_ADDRESS "0x0180c2000011"
#define FC1_MULTICAST_ADDRESS "0x0180c2000022"
*/
#define LC1_MULTICAST_ADDRESS "0x0110b5000001"
#define FC1_MULTICAST_ADDRESS "0x0110b5000002"

#ifdef __GNUC__
#define PRINTFLIKE __attribute__((format(printf,1,2)))
#define noreturn   __attribute__((noreturn))
#else
#define PRINTFLIKE 
#define noreturn
#endif

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

#define ERROR_COMMAND      255

#define MAXIMUM_BUFFER_LENGTH 512
#define MAXIMUM_STRING_LENGTH				(MAXIMUM_BUFFER_LENGTH - 1)
#define MAXIMUM_LINECARD 32
#define MAXIMUM_FABRICCARD 16

#define BUFFSIZE 10000

#define DIAG_DEFAULT_INFORMATIN_FILE	"/tmp/.accton_diag_test_message.txt"
#define DIAG_DEFAULT_INFORMATIN_FILE_1	"/tmp/.accton_diag_test_message1.txt"


#define TIME_CHASSIS_PING       10
#define COUNT_CHASSIS_PING      6
#define TIME_CHASSIS_POWER      10
#define COUNT_CHASSIS_POWER     6
#define TIME_HW_INFORM      3
#define COUNT_HW_INFORM     6

#define TIME_TRAFFIC        3
#define TIME_TRAFFIC_1      240
#define COUNT_TRAFFIC     90

#define COUNT_HEALTH     100
#if 0
int Wprintf(char *fmt, ...) PRINTFLIKE;
void Eprintf(char *fmt, ...) PRINTFLIKE;
void SYSERRprintf(char *fmt, ...) PRINTFLIKE;
void Lprintf(char *fmt, ...) PRINTFLIKE;
void Gprintf(char *fmt, ...) PRINTFLIKE;
#else
int msg_wprintf(char *fmt, ...);
void msg_eprintf(char *fmt, ...);
void msg_sys_err_printf(char *fmt, ...);
void msg_lprintf(char *fmt, ...);
void msg_gprintf(char *fmt, ...);
#endif

enum option_ranges {
       O_COMMON = 500,
       O_DISKDB = 1000,
};

extern int open_logfile(char *fn);

enum syslog_opt { 
	SYSLOG_LOG = (1 << 0),		/* normal decoding output to syslog */
	SYSLOG_REMARK = (1 << 1), 	/* special warnings to syslog */
	SYSLOG_ERROR  = (1 << 2),	/* errors during operation to syslog */
	SYSLOG_ALL = SYSLOG_LOG|SYSLOG_REMARK|SYSLOG_ERROR,
	SYSLOG_FORCE = (1 << 3),
};

enum chassis_status{
    CHASSIS_IS_EXIST = 1,
    CHASSIS_PING_READY = 2,
    CHASSIS_POWER_READY = 4,
    CHASSIS_HWINFO_READY = 8
};


enum port_state{
    PORT_NO_SPANNING_TREE = 0,
    PORT_DISABLED = 1,
    PORT_BLOCKING = 2,
    PORT_LISTENING = 3,
    PORT_LEARNING = 4,
    PORT_FORWARDING = 5 
};


/*original backplane*/
#if (BACKPLANE_VERSION>1)
enum port_5396_id{
    PORT_16_C1=14,
    PORT_16_C2=15,
    PORT_16_LC0=5,
    PORT_16_LC1=4,
    PORT_16_LC2=7,
    PORT_16_LC3=6,
    PORT_16_LC4=1,
    PORT_16_LC5=0,
    PORT_16_LC6=3,
    PORT_16_LC7=2,
    PORT_16_FC=11,
    PORT_16_RJ45=9   
};
#else
/*Ver 0.1 backplane, LC0 and LC1 is reverse*/
enum port_5396_id{
    PORT_16_C1=14,
    PORT_16_C2=15,
    PORT_16_LC0=4,
    PORT_16_LC1=5,
    PORT_16_LC2=6,
    PORT_16_LC3=7,
    PORT_16_LC4=0,
    PORT_16_LC5=1,
    PORT_16_LC6=2,
    PORT_16_LC7=3,
    PORT_16_FC=11,
    PORT_16_RJ45=9   
};
#endif

enum port_5389_id{
    PORT_8_C1=0,
    PORT_8_C2=7,
    PORT_8_FC0=2,
    PORT_8_FC1=3,
    PORT_8_FC2=4,
    PORT_8_FC3=5
};

typedef struct {
       unsigned int us_card_number;
	unsigned char ipaddress[4];
       unsigned char mac[6];
       int type;
       int status;
}US_CARD_DATA_T;

typedef struct {
       unsigned int us_card_number;
       US_CARD_DATA_T us_card[MAXIMUM_MICRO_SERVER_CARD_NUMBER];       
	unsigned char ipaddress[4];
       unsigned char mac[6];
       int type;       
       int status;
       int topology_type;
       int internal_tag_vlan;
       int inetrnal_untag_vlan;
       CTPM_T ctpm;
       int local_Master[4];
}CHASSIS_DATA_T;

CHASSIS_DATA_T chassis_information;

struct i2c_device_data {
	unsigned long bus;
	unsigned long address;
	unsigned long offset;
	unsigned long read_bytes;
	unsigned char old_value[2];
	unsigned char new_value[2];
	unsigned char tmp_value[2];
	int is_failed;
};
typedef struct i2c_device_data I2C_DEVICE_DATA_T;


extern void chassis_usage(void);
extern enum syslog_opt syslog_opt;
extern int syslog_level;

#endif

