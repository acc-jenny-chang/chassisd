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

#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <stdint.h>
#include "tcp_server.h"

int udp_client_initial_socket(void);
int udp_client_close_socket(void);
int udp_client_send(char*ipaddr,char* sendline,int size);
int udp_client_select(int sockfd);
int udp_client_recv(int sockfd,char* recvline, int time);
int udp_client_close(int sockfd);

#endif
