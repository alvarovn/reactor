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

#include "reactor.h"


struct _state {
    char *id;
    Transition *transitions;
    unsigned int refcount;
    /* This pointer acts as identifier of the finite state machine */
    State *fsminitial;
};

State* state_new(struct reactor_d *reactor, const char* id){
    State *ste = NULL;

    if ((ste = (State *) calloc(1, sizeof(State))) == NULL) {
        dbg_e("Error on malloc() the state '%s'", id);
        goto end;
    }

    ste->id = strdup(id);
    ste->refcount = 0;
    reactor_hash_table_insert(reactor->states, ste->id, ste);

end:
    return ste;
}

void state_set_fsminitial(State *ste, State *fsminitial){
    state_ref(fsminitial);
    ste->fsminitial = fsminitial;
}

State* state_get_fsminitial(State *ste){
    return ste->fsminitial;
}

void state_unref(struct reactor_d *reactor, State *ste){
    Transition *aux;
    if(ste == NULL || --ste->refcount > 0) 
        return;
    for(;ste->transitions != NULL;){
        aux = ste->transitions;
        ste->transitions = trans_clist_free_1(reactor, aux);
    }
//     trans_clist_free_full(reactor, ste->transitions);
    state_unref(reactor, ste->fsminitial);
    reactor_hash_table_remove(reactor->states, ste->id);
    info("State '%s' removed", ste->id);
    free(ste->id);
    free(ste);
}

void state_add_trans(State *ste, Transition *trans){
    ste->transitions = trans_clist_merge(ste->transitions, trans);
}

void state_set_trans(State *ste, Transition *trans){
    ste->transitions = trans;
}

const char* state_get_id(State *ste){
    return ste->id;
}

void state_ref(State *ste){
    if(ste == NULL) 
        return;
    ste->refcount++;
}

Transition* state_get_trans(State *ste){
    if(ste == NULL) return NULL;
    return ste->transitions;
}