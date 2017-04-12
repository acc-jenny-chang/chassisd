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

#include "epoll_loop.h"
#include "chassis_bpdu.h"
#include "bpdu.h"

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("udp_client:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

int state = STATE_BLOCK;

unsigned short Default_MessageAge=DEFAULT_MESSAGEAGE;
unsigned short Default_MaxAge=DEFAULT_MAXAGE;
unsigned short Default_ForwardDelay=DEFAULT_FORWARDDELAY;
unsigned short Default_HelloTime=DEFAULT_HELLO_TIME;

extern CHASSIS_DATA_T chassis_information;
extern int chassis_config_port(long state, long pid);
extern int chassis_initial_master_fc(CTPM_T* this);
    
static int chassis_bpdu_ctp_ctpm_iterate_machines (CTPM_T* this);
int chassis_bpdu_in_rx_fc_bpdu(int vlan_id, int port_index, RSTP_BPDU_T* bpdu, unsigned int len);
int chassis_bpdu_in_rx_lc_bpdu(int vlan_id, int port_index, RSTP_BPDU_T* bpdu, unsigned int len);
int chassis_bpdu_in_clear_chassis_member(CTPM_T* ctpm);
int chassis_bpdu_in_update_chassis_member(RSTP_BPDU_T* bpdu, CTPM_T* ctpm);
int chassis_bpdu_in_dec_chassis_member(CTPM_T* ctpm);
    
void chassis_bpdu_clear_root_fc(CTPM_T*  this);
void chassis_bpdu_clear_designed_lc(CTPM_T*  this);
void chassis_bpdu_info_bpdu_flag(unsigned char* flags,unsigned char bpdu_type);

int
chassis_bpdu_compare_times (TIMEVALUES_T *t1,  TIMEVALUES_T *t2)
{
  if (t1->MessageAge < t2->MessageAge)     return -1;
  if (t1->MessageAge > t2->MessageAge)     return  1;

  if (t1->MaxAge < t2->MaxAge)             return -2;
  if (t1->MaxAge > t2->MaxAge)             return  2;

  if (t1->ForwardDelay < t2->ForwardDelay) return -3;
  if (t1->ForwardDelay > t2->ForwardDelay) return  3;

  if (t1->HelloTime < t2->HelloTime)       return -4;
  if (t1->HelloTime > t2->HelloTime)       return  4;

  return 0;
}

void
chassis_bpdu_get_times (BPDU_BODY_T *b, TIMEVALUES_T *v)
{
  unsigned short mt;
  #define chassis_bpdu_set_time2(f, t)        \
     mt = ntohs (f) >> 8;           \
     t=mt;

  chassis_bpdu_set_time2(*b->message_age,v->MessageAge);
  chassis_bpdu_set_time2(*b->max_age,v->MaxAge);
  chassis_bpdu_set_time2(*b->forward_delay,v->ForwardDelay);
  chassis_bpdu_set_time2(*b->hello_time,v->HelloTime);
}

void
chassis_bpdu_set_times (TIMEVALUES_T *v, BPDU_BODY_T *b)
{
  unsigned short mt;
  #define chassis_bpdu_set_time(f, t)        \
     mt = htons (f << 8);           \
     memcpy (t, &mt, 2); 
  
  chassis_bpdu_set_time(v->MessageAge,   b->message_age);
  chassis_bpdu_set_time(v->MaxAge,       b->max_age);
  chassis_bpdu_set_time(v->ForwardDelay, b->forward_delay);
  chassis_bpdu_set_time(v->HelloTime,    b->hello_time);
}

void 
chassis_bpdu_copy_times (TIMEVALUES_T *t, TIMEVALUES_T *f)
{
  t->MessageAge = f->MessageAge;
  t->MaxAge = f->MaxAge;
  t->ForwardDelay = f->ForwardDelay;
  t->HelloTime = f->HelloTime;
}

int chassis_bpdu_initial(CTPM_T * ctpm, int us_number, int state)
{
    unsigned char src_addr[6];
    unsigned short mt;

    memset(ctpm,0,sizeof(CTPM_T));
    memset(src_addr,0,6);
    
    ctpm->port = if_nametoindex(DEFAULT_MGMT);
    mt = htons((4096+us_number));
    memcpy(&ctpm->BrId.prio,&mt,2);
    if(netif_utils_get_hwaddr(DEFAULT_MGMT, src_addr)!=0) printf("Get HW FAIL!\n\r");
    memcpy(ctpm->BrId.addr, src_addr,6);
    ctpm->BrTimes.ForwardDelay = Default_ForwardDelay;
    ctpm->BrTimes.HelloTime =Default_HelloTime;
    ctpm->BrTimes.MaxAge = Default_MaxAge;
    ctpm->BrTimes.MessageAge = Default_MessageAge;
    if(us_number<33){
        ctpm->rootLCPortId = us_number;
        ctpm->rootPortId = 0;
        ctpm->rootPrio.root_path_cost = 2000;
        memcpy(&ctpm->rootPrio.design_bridge, &ctpm->BrId,sizeof(BRIDGE_ID));
        ctpm->rootPrio.design_port = us_number;
        ctpm->rootPrio.bridge_port =ctpm->rootPortId;        
    }
    else{
        ctpm->rootLCPortId = 0;
        ctpm->rootPortId = us_number;
        memcpy(&ctpm->rootPrio.root_bridge, &ctpm->BrId,sizeof(BRIDGE_ID));
        ctpm->rootPrio.root_path_cost = 2000;
        ctpm->rootPrio.design_port = 1;
        ctpm->rootPrio.bridge_port =ctpm->rootPortId;
    }
    chassis_bpdu_copy_times(&ctpm->rootTimes,&ctpm->BrTimes);
    ctpm->vlan_id = 0;
    ctpm->name = NULL;
    /*only CPU A need enable CTP*/
    if(us_number%2==1){
        ctpm->admin_state = state;
        if(state==0)
            ctpm->state = STATE_DISABLE;
    }
    else{
        ctpm->admin_state = CTP_DISABLED;
        ctpm->state = STATE_DISABLE;
    }
    ctpm->timeSince_Topo_Change = 0;
    ctpm->Topo_Change_Count = 0 ;
    ctpm->Topo_Change = 0;

    ctpm->chassis_exist[us_number-1]=5*DEFAULT_FORWARDDELAY+1;
    chassis_bpdu_in_dump_ctpm();
    return 0;
}

int chassis_bpdu_in_dump_ctpm(void)
{
#ifdef PACKET_DEBUG 
    CTPM_T* ctpm = &chassis_information.ctpm;

    printf("CPTM Information:\n\r");
    printf("port :%d\n\r",ctpm->port);
    printf("state :%d\n\r",ctpm->state);
    printf("BRIDGE_ID :%d %d %02x:%02x:%02x:%02x:%02x:%02x\n\r",
        htons(ctpm->BrId.prio)&0xFF00,
        htons(ctpm->BrId.prio)&0x00FF,
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
    printf("              root_bridge :%d %d %02x:%02x:%02x:%02x:%02x:%02x\n\r",
        htons(ctpm->rootPrio.root_bridge.prio)&0xFF00,
        htons(ctpm->rootPrio.root_bridge.prio)&0x00FF,
        ctpm->rootPrio.root_bridge.addr[0],
        ctpm->rootPrio.root_bridge.addr[1],
        ctpm->rootPrio.root_bridge.addr[2],
        ctpm->rootPrio.root_bridge.addr[3],
        ctpm->rootPrio.root_bridge.addr[4],
        ctpm->rootPrio.root_bridge.addr[5]);
    printf("              root_path_cost :%lu\n\r",ctpm->rootPrio.root_path_cost);
    printf("              design_bridge :%d %d %02x:%02x:%02x:%02x:%02x:%02x\n\r",
        htons(ctpm->rootPrio.design_bridge.prio)&0xFF00,
        htons(ctpm->rootPrio.design_bridge.prio)&0x00FF,
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
    printf("admin_state :%d\n\r",ctpm->admin_state);
    printf("timeSince_Topo_Change :%lu\n\r",ctpm->timeSince_Topo_Change);
    printf("Topo_Change_Count :%lu\n\r",ctpm->Topo_Change_Count);
    printf("Topo_Change %d:\n\r",ctpm->Topo_Change);
    printf("END\n\r");
#endif
    return 0;
}

int chassis_bpdu_in_clear_chassis_member(CTPM_T* ctpm)
{
    int us_number = chassis_information.us_card_number;
    
    memset(ctpm->chassis_exist,0,MAXIMUM_MICRO_SERVER_CARD_NUMBER);    
    ctpm->chassis_exist[us_number-1]=5*DEFAULT_FORWARDDELAY+1;

    return 0;
}

int chassis_bpdu_in_update_chassis_member(RSTP_BPDU_T* bpdu, CTPM_T* ctpm)
{
    int i;
    int us_number = chassis_information.us_card_number;
    
    for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
    {
        if(i==(us_number-1)) continue;
        else if(bpdu->chassis_exist[i]>=1&&ctpm->chassis_exist[i]<bpdu->chassis_exist[i]) 
            ctpm->chassis_exist[i]=bpdu->chassis_exist[i];
    }

    
    return 0;
}

int chassis_bpdu_in_dec_chassis_member(CTPM_T* ctpm)
{
    int i;
    int us_number = chassis_information.us_card_number;
    
    for(i=0;i<MAXIMUM_MICRO_SERVER_CARD_NUMBER;i++)
    {
        if(i==(us_number-1)) ctpm->chassis_exist[i]=5*DEFAULT_FORWARDDELAY+1;
        else if(ctpm->chassis_exist[i]>=1)
            ctpm->chassis_exist[i]=ctpm->chassis_exist[i]-1;
    }

    
    return 0;
}

int chassis_bpdu_in_update_lc_root(RSTP_BPDU_T* bpdu)
{
    RSTP_BPDU_T* bpdu_t = bpdu;
    unsigned int root_id=bpdu_t->body.root_id[1];
    unsigned int dbridge_id=bpdu_t->body.bridge_id[1];
    CTPM_T* ctpm = &chassis_information.ctpm;
    unsigned long root_path_cost;

    if(root_id>0)
    {
        if((root_id<ctpm->rootPortId)||(ctpm->rootPortId==0))
        {
            ctpm->rootPortId = root_id;
            ctpm->rootPrio.bridge_port = root_id;
            memcpy (&root_path_cost,&bpdu_t->body.root_path_cost,  4);
            ctpm->rootPrio.root_path_cost = ntohl(root_path_cost);
            memcpy(&ctpm->rootPrio.root_bridge,&bpdu->body.root_id,8);         
        }
    }
    if(dbridge_id>0)
    {
        if((bpdu->hdr.bpdu_type==BPDU_TYPE_HELLO)||(dbridge_id<ctpm->rootLCPortId)||(ctpm->rootLCPortId==0))
        {
            ctpm->rootLCPortId = dbridge_id;
            ctpm->rootPrio.design_port= dbridge_id;
            memcpy(&ctpm->rootPrio.design_bridge,&bpdu->body.bridge_id,8);
        }
    }

    chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
    
    return 0;
}

int chassis_bpdu_in_update_fc_designated(RSTP_BPDU_T* bpdu)
{
    RSTP_BPDU_T* bpdu_t = bpdu;
    unsigned int root_id=bpdu_t->body.root_id[1];
    unsigned int dbridge_id=bpdu_t->body.bridge_id[1];
    CTPM_T* ctpm = &chassis_information.ctpm;
    unsigned long root_path_cost;
    unsigned char forwarding = ((bpdu_t->body.flags&0x20))>>5;

    if(root_id>0)
    {
        if(forwarding==1||(root_id<ctpm->rootPortId)||(ctpm->rootPortId==0))
        {
            ctpm->rootPortId = root_id;
            ctpm->rootPrio.bridge_port = root_id;
            memcpy (&root_path_cost,&bpdu_t->body.root_path_cost,  4);
            ctpm->rootPrio.root_path_cost = ntohl(root_path_cost);
            memcpy(&ctpm->rootPrio.root_bridge,&bpdu->body.root_id,8);       
        }
    }
    if(dbridge_id>0)
    {
        if((dbridge_id<ctpm->rootLCPortId)||(ctpm->rootLCPortId==0))
        {
            ctpm->rootLCPortId = dbridge_id;
            ctpm->rootPrio.design_port= dbridge_id;
            memcpy(&ctpm->rootPrio.design_bridge,&bpdu->body.bridge_id,8);
        }
    }
    
    chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
    
    return 0;
}

static int chassis_bpdu_check_bpdu(RSTP_BPDU_T* bpdu)
{
    RSTP_BPDU_T* bpdu_t = bpdu;
    CTPM_T* ctpm = &chassis_information.ctpm;
    unsigned short root_id=bpdu_t->body.root_id[1];
    unsigned short dbridge_id=bpdu_t->body.bridge_id[1];
    unsigned short ori_root_id=htons(ctpm->rootPrio.root_bridge.prio)&0x00FF;
    unsigned short ori_dbridge_id=htons(ctpm->rootPrio.design_bridge.prio)&0x00FF;
    
    /*Check FC Root*/
    if(root_id!=ori_root_id){
        return 1;
    }
    /*Check LC Designated*/
    if(dbridge_id!=ori_dbridge_id){
        return 1;
    }
    return 0;
}

int chassis_bpdu_in_rx_fc_bpdu(int vlan_id, int port_index, RSTP_BPDU_T* bpdu, unsigned int len)
{
    RSTP_BPDU_T* bpdu_t = bpdu;
    CTPM_T* ctpm = &chassis_information.ctpm;
    unsigned short dbridge_id;
    unsigned short ori_dbridge_id;
    unsigned short root_id;
    unsigned short ori_root_id;
    unsigned int slotid=bpdu_t->body.port_id[1];
    
    if(chassis_information.type==LINE_CARD_TYPE)
    {
        if(ctpm->state==STATE_BLOCK)
        {
            chassis_bpdu_enter_listening(ctpm);
            ctpm->Topo_Change =1;
            ctpm->timeSince_Topo_Change =0;
            ctpm->Topo_Change_Count = 1;
            chassis_bpdu_in_update_lc_root(bpdu_t);
            chassis_bpdu_get_times(&bpdu_t->body,&ctpm->rootTimes);
            chassis_bpdu_in_dump_ctpm();
            chassis_bpdu_tx_info_bpdu(BPDU_TYPE_REPLY);
            chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
        }
        else if(ctpm->state==STATE_LEARNING||ctpm->state==STATE_LISTENING)
        {
            if(chassis_bpdu_check_bpdu(bpdu)!=0) {
                chassis_bpdu_in_update_lc_root(bpdu_t);
                chassis_bpdu_in_dump_ctpm();
            }
            chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
        }
        else if(ctpm->state==STATE_FORWARDING)
        {
            if(bpdu->hdr.bpdu_type==BPDU_TYPE_LC_CHANGE)
            {
                ctpm->timeSince_Topo_Change =0;
                ori_dbridge_id=htons(ctpm->BrId.prio)&0x00FF;
                if((ctpm->rootLCPortId!=ori_dbridge_id)&&(ctpm->chassis_exist[ori_dbridge_id]>=1)) ctpm->chassis_exist[ctpm->rootLCPortId]=0;
                chassis_bpdu_enter_block(ctpm);
                return 0;
            }
           chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
            if(bpdu->hdr.bpdu_type==BPDU_TYPE_HELLO)
            {
                ctpm->timeSince_Topo_Change =0;
                dbridge_id=bpdu_t->body.bridge_id[1];
                ori_dbridge_id=htons(ctpm->BrId.prio)&0x00FF;
                root_id=bpdu_t->body.root_id[1];
                ori_root_id=htons(ctpm->rootPrio.root_bridge.prio)&0x00FF;
                /* I am Designated LC, receive Hello packet from Root FC and forwarding HELLO packet to Other FCs*/
                if(dbridge_id==ori_dbridge_id&& root_id==ori_root_id&&slotid==root_id)
                    chassis_bpdu_tx_info_bpdu(BPDU_TYPE_HELLO);
                else
                    chassis_bpdu_tx_info_bpdu(BPDU_TYPE_REPLY);
            }
            if(bpdu->hdr.bpdu_type==BPDU_TYPE_CONFIG)
                chassis_bpdu_tx_info_bpdu(BPDU_TYPE_REPLY);
        }
    }
    else if(chassis_information.type==FABRIC_CARD_TYPE)
    {

    }
    return 0;
}

int chassis_bpdu_in_rx_lc_bpdu(int vlan_id, int port_index, RSTP_BPDU_T* bpdu, unsigned int len)
{
    RSTP_BPDU_T* bpdu_t = bpdu;
    CTPM_T* ctpm = &chassis_information.ctpm;
    unsigned short dbridge_id;
    unsigned short ori_dbridge_id;
    unsigned short root_id;
    unsigned short my_id;

    if(chassis_information.type==LINE_CARD_TYPE)
    {
        ;
    }
    else if(chassis_information.type==FABRIC_CARD_TYPE)
    {
        if(ctpm->state==STATE_BLOCK)
        {
            chassis_bpdu_enter_listening(ctpm);
            ctpm->Topo_Change =1;
            ctpm->timeSince_Topo_Change =0;
            ctpm->Topo_Change_Count = 1;
            chassis_bpdu_in_update_fc_designated(bpdu_t);
            chassis_bpdu_get_times(&bpdu_t->body,&ctpm->rootTimes);
            chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
            chassis_bpdu_in_dump_ctpm();
        }
        else if(ctpm->state==STATE_LEARNING||ctpm->state==STATE_LISTENING)
        {
            if(chassis_bpdu_check_bpdu(bpdu)!=0) {
                chassis_bpdu_in_update_fc_designated(bpdu_t);
                chassis_bpdu_in_dump_ctpm();
            }
            chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
        }
        else if(ctpm->state==STATE_FORWARDING)
        {
            chassis_bpdu_in_update_chassis_member(bpdu,ctpm);
            if(bpdu->hdr.bpdu_type==BPDU_TYPE_HELLO)
            {
                ctpm->timeSince_Topo_Change = 0;
                dbridge_id=bpdu_t->body.bridge_id[1];
                ori_dbridge_id=htons(ctpm->rootPrio.design_bridge.prio)&0x00FF;
                root_id=bpdu_t->body.root_id[1];
                my_id=htons(ctpm->BrId.prio)&0x00FF;
                /* I am backup FC, receive Hello packet from Designated LC and Reply HELLO packet to LC, let it know i am exist*/
                if(dbridge_id==ori_dbridge_id&& root_id!=my_id){
#ifdef PACKET_DEBUG                    
                    printf("I am designed fc, reply hello packet :%d,%d,%d,%d",dbridge_id,ori_dbridge_id,root_id,my_id);
#endif
                    chassis_bpdu_tx_info_bpdu(BPDU_TYPE_HELLO);
                    }
            }
            if(bpdu->hdr.bpdu_type==BPDU_TYPE_CONFIG)
            {
                chassis_bpdu_tx_info_bpdu(BPDU_TYPE_REPLY);
            }
        }        
    }
    return 0;

   
   return 0;
}

int chassis_bpdu_in_rx_bpdu(int vlan_id, int port_index, RSTP_BPDU_T* bpdu, unsigned int len)
{

    int ret =0;
    RSTP_BPDU_T* bpdu_t = bpdu;
    unsigned int slotid=bpdu_t->body.port_id[1];

    /*if CPU is B, don't support */
    if(chassis_information.us_card_number%2==0) return 0;

    if(33<=slotid&&slotid<=48)
    {
        return chassis_bpdu_in_rx_fc_bpdu(vlan_id,port_index,bpdu,len);
    }
    else if(1<=slotid&&slotid<=32)
    {
        return chassis_bpdu_in_rx_lc_bpdu(vlan_id,port_index,bpdu,len);
    }
    else 
        return -1;
    
    return ret;
}

void
chassis_bpdu_rx_bpdu (int if_index, const unsigned char *data, int len)
{
    int port_index = if_index;
    RSTP_BPDU_T*  bpdu = (RSTP_BPDU_T *) (data);

	switch (bpdu->hdr.bpdu_type) {
	case BPDU_TYPE_RSTP:
		TST(len >= 36,);
               break;
	case BPDU_TYPE_CONFIG:
		TST(len >= 35,);
		/* 802.1w doesn't ask for this */
		/*    TST(ntohs(*(uint16_t*)bpdu.body.message_age)
		//        < ntohs(*(uint16_t*)bpdu.body.max_age), );
		//TST(memcmp(bpdu->body.bridge_id, &ifc->master->bridge_id, 8) != 0
		//    || (ntohs(*(uint16_t *) bpdu->body.port_id) & 0xfff) !=
		//    ifc->port_index,);*/
		break;
	case BPDU_TYPE_HELLO:
              TST(len >= 35,);
              break;
	case BPDU_TYPE_REPLY:
		break;
       case BPDU_TYPE_LC_CHANGE:
		break;
       case BPDU_TYPE_FC_CHANGE:
		break;
	case BPDU_TOPO_CHANGE_TYPE:
		break;
	default:
		LOG("Receive unknown bpdu type %x", bpdu->hdr.bpdu_type);
		return;
	}
    chassis_bpdu_in_rx_bpdu(0, port_index, bpdu, len);
    
}

void chassis_bpdu_info_bpdu_flag(unsigned char* flags,unsigned char bpdu_type)
{
    CTPM_T* ctpm = &chassis_information.ctpm;
    unsigned int root_id=htons(ctpm->rootPrio.root_bridge.prio)&0x00FF;
    unsigned int bridge_id=htons(ctpm->rootPrio.design_bridge.prio)&0x00FF;
    unsigned int my_id=htons(ctpm->BrId.prio)&0x00FF;
    
    *flags = 0x0;

    if(ctpm->state==STATE_FORWARDING){
        *flags |= 1<<5;
        *flags |= 1<<4;
    }
    else if(ctpm->state==STATE_LEARNING||ctpm->state==STATE_LEARNING)
    {
        *flags |= 1<<4;
    }
    switch (bpdu_type)
    {
        case BPDU_TYPE_FC_CHANGE:
        case BPDU_TYPE_LC_CHANGE:
             *flags |= 1<<0;
             break;
        case BPDU_TYPE_CONFIG:
             *flags |= 1<<1;
             break;
        case BPDU_TYPE_REPLY:
             *flags |= 1<<6;
             break;  
        case BPDU_TYPE_HELLO:
            //*flags |= 1<<1;
            break;
        case BPDU_TYPE_TCN:
            *flags |= 1<<7;
            break;            
            default:
                break;
    }
    
    if(my_id==root_id)
    {
         *flags |= 0x3<<2;
    }
    else if(my_id==bridge_id)
    {
         *flags |= 0x2<<2;
    }
    else{
        *flags |= 0x1<<2;
    }
    
    return;
}


void
chassis_bpdu_tx_info_bpdu(unsigned char bpdu_type)
{
    CTPM_T* ctpm = &chassis_information.ctpm;
    int us_number = chassis_information.us_card_number;
    char mac[6];
    /*BPDU mutlicast address 1*/
    char mac1[6]={0x1,0x10,0xb5,0x0,0x0,0x2};
    /*BPDU mutlicast address 2*/
    char mac2[6]={0x1,0x10,0xb5,0x0,0x0,0x1};
    char sendline[1000];
    int size=sizeof(RSTP_BPDU_T);
    RSTP_BPDU_T bpdu_t;
    unsigned short len8023;
    unsigned long root_path_cost;
  
    memset(&bpdu_t,0,size);
    
    memcpy(bpdu_t.mac.src_mac, ctpm->BrId.addr, 6);
    if((1<=us_number&&us_number<=16)||(41<=us_number&&us_number<=48))
        memcpy(mac, mac1, 6);
    else
        memcpy(mac, mac2, 6);
    memcpy(bpdu_t.mac.dst_mac, mac, 6);
    
    bpdu_t.eth.dsap=0x42;
    bpdu_t.eth.ssap=0x42;
    bpdu_t.eth.llc=0x3;
    
    bpdu_t.hdr.protocol[0]=0x0;
    bpdu_t.hdr.version = 0;
    bpdu_t.hdr.bpdu_type = bpdu_type;

    chassis_bpdu_info_bpdu_flag(&bpdu_t.body.flags,bpdu_type);
    memcpy(bpdu_t.body.root_id, &ctpm->rootPrio.root_bridge, 8);
    root_path_cost = htonl (ctpm->rootPrio.root_path_cost);
    memcpy (bpdu_t.body.root_path_cost, &root_path_cost, 4);
    memcpy(bpdu_t.body.bridge_id, &ctpm->rootPrio.design_bridge, 8);
    bpdu_t.body.port_id[1] = us_number;
    
    chassis_bpdu_set_times(&ctpm->rootTimes,&bpdu_t.body);

    len8023 = htons ((unsigned short) (sizeof (BPDU_HEADER_T) + sizeof (BPDU_BODY_T) + 3+MAXIMUM_MICRO_SERVER_CARD_NUMBER));
    memcpy (bpdu_t.eth.len8023, &len8023, 2);

    chassis_bpdu_in_dec_chassis_member(ctpm);

    memcpy(bpdu_t.chassis_exist,ctpm->chassis_exist,MAXIMUM_MICRO_SERVER_CARD_NUMBER);

    
    memcpy(sendline,&bpdu_t,size);
    bpdu_client_send(mac,sendline,size);
#ifdef PACKET_DEBUG
    printf("bpdu_client_send, bpdu type = %d, state=%d\n\r",bpdu_type,ctpm->state);
#endif
    return;
}

int
chassis_bpdu_one_second (void)
{
    chassis_bpdu_ctpm_one_second(&chassis_information.ctpm);
    return 0;
}

/*
static unsigned char
_check_topoch (CTPM_T* this)
{
  return 0;
}
*/
void
chassis_bpdu_ctpm_one_second (CTPM_T* param)
{

    CTPM_T* this = (CTPM_T*) param;

    if (CTP_ENABLED != this->admin_state) return;

  chassis_bpdu_ctpm_update (this);
  /*
  this->Topo_Change = _check_topoch (this);
  if (this->Topo_Change) {
    this->Topo_Change_Count++;
    this->timeSince_Topo_Change = 0;
  } else {
    this->Topo_Change_Count = 0;
    this->timeSince_Topo_Change++;
  }  */
}

void chassis_bpdu_ctpm_stop (CTPM_T* this)
{
    this->admin_state = CTP_DISABLED;    
}

int
chassis_bpdu_ctpm_update (CTPM_T* this) /* returns number of loops */
{
    int     need_state_change=0;
    int      number_of_loops = 0;

    need_state_change = chassis_bpdu_ctp_ctpm_iterate_machines(this);

    if (! need_state_change) return number_of_loops;
    
    number_of_loops++;

    return number_of_loops;
}
int chassis_bpdu_config_1_port(int port, int status)
{
    int us_number = chassis_information.us_card_number;
    
    if(us_number>=1 && us_number <=32)
    {
        switch (port)
        {
            case 33:
            case 34:
            case 41:
            case 42:
                chassis_config_port(status, PORT_8_FC0);
                break;
            case 35:
            case 36:
            case 43:
            case 44:
                chassis_config_port(status, PORT_8_FC1);
                break;
            case 37:
            case 38:
            case 45:
            case 46:                
                chassis_config_port(status, PORT_8_FC2);
                break;
            case 39:
            case 40:
            case 47:
            case 48:
                chassis_config_port(status, PORT_8_FC3); 
                break;
            default:
                printf("Config unexpected port number\n\r");
        }
    }
    else if((us_number>=33 && us_number <=36)||(us_number>=41 && us_number <=44))
    {
        switch(port)
        {
            case 1:
            case 2:
            case 17:
            case 18:        
                chassis_config_port(status, PORT_16_LC0);
                break;
            case 3:
            case 4:
            case 19:
            case 20: 
                chassis_config_port(status, PORT_16_LC1);
                break;
            case 5:
            case 6:
            case 21:
            case 22: 
                chassis_config_port(status, PORT_16_LC2);
                break;
            case 7:
            case 8:
            case 23:
            case 24: 
                chassis_config_port(status, PORT_16_LC3);
                break;
            case 9:
            case 10:
            case 25:
            case 26:
                chassis_config_port(status, PORT_16_LC4);
                break;
            case 11:
            case 12:
            case 27:
            case 28: 
                chassis_config_port(status, PORT_16_LC5);
                break;
            case 13:
            case 14:
            case 29:
            case 30: 
                chassis_config_port(status, PORT_16_LC6);
                break;
            case 15:
            case 16:
            case 31:
            case 32: 
                chassis_config_port(status, PORT_16_LC7);
                break;
            default:
                printf("Config unexpected port number\n\r");        
        }
    }
    else
    {
        switch(port)
        {
            case 1:
            case 2:
            case 17:
            case 18:        
                chassis_config_port(status, PORT_16_LC4);
                break;
            case 3:
            case 4:
            case 19:
            case 20: 
                chassis_config_port(status, PORT_16_LC5);
                break;
            case 5:
            case 6:
            case 21:
            case 22: 
                chassis_config_port(status, PORT_16_LC6);
                break;
            case 7:
            case 8:
            case 23:
            case 24: 
                chassis_config_port(status, PORT_16_LC7);
                break;
            case 9:
            case 10:
            case 25:
            case 26:
                chassis_config_port(status, PORT_16_LC0);
                break;
            case 11:
            case 12:
            case 27:
            case 28: 
                chassis_config_port(status, PORT_16_LC1);
                break;
            case 13:
            case 14:
            case 29:
            case 30: 
                chassis_config_port(status, PORT_16_LC2);
                break;
            case 15:
            case 16:
            case 31:
            case 32: 
                chassis_config_port(status, PORT_16_LC3);
                break;
            default:
                printf("Config unexpected port number\n\r");        
        }
    }
    return 0;
}
int chassis_bpdu_config_all_port(int status)
{
    int us_number = chassis_information.us_card_number;
    
    if(us_number>=1 && us_number <=32)
    {
        chassis_config_port(status, PORT_8_FC0);
        chassis_config_port(status, PORT_8_FC1);
        chassis_config_port(status, PORT_8_FC2);
        chassis_config_port(status, PORT_8_FC3); 
    }
    else
    {
        chassis_config_port(status, PORT_16_LC0);
        chassis_config_port(status, PORT_16_LC1);
        chassis_config_port(status, PORT_16_LC2);
        chassis_config_port(status, PORT_16_LC3);
        chassis_config_port(status, PORT_16_LC4);
        chassis_config_port(status, PORT_16_LC5);
        chassis_config_port(status, PORT_16_LC6);
        chassis_config_port(status, PORT_16_LC7);
        chassis_config_port(status, PORT_16_FC);
    }
    return 0;
}

int chassis_bpdu_enter_block_state(void)
{
    return chassis_bpdu_config_all_port(PORT_BLOCKING);
}

int chassis_bpdu_enter_learning_state(void)
{
    return chassis_bpdu_config_all_port(PORT_LISTENING);
}

int chassis_bpdu_enter_forwarding_state(void)
{
    int us_number = chassis_information.us_card_number;
    CTPM_T* ctpm = &chassis_information.ctpm;
    PORT_ID designed_lc_id = ctpm->rootLCPortId;
    PORT_ID root_fc_id = ctpm->rootPortId;

    if(us_number>=1 && us_number <=32)
    {
        if(designed_lc_id==us_number)
        {
            return chassis_bpdu_config_all_port(PORT_FORWARDING);
        }
        else{
            chassis_bpdu_config_all_port(PORT_BLOCKING);
            switch (root_fc_id)
            {
                case 33:
                case 41:
                    chassis_config_port(PORT_FORWARDING, PORT_8_FC0);
                    break;
                case 35:
                case 43:
                    chassis_config_port(PORT_FORWARDING, PORT_8_FC1);
                    break;
                case 37:
                case 45:
                    chassis_config_port(PORT_FORWARDING, PORT_8_FC2);
                    break;
                case 39:
                case 47:
                    chassis_config_port(PORT_FORWARDING, PORT_8_FC3);
                    break;
                default:
                    printf("unexpected root fc id!\n\r");
            }
        }
    }
    else if((us_number>=33 && us_number <=36)||(us_number>=41 && us_number <=44))
    {
         if(root_fc_id==us_number)
        {
            return chassis_bpdu_config_all_port(PORT_FORWARDING);
        }
        else
        {
            chassis_bpdu_config_all_port(PORT_BLOCKING);
            switch (designed_lc_id)
            {
                case 1:
                case 17:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC0);
                    break;            
                case 3:
                case 19:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC1);
                    break;
                case 5:
                case 21:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC2);
                    break;
                case 7:
                case 23:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC3);
                    break;
                case 9:
                case 25:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC4);
                    break;
                case 11:
                case 27:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC5);
                    break;
                case 13:
                case 29:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC6);
                    break;
                case 15:
                case 31:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC7);
                    break;
                default:
                    printf("unexpected designed lc id!\n\r");
            }
        }  
    }
    else if((us_number>=37 && us_number <=40)||(us_number>=45 && us_number <=48))
    {
         if(root_fc_id==us_number)
        {
            return chassis_bpdu_config_all_port(PORT_FORWARDING);
        }
        else
        {
            chassis_bpdu_config_all_port(PORT_BLOCKING);
            switch (designed_lc_id)
            {
                case 1:
                case 17:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC4);
                    break;            
                case 3:
                case 19:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC5);
                    break;
                case 5:
                case 21:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC6);
                    break;
                case 7:
                case 23:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC7);
                    break;
                case 9:
                case 25:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC0);
                    break;
                case 11:
                case 27:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC1);
                    break;
                case 13:
                case 29:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC2);
                    break;
                case 15:
                case 31:
                    chassis_config_port(PORT_FORWARDING, PORT_16_LC3);
                    break;
                default:
                    printf("unexpected designed lc id!\n\r");
            }
        }  
    }
    
    return 0;
}


