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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "reactor.h"

struct _transition{
    RSList* enrequisites;
    unsigned int noticedevents;
    unsigned int eventnotices;
    State* dest;
    ActionTypes at;
    void *typedaction;
};

/* Action types */
struct _cmdaction{
    int uid;
    char *shell;
    char *cmd;
};


Transition* trans_new(State *dest){
    Transition *trans = NULL;
    
    if((trans = (Transition *) calloc(1, sizeof(Transition))) == NULL){
        dbg_e("Error on malloc() the new transition'", NULL);
        goto end;
    }
    trans->dest = dest;
//     trans->at = CMD_ACTION;
end:
    return trans;
}

bool trans_set_cmd_action(Transition *trans, const char *cmd, const char *shell, int uid){
    // TODO Check shell value for an existing shell (?)
    // TODO Check uid for an existing user (?)
    CmdAction *cmdactn = NULL;
    bool success = true;
    
    if((cmdactn = (CmdAction *) calloc(1, sizeof(CmdAction))) == NULL){
        dbg_e("Error on malloc() the new command action'", NULL);
        success = false;
        goto end;
    }
    cmdactn->cmd = strdup(cmd);
    cmdactn->shell = strdup(shell);
    cmdactn->uid = uid;
    trans->at = CMD_ACTION;
    trans->typedaction = cmdactn;
end:
    return success;
}

static void cmd_free(CmdAction *cmdactn){
    free(cmdactn->cmd);
    free(cmdactn->shell);
    free(cmdactn);
}

void trans_free(Transition *trans){
    reactor_slist_free(trans->enrequisites);
    switch(trans->at){
        case CMD_ACTION:
            cmd_free((CmdAction *) trans->typedaction);
            break;
    }
    free(trans);
    
}

static void cmd_execute(CmdAction *cmd){
    // TODO Create a fork to monitor the execution
    
    switch (fork()) {
        case -1:
            err("Unable to fork, command won't be executed.");
            break;
        case 0:
            /* child process */
            if(setuid(cmd->uid) < 0){
                err("Commands can't be executed as user '%i'.", cmd->uid);
            }
            execl(cmd->shell, cmd->shell, "-c", cmd->cmd, NULL);
            
            /* If everything goes fine, it will never arrive here */
            err("Unable to execute the shell command '%s'", cmd->shell);
            break;
        default:
            /* parent process */
            break;
    }
    
}

bool trans_notice_event(Transition *trans){
    if(++trans->noticedevents < trans->eventnotices)
        return false;
    /* If all the required events happened then we must execute the action */
    switch(trans->at){
        case CMD_ACTION:
            cmd_execute((CmdAction *) trans->typedaction);
            break;
    }
    trans->noticedevents = 0;
    return true;
}

void trans_add_requisite(Transition *trans, EventNotice *en){
    trans->enrequisites = reactor_slist_prepend(trans->enrequisites, en);
    trans->eventnotices++;
}

const State* trans_get_dest(Transition *trans){
    return trans->dest;
}

const RSList* trans_get_enrequisites(Transition *trans){
    return trans->enrequisites;
}