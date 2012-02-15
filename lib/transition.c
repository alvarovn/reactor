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

#include "reactor.h"

typedef struct _Transition{
    RHashTable* enrequisites;
    unsigned int noticedevents;
    State* dest;
    ActionTypes at;
    void *typedaction;
} Transition;

/* Action types */

typedef struct _CmdAction{
    int uid;
    char *shell;
    char *cmd;
} CmdAction;


Transition* trans_new(State *dest){
    Transition *trans = NULL;
    
    if((trans = (Transition *) malloc(sizeof(Transition))) == NULL){
        dbg_e("Error on malloc() the new transition'");
        goto end;
    }
    trans->enrequisites = reactor_hash_table_new(reactor_str_hash, str_eq);
    trans->dest = dest;
end:
    return trans;
}

bool trans_set_cmd_action(Transition *trans, const char *cmd, const char *shell, int uid){
    // TODO Check shell value for an existing shell (?)
    // TODO Check uid for an existing user (?)
    CmdAction *cmdactn = NULL;
    bool success = true;
    
    if((cmdactn = (CmdAction *) malloc(sizeof(CmdAction))) == NULL){
        dbg_e("Error on malloc() the new command action'");
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

static void trans_cmd_free(CmdAction *cmdactn){
    free(cmdactn->cmd);
    free(cmdactn->shell);
    free(cmdactn);
}

void trans_free(Transition *trans){
    reactor_hash_table_destroy(trans->enrequisites);
    switch(trans->at){
        case CMD_ACTION:
            trans_cmd_free((CmdAction *) trans->typedaction);
            break;
    }
    free(trans);
    
}

void trans_notice_event(Transition *trans){
    trans->noticedevents++;
}