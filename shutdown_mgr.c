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

#include <fcntl.h> /* For O_RDWR && open */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/ioctl.h>

#include "shutdown_mgr.h"


extern int  tcp_server_doShutdownToClinet(int  slotid, int option);
static int
shutdown_mgr_shutdown_linecard_remote(int lc_slot_id);

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)										 \
		printf ("shutdown_mgr:%s[%d]: " fmt "\r\n", __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#define SYS_LOG(priority, fmt, args...) 					\
	do {													\
		DEBUG_PRINT(fmt, ##args);							\
		openlog("chassisd", LOG_CONS | LOG_PID, LOG_DAEMON);\
		syslog(priority, fmt, ##args);						\
		closelog();											\
	} while (0)

#define SYS_LOG_INFO(fmt, args...)		SYS_LOG(LOG_INFO,    fmt, ##args)
#define SYS_LOG_WARNING(fmt, args...)	SYS_LOG(LOG_WARNING, fmt, ##args)
#define SYS_LOG_EMERG(fmt, args...)		SYS_LOG(LOG_EMERG,   fmt, ##args)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MAX_PATH_LEN	64

#define CARD_TYPE_LC	0
#define CARD_TYPE_FC	1
#define NUM_OF_THERMAL	72
#define NUM_OF_FC		2
#define NUM_OF_LC		8
#define POWER_OFF_VALUE	0
#define POWER_ON_VALUE	1
#define RESET_VALUE		1
#define SD_MGR_UPDATE_INTERVAL				3000  /* ms */
#define SD_MGR_IOERR_SHUTDOWN_THRESHOLD		60000 /* ms */
#define SD_MGR_IOERR_RETRY_COUNT			2

#define VALIDATE_FC_SLOT_ID(id) \
	do {						\
		if (id < 0 || id > 3) {	\
			return -EINVAL;		\
		}						\
	} while (0)

#define VALIDATE_LC_SLOT_ID(id) \
	do {						\
		if (id < 0 || id > 7) {	\
			return -EINVAL;		\
		}						\
	} while (0)

#define VALIDATE_THERMAL_ID(id) 			\
	do {									\
		if (id < LC1_TEMP1_INPUT || 		\
			id > FC2_CPUB_TEMP6_INPUT) {	\
			return -EINVAL;					\
		}									\
	} while (0)		

#define VALIDATE_ERROR(f)	\
	do {					\
		int ret = f;		\
		if (ret < 0) {		\
			return ret;		\
		}					\
	} while (0)

#define VALIDATE_ERROR_RET(f, ret)		\
	do {							\
		if ((f) < 0) {				\
			return ret;				\
		}							\
	} while (0)

#define VALIDATE_ERROR_EIO(func)	VALIDATE_ERROR_RET(func, -EIO)
#define VALIDATE_ERROR_EINVAL(func)	VALIDATE_ERROR_RET(func, -EINVAL)

#define LC_TEMP_INPUT(LID) 	\
	LC##LID##_TEMP1_INPUT,	\
	LC##LID##_TEMP2_INPUT,	\
	LC##LID##_TEMP3_INPUT,	\
	LC##LID##_TEMP4_INPUT,	\
	LC##LID##_TEMP5_INPUT,	\
	LC##LID##_TEMP6_INPUT

#define FC_TEMP_INPUT(FID, CID)			\
	FC##FID##_CPU##CID##_TEMP1_INPUT,	\
	FC##FID##_CPU##CID##_TEMP2_INPUT,	\
	FC##FID##_CPU##CID##_TEMP3_INPUT,	\
	FC##FID##_CPU##CID##_TEMP4_INPUT,	\
	FC##FID##_CPU##CID##_TEMP5_INPUT,	\
	FC##FID##_CPU##CID##_TEMP6_INPUT

enum thermal_id {
	RESERVED,
	LC_TEMP_INPUT(1),
	LC_TEMP_INPUT(2),
	LC_TEMP_INPUT(3),
	LC_TEMP_INPUT(4),
	FC_TEMP_INPUT(1, A),
	FC_TEMP_INPUT(2, A),
	LC_TEMP_INPUT(5),
	LC_TEMP_INPUT(6),
	LC_TEMP_INPUT(7),
	LC_TEMP_INPUT(8),
	FC_TEMP_INPUT(1, B),
	FC_TEMP_INPUT(2, B)
};

enum MAC_ID {
	MAC_A,
	MAC_B,
	NUM_OF_MAC
};

enum CPU_ID {
	CPU_A,
	CPU_B,
	NUM_OF_CPU
};

#define TEMP_MAX	127000

#if 0
static char* cpu_coretemp_files[] =
{
    "/sys/devices/platform/coretemp.0/temp2_input",
    "/sys/devices/platform/coretemp.0/temp3_input",
    "/sys/devices/platform/coretemp.0/temp4_input",
    "/sys/devices/platform/coretemp.0/temp5_input",
    NULL,
};
#endif

#define FAN_BOARD_1_PREFIX 	 "/sys/bus/i2c/devices/8-0066/"
#define FAN_BOARD_2_PREFIX 	 "/sys/bus/i2c/devices/2-0066/"

static int shutdown_mgr_file_write(char *file, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0)) {
        return -EINVAL;
    }

    if ((fd = open(file, O_WRONLY, S_IWUSR)) == -1) {
        return -EIO;
    }

    if ((len = write(fd, buffer, buf_size)) < 0) {
        close(fd);
        return -EIO;
    }

    if ((close(fd) == -1)) {
        return -EIO;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len)) {
        return -EIO;
    }

    return 0;
}

