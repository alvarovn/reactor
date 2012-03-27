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
#include <sys/socket.h>
#include <stdlib.h>

#include "libreactor.h"
#include "libreactor-private.h"

int cntrl_listen(Cntrl* cntrl){
    int error = 0;
    const int BACKLOG = 5;
    if(cntrl == NULL){
        dbg("NULL cntrl to make listen", NULL);
        error = -1;
        goto end;
    }
    /* TODO Backlog is 5 as a random number, change it to make sense */
    if(listen(cntrl->sfd, BACKLOG) == -1){
        dbg_e("Control socket listening failed", NULL);
        error = -1;
        goto end;
    }
    cntrl->listening = true;
end:
    return error;
}

Cntrl* cntrl_new(bool server){
    Cntrl *cntrl;
    int *sfd;
    
    if((cntrl = (Cntrl *) calloc(1, sizeof(Cntrl))) == NULL) {
        dbg_e("Error on malloc() the Cntrl struct.", NULL);
        goto error;
    }
    if(server) sfd = &cntrl->sfd;
    else sfd = &cntrl->psfd;
    
    *sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if(*sfd  == -1){
        dbg_e("Error creating control socket file descriptor.", NULL);
        goto error;
    }
    
    cntrl->listening = false;
    cntrl->server = false;
    cntrl->connected = false;
    cntrl->saddr.sun_family = AF_UNIX;
    strncpy(cntrl->saddr.sun_path, SOCK_PATH, sizeof(cntrl->saddr.sun_path)-1);
    
    if(server){
        cntrl->server = true;
        unlink(cntrl->saddr.sun_path);
        if(bind(*sfd, (struct sockaddr *) &cntrl->saddr, sizeof(struct sockaddr_un)) == -1){
            dbg_e("Control socket can't be bound. Probably a permissions issue.", NULL);
            goto error;
        }
    }
    return cntrl;
error:
    cntrl_free(cntrl);
    return NULL;
}