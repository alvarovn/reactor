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
#include <sys/types.h>

#include "reactor.h"

struct _transition{
    RSList* enrequisites;
    unsigned int noticedevents;
    unsigned int eventnotices;
    State* dest;
    struct r_action *raction;
    /* Transition circular list */
    Transition *clistnext;
    Transition *clistprev;
};

Transition* trans_new(State *dest){
    Transition *trans = NULL;
    
    if((trans = (Transition *) calloc(1, sizeof(Transition))) == NULL){
        dbg_e("Error on malloc() the new transition'", NULL);
        goto end;
    }
    trans->dest = dest;
    trans->clistnext = trans;
    trans->clistprev = trans;
    trans->raction = NULL;
end:
    return trans;
}

Transition* trans_clist_merge(Transition* clist1, Transition* clist2){
    Transition *aux;
    
    if(clist1 == NULL){
        clist1 = clist2;
        goto end;
    }
    aux = clist1->clistnext;
    clist1->clistnext = clist2->clistnext;
    clist2->clistnext = aux;
    clist2->clistnext->clistprev = clist2;
    clist1->clistnext->clistprev = clist1;
end:
    return clist1;
}

Transition* trans_clist_remove_link(Transition* trans){
    Transition *ret;
    
    if(trans == trans->clistnext) return NULL;
    ret = trans->clistnext;
    trans->clistnext->clistprev = trans->clistprev;
    trans->clistprev->clistnext = trans->clistnext;
    trans->clistnext = trans;
    trans->clistprev = trans;
    return trans->clistnext;
}

void trans_clist_free_full(Transition *trans){
    Transition *aux;
    
    for(;trans->clistnext == trans;){
      aux = trans;
      trans = trans_clist_remove_link(trans);
      trans_free(aux);
    }
    trans_free(trans);
}

Transition* trans_clist_next(Transition *clist){
    return clist->clistnext;
}

/* TODO This way to clear the currtrans no more needed is very inefficient. Fix it */
void trans_clist_clear_curr_trans(Transition *clist){
    Transition *aux;
    aux = clist;
    if(clist != NULL){
        do{
            clist->noticedevents = 0;
            reactor_slist_foreach(clist->enrequisites, en_remove_one_curr_trans, clist);
            clist = trans_clist_next(clist);
        }while(clist != aux);
    }
}

bool trans_set_action(Transition *trans, struct r_action *action){
    // TODO Check shell value for an existing shell (?)
    // TODO Check uid for an existing user (?)
    bool success = true;
    
    action_free(trans->raction);
    trans->raction = action;
end:
    return success;
}

void trans_free(Transition *trans){
    reactor_slist_free(trans->enrequisites);
    action_free(trans->raction);
    free(trans);
    
}

bool trans_notice_event(Transition *trans){
    if(++trans->noticedevents < trans->eventnotices)
        return false;
    /* If all the required events happened then we must execute the action */
    action_do(trans->raction);
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
