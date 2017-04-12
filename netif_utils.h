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

#ifndef NETIF_UTILS_H
#define NETIF_UTILS_H

/* An inet socket for everyone to use for ifreqs. */
int netif_utils_netsock_init(void);

int netif_utils_get_hwaddr(char *ifname, unsigned char *hwaddr);

int netif_utils_ethtool_get_speed_duplex(char *ifname, int *speed, int *duplex);

/********* Sysfs based utility functions *************/
int netif_utils_is_bridge(char *if_name);

int netif_utils_is_bridge_slave(char *br_name, char *if_name);

int netif_utils_get_bridge_portno(char *if_name);

#endif
