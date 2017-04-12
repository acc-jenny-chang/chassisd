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

#include <stdint.h>

#define ONLP 0
#define VLAN_INTERNAL_INTERFACE 1
#define THERMAL_PLAN 0

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


#define ERROR_COMMAND      FF

#define MAXIMUM_MICRO_SERVER_CARD_NUMBER 48

#if (ONLP>0)
#define CONFIG_US_CARD_NUMBER "/usr/sbin/us_card_number"
#else
#define CONFIG_US_CARD_NUMBER "/usr/local/accton/parameter/us_card_number"
#endif
typedef struct {	
    unsigned int test_result[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
} __attribute__((__packed__)) CHASSIS_PING_TEST_T;

typedef struct {	
    unsigned int test_result[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
} __attribute__((__packed__)) CHASSIS_POWER_CYCLE_T;

typedef struct {	
    unsigned int test_result[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
} __attribute__((__packed__)) CHASSIS_HW_TEST_T;

struct tcp_packet_command_header{
    uint16_t magic;
    uint16_t length;
    uint16_t uSnumber;
    uint16_t commandID;
    uint16_t externalMode;
}__attribute__((packed));

struct tcp_packet_ping_command { 
    struct tcp_packet_command_header header;
	union {
		uint16_t payload;
		CHASSIS_PING_TEST_T chassis_payload;
	};
    uint8_t csum;
} __attribute__((packed));

struct tcp_packet_ping_reply_command { 
    struct tcp_packet_command_header header;
	union {
		uint16_t payload;
		CHASSIS_PING_TEST_T chassis_payload;
	};
    uint8_t csum;
} __attribute__((packed));

struct tcp_packet_power_command { 
    struct tcp_packet_command_header header;
	union {
		uint16_t payload;
		CHASSIS_POWER_CYCLE_T chassis_payload;
	};
    uint8_t csum;
} __attribute__((packed));

struct tcp_packet_power_reply_command { 
    struct tcp_packet_command_header header;
	union {
		uint16_t payload;
		CHASSIS_POWER_CYCLE_T chassis_payload;
	};
    uint8_t csum;
} __attribute__((packed));


typedef struct US_HW_DATA_T{
       unsigned int us_card_number;
	unsigned char bios_vender[32];
	unsigned char bios_version[16];
	unsigned char bios_revision[4];
	unsigned char bios_fw_revision[4];
	unsigned char cpu_manufacturer[32];
	unsigned char cpu_version[64];
	unsigned char cpu_clock[8];
	unsigned char cpu_core[4];
	unsigned char cpu_maxspeed[8];
	unsigned char dimm_size[8];
	unsigned char dimm_factor[8];
	unsigned char dimm_type[8];
	unsigned char dimm_speed[8];
	unsigned char dimm_manufacturer[16];
	unsigned char dimm_sn[16];
	unsigned char dimm_pn[32];
	unsigned char ssd_vender[32];
	unsigned char ssd_sn[32];
	unsigned char ssd_fw_version[8];
} __attribute__((__packed__)) US_HW_INFORMATION_T;

US_HW_INFORMATION_T us_hw_information[MAXIMUM_MICRO_SERVER_CARD_NUMBER];

struct tcp_packet_hwinforequest_command { 
    struct tcp_packet_command_header header;
    US_HW_INFORMATION_T payload;
    uint8_t csum;
} __attribute__((packed));


struct tcp_packet_hwinforeplay_command { 
    struct tcp_packet_command_header header;
    US_HW_INFORMATION_T payload[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
    uint8_t csum;
} __attribute__((packed));

struct tcp_packet_command_traffic_payload {
	unsigned int result[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
} __attribute__((__packed__));


struct tcp_packet_command_anytoany_command_parameter{
 	unsigned int select;
	unsigned int input_card;
       unsigned int input_port;
       unsigned int output_card;
       unsigned int output_port;
       unsigned int src_bp;
       unsigned int dest_bp;
} __attribute__((__packed__));

struct tcp_packet_traffic_command {
	struct tcp_packet_command_header header;
	union {
		uint16_t payload;
		struct tcp_packet_command_traffic_payload chassis_payload;
     		struct tcp_packet_command_anytoany_command_parameter paras;
	};

	uint8_t csum;
} __attribute__((__packed__));


