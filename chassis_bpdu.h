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

#ifndef CHASSIS_BPDU_H
#define CHASSIS_BPDU_H

#include <stdint.h>
#include "log.h"
#include "netif_utils.h"

#define ONLP 0
#define VLAN_INTERNAL_INTERFACE 1
#define THERMAL_PLAN 1


#define PREFIX ""

#define LOG_DEV_FILENAME "/dev/chassis"
#define DIMM_DB_FILENAME PREFIX "/var/lib/memory-errors"
#if (ONLP>0)
#define CONFIG_FILENAME PREFIX "/usr/sbin/chassis.conf"
#define CONFIG_US_CARD_NUMBER PREFIX  "/usr/sbin/us_card_number"
#define CONFIG_ROOT_FC PREFIX  "/usr/sbin/root_fc"
#define INTERNAL_INTERFACE "ma1:0"
#define INTERNAL_VLAN_INTERFACE "ma1.100"
#define DEFAULT_MGMT	"ma1"
#else
#define CONFIG_FILENAME PREFIX "/usr/local/accton/bin/chassis.conf"
#define CONFIG_US_CARD_NUMBER PREFIX  "/usr/local/accton/parameter/us_card_number"
#define CONFIG_ROOT_FC PREFIX  "/usr/local/accton/parameter/root_fc"
#define INTERNAL_INTERFACE "eth0:0"
#define INTERNAL_VLAN_INTERFACE "eth0.100"
#define DEFAULT_MGMT	"eth0"
#endif

#define DEFAULT_IF	DEFAULT_MGMT

//#define DEFAULT_IF	INTERNAL_INTERFACE

#define INTERFACE_NAME_LENGTH 15
char DEFAULT_INTERFACE[INTERFACE_NAME_LENGTH];

#define MAXIMUM_MICRO_SERVER_CARD_NUMBER 48

#define BPDU_TYPE_CONFIG 0
#define BPDU_TYPE_HELLO 0xff
#define BPDU_TYPE_REPLY 0x01
#define BPDU_TYPE_LC_CHANGE 0x03
#define BPDU_TYPE_FC_CHANGE 0x04
#define BPDU_TYPE_TCN 0x80
#define BPDU_TYPE_RSTP 0x02

#define MIN_BPDU                7
#define BPDU_L_SAP              0x42
#define LLC_UI                  0x03
#define BPDU_PROTOCOL_ID        0x0000
#define BPDU_VERSION_ID         0x00
#define BPDU_VERSION_RAPID_ID   0x02

#define BPDU_TOPO_CHANGE_TYPE   0x80
#define BPDU_CONFIG_TYPE        0x00
#define LC_BPDU_CONFIG_TYPE        0x01
#define BPDU_RSTP 0x02


#define TOLPLOGY_CHANGE_BIT     0x01
#define PROPOSAL_BIT            0x02
#define PORT_ROLE_OFFS          2   /* 0x04 & 0x08 */
#define PORT_ROLE_MASK          (0x03 << PORT_ROLE_OFFS)
#define LEARN_BIT               0x10
#define FORWARD_BIT             0x20
#define AGREEMENT_BIT           0x40
#define TOLPLOGY_CHANGE_ACK_BIT 0x80

#define FABRIC_CARD_TYPE 0
#define LINE_CARD_TYPE 1


#define DEFAULT_INTERVAL_TIME 200*1000 /*200 mili second*/
#define DEFAULT_HELLO_TIME  200
#define DEFAULT_MAXAGE  15
#define DEFAULT_FORWARDDELAY 3
#define DEFAULT_MESSAGEAGE  1

typedef struct mac_header_t {
  unsigned char dst_mac[6];
  unsigned char src_mac[6];
} MAC_HEADER_T;

typedef struct eth_header_t {
  unsigned char len8023[2];
  unsigned char dsap;
  unsigned char ssap;
  unsigned char llc;
} ETH_HEADER_T;

