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

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdint.h>
#include "chassis.h"

int tcp_server_init(void);
int tcp_server_accept(int sockfd);
void tcp_server_do_processing (int sock);
int tcp_server_close(int sock);

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

US_HW_DATA_T us_hw_information[MAXIMUM_MICRO_SERVER_CARD_NUMBER];

struct tcp_packet_hwinforequest_command { 
    struct tcp_packet_command_header header;
    US_HW_DATA_T payload;
    uint8_t csum;
} __attribute__((packed));


struct tcp_packet_hwinforeplay_command { 
    struct tcp_packet_command_header header;
    US_HW_DATA_T us_hw_information[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
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

int tcp_server_diag_function_search_string(char *file_name, char *delim, char *out_string_buf, int buf_max_len, int string_line, int string_number);

#endif
