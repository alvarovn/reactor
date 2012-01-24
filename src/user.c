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

#include "user.h"

void free_users(User* u){
    User* uaux;
    
    if(!u)
        return;
    
    do{
        uaux = u->next;
        free(u->name);
        free(u);
    }while(uaux);
}

static User* load_user(const char* uname){
    User *u;
    
    if((u = (User *) malloc(sizeof(User))) == NULL)
        goto end;
    u->next = NULL;
    if((u->name = strdup(uname)) == NULL){
        free_users(u);
        u = NULL;
        goto end;
    }
end:
    return u;
}

User* load_users(const char* grname){
    User *u, *uaux;
    struct group *gr;
    char root;
    char **gmem;
    
    u = NULL;
    root = 0;
    if ((gr = getgrnam(grname)) == NULL)
      goto end;
    for(gmem = gr->gr_mem; *gmem != NULL; gmem++){
        if(!root && str_eq(*gmem, "root"))
            root=1;
        if((uaux = load_user(*gmem)) == NULL){
            free_users(u);
            goto end;
        }
        uaux->next = u;
        u = uaux;
    }
    if(!root){
        if((uaux = load_user("root")) == NULL){
            free_users(u);
            goto end;
        }
        uaux->next = u;
        u = uaux;
    }
end:        
    return u;
}