typedef struct bpdu_header_t {
  unsigned char protocol[2];
  unsigned char version;
  unsigned char bpdu_type;
} BPDU_HEADER_T;

typedef struct bpdu_body_t {
  unsigned char flags;
  unsigned char root_id[8];
  unsigned char root_path_cost[4];
  unsigned char bridge_id[8];
  unsigned char port_id[2];
  unsigned char message_age[2];
  unsigned char max_age[2];
  unsigned char hello_time[2];
  unsigned char forward_delay[2];
} BPDU_BODY_T;

typedef struct stp_bpdu_t {
  unsigned char dsap;
  unsigned char ssap;
  unsigned char llc;    
  BPDU_HEADER_T hdr;
  BPDU_BODY_T   body;
  unsigned char ver_1_len[2];
} BPDU_T;


typedef struct rstp_bpdu_t {
  MAC_HEADER_T mac;
  ETH_HEADER_T  eth;
  BPDU_HEADER_T hdr;
  BPDU_BODY_T   body;
  unsigned char ver_1_len[2];
  char chassis_exist[MAXIMUM_MICRO_SERVER_CARD_NUMBER];
} RSTP_BPDU_T;

struct ctp_header {
	uint8_t dsap;
	uint8_t ssap;
	uint8_t ctrl;
	uint16_t pid;
	uint8_t vers;
	uint8_t type;
};

struct ctp_config_pdu {
	uint8_t flags;
	uint8_t root[8];
	uint8_t root_cost[4];
	uint8_t sender[8];
	uint8_t port[2];
	uint8_t msg_age[2];
	uint8_t max_age[2];
	uint8_t hello_time[2];
	uint8_t forward_delay[2];
};


struct ctp_tcn_bpdu{
	uint16_t pid;
	uint8_t vers;
	uint8_t type;
} ;

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
  //PROTOCOL_VERSION_T    ForceVersion;   /* 17.12, 17.16.1 */
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


int chassis_bpdu_compare_times (TIMEVALUES_T *t1, TIMEVALUES_T *t2);
void chassis_bpdu_get_times (BPDU_BODY_T *b, TIMEVALUES_T *v);
void chassis_bpdu_set_times (TIMEVALUES_T *v, BPDU_BODY_T *b);
void chassis_bpdu_copy_times (TIMEVALUES_T *t, TIMEVALUES_T *f);
int chassis_bpdu_initial(CTPM_T* ctpm, int us_number, int state);
int chassis_bpdu_in_rx_bpdu(int vlan_id, int port_index, RSTP_BPDU_T* bpdu, unsigned int  len);
void chassis_bpdu_rx_bpdu (int if_index, const unsigned char *data, int len);
int chassis_bpdu_one_second (void);
/*static unsigned char _check_topoch (CTPM_T* this);*/
void chassis_bpdu_ctpm_one_second (CTPM_T* param);
void chassis_bpdu_ctpm_stop (CTPM_T* this);
int chassis_bpdu_ctpm_update (CTPM_T* this);
void chassis_bpdu_tx_info_bpdu(unsigned char bpdu_type);
int chassis_bpdu_in_update_lc_root(RSTP_BPDU_T* bpdu);
int chassis_bpdu_in_dump_ctpm(void);
int chassis_bpdu_in_update_fc_designated(RSTP_BPDU_T* bpdu);

void chassis_bpdu_enter_disable(CTPM_T* this);
void chassis_bpdu_enter_block(CTPM_T* this);
void chassis_bpdu_enter_listening(CTPM_T* this);
void chassis_bpdu_enter_learning(CTPM_T* this);
void chassis_bpdu_enter_forwarding(CTPM_T* this);

int chassis_bpdu_config_1_port(int port, int status);
int chassis_bpdu_config_all_port(int status);
int chassis_bpdu_enter_block_state(void);
int chassis_bpdu_enter_learning_state(void);
int chassis_bpdu_enter_forwarding_state(void);

#endif

