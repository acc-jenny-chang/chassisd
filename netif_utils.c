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

#include "netif_utils.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>

#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#include "log.h"

int netsock = -1;

int netif_utils_netsock_init(void)
{
	LOG("");
	netsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (netsock < 0) {
		ERROR("Couldn't open inet socket for ioctls: %m\n");
		return -1;
	}
	return 0;
}

int netif_utils_get_hwaddr(char *ifname, unsigned char *hwaddr)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(netsock, SIOCGIFHWADDR, &ifr) < 0) {
		ERROR("%s: get hw address failed: %m", ifname);
		return -1;
	}
	memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	return 0;
}

int netif_utils_ethtool_get_speed_duplex(char *ifname, int *speed, int *duplex)
{
	struct ifreq ifr;
    	struct ethtool_cmd ecmd;
        
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);


	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (caddr_t) & ecmd;
	if (ioctl(netsock, SIOCETHTOOL, &ifr) < 0) {
		ERROR("Cannot get link status for %s: %m\n", ifname);
		return -1;
	}
	*speed = ecmd.speed;	/* Ethtool speed is in Mbps */
	*duplex = ecmd.duplex;	/* We have same convention as ethtool.
				   0 = half, 1 = full */
	return 0;
}

/********* Sysfs based utility functions *************/

/* This sysfs stuff might break with interface renames */
int netif_utils_is_bridge(char *if_name)
{
	char path[32 + IFNAMSIZ];
	sprintf(path, "/sys/class/net/%s/bridge", if_name);
	return (access(path, R_OK) == 0);
}

/* This will work even less if the bridge port is renamed after being
   joined to the bridge.
*/
int netif_utils_is_bridge_slave(char *br_name, char *if_name)
{
	char path[32 + 2 * IFNAMSIZ];
	sprintf(path, "/sys/class/net/%s/brif/%s", br_name, if_name);
	return (access(path, R_OK) == 0);
}

int netif_utils_get_bridge_portno(char *if_name)
{
	char path[32 + IFNAMSIZ];
	char buf[128];
	int fd;
	long res = -1;
    	int l;
     	char *end;
        
    	sprintf(path, "/sys/class/net/%s/brport/port_no", if_name);

	TSTM((fd = open(path, O_RDONLY)) >= 0, -1, "%m");

	TSTM((l = read(fd, buf, sizeof(buf) - 1)) >= 0, -1, "%m");
	if (l == 0) {
		ERROR("Empty port index file");
		goto out;
	} else if (l == sizeof(buf) - 1) {
		ERROR("port_index file too long");
		goto out;
	}
	buf[l] = 0;
	if (buf[l - 1] == '\n')
		buf[l - 1] = 0;

	res = strtoul(buf, &end, 0);
	if (*end != 0 || res > INT_MAX) {
		ERROR("Invalid port index %s", buf);
		res = -1;
	}
      out:
	close(fd);
	return res;
}