static int shutdown_mgr_file_write_integer(char *file, int value)
{
	int  ret = 0;
    char buf[8] = {0};
    sprintf(buf, "%d", value);

	DEBUG_PRINT("Write Value (%d) to file(%s)", value, file);
    ret = shutdown_mgr_file_write(file, buf, (int)strlen(buf), 0);
	if (ret < 0) {
		DEBUG_PRINT("Unable to write data to file (%s)\r\n", file);
	}

	return ret;
}

int shutdown_mgr_file_read(char *file, char *buffer, int buf_size, int data_len)
{
    int fd;
    int len;

    if ((buffer == NULL) || (buf_size < 0)) {
        return -EINVAL;
    }

    if ((fd = open(file, O_RDONLY)) == -1) {
        return -EIO;
    }

    if ((len = read(fd, buffer, buf_size)) < 0) {
        close(fd);
        return -EIO;
    }

    if ((close(fd) == -1)) {
        return -EIO;
    }

    if ((len > buf_size) || (data_len != 0 && len != data_len)) {
        return -EIO;
    }

    return 0;
}

int shutdown_mgr_file_read_string(char *file, char *buffer, int buf_size, int data_len)
{
    int ret;

    if (data_len >= buf_size) {
	    return -EINVAL;
	}

	ret = shutdown_mgr_file_read(file, buffer, buf_size-1, data_len);

    if (ret == 0) {
        buffer[buf_size-1] = '\0';
    }

    return ret;
}

static int
shutdown_mgr_file_read_int(char *file, int *value)
{
    int ret = 0;
    char buf[8] = {0};

    ret = shutdown_mgr_file_read_string(file, buf, sizeof(buf), 0);
    *value = (ret == 0) ? atoi(buf) : 0;

	if (ret < 0) {
		DEBUG_PRINT("Unable to read from file (%s)\r\n", file);
	}

    return ret;
}

#if 0
static int
shutdown_mgr_file_read_int_max(char **files, int *value)
{
	int ret = 0, i, temp = 0;

	for (i = 0; files[i] != NULL; i++) {
		ret = shutdown_mgr_file_read_int(files[i], &temp);

		if (ret < 0) {
			break;
		}

		if (i == 0) {
			*value = temp;
		}
		else if (temp > *value) {
			*value = temp;
		}
	}

	return ret;
}

/* Update CPU/MAC temperature to CPLD
 */
