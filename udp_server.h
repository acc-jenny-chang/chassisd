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

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <stdint.h>
#include "tcp_server.h"
#include "chassis.h"

int upd_server_init(void);
int upd_server_recev(int fd);
int upd_server_close(int fd);


struct udp_packet_command_header{
    unsigned int us_card_number;
    uint16_t type;
    uint16_t length;
    char params[256];
    uint16_t params_length;
}__attribute__((packed));

#endif
