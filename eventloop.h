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

#include <poll.h>

typedef void (*poll_cb_t)(struct pollfd *pfd, void *data);

int eventloop_register_pollcb(int fd, int events, poll_cb_t cb, void *data);
void eventloop_unregister_pollcb(struct pollfd *pfd);
void eventloop_main(void);
int eventloop_event_signal(int sig);

