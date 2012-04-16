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
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>

#include "reactor.h"
#include <netdb.h>

int listen_remote(){
    int sfd,
        optval;
    /* TODO Backlog is 5 as a random number, change it to make sense */
    const int BACKLOG = 5;
    struct addrinfo hints, 
                    *result, 
                    *rp;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    if (getaddrinfo(NULL, REACTOR_PORT, &hints, &result) != 0){
        dbg_e("Internet socket creation failed", NULL);
        sfd = -1;
        goto end;
    }
    optval = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
            dbg_e("setsockopt() failed", NULL);
            sfd = -1;
            goto end;
        }
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(sfd);
    }
    if (rp == NULL){
        dbg_e("Could not bind socket to any address", NULL);
        close(sfd);
        sfd = -1;
        goto end;
    }
    if(listen(sfd, BACKLOG) == -1){
        dbg_e("Control socket listening failed", NULL);
        close(sfd);
        sfd = -1;
        goto end;
    }
end:
    freeaddrinfo(result);
    return sfd;
}

int connect_remote(char *host, int port){
    int psfd;
    struct addrinfo hints, result, rp;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;
    if (getaddrinfo(host, port, &hints, &result) != 0){
        dbg_e("Internet socket creation failed", NULL);
        psfd = -1;
        goto end;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        psfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (psfd == -1)
            continue;
        if (connect(psfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;
        close(psfd);
    }
    if (rp == NULL){
        dbg_e("Could not connect socket to the address", NULL);
        close(psfd);
        psfd = -1;
        goto end;
    }
end:
    freeaddrinfo(result);

    return psfd; 
}
