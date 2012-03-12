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
    unsigned int ntranspointers;
    /* This pointer acts as identifier of the finite state machine */
    Transition *fsminitial;
};

State* state_new(const char* id){
    State *ste = NULL;

    if ((ste = (State *) calloc(1, sizeof(State))) == NULL) {
        dbg_e("Error on malloc() the state '%s'", id);
        goto end;
    }

    ste->id = strdup(id);
end:
    return ste;
}

void state_set_fsminitial(State *ste, Transition *fsminitial){
    ste->fsminitial= fsminitial;
}

Transition* state_get_fsminitial(State *ste){
    return ste->fsminitial;
}

bool state_free(State *ste){
    if(ste->ntranspointers-- > 0) return false;

    trans_clist_free_full(ste->transitions);
    free(ste->id);
    free(ste);
    return true;
}

void state_add_trans(State *ste, Transition *trans){
    trans_clist_merge(ste->transitions, trans);
}

const char* state_get_id(State *ste){
    return ste->id;
}

void state_add_transpointer(State *ste){
    ste->ntranspointers++;
}

Transition* state_get_trans(State *ste){
    return ste->transitions;
}