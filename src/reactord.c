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
#include <pwd.h>
#include <stdlib.h>

#include "reactor.h"

#define SOCK_BUFF_SIZE 1024

struct reactor_d reactor;

// TODO signal handlers

/* control messages handlers */

static enum rmsg_type reactor_add_rule_handler(struct r_rule *rule){
    Transition *trans, *fsminitial = NULL;
    State *from, *to = NULL;
    EventNotice *en = NULL;
    bool init;
    enum rmsg_type cmt = ACK;
    
    if(rule == NULL){
        cmt = ARG_MALFORMED;
        warn("Rule malformed.");
        goto end;
    }
        
    init = (from = (State *) reactor_hash_table_lookup(reactor.states, rule->from)) == NULL;
    if(init){
        from = state_new(&reactor, rule->from);
//         state_set_fsminitial(from, from);
    }
    
    to = (State *) reactor_hash_table_lookup(reactor.states, rule->to);
    if(to == NULL) {
        to = state_new(&reactor, rule->to);
        fsminitial = (state_get_fsminitial(from) == NULL) ? from : state_get_fsminitial(from);
        state_set_fsminitial(to, fsminitial);
    }
    else{
        if( (state_get_fsminitial(from) != state_get_fsminitial(to)) && 
            (state_get_fsminitial(from) != to)){
            /* TODO Make a copy of the portion of the state machine that they will share */ 
            warn("Trying to set multiple initial transitions to the same state machine. The transition won't be added.");
            cmt = RULE_MULTINIT;
            goto end;
        }
    }
    trans = trans_new(to);
    /* TODO     While we don't get from the event the shell to execute 
     *          the command, we should get the current shell and use it.
     */
    trans_set_action(trans, rule->raction);

    for(; rule->enids != NULL; rule->enids = reactor_slist_next(rule->enids)){
        en = (EventNotice *) reactor_hash_table_lookup(reactor.eventnotices, rule->enids->data);
        if(en == NULL){
            en = en_new(&reactor, rule->enids->data);
        }
        trans_add_requisite(trans, en);
        /* As it is an initial transition it should also be a current transition */
        if(init) en_add_curr_trans(en, trans);
    }
    if(init){
        info("New transition from initial state '%s' to state '%s'.", rule->from, rule->to);
    }
    else{
        info("New transition from state '%s' to state '%s'.", rule->from, rule->to);
    }
    state_add_trans(from, trans);
end:
    return cmt;
}

/* TODO User r_event */
static int reactor_event_handler(const char *msg){
    int error;
    EventNotice *en;
    RSList  *currtrans = NULL,
            **currtransref = NULL, 
            *nexttrans = NULL, 
            *nextens = NULL;
    Transition  *trans = NULL, 
                *transinit = NULL, 
                *transindex = NULL;
    
    error = 0;
    en = (EventNotice *) reactor_hash_table_lookup(reactor.eventnotices, msg);
    if(en == NULL) {
        info("Unknown event '%s' happened.", msg);
        error = -1;
        return error;
    }
    currtransref = en_get_currtrans_ref(en);
    currtrans = *currtransref;
    while (currtrans != NULL){
    
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
                /* clear all currtrans from the same state */
                trans_clist_clear_curr_trans(trans);
                currtrans = *currtransref;
            }
            else currtrans = reactor_slist_next(currtrans);
    }
    /* In case no action was executed, so no currtrans was cleared, we clear the currtrans of the las event */
    en_clear_curr_trans(en);
    
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

