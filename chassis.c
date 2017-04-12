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

#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include "chassis.h"
#include "eventloop.h"
#include "tcp_server.h"
#include "udp_server.h"
#include "udp_client.h"
#include "bpdu.h"
#include "memutil.h"
#include "log.h"


#include "epoll_loop.h"
#include "netif_utils.h"
#include "log.h"

#include <stdarg.h>
#include <time.h>

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("chassis:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

enum chassis_state { 
	S_NORMAL,
	S_BURNIN,
};

/*static int become_daemon = 1;*/
static int is_daemon = 0;
int log_level = LOG_LEVEL_DEFAULT;

int daemon_mode;
/*static int foreground;*/
static char pidfile_default[] = PID_FILE;
/*static char logfile_default[] = LOG_FILE;*/
static char *pidfile = pidfile_default;
/*static char *logfile;*/
static int state = S_NORMAL;
/*static int tcp_sock;*/
static int udp_sock;
static int bpdu_sock;

/*static int modifier(int opt);
static void modifier_finish(void);
static void setup_pidfile(char *s);*/

int chassis_initial_ip(void);
int chassis_initial_topology(void);
int chassis_initial_default_topology(void);
int chassis_initial_static_fc(void);
int chassis_initial_static_lc(void);
int chassis_config_port(long state, long  pid);
void chassis_state_to_string(unsigned long state, char** res);
int chassis_set_us_card_number_of_switch_board(void);
static int chassis_diag_function_delete_temporary_file(void);
int chassis_initial_spi(void);
int chassis_initial_phy(void);
int chassis_set_serdesmode(void);
int chassis_set_slavemode(void);
int chassis_config_lc_stp(int status);
int chassis_config_fc_stp(int status);
int chassis_add_static_arl(int us_number);
int chassis_config_multicast(int us_number);

int chassis_check_master_fc(CTPM_T* this);
int chassis_initial_master_fc(CTPM_T* this);
    
static void chassis_remove_pidfile(void)
{
	unlink(pidfile);
	if (pidfile != pidfile_default)
		free(pidfile);
}

static void chassis_signal_exit(int sig)
{
    /*close tcp and udp socket*/
    udp_client_close_socket();
    upd_server_close(udp_sock);
    //tcp_server_close(tcp_sock);
    bpdu_close_socket();
    chassis_remove_pidfile();
   
    _exit(EXIT_SUCCESS);
}
/*
static void setup_pidfile(char *s)
{
	char cwd[PATH_MAX];
	char *c;

	if (*s != '/') {
		c = getcwd(cwd, PATH_MAX);
		if (!c)
			return;
		asprintf(&pidfile, "%s/%s", cwd, s);
	} else {
		asprintf(&pidfile, "%s", s);
	}

	return;
}
*/
static void chassis_write_pidfile(void)
{
	FILE *f;
	atexit(chassis_remove_pidfile);
	signal(SIGTERM, chassis_signal_exit);
	signal(SIGINT, chassis_signal_exit);
	signal(SIGQUIT, chassis_signal_exit);
	f = fopen(pidfile, "w");
	if (!f) {
		DEBUG_PRINT("Cannot open pidfile `%s'", pidfile);
		return;
	}
	fprintf(f, "%u", getpid());
	fclose(f);
}

void chassis_usage(void)
{
	fprintf(stderr, 
"Usage:\n"
"  chassis [options]  [mcelogdevice]\n"
		);
	exit(1);
}

enum options { 
	O_LOGFILE = O_COMMON,
	O_SYSLOG,
     	O_NO_SYSLOG,
     	O_SYSLOG_ERROR,
     	O_DAEMON,
};

static struct option options[] = {
	{ "logfile", 1, NULL, O_LOGFILE },
     	{ "daemon", 0, NULL, O_DAEMON },
	{}
};

#if 0
static int modifier(int opt)
{
	switch (opt) { 
	case O_LOGFILE:
		logfile = optarg;
		break;
	case O_SYSLOG:
		openlog("chassis", 0, LOG_DAEMON);
		syslog_opt = SYSLOG_ALL|SYSLOG_FORCE;
		break;
	case O_NO_SYSLOG:
		syslog_opt = SYSLOG_FORCE;
		break;
	case O_SYSLOG_ERROR:
		syslog_level = LOG_ERR;
		syslog_opt = SYSLOG_ALL|SYSLOG_FORCE;
		break;
	case O_DAEMON:
		daemon_mode = 1;
		if (!(syslog_opt & SYSLOG_FORCE))
			syslog_opt = SYSLOG_REMARK|SYSLOG_ERROR;
		break;
	case 0:
		break;
	default:
		return 0;
	}
	return 1;
} 

static void modifier_finish(void)
{
	if(!foreground && daemon_mode && !logfile && !(syslog_opt & SYSLOG_LOG)) {
		logfile = logfile_default;
	}
	if (logfile) {
		if (open_logfile(logfile) < 0) {
			if (daemon_mode && !(syslog_opt & SYSLOG_FORCE))
				syslog_opt = SYSLOG_ALL;
			SYSERRprintf("Cannot open logfile %s", logfile);
			if (!daemon_mode)
				exit(1);
		}
	}			
}
#endif
static void chassis_handle_sigusr1(int sig)
{
    DEBUG_PRINT("Enter burn-in state\n");
    state = S_BURNIN;
}

static void chassis_handle_sigusr2(int sig)
{
    DEBUG_PRINT("Enter normal state\n");
    state = S_NORMAL;
}

static void chassis_handle_readyping(int sig)
{
    DEBUG_PRINT("ready to ping\n");
}

static void chassis_handle_leftping(int sig)
{
    DEBUG_PRINT("left from ping\n");
}

static void chassis_handle_readypowercycle(int sig)
{
    DEBUG_PRINT("ready to power cycle\n");
}

/* Remove leading/trailing white space */
static char *chassis_strstrip(char *s)
{
	char *p;
	while (isspace(*s))
		s++;
	p = s + strlen(s) - 1;
	if (p <= s)
		return s;
	while (isspace(*p) && p >= s)
		*p-- = 0;
	return s;
}

static int chassis_empty(char *s)
{
	while (isspace(*s))
		++s;
	return *s == 0;
}

static void noreturn chassis_parse_error(int line, char *msg)
{
	DEBUG_PRINT("config file line %d: %s\n", line, msg);
	exit(1);
}

static int chassis_parse_config_file(const char *fn)
{
	FILE *f;
	char *line = NULL;
	size_t linelen = 0;

	char *name;
	char *val;
	int lineno = 1;
       unsigned int us_number;
       int ipaddr1,ipaddr2,ipaddr3,ipaddr4;
       char type[20];
       char buf[10000] = {0};
       int ret = -1;
   
       memset(&chassis_information,0,sizeof(CHASSIS_DATA_T));

	ret = tcp_server_diag_function_search_string(CONFIG_US_CARD_NUMBER, " :,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 1, 1);
	if (ret != 0)
	{
		printf("diag_function_search_string(%d): Error!\r\n", ret);
		fflush(stdout);
		ret = -13;
	}
	chassis_information.us_card_number = (unsigned int) strtoul(buf, NULL, 10);

	f = fopen(fn, "r");
	if (!f)
		return -1;

	while (getline(&line, &linelen, f) > 0) {
		char *s = strchr(line, '#');
		if (s) 
			*s = 0;
        
		s = chassis_strstrip(line);
		if ((val = strchr(line, '=')) != NULL) { 
			*val++ = 0;
			name = chassis_strstrip(s);
			val = chassis_strstrip(val);
                    if(strcmp(name,"usNumber")==0){
                        ;/*chassis_information.us_card_number = atoi(val);*/
                    }else if(strcmp(name,"chassis")==0)
                    {
                        sscanf(val,"{%d,%d.%d.%d.%d,%s}",&us_number,&ipaddr1,&ipaddr2,&ipaddr3,&ipaddr4,type);
                        DEBUG_PRINT("us number:%d,%d.%d.%d.%d,%s\n",us_number,ipaddr1,ipaddr2,ipaddr3,ipaddr4,type);
                        chassis_information.us_card[us_number-1].us_card_number = us_number;
                        chassis_information.us_card[us_number-1].ipaddress[0]=ipaddr1;
                        chassis_information.us_card[us_number-1].ipaddress[1]=ipaddr2;
                        chassis_information.us_card[us_number-1].ipaddress[2]=ipaddr3;
                        chassis_information.us_card[us_number-1].ipaddress[3]=ipaddr4;
                        if(strstr(type,"LC")!=0) chassis_information.us_card[us_number-1].type = LINE_CARD_TYPE;
                        else chassis_information.us_card[us_number-1].type = FABRIC_CARD_TYPE;
                        chassis_information.us_card[us_number-1].status  = 1;                      
                        if(chassis_information.us_card_number==us_number)
                        {
                            chassis_information.ipaddress[0]=ipaddr1;
                            chassis_information.ipaddress[1]=ipaddr2;
                            chassis_information.ipaddress[2]=ipaddr3;
                            chassis_information.ipaddress[3]=ipaddr4;
                            if(strstr(type,"LC")!=0) chassis_information.type = LINE_CARD_TYPE;
                            else chassis_information.type = FABRIC_CARD_TYPE;
                            chassis_information.status=1;
                        }
                    }
                    else if(strcmp(name,"topology")==0)
                    {
                        if(strcmp(val,"static")==0)
                        {
                            chassis_information.topology_type = 0;
                        }
                        else if(strcmp(val,"dynamic")==0)
                        {
                            chassis_information.topology_type = 1;
                        }
                    }
                    else if(strcmp(name,"internal_tag_vlan")==0)
                    {
                        chassis_information.internal_tag_vlan = atoi(val);  
                    }
                    else if(strcmp(name,"internal_untag_vlan")==0)
                    {
                        chassis_information.inetrnal_untag_vlan = atoi(val);  
                    }
		} else if (!chassis_empty(s)) {
			chassis_parse_error(lineno, "config file line not field nor header");
		}
		lineno++;
	}
	fclose(f);
	free(line);
#if 0
        for(lineno=0;lineno<MAXIMUM_MICRO_SERVER_CARD_NUMBER;lineno++)
        {
            printf("us number:%d,%d.%d.%d.%d,%d,%d\n",
                chassis_information.us_card[lineno].us_card_number,
                chassis_information.us_card[lineno].ipaddress[0],
                chassis_information.us_card[lineno].ipaddress[1],
                chassis_information.us_card[lineno].ipaddress[2],
                chassis_information.us_card[lineno].ipaddress[3],
                chassis_information.us_card[lineno].type,
                chassis_information.us_card[lineno].status);
        }
            printf("main us number:%d,%d.%d.%d.%d,%d\n",
                chassis_information.us_card_number,
                chassis_information.ipaddress[0],
                chassis_information.ipaddress[1],
                chassis_information.ipaddress[2],
                chassis_information.ipaddress[3],
                chassis_information.type);
#endif
	chassis_bpdu_initial(&chassis_information.ctpm,chassis_information.us_card_number,chassis_information.topology_type);
	return 0;
}

int chassis_initial_ip(void)
{
#define PREFIX_1 ""
    int us_number = chassis_information.us_card_number;
    char buffer[MAXIMUM_BUFFER_LENGTH]= {0};
    int ret =0;
#if(VLAN_INTERNAL_INTERFACE >0)
#if (ONLP>0)
    /* insert vlan mod and vconfig tools first*/
    //sprintf(buffer,"insmod /usr/sbin/8021q.ko;sleep 3;dpkg -i /usr/sbin/vlan_1.9-3_amd64.deb;sleep 3;");
    //if (system(buffer) != 0)
    //{
    //    printf("chassisd initial ip address fail!\n\r");
    //    return -1;
   // }
#endif
    sprintf(buffer,"vconfig add "DEFAULT_MGMT" %d;sleep 3;ifconfig "DEFAULT_MGMT".%d %d.%d.%d.%d up\n",
        chassis_information.internal_tag_vlan,
        chassis_information.internal_tag_vlan,
        chassis_information.us_card[us_number-1].ipaddress[0],
        chassis_information.us_card[us_number-1].ipaddress[1],
        chassis_information.us_card[us_number-1].ipaddress[2],
        chassis_information.us_card[us_number-1].ipaddress[3]);
#else
    //sprintf(buffer,PREFIX_1"ifconfig "INTERNAL_INTERFACE" %d.%d.%d.%d up\n",
    sprintf(buffer,PREFIX_1"ifconfig "DEFAULT_MGMT":%d %d.%d.%d.%d up\n",
        chassis_information.internal_tag_vlan,
        chassis_information.us_card[us_number-1].ipaddress[0],
        chassis_information.us_card[us_number-1].ipaddress[1],
        chassis_information.us_card[us_number-1].ipaddress[2],
        chassis_information.us_card[us_number-1].ipaddress[3]);
#endif
    printf("us number is %d\n",us_number);
    printf("%s",buffer);
    
    if (system(buffer) != 0)
    {
        printf("chassisd initial ip address fail!\n\r");
        return -1;
    }

    printf("chassisd initial ip address success!\n\r");

#if 0

    //memset(DEFAULT_INTERFACE,0,INTERFACE_NAME_LENGTH);
 #if(VLAN_INTERNAL_INTERFACE >0)
    sprintf(DEFAULT_INTERFACE,DEFAULT_MGMT":%d",chassis_information.internal_tag_vlan);
 #else
    sprintf(DEFAULT_INTERFACE,DEFAULT_MGMT".%d",chassis_information.internal_tag_vlan);
 #endif
 #endif
 
#if(VLAN_INTERNAL_INTERFACE >0)

    //printf("chassisd internal tag vlan :%d %x!\n\r",chassis_information.internal_tag_vlan,chassis_information.internal_tag_vlan);
    //printf("chassisd internal untag vlan :%d %x!\n\r",chassis_information.inetrnal_untag_vlan,chassis_information.inetrnal_untag_vlan);
    
    if(us_number>=33&& us_number<=48&&us_number%2==1){
        memset(buffer,0,MAXIMUM_BUFFER_LENGTH);
        sprintf(buffer, "\
bcm5396 -w 0x61 -p 0x5 -l 2 -v 0x00%x;\
bcm5396 -w 0x63 -p 0x5 -l 8 -v 00000000007f7fc1;\
bcm5396 -w 0x60 -p 0x5 -l 1 -v 0x80;\
bcm5396 -w 0x61 -p 0x5 -l 2 -v 0x00%x;\
bcm5396 -w 0x63 -p 0x5 -l 8 -v 000000ffffffffc1;\
bcm5396 -w 0x60 -p 0x5 -l 1 -v 0x80;",
                        chassis_information.internal_tag_vlan,
                        chassis_information.inetrnal_untag_vlan);
        ret = system(buffer);
        if (ret != 0)
        {
            printf("Initial VLAN GROUP config (%d): Error!\r\n", ret);
            fflush(stdout);
            return ret;
        }
      
        memset(buffer,0,MAXIMUM_BUFFER_LENGTH);
        sprintf(buffer,"\
bcm5396 -w 0x10 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x12 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x14 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x16 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x18 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x1a -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x1c -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x1e -p 0x34 -l 2 -v 0x00%x;",
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan);
         ret = system(buffer);        
        if (ret != 0)
        {
            printf("Initial VLAN PVID 1 config (%d): Error!\r\n", ret);
            fflush(stdout);
            return ret;
        }

        memset(buffer,0,MAXIMUM_BUFFER_LENGTH);
        sprintf(buffer, "\
bcm5396 -w 0x20 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x22 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x24 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x26 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x28 -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x2a -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x2c -p 0x34 -l 2 -v 0x00%x;\
bcm5396 -w 0x2e -p 0x34 -l 2 -v 0x00%x;",
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan,
                        chassis_information.inetrnal_untag_vlan);
                        //bcm5396 -w 0x30 -p 0x34 -l 2 -v 0x00%x;");
        ret = system(buffer);        
        if (ret != 0)
        {
            printf("Initial VLAN PVID 2 config (%d): Error!\r\n", ret);
            fflush(stdout);
            return ret;
        }
   
        memset(buffer,0,MAXIMUM_BUFFER_LENGTH);
        sprintf(buffer, "bcm5396 -w 0x0 -p 0x34 -v 0xe2;");
        ret = system(buffer);
        if (ret != 0)
        {
            printf("Initial VLAN config (%d): Error!\r\n", ret);
            fflush(stdout);
            return ret;
        }
       
        printf("chassisd initial vlan success!\n\r");
    }
#endif

    return 0;
}

static int chassis_diag_function_delete_temporary_file(void)
{
    char command_buf[MAXIMUM_BUFFER_LENGTH] = {0};


    snprintf(command_buf, MAXIMUM_STRING_LENGTH, "rm -f %s", DIAG_DEFAULT_INFORMATIN_FILE);
    if (system(command_buf) != 0)
    {
        printf("Delete %s file is failed!\r\n", DIAG_DEFAULT_INFORMATIN_FILE);
        fflush(stdout);
        return -1;
    }

    return 0;
}

int chassis_set_us_card_number_of_switch_board(void)
{
    int ret =0;
    unsigned int us_card_number = 0, us_card_number_of_switch_board = 0, switch_board_type = 0, board_slot_id = 0,cpu_id =0;
    char command_buf[MAXIMUM_BUFFER_LENGTH] = {0},buf[MAXIMUM_BUFFER_LENGTH]={0};


    if (chassis_diag_function_delete_temporary_file() != 0)
	return -1;
#if (ONLP>0)
    /* Get from onlp bus first, if can't find, try to get original i2c bus*/
    sprintf(buf, "i2cget -f -y 4 0x60 0x2 > %s", DIAG_DEFAULT_INFORMATIN_FILE);
    ret = system(buf);
    if (ret == 0)
    {
        printf("i2cget -f -y 4 0x60 0x2 (%d): Success!\r\n", ret);    
    }
  else
#endif
  {
    sprintf(buf, "i2cset -f -y 0 0x76 0 0x4 > %s", DIAG_DEFAULT_INFORMATIN_FILE_1);
    ret = system(buf);
    if (ret != 0)
    {
        printf("2cset -f -y 0 0x76 0 0x4 (%d): Error!\r\n", ret);
        fflush(stdout);
        chassis_diag_function_delete_temporary_file();
        return -1;
    }
    sprintf(buf, "i2cget -f -y 0 0x60 0x2 > %s", DIAG_DEFAULT_INFORMATIN_FILE);
    ret = system(buf);
    if (ret != 0)
    {
        printf("i2cget -f -y 0 0x60 0x2 (%d): Error!\r\n", ret);
        fflush(stdout);
        chassis_diag_function_delete_temporary_file();
        return -1;
    }
  }
    //sprintf(buf, "echo 0x13 > %s", DIAG_DEFAULT_INFORMATIN_FILE);
    //system(buf);
    ret = tcp_server_diag_function_search_string(DIAG_DEFAULT_INFORMATIN_FILE, ":,\r\n", buf, MAXIMUM_BUFFER_LENGTH, 1, 1);
    printf("Slot value :%s\n\r",buf);    
    board_slot_id = (unsigned int) strtoul(buf, NULL, 16);

    cpu_id = (board_slot_id & 0x80)?1:0;
        
    switch_board_type = (board_slot_id & 0x10)?1:0;

    us_card_number_of_switch_board = (board_slot_id & 0x08)?1:0;

    /* LC */
    if(switch_board_type==0)
        us_card_number = ((board_slot_id & 0x07))*2 + us_card_number_of_switch_board*MAXIMUM_LINECARD/2 + cpu_id+1;
    /* FC */
    else
        us_card_number = ((board_slot_id & 0x07))*2 + us_card_number_of_switch_board*MAXIMUM_FABRICCARD/2 + switch_board_type*MAXIMUM_LINECARD+cpu_id+1;

    printf("board_slot_id:%d, cpu_id:%d, switch_board_type:%d, us_card_number_of_switch_board:%d, us_card_number:%d\n\r",
        board_slot_id,
        cpu_id,
        switch_board_type,
        us_card_number_of_switch_board,
        us_card_number);
    chassis_diag_function_delete_temporary_file();
         
    snprintf(command_buf, MAXIMUM_STRING_LENGTH, "echo \"%u\" > %s", us_card_number, CONFIG_US_CARD_NUMBER);
    if (system(command_buf) != 0)
	return -1;

    return ret;
}

void chassis_state_to_string(unsigned long state, char** res)
{
    const char state_string[][MAXIMUM_BUFFER_LENGTH]={
            {"PORT_NO_SPANNING_TREE"},
             {"PORT_DISABLED"},
             {"PORT_BLOCKING"},
             {"PORT_LISTENING"},
             {"PORT_LEARNING"},
             {"PORT_FORWARDING"},
         };
    char buff[MAXIMUM_BUFFER_LENGTH]="Unknown";

    *res =(char *)malloc(MAXIMUM_BUFFER_LENGTH);  

    if (state>=(sizeof(state_string)/MAXIMUM_BUFFER_LENGTH)) 
    {
        strncpy(*res,buff,MAXIMUM_BUFFER_LENGTH);
    }
    else
    {
        strncpy(*res,state_string[state],MAXIMUM_BUFFER_LENGTH);
    }
}

int chassis_config_port(long state, long pid)
{
    int ret =0;
    int us_number = chassis_information.us_card_number;
    char *buff;
    int port_state_array[]={0x00,0x20,0x40,0x60,0x80,0xA0};
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    
    if(us_number%2==0) return -1;

    chassis_state_to_string(state,&buff);
    printf("Config port:%lu, state:%s\n\r",pid,buff );

    if(us_number>=1 && us_number <=32)
    {
        sprintf(buf, "bcm5389 -w 0x%x -p 0 -v 0x%2x -l 1",(int)pid,port_state_array[state]);
        DEBUG_PRINT("Command:%s\n\r",buf);
        ret = system(buf);
        if (ret != 0)
        {
            DEBUG_PRINT("Config port:%lu (%d): Error!\r\n", pid,ret);
            fflush(stdout);
            free(buff);
            return -1;
        }      
    }
    else
    {
        sprintf(buf, "bcm5396 -w 0x%x -p 0 -v 0x%2x -l 1",(int)pid,port_state_array[state]);
        DEBUG_PRINT("Command:%s\n\r",buf);
        ret = system(buf);
        if (ret != 0)
        {
            DEBUG_PRINT("Config port:%lu (%d): Error!\r\n", pid,ret);
            fflush(stdout);
            free(buff);
            return -1;
        }  
        
    }

    free(buff);
    return ret;
}

int chassis_initial_static_lc(void)
{
    int us_number = chassis_information.us_card_number;
    int ret =0;

    /* If CPU is B , do nothing*/
    if(us_number%2 == 0) return 0;
    /*CPU A*/

    ret|=chassis_config_port(PORT_FORWARDING, PORT_8_FC0);
    
    if(us_number == FIRST_LC_SLOT_ID||us_number == SECOND_LC_SLOT_ID)
    {
        ret|=chassis_config_port(PORT_FORWARDING, PORT_8_FC1);
        ret|=chassis_config_port(PORT_FORWARDING, PORT_8_FC2);
        ret|=chassis_config_port(PORT_FORWARDING, PORT_8_FC3);
    }


    return ret;
}


int chassis_initial_static_fc(void)
{
    int us_number = chassis_information.us_card_number;

    /* If CPU is B , do nothing*/
    if(us_number%2 == 0) return 0;
    
    /*CPU A*/    
    if(us_number == 33||us_number == 41)
    {
        chassis_config_port(PORT_FORWARDING, PORT_16_LC0);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC1);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC2);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC3);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC4);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC5);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC6);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC7);
        chassis_config_port(PORT_FORWARDING, PORT_16_FC);
        chassis_config_port(PORT_FORWARDING, PORT_16_RJ45);
    }
    
    chassis_config_port(PORT_FORWARDING, PORT_16_LC0);
    chassis_config_port(PORT_FORWARDING, PORT_16_RJ45);

    return 0;
}

