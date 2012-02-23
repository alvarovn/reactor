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

#include "reactor.h"
#include "reactord.h"

typedef struct _reactorevent{
    char *msg;
    int uid;
    /* In the future this field may contain more information about the font 
     * than the pid.
     */ 
    pid_t fontpid;  
} ReactorEvent;

GHashTable *eventnotices;
GHashTable *states;

// TODO signal handlers

// static char* get_states_representation(int rep){
//     
// }

static Transition* add_init_trans(char *action, char **enids, char *to, bool init){
    State *stt;
    Transition *trans;
    EventNotice *en;
    
    stt = (State *) reactor_hash_table_lookup(states, to);
    if(stt == NULL) stt = state_new(to);
    
    trans = trans_new(stt);
    state_add_transpointer(stt);
    /* TODO     While we don't get from the event the shell to execute 
     *          the command, we should get the current shell and use it.
     */
    trans_set_cmd_action(trans, action, "/bin/sh", 0);
    
    for(; *enids != NULL; enids++){
        en = (EventNotice *) reactor_hash_table_lookup(eventnotices, *enids);
        if(en == NULL){
            en = en_new(*enids);
            reactor_hash_table_insert(eventnotices, *enids, en);
        }
        trans_add_requisite(trans, en);
        en_add_transpointer(en);
        /* As it is an initial transition it should also be a current transition */
        if(init) en_add_curr_trans(en, trans);
    }
    if(init) info("New transition to state '%s'.", to);
    return trans;
}

static Transition* add_trans(char *action, char **enids, char *to, char *from){
    Transition *trans = NULL;
    State *stt;
    
    stt = reactor_hash_table_lookup(states, from);
    if(stt == NULL){
        warn("Origin state '%s' must exist and it doesn't. The transition won't be added.", from);
           goto error;
    }
    
    trans = add_init_trans(action, enids, to, false);
    state_add_trans(stt, trans);
error:
    info("New transition from state '%s' to state '%s'.", from, to);
    return trans;
}

static void notice_reactor_event(const ReactorEvent* revent){
    EventNotice *en;
    const RSList *currtrans;
    RSList *ftrans = NULL;
    
    en = (EventNotice *) reactor_hash_table_lookup(eventnotices, revent->msg);
    if(en == NULL) {
        info("Unknown event '%s' happened", revent->msg);
        return;
    }

    currtrans = en_get_currtrans(en);
    
    for (RSList *rsl = currtrans; rsl != NULL; rsl = reactor_slist_next(rsl)){
        
        trans_notice_event((Transition *) rsl->data);
        
        for(RSList *rsl2 = state_get_trans( trans_get_dest((Transition *) rsl->data) );
            rsl2 != NULL;
            rsl2 = reactor_slist_next(rsl2)){
                reactor_slist_prepend(ftrans, rsl2->data);
        }
    }
    en_clear_curr_trans(en);
    
    /* Insert into the events the current valid transitions */
    
    for (RSList *rsl = ftrans; rsl != NULL; rsl = reactor_slist_next(rsl)){
        
        for(RSList *rsl2 = trans_get_enrequisites(rsl->data);
            rsl2 != NULL;
            reactor_slist_next(rsl2)){
                en_add_curr_trans(rsl2->data, rsl->data);
            }
    }
    
}

// static int main_loop(){
//     
// }

int main(int argc, char *argv[]) {
    
    pid_t pid, sid;
    
    info("Starting the reactor...");
    dbg("...freeing Mars.", NULL);
    // TODO root user check
    // TODO change ruid to euid (root)
    // TODO help
    // TODO version
    // TODO daemon lock file (?)
    // TODO trap signals
    
    /* daemonize */
    pid = fork();
//     switch(pid){
//         case 0:
//             /* child */
//             dbg("Forked child running.", NULL);
//             break;
//         case -1:
//             err("Unable to daemonize");
//             fprintf(stderr, "Unable to daemonize\n");
//         default:
//             /* parent */
//             goto exit;
//     }
    
    /* redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
    
    // TODO cancel child signals ?
    
    sid = setsid();
//     if (sid < 0) {
//         dbg_e("setsid() returned with errors.", NULL);
//         err("Unable to create a new session.");
//         goto exit;
//     }
//     
    /* change current directory */
    if ((chdir("/")) < 0) {
        err("Unable to change current directory to /");
        goto exit;
    }

    // TODO open an event-listener socket
    // TODO enqueue events
    // TODO load users state machines
    // TODO open a control socket
    
    eventnotices = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    states = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    
     char *ens[] = { "event1",
                     "event2",
                     "event3",
                     "event4",
                     NULL
                   };
    add_init_trans("touch /home/lostevil/toma_ya", ens, "A", true);
    ReactorEvent event1 = {"event1", 0, 1};
    ReactorEvent event2 = {"event2", 0, 1};
    ReactorEvent event3 = {"event3", 0, 1};
    ReactorEvent event4 = {"event4", 0, 1};
    ReactorEvent event5 = {"event5", 0, 1};
    ReactorEvent event14 = {"event14", 0, 1};
    notice_reactor_event(&event1);
    notice_reactor_event(&event14);
    notice_reactor_event(&event2);
    notice_reactor_event(&event3);
    notice_reactor_event(&event4);
    notice_reactor_event(&event5);
    
exit:
    return 0;
}