static int
shutdown_mgr_update_cpld_temperatue(void)
{
	int   temp_input, rc = 0;
	char *path[] = {"/sys/bus/i2c/devices/4-0060/temp1_input",
					"/sys/bus/i2c/devices/4-0060/temp2_input"};

	/* Read temperature from CPU
	 */
	if (shutdown_mgr_file_read_int_max(cpu_coretemp_files, &temp_input) < 0) {
		rc = -EIO;
		temp_input = 0x40; /* Treat temperature as max if read failed */
		DEBUG_PRINT("Error reading temperature from CPU");
	}

	if (shutdown_mgr_file_write_integer(path[0], temp_input/1000) < 0) {
		rc = -EIO;
	}
	//DEBUG_PRINT("CPU core temp = (%d)", temp_input);


	/* Read temperature from tomahawk
	 */
	if (shutdown_mgr_file_read_int("/var/run/broadcom/temp0", &temp_input) < 0) {
		rc = -EIO;
		temp_input = 0x52; /* Treat temperature as max if read failed */
		DEBUG_PRINT("Error reading temperature from Tomahawk");
	}

	if (shutdown_mgr_file_write_integer(path[1], temp_input/1000) < 0) {
		rc = -EIO;
	}
	//DEBUG_PRINT("Tomahawk temp = (%d)", temp_input);

	return rc;
}
#endif

#define FAN_BOARD_CHAN1_DIR  "/sys/bus/i2c/devices/2-0066/"
#define FAN_BOARD_CHAN7_DIR  "/sys/bus/i2c/devices/8-0066/"

static char*
shutdown_mgr_get_fan_board_dir(int fc_slot, int tid)
{
	if (tid < LC1_TEMP1_INPUT || tid > FC2_CPUB_TEMP6_INPUT) {
		return NULL;
	}	

	if (fc_slot <= 1) {
		return (tid <= 36) ? FAN_BOARD_CHAN7_DIR : FAN_BOARD_CHAN1_DIR;
	}
	else {
		return (tid <= 36) ? FAN_BOARD_CHAN1_DIR : FAN_BOARD_CHAN7_DIR;
	}	
}

static int
shutdown_mgr_temperature_get(int fc_slot, int tid, int *temperature, int *warning, int *shutdown)
{
	VALIDATE_THERMAL_ID(tid);

    int   cid, index;
	char  path[MAX_PATH_LEN] = {0};
	int   value;
	char *fan_dir = NULL;
	char  fan_path[MAX_PATH_LEN] 		= {0};
	char  warning_path[MAX_PATH_LEN] 	= {0};
	char  shutdown_path[MAX_PATH_LEN] 	= {0};
	int   t, w, s; /* temp, warning, shutdown */

	*temperature = 0;
	*warning 	 = 0;
	*shutdown 	 = 0;
	
	fan_dir = shutdown_mgr_get_fan_board_dir(fc_slot, tid);
	if (NULL == fan_dir) {
		return -EINVAL;
	}
	
	switch (tid) {
		case LC1_TEMP1_INPUT ... LC4_TEMP6_INPUT:
			cid    = (tid - LC1_TEMP1_INPUT) / 6 + 1;
			index  = (tid - LC1_TEMP1_INPUT) % 6 + 1;
			break;
		case LC5_TEMP1_INPUT ... LC8_TEMP6_INPUT:
			cid    = (tid - LC5_TEMP1_INPUT) / 6 + 1;
			index  = (tid - LC5_TEMP1_INPUT) % 6 + 1;
			break;
		case FC1_CPUA_TEMP1_INPUT ... FC1_CPUA_TEMP6_INPUT:
			cid    = 1;
			index  = (tid - FC1_CPUA_TEMP1_INPUT) % 6 + 1;
			break;
		case FC1_CPUB_TEMP1_INPUT ... FC1_CPUB_TEMP6_INPUT:
			cid    = 1;
			index  = (tid - FC1_CPUB_TEMP1_INPUT) % 6 + 1;
			break;
		case FC2_CPUA_TEMP1_INPUT ... FC2_CPUA_TEMP6_INPUT:
			cid    = 2;
			index  = (tid - FC2_CPUA_TEMP1_INPUT) % 6 + 1;
			break;
		case FC2_CPUB_TEMP1_INPUT ... FC2_CPUB_TEMP6_INPUT:
			cid    = 2;
			index  = (tid - FC2_CPUB_TEMP1_INPUT) % 6 + 1;
			break;
		default:
			return -EINVAL;
	};

	switch (tid) {
		case LC1_TEMP1_INPUT ... LC4_TEMP6_INPUT:
		case LC5_TEMP1_INPUT ... LC8_TEMP6_INPUT:
			sprintf(path, "%slc%d_temp%d_input", fan_dir, cid, index);
			sprintf(warning_path,  "%slc%d_temp%d_warning",  fan_dir, cid, index);
			sprintf(shutdown_path, "%slc%d_temp%d_shutdown", fan_dir, cid, index);
			break;
		case FC1_CPUA_TEMP1_INPUT ... FC2_CPUA_TEMP6_INPUT:
		case FC1_CPUB_TEMP1_INPUT ... FC2_CPUB_TEMP6_INPUT:
			sprintf(path, "%sfc%d_temp%d_input", fan_dir, cid, index);
			sprintf(warning_path,  "%sfc%d_temp%d_warning", fan_dir, cid, index);
			sprintf(shutdown_path, "%sfc%d_temp%d_shutdown", fan_dir, cid, index);
			break;
		default:
			return -EINVAL;
	};

	/* Check if fan board is present
	 */
	sprintf(fan_path, "%s""fan_present", fan_dir);
	VALIDATE_ERROR(shutdown_mgr_file_read_int(fan_path, &value));
	
	if (value == 0) {
		/* Fan board is not present*/
		DEBUG_PRINT("Fan board is not present(%s)", fan_path);
		return -ENXIO;
	}

	//DEBUG_PRINT("thermal (%d) Path = %s", tid, path);
	//DEBUG_PRINT("thermal (%d) warning path = %s", tid, warning_path);
	//DEBUG_PRINT("thermal (%d) shutdown path = %s", tid, shutdown_path);
    VALIDATE_ERROR(shutdown_mgr_file_read_int(path, &t));
    VALIDATE_ERROR(shutdown_mgr_file_read_int(warning_path, &w));
    VALIDATE_ERROR(shutdown_mgr_file_read_int(shutdown_path, &s));

	*temperature = t;
	*warning 	 = w;
	*shutdown	 = s;

    return 0;								
}

