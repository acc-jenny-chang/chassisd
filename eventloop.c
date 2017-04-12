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

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <signal.h>
#include "chassis.h"
#include "eventloop.h"

#define MAX_POLLFD 10

static int max_pollfd;

struct pollcb { 
	poll_cb_t cb;
	int fd;
	void *data;
};

static struct pollfd pollfds[MAX_POLLFD];
static struct pollcb pollcbs[MAX_POLLFD];	

static sigset_t event_sigs;

static int eventloop_closeonexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) { 
		msg_sys_err_printf("Cannot set FD_CLOEXEC flag on fd");
		return -1;
	}
	return 0;
}

int eventloop_register_pollcb(int fd, int events, poll_cb_t cb, void *data)
{
	int i = max_pollfd;

	if (eventloop_closeonexec(fd) < 0)
		return -1;

	if (i >= MAX_POLLFD) {
		msg_eprintf("poll table overflow");
		return -1;
	}
	max_pollfd++;

	pollfds[i].fd = fd;
	pollfds[i].events = events;
	pollcbs[i].cb = cb;
	pollcbs[i].data = data;
	return 0;
}

/* Could mark free and put into a free list */
void eventloop_unregister_pollcb(struct pollfd *pfd)
{
	int i = pfd - pollfds;
	assert(i >= 0 && i < max_pollfd);
	memmove(pollfds + i, pollfds + i + 1, 
		(max_pollfd - i - 1) * sizeof(struct pollfd));
	memmove(pollcbs + i, pollcbs + i + 1, 
		(max_pollfd - i - 1) * sizeof(struct pollcb));
	max_pollfd--;
}

static void eventloop_poll_callbacks(int n)
{
	int k;

	for (k = 0; k < max_pollfd && n > 0; k++) {
		struct pollfd *f = pollfds + k;
		if (f->revents) { 
			struct pollcb *c = pollcbs + k;
			c->cb(f, c->data);
			n--;
		}
	}
}

/* Run signal handler only directly after event loop */
int eventloop_event_signal(int sig)
{
	static int first = 1;
	sigset_t mask;
	if (first && sigprocmask(SIG_BLOCK, NULL, &event_sigs) < 0) 
		return -1;
	first = 0;
	if (sigprocmask(SIG_BLOCK, NULL, &mask) < 0)
		return -1;
	sigaddset(&mask, sig);
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
		return -1;
	return 0;
}

/* Handle old glibc without ppoll. */
static int eventloop_ppoll_fallback(struct pollfd *pfd, nfds_t nfds, 
			  const struct timespec *ts, const sigset_t *ss)
{
	sigset_t origmask;
	int ready;
	sigprocmask(SIG_SETMASK, ss, &origmask);
	ready = poll(pfd, nfds, ts ? ts->tv_sec : -1);
	sigprocmask(SIG_SETMASK, &origmask, NULL);
	return ready;
}

static int (*ppoll_vec)(struct pollfd *, nfds_t, const struct timespec
			*, const sigset_t *);

void eventloop_main(void)
{
    int n;
	if (!ppoll_vec) 
		ppoll_vec = eventloop_ppoll_fallback;

	/*for (;;) { */
		n = ppoll_vec(pollfds, max_pollfd, NULL, &event_sigs);
		if (n <= 0) {
			if (n < 0 && errno != EINTR)
				msg_sys_err_printf("poll error");
			/*continue;*/
		}
		eventloop_poll_callbacks(n); 
	/*}*/			
}