static enum rmsg_type reactor_rm_trans_handler(char *msg){
    int msgln = 0, 
        msgp = 0, 
        transnum,
        i = 0;
    char *transnumend;
    Transition  *trans;
    State *state;
    enum rmsg_type rmt = ACK;
    /* TODO Not the most efficient way to find the last '.' if any. Fix it. */
    msgln = strlen(msg);
    for(msgp = msgln-1; (msg[msgp] != '.') && (msgp > 0); msgp--);
    if (msgp <= 0 || msgp >= msgln-1){
        goto malformed;
    }
    transnum = (int) strtol(&msg[msgp+1], &transnumend, 10);
    if( *transnumend != NULL || transnum <= 0 ){
        goto malformed;
    }
    msg[msgp] = '\0';
    state = reactor_hash_table_lookup(reactor.states, (void *) msg);
    state_ref(state);
    if(state == NULL){
        rmt = NO_TRANS;
        goto end;
    }
    msg[msgp] = '.';
    info("Removing transition %s...", msg);
    trans = state_get_trans(state);
    for (i = 1; i < transnum; i++){
        trans = trans_clist_next(trans);
    }
    state_set_trans(state, trans_clist_free(&reactor, trans));
    state_unref(&reactor, state);
    info("Transition %s removed", msg);
end:
    return rmt;
malformed:
    return ARG_MALFORMED;
}

/* libevent control socket callback */

static void attend_cntrl_msg(int sfd, short ev, void *arg){
    int psfd;
    struct r_msg response;
    struct r_msg *msg;
    void *data;
    response.hd.mtype = ACK;
    response.hd.size = 0;
    response.msg = NULL;
    psfd = accept(sfd, NULL, NULL);
    if((msg = receive_cntrl_msg(psfd)) == NULL){
        err("Error in the communication with 'reactorctl'");
        goto end;
    }
    switch(msg->hd.mtype){
        case EVENT:
            reactor_event_handler(msg->msg);
            send_cntrl_msg(psfd, &response);
            free(msg->msg);
            break;
        case ADD_RULE:
            /* TODO Change uid to the user who sent the rule */
            data = (void *) rule_parse(msg->msg, 0);
            response.hd.mtype = reactor_add_rule_handler((struct r_rule *) data);
            rules_free((struct r_rule *) data);
            send_cntrl_msg(psfd, &response);
            free(msg->msg);
            break;
        case RM_TRANS:
            response.hd.mtype = reactor_rm_trans_handler(msg->msg);
            send_cntrl_msg(psfd, &response);
            free(msg->msg);
        default:
            break;
    }
end:
    close(psfd);
    
    free(msg);
}

/* TODO We need a way to distinguish states with the same name but from different users */
/* TODO Main admin rules file */
static void init_rules(){
    struct r_user *users;
    struct r_rule *rules;
    char filename[PATH_MAX];
    unsigned int fnln;
    
    for(users = load_users(R_GRP); users != NULL; users = users->next){
        strcpy(filename, users->pw->pw_dir);
        fnln = strlen(users->pw->pw_dir);
        strcpy(&filename[fnln], "/.");
        fnln += strlen("/.");
        strcpy(&filename[fnln], RULES_FILE);
        for(rules = parse_rules_file(filename, users->pw->pw_uid);
            rules != NULL;
            rules = rules->next){
                reactor_add_rule_handler(rules);
        }
        rules_free(rules);
    }
    
}

int main(int argc, char *argv[]) {
    int error, cntrlsfd, remotesfd;
    struct event cntrlev, remoteev;
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
    // TODO open a control socket
    
    reactor.eventnotices = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    reactor.states = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    init_rules();
    /* sockets setup and poll */
    event_init();
    if((cntrlsfd = listen_cntrl()) == -1){
        err("Unable to create the control socket and bind it to '%s'. Probably a permissions issue.", SOCK_PATH);
        goto exit;
    }
    event_set(&cntrlev, cntrlsfd, EV_READ | EV_PERSIST, &attend_cntrl_msg, NULL);
    event_add(&cntrlev, NULL);
    event_set(&remoteev, remotesfd, EV_READ | EV_PERSIST, &attend_cntrl_msg, NULL);
    event_add(&remoteev, NULL);
    event_dispatch();

    
exit:
    close_cntrl(cntrlsfd);
    close(remotesfd);
    return error;
}