/*  
LC0:( 1 <= tid <=  6) 
LC1:( 7 <= tid <= 12)
LC2:(13 <= tid <= 18)
LC3:(19 <= tid <= 24)
FC1:(25 <= tid <= 30)
FC2:(31 <= tid <= 36)
LC4:(37 <= tid <= 42)
LC5:(43 <= tid <= 48)
LC6:(49 <= tid <= 54)
LC7:(55 <= tid <= 60)
FC1:(61 <= tid <= 66)
FC2:(67 <= tid <= 72)
*/
static int shutdown_mgr_is_linecard_thermal(int tid)
{
	return ((1 <= tid && tid <= 24) || (37 <= tid && tid <= 60));
}

static int shutdown_mgr_is_fabriccard_thermal(int tid)
{
	return ((25 <= tid && tid <= 36) || (61 <= tid && tid <= 72));
}

static int shutdown_mgr_thermal_id_to_slot_id(int tid)
{
	int slot_id = -1;

	switch (tid) {
	case 1 ... 6:	/* LC0 */
		slot_id = 0;
		break;
	case 7 ... 12:	/* LC1 */
		slot_id = 1;
		break;
	case 13 ... 18:	/* LC2 */
		slot_id = 2;
		break;
	case 19 ... 24:	/* LC3 */
		slot_id = 3;
		break;
	case 37 ... 42:	/* LC4 */
		slot_id = 4;
		break;
	case 43 ... 48:	/* LC5 */
		slot_id = 5;
		break;
	case 49 ... 54:	/* LC6 */
		slot_id = 6;
		break;
	case 55 ... 60:	/* LC7 */
		slot_id = 7;
		break;
	case 25 ... 30:	/* FC0 */
	case 61 ... 66:
		slot_id = 0;
		break;
	case 31 ... 36:	/* FC1 */
	case 67 ... 72:
		slot_id = 1;
		break;
	default:
		slot_id = -1;
		break;
	}

	return slot_id;
}