void chassis_bpdu_clear_root_fc(CTPM_T*  this)
{
    int us_number = chassis_information.us_card_number;
    
    this->rootLCPortId = us_number;
    this->rootPortId = 0;
    memset(&this->rootPrio,0,sizeof(PRIO_VECTOR_T));
    this->rootPrio.root_path_cost = 2000;
    memcpy(&this->rootPrio.design_bridge, &this->BrId,sizeof(BRIDGE_ID));
    this->rootPrio.design_port = us_number;
    this->rootPrio.bridge_port =this->rootPortId;
}
void chassis_bpdu_clear_designed_lc(CTPM_T*  this)
{
    int us_number = chassis_information.us_card_number;

        this->rootLCPortId = 0;
        this->rootPortId = us_number;
        memset(&this->rootPrio,0,sizeof(PRIO_VECTOR_T));
        memcpy(&this->rootPrio.root_bridge, &this->BrId,sizeof(BRIDGE_ID));
        this->rootPrio.root_path_cost = 2000;
        this->rootPrio.design_port = 1;
        this->rootPrio.bridge_port =this->rootPortId;
}


void chassis_bpdu_enter_disable(CTPM_T* this)
{
    /*Change Port to disable State*/
    this->state = STATE_DISABLE;
    chassis_bpdu_config_all_port(STATE_DISABLE);
    printf("This Card is enter disable state.\n\r");
    return;
}

