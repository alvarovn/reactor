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
#include <stdlib.h>
#include <string.h>

#include "rctrplugin.h"

// int worker_exit(){
//     return 0;
// }

void* scheduler(void *params){
    struct rp_services *serv;
    
    if(params == NULL){
        // We can't log, because the callbacks should be on params.
        return NULL;
    }
    serv = (struct rp_services *) params;
    serv->eventhandler("e1");
}

__attribute__ ((visibility("default"))) struct rp_info* rp_init_plugin(const struct rp_services *params){
    int res = 0;
    struct rp_info *info;
    if((info = (struct rp_info *)calloc(1, sizeof(struct rp_info))) == NULL){
        // TODO Log error
        return NULL;
    }
    info->version.major = 0;
    info->version.minor = 1;
    info->name = strdup("Files");
    info->mainfunc = scheduler;
    info->next = NULL;
    
    if (res < 0)
        return NULL;
    return info;
}