static int
shutdown_mgr_shutdown_card(char *shutdown_dir)
{
	char path[MAX_PATH_LEN] = {0};

	if (!shutdown_dir) {
		return -EINVAL;
	}

    sprintf(path, "/sys/bus/i2c/devices/%s/hot_swap_on", shutdown_dir);
    VALIDATE_ERROR(shutdown_mgr_file_write_integer(path, POWER_OFF_VALUE));

	return 0;
}

static int
shutdown_mgr_poweron_card(char *poweron_dir)
{
	char path[MAX_PATH_LEN] = {0};

	if (!poweron_dir) {
		return -EINVAL;
	}

    sprintf(path, "/sys/bus/i2c/devices/%s/hot_swap_on", poweron_dir);

    if (shutdown_mgr_file_write_integer(path, POWER_ON_VALUE) < 0) {
        return -EIO;
    }

	return 0;
}

int
shutdown_mgr_poweron_linecard_local(int lc_slot_id)
{
	VALIDATE_LC_SLOT_ID(lc_slot_id);
	
	char *lc_poweron_dir[] = {"6-0013",	/* LC0/LC4 */
							   "6-0010",	/* LC1/LC5 */
							   "6-0053",	/* LC2/LC6 */
							   "6-0050"};	/* LC3/LC7 */

	SYS_LOG_EMERG("Prepare to power on LC(%d)", lc_slot_id);
	DEBUG_PRINT("Prepare to power on LC(%d)", lc_slot_id);
	return shutdown_mgr_poweron_card(lc_poweron_dir[lc_slot_id % 4]);
}

int
shutdown_mgr_power_linecard_local(int lc_slot_id, int option)
{
    if(option==0){
        //return shutdown_mgr_shutdown_linecard_remote(lc_slot_id);
        return shutdown_mgr_shutdown_linecard_local(lc_slot_id);
    }
    else
    {
        return shutdown_mgr_poweron_linecard_local(lc_slot_id);
    }
}

int
shutdown_mgr_shutdown_linecard_local(int lc_slot_id)
{
	VALIDATE_LC_SLOT_ID(lc_slot_id);
	
	char *lc_shutdown_dir[] = {"6-0013",	/* LC0/LC4 */
							   "6-0010",	/* LC1/LC5 */
							   "6-0053",	/* LC2/LC6 */
							   "6-0050"};	/* LC3/LC7 */

	SYS_LOG_EMERG("Prepare to shutdown LC(%d)", lc_slot_id);
	return shutdown_mgr_shutdown_card(lc_shutdown_dir[lc_slot_id % 4]);
}

static int
shutdown_mgr_shutdown_linecard_remote(int lc_slot_id)
{
    return tcp_server_do_shutdown_to_client(lc_slot_id,POWER_OFF_VALUE);
    //return 0;
}

static int
shutdown_mgr_shutdown_linecard(int fc_slot, int lc_slot)
{
	VALIDATE_FC_SLOT_ID(fc_slot);
	VALIDATE_LC_SLOT_ID(lc_slot);
	DEBUG_PRINT("Prepare to shutdown LC(%d) via FC(%d)", lc_slot, fc_slot);

	/* If the card to be shutdown belongs to remote FC,
	 * Send command to the master FC to handle it 
	 */
	if (lc_slot >= 0 && lc_slot <= 3) {
		return (fc_slot <= 1) ? shutdown_mgr_shutdown_linecard_local(lc_slot) :
								shutdown_mgr_shutdown_linecard_remote(lc_slot);
	}
	else /* if (lc_slot_id >= 4 && lc_slot_id <= 7) */{
		return (fc_slot <= 1) ? shutdown_mgr_shutdown_linecard_remote(lc_slot) :
								shutdown_mgr_shutdown_linecard_local(lc_slot);
	}
}

static int
shutdown_mgr_shutdown_fabriccard(int fc_slot)
{
	VALIDATE_FC_SLOT_ID(fc_slot);

	char *fc_shutdown_dir[] = {"6-0044" /* FC0/FC2 */, 
							   "6-0047" /* FC1/FC3 */};
	
	DEBUG_PRINT("Prepare to shutdown FC(%d)", fc_slot);
	return shutdown_mgr_shutdown_card(fc_shutdown_dir[fc_slot % 2]);
}

