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

#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#include "libreactor.h"
#include "libreactor-private.h"

R_EXPORT int listen_cntrl(){
    struct sockaddr_un saddr;
    int sfd;
    /* TODO Backlog is 5 as a random number, change it to make sense */
    const int BACKLOG = 5;
    
    if((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        dbg_e("Error creating control socket file descriptor", NULL);
        sfd = -1;
        goto end;
    }
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, SOCK_PATH, sizeof(saddr.sun_path)-1);
    unlink(saddr.sun_path);
    if(bind(sfd, (struct sockaddr *) &saddr, sizeof(saddr)) == -1){
        dbg_e("Control socket with '%s' can't be bound. Probably a permissions issue", SOCK_PATH);
        sfd = -1;
        goto end;
    }
    if(chmod(SOCK_PATH, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1){
        dbg_e("Permissions on the '%s' control socket can't be changed", SOCK_PATH);
        close_cntrl(sfd);
        sfd = -1;
        goto end;
    }
    if(listen(sfd, BACKLOG) == -1){
        dbg_e("Control socket listening failed", NULL);
        sfd = -1;
        close_cntrl(sfd);
        goto end;
    }
end:
    return sfd;
}

R_EXPORT int connect_cntrl(){
    struct sockaddr_un saddr;
    int psfd;
    
    if((psfd = socket(AF_UNIX, SOCK_STREAM, 0))  == -1){
        dbg_e("Error creating control socket file descriptor", NULL);
        psfd = -1;
        goto end;
    }
    saddr.sun_family = AF_UNIX;
    strncpy(saddr.sun_path, SOCK_PATH, sizeof(saddr.sun_path)-1);
    if(connect(psfd, (struct sockaddr *) &saddr, sizeof(saddr)) == -1){
        dbg_e("Failed to connect to the control socket", NULL);
        psfd = -1;
        goto end;
    }
end:
    return psfd;
}
static int socket_write(int psfd, void *data, int size){
    int zerocount = 0,
        sent = 0,
        error = 0;
    void *tosend = data;
    while((sent = reactor_write(psfd, tosend, size)) != size){
        if(sent == 0) zerocount++;
        else zerocount = 0;
        
        if(zerocount > MAX_ZERO_WRITES || sent == -1){
            dbg_e("Error writing to the socket", NULL);
            error = -1;
            goto end;
        }
        tosend += sent;
        if((size -= sent) == 0)
            break;
    }
end:
    return error;
}
R_EXPORT int send_cntrl_msg(int psfd, const struct r_msg *msg){
    int error = 0;
    struct r_msg *response = NULL;
    struct rmsg_hd hd;

    hd.size = htonl((uint32_t)msg->hd.size);
    hd.mtype = htonl((uint32_t)msg->hd.mtype);
    
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) 
        dbg_e("signal() failed", NULL);

    if(socket_write(psfd, (void *) &hd, sizeof(struct rmsg_hd)) != 0){
        error = -1;
        goto end;
    }

    switch(msg->hd.mtype){
        case ADD_RULE:
        case EVENT:
        case RM_TRANS:
            if(socket_write(psfd, msg->msg, msg->hd.size) != 0){
                error = -1;
                goto end;
            }
            response = receive_cntrl_msg(psfd);
            if(response == NULL){
                error = -1;
                goto end;
            }
            error = (int) response->hd.mtype;
            free(response->msg);
            free(response);
            break;
    }
    if(signal(SIGPIPE, SIG_DFL) == SIG_ERR) 
        dbg_e("signal() failed", NULL);

end:
    return error;
}

R_EXPORT struct r_msg* receive_cntrl_msg(int psfd){
    int *sfd;
    int count = 0;
    struct r_msg * rmsg = NULL;

    if((rmsg = (struct r_msg *) calloc(1, sizeof(struct r_msg))) == NULL){
        goto malloc_error;
    }
    rmsg->hd.size = 0;
    count = reactor_read(psfd, (char *) &rmsg->hd, sizeof(struct rmsg_hd));
    if(count != sizeof(struct rmsg_hd)){
        dbg_e("Error reading from the socket", NULL);
        free(rmsg->msg);
        free(rmsg);
        rmsg = NULL;
        goto end;
    }
    rmsg->hd.mtype = ntohl((u_int32_t) rmsg->hd.mtype);
    rmsg->hd.size = ntohl((u_int32_t) rmsg->hd.size);
    
    if(rmsg->hd.size > SSIZE_MAX){
        free(rmsg);
        goto malloc_error;
    }
    if((rmsg->msg = (char *) calloc(1, rmsg->hd.size)) == NULL){
        free(rmsg);
        goto malloc_error;
    }

    if(reactor_read(psfd, rmsg->msg, rmsg->hd.size) != rmsg->hd.size){
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

R_EXPORT void close_cntrl(int sfd){
    close(sfd);
    unlink(SOCK_PATH);
}