int chassis_config_lc_stp(int status)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    int state=0x6;

    printf("Chassis_Config_STP\n\r");

    if(status==1) state |=0x1;
    /*Enable manual mode*/
    sprintf(buf, "bcm5389 -w 0xb -p 0 -v %d -l 1",state);
    ret = system(buf);
    printf("%s\r\n",buf);    
    if (ret != 0)
    {
        printf("Config LC STP (%d): Error!\r\n", ret);
        fflush(stdout);
        return -1;
    }
    printf("Config LC STP Success!\r\n");
    fflush(stdout);
    return 0;
}

int chassis_config_fc_stp(int status)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    int state=0x6;

    printf("Chassis_Config_STP\n\r");

    if(status==1) state |=0x1;
    /*Enable manual mode*/
    sprintf(buf, "bcm5396 -w 0x20 -p 0 -v %d -l 1",state);
    ret = system(buf);
    printf("%s\r\n",buf);    
    if (ret != 0)
    {
            printf("Config FC STP (%d): Error!\r\n", ret);
            fflush(stdout);
            return -1;
    }
    printf("Config FC STP Success!\r\n");
    fflush(stdout);
    return 0;
}
int chassis_initial_spi(void)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    
    sprintf(buf, "i2cset -f -y 0 0x76 0x0 0x4;i2cset -y -f 0 0x60 0x48 0x99");
    ret = system(buf);
    if (ret != 0)
    {
        printf("Initial SPI (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Initial SPI Success!\r\n");
    fflush(stdout);
    
    return ret;
}