static int
shutdown_mgr_thermal_above_warning(int tid, int temperature, int warning, int fc_slot)
{
	VALIDATE_THERMAL_ID(tid);
	VALIDATE_FC_SLOT_ID(fc_slot);

	int   slot = 0;
	char *card = NULL;
	char *cpu  = NULL;
	char *thermal_name[] = {"CPU core", "MAC", "LM75-A", "LM75-B", "LM75-C", "LM75-D"};

	if (shutdown_mgr_is_fabriccard_thermal(tid)) {
		slot = fc_slot;
		card = "FC";

		if ((FC1_CPUA_TEMP1_INPUT <= tid && tid <= FC1_CPUA_TEMP6_INPUT) ||
			(FC2_CPUA_TEMP1_INPUT <= tid && tid <= FC2_CPUA_TEMP6_INPUT)) {
			cpu = (slot <= 1) ? "A" : "B";
		}
		else { /* FC1_CPUB_TEMP1_INPUT ... FC1_CPUB_TEMP6_INPUT 
			      FC2_CPUB_TEMP1_INPUT ... FC2_CPUB_TEMP6_INPUT*/
			cpu = (slot <= 1) ? "B" : "A";
		}
	}
	else /* if (shutdown_mgr_is_linecard_thermal() */ {
		slot = shutdown_mgr_thermal_id_to_slot_id(tid);
		card = "LC";
		cpu  = (fc_slot <= 1) ? "A" : "B";
	}

	SYS_LOG_WARNING("The temperature of Sensor(%d) is above warning!! (%s slot-%d, CPU-%s, Sensor-%s, T=%d, Warning threshold=%d)\n",
				    tid, card, slot, cpu, thermal_name[(tid-1) % ARRAY_SIZE(thermal_name)], temperature, warning);
	return 0;
}

struct shutdown_mgr_thermal_data {
	int slot; 							/* FC slot id */
	int t_temp[NUM_OF_THERMAL+1];		/* The temperature of each thermal sensor */
	int t_shutdown[NUM_OF_THERMAL+1];	/* The shutdown threshold of each thermal sensor */
	int t_warning[NUM_OF_THERMAL+1];	/* The warning threshold of each thermal sensor */
	int t_absent[NUM_OF_THERMAL+1];		/* thermal absent data, index 0 is not used */
	int t_error[NUM_OF_THERMAL+1]; 		/* number of consecutive io error of the thermal, index 0 is not used */
	//int counter;						/* The number of times shutdown_mgr had been called */
};

static int
shutdown_mgr_get_slot_id(struct shutdown_mgr_thermal_data *d)
{
	char *file = "/sys/bus/i2c/devices/4-0060/card_slot_id";

    VALIDATE_ERROR(shutdown_mgr_file_read_int(file, &d->slot));
	//DEBUG_PRINT("FC Slot ID = (%d)", d->slot);

	return 0;
}

static int
shutdown_mgr_get_thermal_status(struct shutdown_mgr_thermal_data *d)
{
	int i;

	for (i = 1; i < ARRAY_SIZE(d->t_temp); i++) {
		int ret;

		ret = shutdown_mgr_temperature_get(d->slot, i, &d->t_temp[i], &d->t_warning[i], &d->t_shutdown[i]);
		if (ret == -ENXIO) {
			d->t_absent[i] = 1;
			continue;
		}

		if (ret < 0) {
			SYS_LOG_WARNING("Failed to get temperature from thermal(%d)", i);
			d->t_error[i]++;
			continue;
		}

		//DEBUG_PRINT("Thermal(%d), temp=(%d), warning =(%d), shutdown=(%d)", 
		//			i, d->t_temp[i], d->t_warning[i], d->t_shutdown[i]);
		d->t_error[i]	 = 0;
	}

	return 0;
}

