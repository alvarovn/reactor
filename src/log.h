/*
    This file is part of reactor.
    
    Copyright (C) 2011  Álvaro Villalba Navarro <vn.alvaro@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Reactor uses syslog with four levels of loging:
 *  - LOG_DEBUG: Verbose output about the state of the execution. Only if
 *      DEBUG is defined.
 *  - LOG_NOTICE: Normal operational messages. May be harvested for reporting, 
 *      measuring throughput, etc.
 *  - LOG_WARNING: Not an error, but indication that something would go wrong 
 *      and may be needs action to be taken.
 *  - LOG_ERR: Failures, something already went wrong.
 */

#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#include "global.h"

#ifdef DEBUG
#define DEBUG_TEST 1

#else /* !DEBUG */
#define DEBUG_TEST 0

#endif


#define FACILITY LOG_DAEMON
#define MAX_LENGTH 256

#ifdef __GNUC__
#define PRINTF_ATTR(n, m) __attribute__ ((format (printf, n, m)))
   
#else /* !__GNUC__ */
#define PRINTF_ATTR(n, m)

#endif /* !__GNUC__ */

void l_notice(const char *format, ...) PRINTF_ATTR(1, 2);
void l_notice_e(const char *format, ...) PRINTF_ATTR(1, 2);
void l_warning(const char *format, ...) PRINTF_ATTR(1, 2);
void l_warning_e(const char *format, ...) PRINTF_ATTR(1, 2);
void l_error(const char *format, ...) PRINTF_ATTR(1, 2);
void l_error_e(const char *format, ...) PRINTF_ATTR(1, 2);
void die(const char *format, ...) PRINTF_ATTR(1, 2);
void die_e(const char *format, ...) PRINTF_ATTR(1, 2);
void close_log(void);

#ifdef DEBUG_TEST

void l_debug(const char *format, ...) PRINTF_ATTR(1, 2);
void l_debug_e(const char *format, ...) PRINTF_ATTR(1, 2);

#define debug(format, ...) l_debug("%s:%d:%s(): " format, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__)
#define debug_e(format, ...) l_debug_e("%s:%d:%s(): " format, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__)

#else/* !DEBUG */
#define debug(format, ...) (void)(0)
#define debug_e(format, ...) (void)(0)

#endif/* !DEBUG */

#endif