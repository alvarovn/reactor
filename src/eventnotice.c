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
#include <string.h>
#include <stdbool.h>

#include "reactor.h"

struct _eventnotice {
    char *id;
    RSList *currtrans;
    unsigned int refcount;
};

EventNotice* en_new(struct reactor_d *reactor, const char* id) {
    EventNotice *en = NULL;

    if ((en = (EventNotice *) calloc(1, sizeof(EventNotice))) == NULL) {
        dbg_e("Error on malloc() the event notice '%s'", id);
        goto end;
    }
    en->id = strdup(id);
    en->refcount = 0;
    reactor_hash_table_insert(reactor->eventnotices, en->id, en);
end:
    return en;
}

void en_unref(struct reactor_d *reactor, EventNotice *en) {
    if (en == NULL) return;
    if (--en->refcount > 0) return;
    reactor_slist_free(en->currtrans);
    en_clear_curr_trans(en);
    reactor_hash_table_remove(reactor->eventnotices, en->id);
    info("Event '%s' not expected anymore", en->id);
    free(en->id);
    free(en);
}

void en_add_curr_trans(EventNotice *en, Transition *trans) {
    en->currtrans = reactor_slist_prepend(en->currtrans, trans);
}

void en_clear_curr_trans(EventNotice *en) {
    reactor_slist_free(en->currtrans);
    en->currtrans = NULL;
}

const char* en_get_id(EventNotice *en) {
    return en->id;
}

const RSList** en_get_currtrans_ref(EventNotice *en){
    return &en->currtrans;
}

void en_ref(EventNotice *en) {
    if(en == NULL) return;
    en->refcount++;
}

void en_remove_one_curr_trans(EventNotice *en, Transition *trans){
    en->currtrans = reactor_slist_remove_all(en->currtrans, (void *)trans);
}