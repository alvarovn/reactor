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
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "reactord.h"

typedef struct _cntrl{
    struct sockaddr_un saddr;
    int sfd;
    int psfd;
    bool listening;
    bool server;
    bool connected;
}Cntrl;

int cntrl_send_msg(Cntrl *cntrl, const struct r_msg *msg){
    int error;
    struct r_msg *response = NULL;
    struct rmsg_hd hd;
    
    error = 0;
    if(cntrl == NULL){
        error = -1;
        dbg("Trying to send a message to a 'NULL' cntrl", NULL);
        goto end;
    }
    hd.size = htonl((uint32_t)msg->hd.size);
    hd.mtype = htonl((uint32_t)msg->hd.mtype);
    write(cntrl->psfd, (const void *) &hd, sizeof(struct rmsg_hd));
    switch(msg->hd.mtype){
        case RULE:
        case REACTOR_EVENT:
           write(cntrl->psfd, msg->msg, msg->hd.size);
           response = cntrl_receive_msg(cntrl);
           error = (int) response->hd.mtype;
           free(response->msg);
           free(response);
           break;
    }
    
end:
    return error;
}

struct r_msg* cntrl_receive_msg(Cntrl *cntrl){
    int *sfd, readcount;
    struct r_msg * rmsg = NULL;
    
    readcount = 0;
    if(cntrl == NULL){
        dbg("Trying to receive a message from a 'NULL' cntrl", NULL);
        goto end;
    }
    if(cntrl->server){
        if(!cntrl->connected) cntrl->psfd = accept(cntrl->sfd, NULL, NULL);
        cntrl->connected = true;
    }

    

    if((rmsg = (struct r_msg *) calloc(1, sizeof(struct r_msg))) == NULL){
        goto malloc_error;
    }
    
    rmsg->hd.size = 0;
    read(cntrl->psfd, (char *) &rmsg->hd, sizeof(struct rmsg_hd));
    rmsg->hd.mtype = ntohl((u_int32_t) rmsg->hd.mtype);
    rmsg->hd.size = ntohl((u_int32_t) rmsg->hd.size);

    /* TODO: This is pretty insecure since it will malloc any size sent. Fix it.
    */
    if((rmsg->msg = (char *) calloc(1, rmsg->hd.size)) == NULL){
        free(rmsg);
        goto malloc_error;
    }

    if((readcount = read(cntrl->psfd, rmsg->msg, rmsg->hd.size)) < rmsg->hd.size){
        dbg_e("Error reading from the socket", NULL);
        free(rmsg->msg);
        free(rmsg);
        rmsg = NULL;
        goto end;
    }
    /* TODO check credentials */
    
    /* Call deserializers if needed */
end:
    return rmsg;
malloc_error:
    dbg_e("Error on malloc() the new message", NULL);
    return NULL;

}

void cntrl_peer_close(Cntrl *cntrl){
    cntrl->connected = false;
    close(cntrl->psfd);
}

void cntrl_free(Cntrl *cntrl){
    cntrl_peer_close(cntrl);
    close(cntrl->sfd);
    free(cntrl);
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

int cntrl_connect(Cntrl *cntrl){
    int error = 0;
    if(cntrl->server){
        dbg("Trying to connect to cntrl server from the cntrl server", NULL);
        error = -1;
        goto end;
    }
    if( connect(    cntrl->psfd, 
                    (struct sockaddr *) &cntrl->saddr, 
                    sizeof(struct sockaddr_un)) == -1){
                       
        dbg_e("Failed to connect to the control socket", NULL);
        error = -1;
        goto end;
    }
end:
    return error;
}

int cntrl_listen(Cntrl* cntrl){
    int error = 0;
    const int BACKLOG = 5;
//     int sopt = 1;
    if(cntrl == NULL){
        dbg("NULL cntrl to make listen.", NULL);
        error = -1;
        goto end;
    }
    /* TODO Backlog is 5 as a random number, change it to make sense */
    if(listen(cntrl->sfd, BACKLOG) == -1){
        dbg_e("Control socket listening failed.", NULL);
        error = -1;
        goto end;
    }
//     setsockopt(cntrl->sfd, SOL_SOCKET, SO_PASSCRED, &sopt, sizeof(sopt));
    cntrl->listening = true;
end:
    return error;
}

int cntrl_get_fd(Cntrl *cntrl){
    if(cntrl == NULL){
        return -1;
    }
    return cntrl->sfd;
}