int chassis_initial_phy(void)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    
    sprintf(buf, "bcm5396 -w 0x86 -p 0x0 -v 0x20;i2cset -f -y 0 0x60 0x8 0xfe;i2cset -f -y 0 0x60 0x8 0xff;bcm5396 -w 0x8 -p 0x8a -l 2 -v 0x3e1");
    ret = system(buf);
    if (ret != 0)
    {
        printf("Initial FC PHY (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Initial FC PHY Success!\r\n");
    fflush(stdout);
    
    return ret;
}
/*set LC serdes mode*/
int chassis_set_serdesmode(void)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    
    sprintf(buf, "bcm5389 -w 0x20 -p 0x12 -v 0x01c1 -l 2;\
bcm5389 -w 0x20 -p 0x13 -v 0x01c1 -l 2;\
bcm5389 -w 0x20 -p 0x14 -v 0x01c1 -l 2;\
bcm5389 -w 0x20 -p 0x15 -v 0x01c1 -l 2;");

    ret = system(buf);
    if (ret != 0)
    {
        printf("Initial LC Serdes mode (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Initial LC Serdes mode Success!\r\n");
    fflush(stdout);
    
    return ret;
}

/*set FC serdes and slave mode*/
int chassis_set_slavemode(void)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    
    sprintf(buf, "bcm5396 -w 0x20 -p 0x10 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x11 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x12 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x13 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x14 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x15 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x16 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x17 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x18 -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x1a -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x1b -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x1c -v 0x01d1 -l 2;\
bcm5396 -w 0x20 -p 0x1d -v 0x01d1 -l 2;");

    ret = system(buf);
    if (ret != 0)
    {
        printf("Initial FC Serdes and Slave mode (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Initial FC Serdes and Slave mode Success!\r\n");
    fflush(stdout);
    
    return ret;
}

