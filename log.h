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

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_RSTPLIB 4
#define LOG_LEVEL_MAX 100

#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO

extern void chassis_dprintf(int level, const char *fmt, ...);
extern void chassis_vdprintf(int level, const char *fmt, va_list ap);
extern int log_level;

#define PRINT(_level, _fmt, _args...) chassis_dprintf(_level, _fmt, ##_args)

#define TSTM(x,y, _fmt, _args...)						\
	do if (!(x)) { 								\
			PRINT(LOG_LEVEL_ERROR,					\
				"Error in %s at %s:%d verifying %s. " _fmt,	\
				__PRETTY_FUNCTION__, __FILE__, __LINE__, 	\
				#x, ##_args);					\
			return y;						\
       } while (0)

#define TST(x,y) TSTM(x,y,"")

#define LOG(_fmt, _args...) \
	PRINT(LOG_LEVEL_DEBUG, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#define INFO(_fmt, _args...) \
	PRINT(LOG_LEVEL_INFO, "%s: " _fmt, __PRETTY_FUNCTION__, ##_args)

#define ERROR(_fmt, _args...) \
	PRINT(LOG_LEVEL_ERROR, "error, %s: " _fmt, __PRETTY_FUNCTION__, ##_args)

static inline void dump_hex(void *b, int l)
{
	unsigned char *buf = b;
	char logbuf[80];
	int i, j;
	for (i = 0; i < l; i += 16) {
		for (j = 0; j < 16 && i + j < l; j++)
			sprintf(logbuf + j * 3, " %02x", buf[i + j]);
		PRINT(LOG_LEVEL_INFO, "%s", logbuf);
	}
	PRINT(LOG_LEVEL_INFO, "\n");
}

#endif
