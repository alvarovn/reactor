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

#include <stdbool.h>

#define RW_MAJOR_VERSION 0
#define RW_MINOR_VERSION 1

#ifndef RCTLPLUGIN
#define RCTLPLUGIN

struct rp_version{
    int major;
    int minor;
};
struct rp_info;
struct rp_services;
typedef struct rp_info* (*RPInitFunc)(const struct rp_services *);
typedef void *(*RPMainFunc)(void *);

struct rp_info{
    struct rp_version version;
    char *name;
    RPInitFunc initfunc;
    RPMainFunc mainfunc;
    struct rp_info *next;
};

typedef int (*RPEventHandleFunc)(char *eid);

// TODO Function to get the rules files

struct rp_services{
    struct rp_version version;
    RPEventHandleFunc eventhandler;
};


struct rp_info* rp_init_plugin(const struct rp_services *params);

#endif