int chassis_check_master_fc(CTPM_T* this)
{
    int ret=0;
    int i=0,j=0;
    int us_number = chassis_information.us_card_number;
    PORT_ID root_id=this->rootPortId;
    PORT_ID  dbridge_id=htons(this->BrId.prio)&0x00FF;
    PORT_ID  local_Master[4]={0,0,0,0};
    if(us_number>=1 && us_number <=32)
    {
        return 0;
    }
    /*if it is Root FC, it must be also Master FC*/
    /*if(root_id==dbridge_id)
    {
        return 1;
    }*/
    /*Find Other Local Master FC*/
    memset(local_Master,0,4);
    for (i=0;i<4;i++)
    {          
        for (j=i*4+33;j<33+(i+1)*4;j++)
        {
            if(this->chassis_exist[j-1]>0)
            {
                if(root_id==j)
                {
                    local_Master[i] = j;
                }
                else if(local_Master[i]==0 || (j<local_Master[i]))
                {
                    local_Master[i] = j;
                }
            }
        }
    }
    /* log which is local Master*/
    for(i=0;i<4;i++)
    {
        chassis_information.local_Master[i] = local_Master[i];
    }
   
    for (i=0;i<4;i++)
    {
        if(dbridge_id==local_Master[i])
        {
            return 1;            
        }
    }
    return ret;
}
int chassis_initial_master_fc(CTPM_T* this)
{
    int ret =0;
#if (ONLP>0)
    char buf[MAXIMUM_BUFFER_LENGTH]={0};
    int us_number = chassis_information.us_card_number;
    int enable =0;

    if(us_number>=1 && us_number <=32)
    {
        return 0;
    }
    enable = chassis_check_master_fc(this);

    switch(enable){
        case 1:
            sprintf(buf, "echo 1 > /sys/bus/i2c/devices/2-0066/fan_enable;\
                echo 1 > /sys/bus/i2c/devices/8-0066/fan_enable;\
                echo 1 > /sys/bus/i2c/devices/10-0061/pdu_enable;");
            break;
        default:
            sprintf(buf, "echo 0 > /sys/bus/i2c/devices/2-0066/fan_enable;\
                echo 0 > /sys/bus/i2c/devices/8-0066/fan_enable;\
                echo 0 > /sys/bus/i2c/devices/10-0061/pdu_enable;");
            break;
    }
    ret = system(buf);
    if (ret != 0)
    {
        printf("Initial Master FC (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Initial Master FC (%d): Success!\r\n", enable);
#endif

    return ret;
}
int chassis_initial_default_topology(void)
{
    int us_number = chassis_information.us_card_number;

    if(us_number>=1 && us_number <=32)
    {
        if((us_number%2)==1) 
        {
            chassis_set_serdesmode();
            chassis_config_lc_stp(1);
            chassis_add_static_arl(us_number);
            chassis_config_multicast(us_number);
        }

        chassis_config_port(PORT_BLOCKING, PORT_8_FC0);
        chassis_config_port(PORT_BLOCKING, PORT_8_FC1);
        chassis_config_port(PORT_BLOCKING, PORT_8_FC2);
        chassis_config_port(PORT_BLOCKING, PORT_8_FC3);
        chassis_config_port(PORT_FORWARDING, PORT_8_C1);
        chassis_config_port(PORT_FORWARDING, PORT_8_C2);
    }
    else
    {
        if((us_number%2)==1) 
        {
            chassis_initial_phy();
           
            chassis_set_slavemode();
            chassis_config_fc_stp(1);
            chassis_add_static_arl(us_number);
            chassis_config_multicast(us_number);
        }
        chassis_config_port(PORT_FORWARDING, PORT_16_LC0);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC1);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC2);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC3);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC4);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC5);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC6);
        chassis_config_port(PORT_FORWARDING, PORT_16_LC7);
        chassis_config_port(PORT_FORWARDING, PORT_16_FC);
        chassis_config_port(PORT_FORWARDING, PORT_16_RJ45);
        chassis_config_port(PORT_FORWARDING, PORT_16_C1);
        chassis_config_port(PORT_FORWARDING, PORT_16_C2);
    }


    printf("Initial Default Topology complete!\n\r");
    return 0;
}