static int
shutdown_mgr_handle_shutdown(struct shutdown_mgr_thermal_data *d)
{
	int i;
	int lc_overheat[NUM_OF_LC] = {0};	/* LC overheat data */
	int fc_overheat[NUM_OF_FC] = {0}; 	/* FC overheat data */

	/* Collect all overheat cards */
	for (i = 1; i < ARRAY_SIZE(d->t_temp); i++) {
		if (d->t_temp[i] <= d->t_shutdown[i]) {
			continue;
		}

		if (shutdown_mgr_is_linecard_thermal(i)) {
			SYS_LOG_EMERG("LC(%d) Thermal(%d) overheat!! temp=(%d), shutdown threshold=(%d)", shutdown_mgr_thermal_id_to_slot_id(i), i, d->t_temp[i], d->t_shutdown[i]);
			lc_overheat[shutdown_mgr_thermal_id_to_slot_id(i)] = 1;
		}
		else /* if (shutdown_mgr_is_fabriccard_thermal(i))*/ {
			SYS_LOG_EMERG("FC(%d) Thermal(%d) overheat!! temp=(%d), shutdown threshold=(%d)", shutdown_mgr_thermal_id_to_slot_id(i), i, d->t_temp[i], d->t_shutdown[i]);
			fc_overheat[shutdown_mgr_thermal_id_to_slot_id(i)] = 1;
		}
	}


	/* Shutdown Line card */
	for (i = 0; i < ARRAY_SIZE(lc_overheat); i++) {
		if (!lc_overheat[i]) {
			continue;
		}

		SYS_LOG_EMERG("Shutdown LC(%d), Reason: overheat", i);
		shutdown_mgr_shutdown_linecard(d->slot, i);
	}


	/* Shutdown neighborhood Fabric card */
	for (i = 0; i < ARRAY_SIZE(fc_overheat); i++) {
		int fc_slot_target;

		if (i == (d->slot % 2)) {
			/* Skip self FC here, will handle it later */
			continue;
		}

		if (!fc_overheat[i]) {
			continue;
		}

		fc_slot_target = (d->slot > 1) ? (i+2) : (i);
		SYS_LOG_EMERG("Shutdown slave FC(%d), Reason: overheat", fc_slot_target);
		shutdown_mgr_shutdown_fabriccard(fc_slot_target);
	}


	/* Shutdown FC itself */
	for (i = 0; i < ARRAY_SIZE(fc_overheat); i++) {
		int fc_slot_target;

		if (i != (d->slot % 2)) {
			/* Only handle self FC here */
			continue;
		}

		if (!fc_overheat[i]) {
			continue;
		}

		fc_slot_target = (d->slot > 1) ? (i+2) : (i);
		SYS_LOG_EMERG("Shutdown master FC(%d), Reason: overheat", fc_slot_target);
		shutdown_mgr_shutdown_fabriccard(fc_slot_target);
	}

	return 0;
}

static int
shutdown_mgr_handle_warning(struct shutdown_mgr_thermal_data *d)
{
	int i;

	for (i = 1; i < ARRAY_SIZE(d->t_temp); i++) {
		if (d->t_temp[i] <= d->t_warning[i]) {
			continue;
		}

		shutdown_mgr_thermal_above_warning(i, d->t_temp[i], d->t_warning[i], d->slot);
	}

	return 0;
}

static int
shutdown_mgr_handle_thermal_absent(struct shutdown_mgr_thermal_data *d)
{
	return 0;
}

static int
shutdown_mgr_reset_fabriccard_mac(int fc_slot, enum MAC_ID mac)
{
	VALIDATE_FC_SLOT_ID(fc_slot);

	char  path[MAX_PATH_LEN]= {0};
	char *mac_id[] 			= {"a", "b"};
	char *mac_reset_dir[] 	= {"6-0060",	/* FC0/FC2 */
							   "6-0061"}; 	/* FC1/FC3 */

    sprintf(path, "/sys/bus/i2c/devices/%s/reset_mac_%s", mac_reset_dir[fc_slot % NUM_OF_FC], mac_id[mac]);
    VALIDATE_ERROR(shutdown_mgr_file_write_integer(path, RESET_VALUE));

	return 0;
}

static int
shutdown_mgr_reset_fabriccard_cpu(int fc_slot, enum CPU_ID cpu)
{
	VALIDATE_FC_SLOT_ID(fc_slot);

	char  path[MAX_PATH_LEN]= {0};
	char *cpu_id[] 			= {"a", "b"};
	char *cpu_reset_dir[] 	= {"6-0060",	/* FC0/FC2 */
							   "6-0061"}; 	/* FC1/FC3 */

    sprintf(path, "/sys/bus/i2c/devices/%s/reset_cpu_%s", cpu_reset_dir[fc_slot % NUM_OF_FC], cpu_id[cpu]);
    VALIDATE_ERROR(shutdown_mgr_file_write_integer(path, RESET_VALUE));

	return 0;
}

