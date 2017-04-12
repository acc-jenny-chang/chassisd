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


#include "epoll_loop.h"

#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "chassis.h"
#include "chassis_bpdu.h"
#include "shutdown_mgr.h"

extern int chassis_check_master_fc(CTPM_T* this);

int epoll_loop_time_diff(struct timeval *second, struct timeval *first);
void epoll_loop_run_timeouts(void);
void epoll_loop_run_thermal_plan(void);
    
// globals
int epoll_fd = -1;
struct timeval nexttimeout;

int poll_count =0;

int epoll_loop_init(void)
{
	int r = epoll_create(128);
	if (r < 0) {
		fprintf(stderr, "epoll_create failed: %m\n");
		return -1;
	}
	epoll_fd = r;
	return 0;
}

int epoll_loop_add_epoll(struct epoll_event_handler *h)
{
    int r;
	struct epoll_event ev = {
		.events = EPOLLIN,
		.data.ptr = h,
	};
	h->ref_ev = NULL;
	r = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, h->fd, &ev);
	if (r < 0) {
		fprintf(stderr, "epoll_ctl_add: %m\n");
		return -1;
	}

	return 0;
}

int epoll_loop_remove_epoll(struct epoll_event_handler *h)
{
	int r = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, h->fd, NULL);
	if (r < 0) {
		fprintf(stderr, "epoll_ctl_del: %m\n");
		return -1;
	}
	if (h->ref_ev && h->ref_ev->data.ptr == h) {
		h->ref_ev->data.ptr = NULL;
		h->ref_ev = NULL;
	}
	return 0;
}

void epoll_loop_clear_epoll(void)
{
	if (epoll_fd >= 0)
		close(epoll_fd);
}

int epoll_loop_time_diff(struct timeval *second, struct timeval *first)
{
	return (second->tv_sec - first->tv_sec) * 1000
	    + (second->tv_usec - first->tv_usec) / 1000;
}

void epoll_loop_run_thermal_plan(void)
{
#if (ONLP>0)
    int i=0;
    CTPM_T* this = &chassis_information.ctpm;
    int master=0;
    
    /*Check chassis daemon is enbale*/
    if (CTP_ENABLED != this->admin_state) return;

    /*check this card is Master fabric card*/
    master= chassis_check_master_fc(this);

    if(master==0) return;
    
    poll_count++;
    if(poll_count>=DEFAULT_MAXAGE)
    {
        poll_count =0;
        shutdown_mgr();
    }
#endif
}

void epoll_loop_run_timeouts(void)
{
    	chassis_bpdu_one_second();
       /* Run Thermal Plan*/
#if(THERMAL_PLAN>0)       
       epoll_loop_run_thermal_plan();
#endif
	//nexttimeout.tv_sec=nexttimeout.tv_sec+2;
       nexttimeout.tv_usec=nexttimeout.tv_usec+DEFAULT_INTERVAL_TIME;
    
}

int epoll_loop_main_loop(void)
{
    #define EV_SIZE 8
	struct epoll_event ev[EV_SIZE];

	gettimeofday(&nexttimeout, NULL);
	nexttimeout.tv_sec++;


	while (1) {
		int r, i;
		int timeout;

		struct timeval tv;
		gettimeofday(&tv, NULL);
		timeout = epoll_loop_time_diff(&nexttimeout, &tv);
		if (timeout < 0) {
			epoll_loop_run_timeouts();
			timeout = 0;
		}
		r = epoll_wait(epoll_fd, ev, EV_SIZE, timeout);
		if (r < 0 && errno != EINTR) {
			fprintf(stderr, "epoll_wait: %m\n");
			return -1;
		}
		for (i = 0; i < r; i++) {
			struct epoll_event_handler *p = ev[i].data.ptr;
			if (p != NULL)
				p->ref_ev = &ev[i];
		}
		for (i = 0; i < r; i++) {
			struct epoll_event_handler *p = ev[i].data.ptr;
			if (p && p->handler)
				p->handler(ev[i].events, p);
		}
		for (i = 0; i < r; i++) {
			struct epoll_event_handler *p = ev[i].data.ptr;
			if (p != NULL)
				p->ref_ev = NULL;
		}
	}

	return 0;
}
