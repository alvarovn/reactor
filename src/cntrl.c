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

#include "reactor.h"
#include "reactord.h"

typedef struct _cntrl{
    struct sockaddr_un saddr;
    int sfd;
    int psfd;
    bool listening;
    bool server;
    bool connected;
}Cntrl;



/* AddTransMsg serialization:
 * string action;
 * int enidscount
 * string enids[?];
 * string to; 
 * string from; NULL if initial trans.
 */
static AddTransMsg* cntrl_deserialize_atm(char *msg){
    AddTransMsg *atm = NULL;
    int i, enidscount;
            
    if((atm = (AddTransMsg *) calloc(1, sizeof(AddTransMsg))) == NULL){
       goto malloc_error;
    }
    
    atm->action = strdup(msg);
    msg += strlen(msg)+1;
    enidscount = (int) *msg;
    msg += sizeof(int);
    if((atm->enids = (char **) calloc(enidscount+1, sizeof(char*))) == NULL){
        cntrl_atm_free(atm);
        atm = NULL;
        goto malloc_error;
    }
    for(i=0; i<enidscount; i++){
        atm->enids[i] = strdup(msg);
        msg += strlen(msg)+1;
    }
    atm->enids[enidscount] = NULL;
    atm->to =strdup(msg);
    msg += strlen(msg)+1;
    atm->from = strdup(msg);
    return atm;
    
malloc_error:
    dbg_e("Error on malloc AddTransMsg", NULL);
    return atm;
}
/* ReactorEventMsg serialization:
 * string eid;
 */
static ReactorEventMsg* cntrl_deserialize_rem(char *msg){
    ReactorEventMsg *rem;
    if((rem = (ReactorEventMsg *) calloc(1, sizeof(ReactorEventMsg))) == NULL){
       goto malloc_error;
    }
    /* TODO Set uid and pid */
    rem->eid = strdup(msg);
    return rem;
malloc_error:
    dbg_e("Error on malloc ReactorEventMsg", NULL);
    return rem;
}

static char* cntrl_serialize_rem(const CntrlMsg *msg){
    ReactorEventMsg *rem;
    char *sermsg;
    CntrlHeader *header;
    int size;
    
    rem = (ReactorEventMsg *) msg->cm;
    size = strlen(rem->eid)+1;
    if((sermsg = calloc(1, size + sizeof(header))) == NULL){
        dbg_e("Error on malloc() the serialized message.", NULL);
        goto end;
    }
    header = (CntrlHeader *) sermsg;
    header->size = size;
    header->cmt = REACTOR_EVENT;
    strncpy(&sermsg[sizeof(header)],rem->eid, size);
    
end:
    return sermsg;
}

/* TODO Needs to be fixed, calls strlen too much. Inefficient */
static char* cntrl_serialize_atm(const CntrlMsg *msg){
    AddTransMsg *atm;
    char **ienids;
    int size, enidscount;
    char *sermsg, *ismsg;
    CntrlHeader *header;
    
    atm = (AddTransMsg *) msg->cm;
    size = sizeof(int);
    enidscount = 0;
    for(ienids = atm->enids; *ienids!=NULL; ienids++){
        size += strlen(*ienids)+1;
        enidscount++;
    }
    size += strlen(atm->action)+1;
    size += strlen(atm->to)+1;
    size += strlen(atm->from)+1;
    if((sermsg = (char *) calloc(1, size + sizeof(header))) == NULL){
        dbg_e("Error on malloc() the serialized message.", NULL);
        goto end;
    }
    header = (CntrlHeader *) sermsg;
    header->size = size;
    header->cmt = ADD_TRANSITION;
    ismsg = &sermsg[sizeof(header)];
    strcpy(ismsg, atm->action);
    ismsg += strlen(atm->action)+1;
    strncpy(ismsg, (char *)&enidscount, sizeof(int));
    ismsg += sizeof(int);
    for(ienids = atm->enids; *ienids!=NULL; ienids++){
        strcpy(ismsg, *ienids);
        ismsg += strlen(*ienids)+1;
    }
    strcpy(ismsg, atm->to);
    ismsg += strlen(atm->to)+1;
    strcpy(ismsg, atm->from);
end:
    return sermsg;
}

int cntrl_send_msg(Cntrl *cntrl, const CntrlMsg *msg){
    int error, size;
    CntrlMsg *response;
    char *sermsg;
    CntrlHeader header;
    
    error = 0;
    if(cntrl == NULL){
        dbg("Trying to send a message to a 'NULL' cntrl", NULL);
        goto end;
    }
    switch(msg->cmt){        
        case ACK:
        case AT_NOFROM:
            header.cmt = msg->cmt;
            header.size = 0;
            write(cntrl->psfd, (char *) &header, sizeof(header));
            goto end;
            break;
        case REACTOR_EVENT:
            sermsg = cntrl_serialize_rem(msg);
            break;
        case ADD_TRANSITION:
            sermsg = cntrl_serialize_atm(msg);
            break;
    }
    header = *(CntrlHeader *)sermsg;
    write(cntrl->psfd, (char *) sermsg, header.size + sizeof(header));
    response = cntrl_receive_msg(cntrl);
    error = (int) response->cmt;
    cntrl_msg_free(response);
    cntrl_msg_free(sermsg);
end:
    return error;
}

CntrlMsg* cntrl_receive_msg(Cntrl *cntrl){
    int *sfd;
    CntrlMsg * cm = NULL;
    CntrlHeader ch;
    char *msg;
    
    if(cntrl == NULL){
        dbg("Trying to receive a message from a 'NULL' cntrl", NULL);
        goto end;
    }
    ch.size =0;
    if(cntrl->server){
        if(!cntrl->connected) cntrl->psfd = accept(cntrl->sfd, NULL, NULL);
        cntrl->connected = true;
    }

    read(cntrl->psfd, (char *) &ch, sizeof(CntrlHeader));
    
    /* TODO: This is pretty insecure since it will malloc any size sent. Fix it.
    */
    if((msg = (char *) calloc(1, ch.size)) == NULL){
        goto malloc_error;
    }
    if((cm = (CntrlMsg *) calloc(1, sizeof(CntrlMsg))) == NULL){
        free(msg);
        goto malloc_error;
    }
    read(cntrl->psfd, msg, ch.size);
    /* TODO check credentials */
    cm->cmt = ch.cmt;
    /* Call deserializers if needed */
    switch(ch.cmt){
        case REACTOR_EVENT:
            cm->cm = (CntrlMsg *) cntrl_deserialize_rem(msg);
            break;
        case ADD_TRANSITION:
            cm->cm = (CntrlMsg *) cntrl_deserialize_atm(msg);
            break;
    }
    free(msg);
end:
    return cm;
malloc_error:
    dbg_e("Error on malloc() the new message", NULL);
    return cm;

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

void cntrl_atm_free(AddTransMsg *atm){
    char **i;
    free(atm->action);
    for(i=atm->enids; *i!=NULL; i++){
        free(*i);
    }
    free(atm->from);
    free(atm->to);
    free(atm);
}

void cntrl_rem_free(ReactorEventMsg *rem){
    free(rem->eid);
    free(rem);
}

void cntrl_msg_free(CntrlMsg *msg){
    switch(msg->cmt){
        case REACTOR_EVENT:
            cntrl_rem_free((ReactorEventMsg *) msg->cm);
            break;
        case ADD_TRANSITION:
            cntrl_atm_free((AddTransMsg *) msg->cm);
            break;
    }
}