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

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <event.h>


#include "reactor.h"
#include "reactord.h"

#define SOCK_BUFF_SIZE 1024

GHashTable *eventnotices;
GHashTable *states;
Cntrl *cntrl;

// TODO signal handlers

/* control messages handlers */

static CntrlMsgType add_trans_handler(AddTransMsg *msg){
    Transition *trans, *fsminitial = NULL;
    State *from, *to = NULL;
    EventNotice *en = NULL;
    bool init;
    CntrlMsgType cmt = ACK;
    
    init = *(msg->from) == NULL || msg->from == NULL;
    
    if(!init){
        from = reactor_hash_table_lookup(states, msg->from);
        if(from == NULL){
            warn("Origin state '%s' must exist and it doesn't. The transition won't be added.", msg->from);
            cmt = AT_NOFROM;
            goto end;
        }
    }
    
    to = (State *) reactor_hash_table_lookup(states, msg->to);
    if(to == NULL) {
        to = state_new(msg->to);
    }
    else{
        if(init || state_get_fsminitial(from) != state_get_fsminitial(to)){
            /* User is trying to put multiple initial transitions to the same state machine */
            /* TODO Make a copy of the portion of the state machine that they will share */ 
            warn("Trying to set multiple initial transitions to the same state machine. The transition won't be added");
            cmt = AT_MULTINIT;
            goto end;
        }
    }
      
    reactor_hash_table_insert(states, state_get_id(to), to);
    
    trans = trans_new(to);
    state_add_transpointer(to);
    if(init) state_set_fsminitial(to, trans);
    else state_set_fsminitial(to, state_get_fsminitial(from));
    /* TODO     While we don't get from the event the shell to execute 
     *          the command, we should get the current shell and use it.
     */
    trans_set_cmd_action(trans, msg->action, "/bin/sh", 0);
    
    for(; *msg->enids != NULL; msg->enids++){
        en = (EventNotice *) reactor_hash_table_lookup(eventnotices, *msg->enids);
        if(en == NULL){
            en = en_new(*msg->enids);
            reactor_hash_table_insert(eventnotices, *msg->enids, en);
        }
        trans_add_requisite(trans, en);
        en_add_transpointer(en);
        /* As it is an initial transition it should also be a current transition */
        if(init)en_add_curr_trans(en, trans);

    }
    if(init) info("New initial transition to state '%s'.", msg->to);
    else{
        state_add_trans(from, trans);
        info("New transition from state '%s' to state '%s'.", msg->from, msg->to);
    }
end:
    return cmt;
}

static int reactor_event_handler(const ReactorEventMsg *msg){
    int error;
    EventNotice *en;
    RSList  **currtransref = NULL, 
            *currtrans = NULL, 
            *nexttrans = NULL, 
            *nextens = NULL;
    Transition  *trans = NULL, 
                *transinit = NULL, 
                *transindex = NULL;
    
    error = 0;
    en = (EventNotice *) reactor_hash_table_lookup(eventnotices, msg->eid);
    if(en == NULL) {
        info("Unknown event '%s' happened.", msg->eid);
        error = -1;
        return error;
    }
    
    for (currtransref = en_get_currtrans_ref(en); 
         *currtransref != NULL; 
         /* clear currtrans from the current state */
         trans_clist_clear_curr_trans(trans)){
        
            currtrans = *currtransref;
            trans = (Transition *) currtrans->data;
            if(trans_notice_event(trans)){
                /* forward on the state machine */
                transinit = state_get_trans(trans_get_dest(trans));
                if(transinit != NULL){
                transindex = transinit;
                    do{
                        nexttrans = reactor_slist_prepend(nexttrans, (void *)transindex);
                        transindex = trans_clist_next(transindex);
                    }while(transinit != transindex);
                }
            }
    }
    
    /* Insert into eventnotices the new current valid transitions */
    
    for (; nexttrans != NULL; nexttrans = reactor_slist_next(nexttrans)){
        for(nextens = trans_get_enrequisites((Transition *)nexttrans->data);
            nextens != NULL;
            nextens = reactor_slist_next(nextens)){
                en_add_curr_trans((EventNotice *)nextens->data, (Transition *)nexttrans->data);
            }
    }
end:
    return error;
}

/* libevent control socket callback */

static void receive_cntrl_msg(int fd, short ev, void *arg){
    CntrlMsg response;
    CntrlMsg *msg;
    response.cmt = ACK;
    response.cm = NULL;
    cntrl_listen(cntrl);
    if((msg = cntrl_receive_msg(cntrl)) == NULL){
        err("Error in the communication with 'reactorctl'");
        goto end;
    }
    switch(msg->cmt){
        case REACTOR_EVENT:
            reactor_event_handler((ReactorEventMsg *) msg->cm);
            cntrl_send_msg(cntrl, &response);
            cntrl_rem_free(msg->cm);
            break;
        case ADD_TRANSITION:
            response.cmt = add_trans_handler((AddTransMsg *) msg->cm);
            cntrl_send_msg(cntrl, &response);
            cntrl_atm_free(msg->cm);
            break;
    }
end:
    cntrl_peer_close(cntrl);
    free(msg);
}

int main(int argc, char *argv[]) {
    int error;
    struct event ev;
    pid_t pid, sid;
    
    error = 0;
    info("Starting the reactor...");
    dbg("...freeing Mars.", NULL);
    // TODO root user check
    // TODO change ruid to euid (root)
    // TODO help
    // TODO version
    // TODO daemon lock file (?)
    // TODO trap signals
    
    /* daemonize */
#ifndef DEBUG
    pid = fork();
    switch(pid){
        case 0:
            /* child */
            dbg("Forked child running.", NULL);
            break;
        case -1:
            err("Unable to daemonize");
            fprintf(stderr, "Unable to daemonize\n");
        default:
            /* parent */
            goto exit;
    }
#endif
    /* redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
    
    // TODO cancel child signals ?
    
//     sid = setsid();
//     if (sid < 0) {
//         error = 1;
//         dbg_e("setsid() returned with errors.", NULL);
//         err("Unable to create a new session.");
//         goto exit;
//     }
    
    /* change current directory */
    if ((chdir("/")) < 0) {
        error = 1;
        err("Unable to change current directory to '/'.");
        goto exit;
    }

    // TODO open an event-listener socket
    // TODO enqueue events
    // TODO load users state machines
    // TODO open a control socket
    
    eventnotices = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    states = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    
    /* sockets setup and poll */
    event_init();
    if((cntrl = cntrl_new(true)) == NULL){
        err("Unable to create the control socket and bind it to '%s'. Probably a permissions issue.", SOCK_PATH);
        goto exit;
    }
    if(cntrl_listen(cntrl) == -1) {
        goto exit;
    }
    event_set(&ev, cntrl_get_fd(cntrl), EV_READ | EV_PERSIST, &receive_cntrl_msg, NULL);
    event_add(&ev, NULL);
    event_dispatch();

    
exit:
    return error;
}

