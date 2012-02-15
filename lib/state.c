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

/*  For different purpose concept, but almost identical to eventnotice. 
    Sort of code replication.
*/

typedef struct _state {
    char* id;
    RSList* transitions;
} State;

State* state_new(const char* id) {
    State *ste = NULL;

    if ((ste = (State *) malloc(sizeof(State))) == NULL) {
        dbg_e("Error on malloc() the state '%s'", id);
        goto end;
    }

    ste->id = strdup(id);

    return ste;
}

void state_free(State *ste){
    reactor_slist_free_full(ste->transitions, trans_free);
    free(en->id);
    free(en);
}