void chassis_bpdu_enter_block(CTPM_T* this)
{
    int us_number = chassis_information.us_card_number;

    /*Change Port to Block State*/
    this->state = STATE_BLOCK;
    if((1<=us_number)&&(us_number<=32))
    {
        chassis_bpdu_clear_root_fc(this);
        chassis_bpdu_enter_block_state();
    }
    else
    {
        chassis_bpdu_clear_designed_lc(this);
        chassis_bpdu_config_all_port(PORT_FORWARDING);
        chassis_initial_master_fc(this);
    }
    this->timeSince_Topo_Change = 0;
    chassis_bpdu_in_clear_chassis_member(this);
    printf("This Card is enter block state.\n\r");
    return;
}

void chassis_bpdu_enter_listening(CTPM_T* this)
{
    int us_number = chassis_information.us_card_number;

    /*Change Port to Learning State*/
    this->state = STATE_LISTENING;
    if((1<=us_number)&&(us_number<=32))
        chassis_bpdu_enter_learning_state();

    this->timeSince_Topo_Change = 0;
    printf("This Card is enter listening state.\n\r");    
    return;
}

void chassis_bpdu_enter_learning(CTPM_T* this)
{
    int us_number = chassis_information.us_card_number;

    /*Change Port to Learning State*/
    this->state = STATE_LEARNING;
    if((1<=us_number)&&(us_number<=32))
         chassis_bpdu_config_1_port(this->rootPortId,PORT_FORWARDING);

    this->timeSince_Topo_Change=0;
    printf("This Card is enter learning state.\n\r");    
    return;
}