static int
shutdown_mgr_handle_thermal_io_error(struct shutdown_mgr_thermal_data *d)
{
	int i;
	int shutdown_others = 0, shutdown_itself = 0;

	for (i = 1; i < ARRAY_SIZE(d->t_error); i++) {
		if (!d->t_error[i]) {
			continue;
		}
		
		if (d->t_error[i] < SD_MGR_IOERR_RETRY_COUNT) {
			continue;
		}

		if (d->t_error[i] == SD_MGR_IOERR_RETRY_COUNT) {
			shutdown_others = 1;
		}
		else if ((d->t_error[i] * SD_MGR_UPDATE_INTERVAL) >= SD_MGR_IOERR_SHUTDOWN_THRESHOLD) {
			shutdown_itself = 1;
		}
	}

	if (shutdown_others) {
		/* Shutdown Line card */
		for (i = 0; i < NUM_OF_LC; i++) {
			DEBUG_PRINT("Shutdown LC(%d), Reason: fan board IO error", i);
			shutdown_mgr_shutdown_linecard(d->slot, i);
		}

		/* Shutdown neighborhood Fabric card */
		for (i = 0; i < NUM_OF_FC; i++) {
			int fc_slot_target;

			if (i == (d->slot % 2)) {
				/* Skip self FC here, will handle it later */
				continue;
			}

			fc_slot_target = (d->slot > 1) ? (i+2) : (i);
			SYS_LOG_EMERG("Shutdown slave FC(%d), Reason: fan board IO error", fc_slot_target);
			shutdown_mgr_shutdown_fabriccard(fc_slot_target);
		}

		/* Reset the MAC of self FC */
		for (i = 0; i < NUM_OF_MAC; i++) {
			SYS_LOG_EMERG("Reset MAC(%d) of master FC(%d), Reason: fan board IO error", i, d->slot);
			shutdown_mgr_reset_fabriccard_mac(d->slot, i);
		}

		/* Reset the CPU of self FC */
		SYS_LOG_EMERG("Reset CPU-B of master FC(%d), Reason: fan board IO error", d->slot);
		shutdown_mgr_reset_fabriccard_cpu(d->slot, CPU_B);
	}

	/* Shutdown FC itself */
	if (shutdown_itself) {
		SYS_LOG_EMERG("Shutdown master FC(%d), Reason: fan board IO error", d->slot);
		shutdown_mgr_shutdown_fabriccard(d->slot);
	}

	return 0;
}

int
shutdown_mgr(void)
{
	static int init = 0;
	static int card_type;
	static int driver_absent = 0;
	static struct shutdown_mgr_thermal_data data;

	if (driver_absent) {
		return 0;
	}

	if (!init) {
		init = 1;
		memset(&data, 0, sizeof(data));

		/* Check if driver is present */
	    if (shutdown_mgr_file_read_int("/sys/bus/i2c/devices/4-0060/card_type", 
									   &card_type) < 0) {
			driver_absent = 1;
			SYS_LOG_INFO("Shutdown manager is disabled");
	        return -EIO;
	    }
	}

	/* Update CPU/MAC temperature to CPLD on both LC/FC */
	//VALIDATE_ERROR(shutdown_mgr_update_cpld_temperatue());

	if (CARD_TYPE_LC == card_type) {
		return 0;	/* Shutdown manager only available on CARD_TYPE_FC */
	}

	/* Step 0: Get fc slot id first */
	VALIDATE_ERROR(shutdown_mgr_get_slot_id(&data));
	VALIDATE_ERROR(shutdown_mgr_get_thermal_status(&data));
	VALIDATE_ERROR(shutdown_mgr_handle_warning(&data));
	VALIDATE_ERROR(shutdown_mgr_handle_shutdown(&data));
	VALIDATE_ERROR(shutdown_mgr_handle_thermal_io_error(&data));
	VALIDATE_ERROR(shutdown_mgr_handle_thermal_absent(&data));

	return 0;
}

