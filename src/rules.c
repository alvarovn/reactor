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
#include "libreactor-parser.h"


/* TODO Needs refactor */
struct r_rule* r_rule_parse(const char *line){
    struct r_rule *rule;
    struct rr_obj *from,
                    *to,
                    *events,
                    *actiontype,
                    *action,
                    *eventsp,
                    *ractiontkn;
    struct r_action *raction;
    struct rr_expr *expr;
    RSList *eventsrsl;
    int eventsl,
        tokenchari;
    char *tokencharp,
         *nnum;
    

    expr = rule_expr_new(0, ' ', '\0', true);
    expr->subexpr = rule_expr_new(RULE_EVENTS, '&', ' ', true);
    expr->subexpr->next = rule_expr_new(RULE_ACTION, '\0', '\0', true);
    
    rule = tokenize_rule(line, expr);
    rule->file = NULL;
    rule->linen = -1;
    
    if(rule->errors != NULL){
        goto end;
    }
    
    if((from = get_rule_obj(rule->objs, RULE_FROM)) == NULL){
        //Empty line
        goto end;
    }
    
    if((to = get_rule_obj(rule->objs, RULE_TO)) == NULL){
        rule->errors = new_rule_error(from->pos + strlen(from->data), "Unexpected rule end");
        rule_objs_free(rule->objs);
        rule->objs = NULL;
        goto end;
    }
    if((events = get_rule_obj(rule->objs, RULE_EVENTS)) == NULL){
        rule->errors = new_rule_error(to->pos + strlen(to->data), "Unexpected rule end");
        rule_objs_free(rule->objs);
        rule->objs = NULL;
        goto end;
    }
    actiontype = get_rule_obj(rule->objs, RULE_ACTION_TYPE);
    
    if((actiontype == NULL) || (strcmp(actiontype->data, "NONE") == 0)){
        if(get_rule_obj(rule->objs, RULE_ACTION) != NULL){
            rule->errors = new_rule_error(actiontype->pos + strlen(actiontype->data), "The rule was expected to finish here");
            rule_objs_free(rule->objs);
            rule->objs = NULL;
            goto end;
        }
        eventsl = 0;
        for(eventsp = events->down; eventsp != NULL; eventsp = eventsp->next){
            eventsl += strlen(eventsp->data);
        }
        ractiontkn = new_rule_obj(action_new(NONE), NULL, NULL, events->pos + eventsl);
    }
    else{
        if((action = get_rule_obj(rule->objs, RULE_ACTION)) == NULL){
            rule->errors = new_rule_error(actiontype->pos + strlen(actiontype->data), "Unexpected rule end");
            rule_objs_free(rule->objs);
            rule->objs = NULL;
            goto end;
        }
        if(strcmp(actiontype->data, "CMD") == 0){
            ractiontkn = new_rule_obj(action_new(CMD), NULL, NULL, actiontype->pos);
            action_cmd_set_cmd((struct r_action *) ractiontkn->data, strdup(action->down->data));
        }
        else if(strcmp(actiontype->data, "PROP") == 0){
            tokenchari = 0;
            tokencharp = action->down->data;
            skip_noblanks_and(tokencharp, tokenchari, ':');
            ractiontkn = new_rule_obj(action_new(PROP), NULL, NULL, actiontype->pos);
            action_prop_set_addr((struct r_action *) ractiontkn->data, strndup(action->down->data, tokenchari));
            skip_blanks_simple(tokencharp, tokenchari);
            switch( (int) ((char *) (action->down->data))[tokenchari]){
                case ':':
                    skip_blanks_simple(tokencharp, tokenchari);
                    if(((char *)action->down->data)[tokenchari] == '\0'){
                        rule->errors = new_rule_error(action->pos + tokenchari, "Unexpected rule end");
                        action_free(ractiontkn->data);
                        ractiontkn = NULL;
                        rule_objs_free(rule->objs);
                        rule->objs = NULL;
                        goto end;
                    }
                    tokenchari++;
                    skip_blanks_simple(tokencharp, tokenchari);
                    action_prop_set_port((struct r_action *)ractiontkn->data, 
                                         (unsigned short) strtol(&((char*)action->down->data)[tokenchari], &nnum, 10));
                    if(nnum != &((char*)action->down->data)[strlen(action->down->data)]){
                        rule->errors = new_rule_error(action->pos + tokenchari, "The port is not a number");
                        action_free(ractiontkn->data);
                        ractiontkn = NULL;
                        rule_objs_free(rule->objs);
                        rule->objs = NULL;
                        goto end;
                    }
                    break;
                case '\0':
                    // Default port
                    action_prop_set_port((struct r_action *)ractiontkn->data, REACTOR_PORT);
                    break;
                default:
                    rule->errors = new_rule_error(actiontype->pos, "Malformed address");
                    action_free(ractiontkn->data);
                    ractiontkn->data = NULL;
                    rule_objs_free(rule->objs);
                    rule->objs = NULL;
                    goto end;
            }
            eventsp = events->down;
            eventsrsl = NULL;
            eventsrsl = reactor_slist_prepend(eventsrsl, (void *) strdup(eventsp->data));
            eventsp = eventsp->next;
            while(eventsp != NULL){
                eventsrsl = reactor_slist_prepend(eventsrsl, (void *) strdup(eventsp->data));
                eventsp = eventsp->next;
            }
            action_prop_set_enids((struct r_action *)ractiontkn->data, eventsrsl);
        }
        else{
            rule->errors = new_rule_error(action->pos + tokenchari, "Unexpected token");
            rule_objs_free(rule->objs);
            rule->objs = NULL;
            goto end;
        }
    }
    rule_objs_free(actiontype);
    events->next = ractiontkn;
end:
    return rule;
}