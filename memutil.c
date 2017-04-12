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
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "chassis.h"
#include "memutil.h"

void Enomem(void)
{
	msg_eprintf("out of memory");
	exit(ENOMEM);
}

void *xalloc(size_t size)
{
	void *m = calloc(1, size);
	if (!m)
		Enomem();
	return m;
}

void *xalloc_nonzero(size_t size)
{
	void *m = malloc(size);
	if (!m)
		Enomem();
	return m;
}

void *xrealloc(void *old, size_t size)
{
	void *m = realloc(old, size);
	if (!m)
		Enomem();
	return m;
}

char *xstrdup(char *str)
{
	str = strdup(str);
	if (!str)
		Enomem();
	return str;
}

/* Override weak glibc version */
int asprintf(char **strp, const char *fmt, ...)
{
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vasprintf(strp, fmt, ap);
	va_end(ap);
	if (n < 0) 
		Enomem();
	return n;
}

