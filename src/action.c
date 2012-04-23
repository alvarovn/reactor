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
#include <unistd.h>
#include <pthread.h>

#include "reactor.h"

struct r_action* action_new(enum a_types atype){
    struct r_action *raction;
    if( (raction =  calloc(1, sizeof(struct r_action))) == NULL ){
        goto malloc_error;
    }
    raction->atype = atype;
    switch(atype){
        case CMD:
            if( (raction->action = calloc(1, sizeof(struct cmd_action))) == NULL ){
                free(raction);
                goto malloc_error;
            }
            break;
        case PROP:
            if( (raction->action = calloc(1, sizeof(struct prop_action))) == NULL ){
                free(raction);
                goto malloc_error;
            }
            break;
        default:
            break;
    }
    return raction;
malloc_error:
    err("Error malloc() the new action");
    return NULL;
}

static void prop_action_free(struct prop_action *pa){
    reactor_slist_free_full(pa->enids, free);
    free(pa->addr);
    free(pa);
}

static void cmd_action_free(struct cmd_action *cmdactn){
    free(cmdactn->cmd);
    free(cmdactn->shell);
    free(cmdactn);
}

void action_free(struct r_action *raction){
    if(raction == NULL) return;
    switch(raction->atype){
        case CMD:
            cmd_action_free((struct cmd_action *) raction->action);
            break;
        case PROP:
            prop_action_free((struct prop_action *) raction->action);
            break;
    }
}

static void cmd_execute(struct cmd_action *cmd){
    // TODO Monitor the execution
    // TODO Catch execution output
    char *shell;
    shell =     cmd->shell == NULL ? 
                    "/bin/sh" : 
                    cmd->shell; 
    switch (fork()) {
        case -1:
            err("Unable to fork, command won't be executed.");
            break;
        case 0:
            /* child process */
            if(setuid(cmd->uid) < 0){
                err("Commands can't be executed as user with uid '%i'.", cmd->uid);
            }
            execl(shell, shell, "-c", cmd->cmd, NULL);
            
            /* If everything goes fine, it will never arrive here */
            err("Unable to execute shell command '%s'", cmd->cmd);
            break;
        default:
            /* parent process */
            break;
    }
}

static void prop_execute_thread(void *arg){
    int psfd;
    struct prop_action *prop = (struct prop_action*) arg;
    
    if((psfd = connect_remote(prop->addr, prop->port)) == -1){
        warn("Unable to reach '%s:%u' remote reactord", prop->addr, prop->port);
        return;
    }
    send_remote_events(psfd, prop->enids);
    close(psfd);
}

void action_do(struct r_action *raction){
    pthread_t t1;
    int s;
    switch(raction->atype){
        case CMD:
            cmd_execute((struct cmd_action *) raction->action);
            break;
        case PROP:
            s = pthread_create(&t1, NULL, prop_execute_thread, (void *) raction->action);
            if(s != 0)
                dbg("Unable to create the thread to propagate the events", strerror(s));
            break;
        default:
            /* CMD_NONE */
            break;
    }
}

void action_cmd_set_cmd(struct r_action *raction, char *cmd){
    struct cmd_action *action;
    action = (struct cmd_action *) raction->action;
    action->cmd = cmd;
}
void action_prop_set_addr(struct r_action *raction, char *addr){
    struct prop_action *action;
    action = (struct prop_action *) raction->action;
    action->addr = addr;
}
void action_prop_set_port(struct r_action *raction, unsigned short port){
    struct prop_action *action;
    action = (struct prop_action *) raction->action;
    action->port = port;
}
void action_prop_set_enids(struct r_action *raction, RSList *enids){
    struct prop_action *action;
    action = (struct prop_action *) raction->action;
    action->enids = enids;
}