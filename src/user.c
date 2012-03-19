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

#include <string.h>
#include <stdlib.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#include "reactord.h"

void free_users(struct r_user* u){
    struct r_user* uaux;
    
    if(!u) return;
    
    do{
        uaux = u->next;
        free(u);
    }while(uaux);
}

static struct r_user* load_user(const char* uname){
    struct r_user *u = NULL;
    
    if((u = (struct r_user *) calloc(1, sizeof(struct r_user))) == NULL){
        dbg_e("Error on malloc() the user '%s'", uname);
        goto end;
    }
   u->pw = getpwnam(uname);
end:
    return u;
}

struct r_user* load_users(const char* grname){
    struct r_user *u, *uaux;
    struct group *gr;
    struct passwd *cu;
    char **gmem;
    
    u = NULL;
    cu = getpwuid(geteuid());
    if ((gr = getgrnam(grname)) == NULL){
        warn("'%s' group doesn't exist.", grname);
        goto end;
    }
    for(gmem = gr->gr_mem; *gmem != NULL; gmem++){
        if((uaux = load_user(*gmem)) == NULL){
            err("Users loading failure.");
            free_users(u);
            goto end;
        }
        uaux->next = u;
        u = uaux;
    }
end:
    return u;
}

