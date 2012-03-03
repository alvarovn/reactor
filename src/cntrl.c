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
#include <sys/uio.h>
#include <sys/socket.h>

#include "reactor.h"
#include "reactord.h"

/* TODO Socket must be changed to a secure path */

#define SOCK_PATH "/tmp/rcrtlsock"

typedef struct _cntrl{
    struct sockaddr_un saddr;
    int sfd;
    ssize_t numbytes;
    socklen_t addrlen;
    bool listening;
}Cntrl;

/* AddTransMsg serialization:
 * string action; NULL terminated
 * string enid_1[?]; NULL terminated
 * string to; NULL terminated
 * string from; NULL if initial trans.
 */
static AddTransMsg* cntrl_deserialize_atm(char *msg){
    CntrlHeader *mh;
    AddTransMsg *atm = NULL;
    char *action;
    char **enids;
    
    mh = (CntrlHeader *) msg;
        
    if((atm = (AddTransMsg *) calloc(1, sizeof(AddTransMsg))) == NULL){
       goto malloc_error;
    }
    
    atm->action = msg;
    msg =+ strlen(msg);
    atm->enids = msg;
    for(;msg != NULL; msg =+ strlen(msg)){}
    atm->to = msg;
    msg =+strlen(msg);
    atm->from = msg;
    
    return atm;
    
malloc_error:
    dbg_e("Error on malloc AddTransMsg", NULL);
    atm = NULL;
    return atm;
}
/* ReactorEventMsg serialization:
 * string eid; NULL terminated
 */
static ReactorEventMsg* cntrl_deserialize_rem(char *msg){
    ReactorEventMsg *rem;
    /* TODO Set uid and pid */
    rem->eid = msg;
    return rem;
}

CntrlMsg* cntrl_get_msg(Cntrl *cntrl){
    int clsockfd;
    CntrlMsg * cm = NULL;
    CntrlHeader ch;
    char *msg;
    
    if(cntrl == NULL){
        dbg("Trying to get a message from a NULL cntrl", NULL);
        goto end;
    }
    if(!cntrl->listening){
        dbg_e("Trying to get a message from a not listening cntrl", NULL);
        goto end;
    }
    clsockfd = accept(cntrl->sfd, NULL, NULL);
    read(cntrl->sfd, (char *) &ch, sizeof(CntrlHeader));
    
    /* This is pretty insecure since it will malloc any size sent 
     */
    if((msg = (char *) calloc(1, ch.size)) == NULL){
        dbg_e("Error on malloc() the new message", NULL);
        close(clsockfd);
        goto end;
    }
    if((msg = (char *) calloc(1, ch.size)) == NULL){
        dbg_e("Error on malloc() the new message", NULL);
        close(clsockfd);
        free(msg);
        goto end;
    }
    read(cntrl->sfd, msg, ch.size);
    /* TODO check credentials */
    switch(ch.cmt){
        case REACTOR_EVENT:
            cm->cmt = REACTOR_EVENT;
            cm->cm = (CntrlMsg *) cntrl_deserialize_rem(msg);
            break;
        case ADD_TRANSITION:
            cm->cmt = ADD_TRANSITION;
            cm->cm = (CntrlMsg *) cntrl_deserialize_atm(msg);
            break;
    }
    close(clsockfd);
end:
    return cm;    
}

void cntrl_free(Cntrl *cntrl){
    free(cntrl);
}

Cntrl* cntrl_new(){
    Cntrl *cntrl;
    int sfd;

    if((cntrl = (Cntrl *) calloc(1, sizeof(Cntrl))) == NULL) {
        dbg_e("Error on malloc() the Cntrl struct.", NULL);
        goto error;
    }
    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    
    if(sfd == -1){
        dbg_e("Error creating control socket file descriptor.", NULL);
        goto error;
    }
    
    cntrl->listening = false;
    cntrl->sfd = sfd;
    cntrl->saddr.sun_family = AF_UNIX;
    strncpy(cntrl->saddr.sun_path, SOCK_PATH, sizeof(cntrl->saddr.sun_path)-1);
    
    unlink(cntrl->saddr.sun_path);
    if(bind(sfd, (struct sockaddr *) &cntrl->saddr, sizeof(struct sockaddr_un)) == -1){
        dbg_e("Control socket can't be bound. Probably a permissions issue.", NULL);
        goto error;
    }

    return cntrl;
error:
    cntrl_free(cntrl);
    return NULL;
}

int cntrl_listen(Cntrl* cntrl){
    int err = 0;
    int sopt = 1;
    if(cntrl == NULL){
        dbg("NULL cntrl to make listen.", NULL);
        goto error;
    }
    if(listen(cntrl->sfd, 0) < 0){
        dbg_e("Control socket listening failed.", NULL);
        goto error;
    }
//     setsockopt(cntrl->sfd, SOL_SOCKET, SO_PASSCRED, &sopt, sizeof(sopt));
    cntrl->listening = true;
    return 0;
    
error:
    return -1;
}

int cntrl_get_fd(Cntrl *cntrl){
    if(cntrl == NULL){
        return -1;
    }
    return cntrl->sfd;
}

void cntrl_atm_free(AddTransMsg *atm){
    free(atm->action);
    free(atm->enids);
    free(atm->from);
    free(atm->to);
    free(atm);
}

void cntrl_rem_free(ReactorEventMsg *rem){
    free(rem->eid);
    free(rem);
}
