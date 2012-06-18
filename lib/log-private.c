/*
    This file is part of reactor.

    Copyright (C) 2011  Álvaro Villalba Navarro <vn.alvaro@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
  TODO  If DEBUG show the same extra info dbg does for all the logging 
        functions.
  TODO  Remove dbg_e function and make dbg show errno if it's not 'Success'
*/

#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include "libreactor-private.h"

static bool logopen = false;
static char msg[MAX_LENGTH];
static char *toolong = "... (too long)";

static void open_log(){
    if (!logopen){
        openlog("reactord", LOG_PID, FACILITY);
        logopen = true;
    }
}

void close_log(){
    if (logopen) closelog();
    logopen = false;
}

static void build_msg(const char *format, va_list args){
    int mlength;
    
    mlength = vsnprintf(msg, MAX_LENGTH, format, args);
    if (mlength >= MAX_LENGTH - 1){
        strcpy(msg + MAX_LENGTH - sizeof(toolong), toolong);
    }
}

static void rlog(int priority, const char *format, va_list args){
    build_msg(format, args);
    open_log();
    syslog(priority, "%s", msg);
}

static void rlog_e(int priority, const char *format, va_list args){
    int saved_errno;

    saved_errno = errno;
    build_msg(format, args);
    open_log();
    syslog(priority, "%s: %s", msg, strerror(saved_errno));
}

R_EXPORT void info(const char *format, ...){
    va_list args;

    va_start(args, format);
    rlog(LOG_NOTICE, format, args);
    va_end(args);
}

R_EXPORT void warn(const char *format, ...){
    va_list args;

    va_start(args, format);
    rlog(LOG_WARNING, format, args);
    va_end(args);
}

R_EXPORT void err(const char *format, ...){
    va_list args;

    va_start(args, format);
    rlog(LOG_ERR, format, args);
    va_end(args);
}

R_EXPORT void die(const char *format, ...){
    va_list args;

    va_start(args, format);
    rlog(LOG_ERR, format, args);
    va_end(args);
    err("Aborted");

    exit(EXIT_FAILURE);
}

#ifdef DEBUG

R_EXPORT void l_debug(const char *format, ...){
    va_list args;

    va_start(args, format);
    rlog(LOG_DEBUG, format, args);
    va_end(args);
}

R_EXPORT void l_debug_e(const char *format, ...){
    va_list args;

    va_start(args, format);
    rlog_e(LOG_DEBUG, format, args);
    va_end(args);
}

#endif  /* DEBUG */