void chassis_bpdu_enter_forwarding(CTPM_T* this)
{
    int us_number = chassis_information.us_card_number;

    /*Change Port to Forwarding State*/
    this->state = STATE_FORWARDING;
    chassis_bpdu_enter_forwarding_state();
    this->timeSince_Topo_Change =0 ;
    if((1<=us_number)&&(us_number<=32)){
        if(us_number==this->rootLCPortId){
            printf("This card role is : DESIGNED LC.\n\r");
        }
        else
            printf("This card role is : LEAF LC.\n\r");
    }
    else{
        if(us_number==this->rootPortId)
        {
            printf("This card role is : ROOT FC.\n\r");
        }
        else
            printf("This card role is : BACKUP FC.\n\r");
        
        chassis_initial_master_fc(this);
    }
    
    printf("This Card is enter forwarding state.\n\r");
    return;
}

static int chassis_bpdu_ctp_ctpm_iterate_machines (CTPM_T* this)
{
    int us_number = chassis_information.us_card_number;

  switch (this->state)
  {
       case STATE_BLOCK:
            if(33<=us_number&&us_number<=48)
                chassis_bpdu_tx_info_bpdu(BPDU_TYPE_CONFIG);
            break;
       case STATE_LISTENING:
            if(this->timeSince_Topo_Change>Default_ForwardDelay){
                /*lost hello packet, need find new root fc/designed lc*/
                chassis_bpdu_enter_learning(this);
            }
            else{
                /* keep topology*/
                this->timeSince_Topo_Change++;
                if(33<=us_number&&us_number<=48)
                    chassis_bpdu_tx_info_bpdu(BPDU_TYPE_CONFIG);
            }
            break;
       case STATE_LEARNING:
            if(this->timeSince_Topo_Change<Default_ForwardDelay){
                chassis_bpdu_tx_info_bpdu(BPDU_TYPE_CONFIG);
                this->timeSince_Topo_Change++;
             }
            else
                chassis_bpdu_enter_forwarding(this);
            break;
       case STATE_FORWARDING:
            if(us_number==this->rootPortId)
            {
                if(this->timeSince_Topo_Change>Default_MaxAge)
                {
                    chassis_bpdu_enter_block(this);
                    chassis_bpdu_tx_info_bpdu(BPDU_TYPE_LC_CHANGE);
                }
                else
                {
                    chassis_bpdu_tx_info_bpdu(BPDU_TYPE_HELLO);
                    this->timeSince_Topo_Change++;
                }
            }
            else if(this->timeSince_Topo_Change>Default_MaxAge){
                /*lost hello packet, need find new root fc/designed lc*/
                chassis_bpdu_enter_block(this);
            }
            else
                /* keep topology*/
                this->timeSince_Topo_Change++;
            break;
       default:
        ;
  }
  return 0;
}


