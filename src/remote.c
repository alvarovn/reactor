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
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "reactor.h"

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
    if (getaddrinfo(NULL, REACTOR_PORT_STR, &hints, &result) != 0){
        dbg_e("Internet socket creation failed", NULL);
        sfd = -1;
        goto end;
    }
    optval = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if ((sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
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
    int psfd, s;
    struct addrinfo hints, *result, *rp;
    char portstr[PORT_DIGITS + 1];
    
    sprintf(portstr, "%u", port);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;
    if ((s = getaddrinfo(host, portstr, &hints, &result)) != 0){
        dbg_e("Internet socket creation failed: %s", gai_strerror(s));
        psfd = -1;
        goto end;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if ((psfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
            continue;
        if (connect(psfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;
        close(psfd);
    }
    if (rp == NULL){
        dbg_e("Could not connect socket to %s:%s", host, portstr);
        close(psfd);
        psfd = -1;
        goto end;
    }
end:
    freeaddrinfo(result);

    return psfd; 
}

RSList* receive_remote_events(int psfd){
    RSList *eids;
    struct r_msg *rmsg;
    struct sockaddr addr;
    int addrlen = sizeof(struct sockaddr);
    char ip[INET6_ADDRSTRLEN];
    
    ip[0] = NULL;
    if(getpeername(psfd, &addr, &addrlen) == -1){
        dbg_e("Unable to retrieve the address from a remote reactord sending events", NULL);
    }
    else inet_ntop(addr.sa_family, &addr, ip, INET6_ADDRSTRLEN);
    rmsg = receive_cntrl_msg(psfd);
    while(rmsg != NULL && rmsg->hd.mtype != EVENT){
        eids = reactor_slist_prepend(eids, (void *) rmsg->msg);
        rmsg = receive_cntrl_msg(psfd);
    }
    if(rmsg == NULL || rmsg->hd.mtype != EOM){
        dbg_e("Unable to receive the remote events from %s", ip);
        while(eids != NULL){
            free(eids->data);
            eids = reactor_slist_delete_link(eids, eids);
        }
        goto end;
    }
end:
    return eids;
}

int send_remote_events(int psfd, const RSList *eids){
    int error = 0;
    RSList *eidsp;
    struct r_msg rmsg;
    
    rmsg.hd.mtype = EVENT;
    for(eidsp = eids; eidsp != NULL; eidsp = reactor_slist_next(eidsp)){
        rmsg.msg = eidsp->data;
        rmsg.hd.size = strlen(eidsp->data);
        if(send_cntrl_msg(psfd, &rmsg) != 0){
            error = -1;
            goto end;
        }
    }
    rmsg.hd.mtype = EOM;
    rmsg.msg = "";
    rmsg.hd.size = 0;
    if(send_cntrl_msg(psfd, &rmsg) != 0){
        error = -1;
        goto end;
     }
end:
    return error;
}
