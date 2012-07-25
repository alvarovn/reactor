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

enum rmsg_type reactor_add_rule_handler(struct reactor_d *reactor, struct r_rule *rule){
    Transition *trans, *fsminitial = NULL;
    State *from, *to = NULL;
    EventNotice *en = NULL;
    bool init;
    enum rmsg_type cmt = ACK;
    char *fromstr,
         *tostr;
    struct rr_obj *eids;
    
    // TODO Check errors
    if(rule == NULL){
        cmt = ARG_MALFORMED;
        warn("Rule malformed.");
        goto end;
    }
    if(rule->errors != NULL){
        cmt = ARG_MALFORMED;
        while(rule->errors != NULL){
            if((rule->file != NULL) && (rule->linen >= 0)){
                warn("%s:%i:%i: %s", rule->file, rule->linen, rule->errors->pos, rule->errors->msg);
            }
            else{
                warn("\"%s\":%i: %s", rule->line, rule->errors->pos, rule->errors->msg);
            }
            rule->errors = rule->errors->next;
        }
        goto end;
    }
    if(rule->objs == NULL){
        // Comment or empty line
        // This is not valid by command line
        warn("Empty or commented rule received");
        cmt = ARG_MALFORMED;
        goto end;
    }
    fromstr = (get_rule_obj(rule->objs, RULE_FROM))->data;
    tostr = (get_rule_obj(rule->objs, RULE_TO))->data;
    init = (from = (State *) reactor_hash_table_lookup(reactor->states, fromstr)) == NULL;
    if(init){
        from = state_new(reactor, fromstr);
        state_set_fsminitial(from, from);
    }
    
    to = (State *) reactor_hash_table_lookup(reactor->states, tostr);
    if(to == NULL) {
        to = state_new(reactor, tostr);
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
    trans_set_action(trans, (struct r_action *)(get_rule_obj(rule->objs, RULE_RACTION))->data);

    for(eids = (get_rule_obj(rule->objs, RULE_EVENTS))->down; eids != NULL; eids = eids->next){
        en = (EventNotice *) reactor_hash_table_lookup(reactor->eventnotices, eids->data);
        if(en == NULL){
            en = en_new(reactor, eids->data);
        }
        trans_add_requisite(trans, en);
        /* As it is an initial transition it should also be a current transition */
        if(init) en_add_curr_trans(en, trans);
    }
    if(init){
        info("New transition from initial state '%s' to state '%s'.", 
             fromstr, tostr);
    }
    else{
        info("New transition from state '%s' to state '%s'.", 
             fromstr, tostr);
    }
    state_add_trans(from, trans);
end:
    return cmt;
}

R_EXPORT int reactor_event_handler(struct reactor_d *reactor, const char *msg){
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
    en = (EventNotice *) reactor_hash_table_lookup(reactor->eventnotices, msg);
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
                info("Forwarding to state '%s'", state_get_id(trans_get_dest(trans)));
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
    /* In case no action was executed, so no currtrans was cleared, we clear the currtrans of the last event */
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

enum rmsg_type reactor_rm_trans_handler(struct reactor_d *reactor, char *msg){
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
    state = reactor_hash_table_lookup(reactor->states, (void *) msg);
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
    state_set_trans( state, trans_clist_remove_link(trans) );
    trans_free(reactor, trans);
    info("Transition %s removed", msg);
end:
    return rmt;
malformed:
    return ARG_MALFORMED;
}