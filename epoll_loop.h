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

#ifndef EPOLL_LOOP_H
#define EPOLL_LOOP_H

#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

struct epoll_event_handler {
	int fd;
	void *arg;
	void (*handler) (uint32_t events, struct epoll_event_handler * p);
	struct epoll_event *ref_ev;	/* if set, epoll loop has reference to this,
					   so mark that ref as NULL while freeing */
};

int epoll_loop_init(void);

void epoll_loop_clear_epoll(void);

int epoll_loop_main_loop(void);

int epoll_loop_add_epoll(struct epoll_event_handler *h);

int epoll_loop_remove_epoll(struct epoll_event_handler *h);

#endif
