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

#define RW_MAJOR_VERSION 0
#define RW_MINOR_VERSION 1

struct rwa_version{
    int major;
    int minor;
};

// typedef int (*RWExitFunc)(void);
struct rw_info;

typedef struct rw_info* (*RWInitFunc)(const struct rw_services *);
typedef void *(*RWMainFunc)(void *);

struct rw_info{
    struct rwa_version version;
    char *name;
    RWInitFunc initfunc;
    RWMainFunc mainfunc;
//     RWExitFunc exitfunc;
    struct rw_info *next;
};

// struct rw_mainparams{
//     struct rw_services *serv;
// }

// typedef int (*RWRegisterFunc)(const struct rw_info *info);
typedef int (*RWEventHandleFunc)(char *eid);
// typedef int (*RWInvokeServiceFunc)(const char *name, void *params);


struct rw_services{
    struct rwa_version version;
//     RWRegisterFunc registerworker; 
    RWEventHandleFunc eventhandler;
//     RWInvokeServiceFunc invokeservice;
};
struct rw_info* rw_init_plugin(const struct rw_services *params);