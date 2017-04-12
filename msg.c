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

#define _GNU_SOURCE 1
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "chassis.h"
#include "msg.h"
#include "memutil.h"


#define DEBUGLFAG2 0

int msg_open_logfile(char *fn);
void msg_lprintf(char *fmt, ...);
void msg_eprintf(char *fmt, ...);
void msg_sys_err_printf(char *fmt, ...);
int msg_wprintf(char *fmt, ...);
void msg_gprintf(char *fmt, ...);

enum syslog_opt syslog_opt = SYSLOG_REMARK;
int syslog_level = LOG_WARNING;
static FILE *output_fh;
static char *output_fn;

int msg_need_stdout(void)
{
	return !output_fh && (syslog_opt == 0);
}

int msg_open_logfile(char *fn)
{
	output_fh = fopen(fn, "a");
	if (output_fh) { 
		char *old = output_fn;
		output_fn = xstrdup(fn);
		free(old);
		return 0;
	}
	return -1;
}

static void msg_opensyslog(void)
{
	static int syslog_opened;
	if (syslog_opened)
		return;
	syslog_opened = 1;
	openlog("chassis", 0, 0);
}

/* For warning messages that should reach syslog */
void msg_lprintf(char *fmt, ...)
{
	va_list ap;
	if (syslog_opt & SYSLOG_REMARK) { 
		va_start(ap, fmt);
		msg_opensyslog();
		vsyslog(LOG_ERR, fmt, ap);
		va_end(ap);
	}
	if (output_fh || !(syslog_opt & SYSLOG_REMARK)) {
		va_start(ap, fmt);
		msg_opensyslog();
		vfprintf(output_fh ? output_fh : stdout, fmt, ap);
		va_end(ap);
	}
#if(DEBUGLFAG2)    
    		va_start(ap, fmt);
		msg_opensyslog();
		vfprintf(output_fh ? output_fh : stdout, fmt, ap);
		va_end(ap);
#endif
}

/* For errors during operation */
void msg_eprintf(char *fmt, ...)
{
	FILE *f = output_fh ? output_fh : stderr;
	va_list ap;

	if (!(syslog_opt & SYSLOG_ERROR) || output_fh) {
		va_start(ap, fmt);
		fputs("chassis: ", f);
		vfprintf(f, fmt, ap);
		if (*fmt && fmt[strlen(fmt)-1] != '\n')
			fputc('\n', f);
		va_end(ap);
	}
	if (syslog_opt & SYSLOG_ERROR) { 
		va_start(ap, fmt);
		msg_opensyslog();
		vsyslog(LOG_ERR, fmt, ap);
		va_end(ap);
	}
}

void msg_sys_err_printf(char *fmt, ...)
{
	char *err = strerror(errno);
	va_list ap;
	FILE *f = output_fh ? output_fh : stderr;

	if (!(syslog_opt & SYSLOG_ERROR) || output_fh) {
		va_start(ap, fmt);
		fputs("chassis: ", f);
		vfprintf(f, fmt, ap);
		fprintf(f, ": %s\n", err);
		va_end(ap);
	}
	if (syslog_opt & SYSLOG_ERROR) { 
		char *fmt2;
		va_start(ap, fmt);
		msg_opensyslog();
		asprintf(&fmt2, "%s: %s\n", fmt, err);
		vsyslog(LOG_ERR, fmt2, ap);
		free(fmt2);
		va_end(ap);
	}
}

/* Write to syslog with line buffering */
static int msg_vlinesyslog(char *fmt, va_list ap)
{
	static char line[200];
	int n;
	int lend = strlen(line); 
	int w = vsnprintf(line + lend, sizeof(line)-lend, fmt, ap);
	while (line[n = strcspn(line, "\n")] != 0) {
		line[n] = 0;
		syslog(syslog_level, "%s", line);
		memmove(line, line + n + 1, strlen(line + n + 1) + 1);
	}
	return w;
}

/* For decoded machine check output */
int msg_wprintf(char *fmt, ...)
{
	int n = 0;
	va_list ap;
	if (syslog_opt & SYSLOG_LOG) {
		va_start(ap,fmt);
		msg_opensyslog();
		n = msg_vlinesyslog(fmt, ap);
		va_end(ap);
	}
	if (!(syslog_opt & SYSLOG_LOG) || output_fh) {
		va_start(ap,fmt);
		n = vfprintf(output_fh ? output_fh : stdout, fmt, ap);
		va_end(ap);
	}
	return n;
}

/* For output that should reach both syslog and normal log */
void msg_gprintf(char *fmt, ...)
{
	va_list ap;
	if (syslog_opt & (SYSLOG_REMARK|SYSLOG_LOG)) {
		va_start(ap,fmt);
		msg_vlinesyslog(fmt, ap);
		va_end(ap);
	}
	if (!(syslog_opt & SYSLOG_LOG) || output_fh) { 
		va_start(ap,fmt);
		vfprintf(output_fh ? output_fh : stdout, fmt, ap);
		va_end(ap);
	}
}

void msg_flushlog(void)
{
	FILE *f = output_fh ? output_fh : stdout;
	fflush(f);
}

void msg_reopenlog(void)
{
	if (output_fn && output_fh) { 
		fclose(output_fh);
		output_fh = NULL;
		if (msg_open_logfile(output_fn) < 0) 
			msg_sys_err_printf("Cannot reopen logfile `%s'", output_fn);
	}	
}