int chassis_initial_topology(void)
{

    int us_number = chassis_information.us_card_number;

    chassis_initial_spi();
    
    chassis_initial_default_topology();
    
    /*Topology is static */    
    if(chassis_information.topology_type==0)
    {
        if(us_number>=1 && us_number <=32)
        {
            chassis_initial_static_lc();
            printf("Initial static LC %d complete!\n\r",us_number );
        }
        else
        {
            chassis_initial_static_fc();
            printf("Initial static FC %d complete!\n\r",(us_number-32) );
        }
    }
    printf("Initial Topology complete!\n\r");
    return 0;
}

int chassis_add_static_arl(int us_number)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};

    if(us_number%2==0)
    {
        printf("CPU B don't do anything.\n\r");
        return 0;
    }

#if 0    
    /*upper LCs*/
    if(1<=us_number&&us_number<=16)
    {
        sprintf(buf, "bcm5389 -w 0x2 -p 0x5 -l 6 -v 0x0180c2000011;\
            bcm5389 -w 0x8 -p 0x5 -l 2 -v 0x0001;\
            bcm5389 -w 0x10 -p 0x5 -l 8 -v 0x00010180c2000011;\
            bcm5389 -w 0x18 -p 0x5 -l 2 -v 0xe000;\
            bcm5389 -w 0x0 -p 0x5 -l 1 -v 0x80;");
    }
    /*upper FCs*/
    else if(33<=us_number&&us_number<=40)
    {
    
        sprintf(buf, "bcm5396 -w 0x2 -p 0x5 -l 6 -v 0x0180c2000022;\
            bcm5396 -w 0x8 -p 0x5 -l 2 -v 0x0001;\
            bcm5396 -w 0x10 -p 0x5 -l 8 -v 0x00000180c2000022;\
            bcm5396 -w 0x18 -p 0x5 -l 4 -v 0x000003b8;\
            bcm5396 -w 0x0 -p 0x5 -l 1 -v 0x80;");/*
        sprintf(buf, "bcm5396 -w 0x2 -p 0x5 -l 6 -v 0x112233445566;\
            bcm5396 -w 0x8 -p 0x5 -l 2 -v 0x0001;\
            bcm5396 -w 0x10 -p 0x5 -l 8 -v 0x00000112233445566;\
            bcm5396 -w 0x18 -p 0x5 -l 4 -v 0x000003b8;\
            bcm5396 -w 0x0 -p 0x5 -l 1 -v 0x80;");    */        
    }
    /*down LCs*/
    else if(17<=us_number&&us_number<=32)
    {
        sprintf(buf, "bcm5389 -w 0x2 -p 0x5 -l 6 -v 0x0180c2000022;\
            bcm5389 -w 0x8 -p 0x5 -l 2 -v 0x0001;\
            bcm5389 -w 0x10 -p 0x5 -l 8 -v 0x00010180c2000022;\
            bcm5389 -w 0x18 -p 0x5 -l 2 -v 0xe000;\
            bcm5389 -w 0x0 -p 0x5 -l 1 -v 0x80;");
    }
    /*down FCs*/    
    else if(41<=us_number&&us_number<=48)
    {
        sprintf(buf, "bcm5396 -w 0x2 -p 0x5 -l 6 -v 0x0180c2000011;\
            bcm5396 -w 0x8 -p 0x5 -l 2 -v 0x0001;\
            bcm5396 -w 0x10 -p 0x5 -l 8 -v 0x00000180c2000011;\
            bcm5396 -w 0x18 -p 0x5 -l 4 -v 0x000003b8;\
            bcm5396 -w 0x0 -p 0x5 -l 1 -v 0x80;");
    }    
    else
    {
        printf(" Error slot id.\n\r");
        return -1;
    }

    
    ret = system(buf);
    if (ret != 0)
    {
        printf("Add static entry (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Add static entry Success!\r\n");
    fflush(stdout);
#endif
    if(1<=us_number&&us_number<=16)
    {
        sprintf(buf, "bcm5389 -w 0x0 -p 0x4 -l 1 -v 0x10;\
            bcm5389 -w 0x04 -p 0x4 -l 6 -v %s;\
            bcm5389 -w 0x10 -p 0x4 -l 6 -v %s;\
            bcm5389 -w 0x16 -p 0x4 -l 6 -v 0x00000001;",LC1_MULTICAST_ADDRESS,LC1_MULTICAST_ADDRESS);
    }
    /*upper FCs*/
    else if(33<=us_number&&us_number<=40)
    {
    
        sprintf(buf, "bcm5396 -w 0x0 -p 0x4 -l 1 -v 0x10;\
            bcm5396 -w 0x04 -p 0x4 -l 6 -v %s;\
            bcm5396 -w 0x10 -p 0x4 -l 6 -v %s;\
            bcm5396 -w 0x16 -p 0x4 -l 6 -v 0x00004000;",FC1_MULTICAST_ADDRESS,FC1_MULTICAST_ADDRESS);     
    }
    /*down LCs*/
    else if(17<=us_number&&us_number<=32)
    {
        sprintf(buf, "bcm5389 -w 0x0 -p 0x4 -l 1 -v 0x10;\
            bcm5389 -w 0x04 -p 0x4 -l 6 -v %s;\
            bcm5389 -w 0x10 -p 0x4 -l 6 -v %s;\
            bcm5389 -w 0x16 -p 0x4 -l 6 -v 0x00000001;",FC1_MULTICAST_ADDRESS,FC1_MULTICAST_ADDRESS);
    }
    /*down FCs*/    
    else if(41<=us_number&&us_number<=48)
    {
        sprintf(buf, "bcm5396 -w 0x0 -p 0x4 -l 1 -v 0x10;\
            bcm5396 -w 0x04 -p 0x4 -l 6 -v %s;\
            bcm5396 -w 0x10 -p 0x4 -l 6 -v %s;\
            bcm5396 -w 0x16 -p 0x4 -l 6 -v 0x00004000;",LC1_MULTICAST_ADDRESS,LC1_MULTICAST_ADDRESS);
    }    
    else
    {
        printf(" Error slot id.\n\r");
        return -1;
    }

    
    ret = system(buf);
    if (ret != 0)
    {
        printf("Add static entry (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Add static entry Success!\r\n");
    fflush(stdout);
    return ret;
}

int chassis_config_multicast(int us_number)
{
    int ret =0;
    char buf[MAXIMUM_BUFFER_LENGTH]={0};

    if(us_number%2==0)
    {
        printf("CPU B don't do anything.\n\r");
        return 0;
    }
    /*upper LCs*/
    if(1<=us_number&&us_number<=16)
    {
        sprintf(buf, "bcm5389 -w 0x2f -p 0x0 -v 0x0a");
    }
    /*upper FCs*/
    else if(33<=us_number&&us_number<=40)
    {
        sprintf(buf, "bcm5396 -w 0x50 -p 0x0 -v 0x12");
    }
    /*down LCs*/
    else if(17<=us_number&&us_number<=32)
    {
        sprintf(buf, "bcm5389 -w 0x2f -p 0x0 -v 0x12");
    }
    /*down FCs*/    
    else if(41<=us_number&&us_number<=48)
    {
        sprintf(buf, "bcm5396 -w 0x50 -p 0x0 -v 0x0a");
    }    
    else
    {
        printf(" Error slot id.\n\r");
        return -1;
    }

    
    ret = system(buf);
    if (ret != 0)
    {
        printf("Config Multicast table (%d): Error!\r\n", ret);
        fflush(stdout);
        return ret;
    }
    printf("Config Multicast table Success!\r\n");
    fflush(stdout);
    
    return ret;
}

void chassis_vdprintf(int level, const char *fmt, va_list ap)
{		
    struct tm *local_tm;
    int l ;
    time_t clock;
    char logbuf[256];
    logbuf[255] = 0;
    if (level > log_level)
        return;	
    if (!is_daemon)
    {		
        time(&clock);
        local_tm = localtime(&clock);
        l = strftime(logbuf, sizeof(logbuf) - 1, "%F %T ", local_tm);
        vsnprintf(logbuf + l, sizeof(logbuf) - l - 1, fmt, ap);
        printf("%s\n", logbuf);	}
    else 
    {		
        vsyslog((level <= LOG_LEVEL_INFO) ? LOG_INFO : LOG_DEBUG, fmt,			ap);	
     }
}

void chassis_dprintf(int level, const char *fmt, ...)
{	
    va_list ap;
    va_start(ap, fmt);
    chassis_vdprintf(level, fmt, ap);
    va_end(ap);
}

int main(int argc, char **_argv)
{

        /* Our process ID and Session ID */
	pid_t pid, sid;
	int opt;   


	printf("Chassis Version = (%s)\n\r",VERSION_STRING);
	printf("Chassis Managment for Chassis utility\n\r");
    
	while ((opt = getopt_long(argc, _argv, "", options, NULL)) != -1) { 
		if (opt == '?') {
			chassis_usage(); 
		} else if (opt == 0)
			break;		    
	} 

        
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);
                
        /* Open any logs here */        
                
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        

        
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }
        
        /* Close out the standard file descriptors */
        //close(STDIN_FILENO);
        //close(STDOUT_FILENO);
        //close(STDERR_FILENO);
        
        /* Daemon-specific initialization goes here */
        if (pidfile)
			chassis_write_pidfile();
        signal(SIGUSR1, chassis_handle_sigusr1);
        eventloop_event_signal(SIGUSR1);
        signal(SIGUSR2, chassis_handle_sigusr2);
        eventloop_event_signal(SIGUSR2);
        signal(SIGRTMIN, chassis_handle_readyping);
        eventloop_event_signal(SIGRTMIN);
        signal(SIGRTMIN+1, chassis_handle_leftping);
        eventloop_event_signal(SIGRTMIN+1); 
        signal(SIGRTMIN+2, chassis_handle_readypowercycle);
        eventloop_event_signal(SIGRTMIN+2); 

    	TST(netif_utils_netsock_init() == 0, -1);

        /*initial slot id from hardware by i2c*/
        chassis_set_us_card_number_of_switch_board();
        
        /*open config file*/
        chassis_parse_config_file(CONFIG_FILENAME);
      
        /* Initial IP Address */
        chassis_initial_ip();

        /* Initial Topology */
        chassis_initial_topology();        
        /* The Big Loop */

	TST(epoll_loop_init() == 0, -1);
	/*TST(packet_sock_init() == 0, -1);*/

        /*Initial TCP and UDP server*/
        udp_client_initial_socket();
        
        udp_sock=upd_server_init();       
        /*tcp_sock =tcp_server_init();*/
        bpdu_sock = bpdu_initial_socket();  
#if 0       
        while (1) {
           bpdu_client_send(mac,sendline,size);
           /* Do some task here ... */
            sleep(10); /* wait 30 seconds */
           upd_server_recev(udp_sock);
           //tcp_server_accept(tcp_sock);
            //eventloop();
            //bpdu_client_recv(bpdu_sock,receline,1);
        }
#endif
    printf("Chassis daemon is starting...\n\r");
    epoll_loop_main_loop();
    exit(EXIT_SUCCESS);
}


