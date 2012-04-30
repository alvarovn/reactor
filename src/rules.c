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
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "reactor.h"

struct rr_token* new_token(void *data, struct rr_token *next, struct rr_token *down, int pos){
    struct rr_token *token;
    if((token = calloc(1, sizeof(struct rr_token))) == NULL){
        dbg_e("Error on malloc the new token", NULL);
        return NULL;
    }
    token->data = data;
    token->next = next;
    token->down = down;
    token->pos = pos;
    return token;
}

void tokens_free(struct rr_token *tokens){
    if(tokens == NULL) return;
    tokens_free(tokens->next);
    tokens_free(tokens->down);
    free(tokens->data);
    free(tokens);
}

struct rr_token* get_token(struct rr_token *tokens, unsigned int tnum){
    int i = 0;
    while((i != tnum) && (tokens != NULL)){
        tokens = tokens->next;
        i++;
    }
    return tokens;
}

struct rr_error* new_error(int pos, char *msg){
    struct rr_error *error;
    
    if((error = calloc(1, sizeof(struct rr_error))) == NULL){
        dbg_e("Error on malloc() the new error", NULL);
        return NULL;
    }
    error->pos = pos;
    error->msg = strdup(msg);
    
    return error;
}

void errors_free(struct rr_error *errors){
    if(errors == NULL) return;
    errors_free(errors->next);
    free(errors->msg);
    free(errors);
}

struct rr_expr* expr_new(int exprnum, char *tokensep, char *end, bool trim){
    struct rr_expr* expr;
    
    if((expr = calloc(1, sizeof(struct rr_expr))) == NULL){
        dbg_e("Error on malloc() an expression", NULL);
        return NULL;
    }
    expr->exprnum = exprnum;
    expr->tokensep = tokensep;
    expr->end = end;
    expr->trim = trim;
    return expr;
}

void exprs_free(struct rr_expr *expr){
    if(expr == NULL) return;
    exprs_free(expr->next);
    exprs_free(expr->subexpr);
    free(expr);
}

void rules_free(struct r_rule *rrule){
    if(rrule == NULL) return;
    rules_free(rrule->next);
    errors_free(rrule->errors);
    tokens_free(rrule->tokens);
    exprs_free(rrule->expr);
    free(rrule->line);
    free(rrule->file);
    free(rrule);
}

/* TODO Very dirty tokenizer. Refactor. */
static int tokenize(char *line, struct rr_expr *expr, 
                    struct rr_token **tokens, 
                    struct rr_error **errors)
{
    char *linep;
    
    struct rr_error **errorsp;
    struct rr_expr *exprp;
    int tokenlastchar = 0,
        spacelength = 0,
        exprcount,
        innercount,
        nextexpr;
    char *errmsg = NULL;
    struct rr_token **tokenp;
    
    if( line == NULL ||
        expr == NULL ||
        tokens == NULL ||
        errors == NULL
    ){
        goto end;
    }
    exprp = expr->subexpr;
    nextexpr = (exprp == NULL) ? -1 : exprp->exprnum;
    bool finished = false;
    
    linep = line;
    *errors = NULL;
    errorsp = errors;
    *tokens = NULL;
    tokenp = tokens;
    
    skip_blanks(linep, tokenlastchar, spacelength, expr->trim);
    if(*linep == '\0' || *linep == '#') goto end;
        
    exprcount = 0;
    
    while(  (linep[tokenlastchar] != expr->end) && 
            (linep[tokenlastchar] != '\0') &&
            (linep[tokenlastchar] != '#')
    ){
        if(linep[tokenlastchar] == expr->tokensep){
            /* Separator */
            if((tokenlastchar - spacelength) < 1){
                errmsg = "No token before the expression separator";
                goto error;
            }
            *tokenp = new_token((void *)strndup(linep, tokenlastchar - spacelength),
                                NULL, NULL, linep-line);
            tokenp = &(*tokenp)->next;
            exprcount++;
            linep += tokenlastchar;
            linep++;
            tokenlastchar = 0;
            spacelength = 0;
        }
        else if(exprcount == nextexpr){
            /* Expression */
            *tokenp = new_token(NULL, NULL, NULL, linep-line);
            linep += tokenlastchar;
            tokenlastchar = tokenize(linep, exprp, &((*tokenp)->down), errorsp);
            tokenp = &(*tokenp)->next;
            if(*errorsp != NULL){
                tokens_free(*tokens);
                *tokens = NULL;
                linep = line;
                goto end;
            }
            if(tokenlastchar == 0){
                errmsg = "The subexpression is empty";
                goto error;
            }
            exprcount++;
            linep += tokenlastchar;
            tokenlastchar = 0;

            if(*linep == expr->end)
                finished = true;
            else
                linep++;
            exprp = exprp->next;
            nextexpr = (exprp == NULL) ? -1 : exprp->exprnum;
        }
        else{
            /* Non-space token char */
            // If this is the second word from a token, which expressions ends
            // with a space (so there was no separator), then check the last
            // space as a separator.
            if((spacelength > 0) && ( expr->end == ' ')){
                tokenlastchar--;
                spacelength--;
                continue;
            }
            tokenlastchar++;
            spacelength = 0;
            // If this is an space, and space is a separator, don't skip it
            if(expr->tokensep == ' ' && linep[tokenlastchar] == ' ') continue;
        }
        skip_blanks(linep, tokenlastchar, spacelength, expr->trim);
    }
    if(tokenlastchar - spacelength > 0){
        /* Last token */
//         if(exprcount < nextexpr){
//             errmsg = "Unexpected rule end";
//             goto end;
//         }
        *tokenp = new_token((void *)strndup(linep, tokenlastchar - spacelength),
                    NULL, NULL, linep-line);
        tokenp = &(*tokenp)->next;
        spacelength = 0;
        finished = true;
    }
    if(!finished){
        // If there is separator at the end of the expression
        errmsg = "Unexpected expression end";
        goto error;
    }
    linep += tokenlastchar;
//     if(*linep == '\0' && (expr->end != '\0')){
//         errmsg = "Unexpected rule end";
//         goto error;
//     }
end:
    return (linep - line);
    
error:
    *errorsp = new_error(linep-line, errmsg);
    errorsp = &(*errorsp)->next;
    linep = line;
    tokens_free(*tokens);
    *tokens = NULL;
    return 0;
}

void tokenize_rule(struct r_rule *rule){
    int aux = 0;
    
    if(rule == NULL) return;
    
    rule->errors = NULL;
    tokenize(rule->line, rule->expr, &rule->tokens, &rule->errors);
}

/* TODO Needs refactor */
struct r_rule* rule_parse(const char *line, const char *file, int linen, uid_t uid){
    struct r_rule *rule;
    struct rr_token *from,
                    *to,
                    *events,
                    *actiontype,
                    *action,
                    *eventsp,
                    *ractiontkn;
    struct r_action *raction;
    int eventsl,
        tokenchari;
    char *tokencharp,
         *nnum;
    
    if((rule = calloc(1, sizeof(struct r_rule))) == NULL){
        dbg_e("Error on malloc() a rule", NULL);
        rule = NULL;
        goto end;
    }
    
    rule->line = strdup(line);
    if(file == NULL) file = "";
    rule->file = strdup(file);
    rule->linen = linen;
    rule->expr = expr_new(0, ' ', '\0', true);
    rule->expr->subexpr = expr_new(RULE_EVENTS, '&', ' ', true);
    rule->expr->subexpr->next = expr_new(RULE_ACTION, '\0', '\0', true);
    
    tokenize_rule(rule);
    
    if(rule->errors != NULL){
        goto end;
    }
    
    if((from = get_token(rule->tokens, RULE_FROM)) == NULL){
        //Empty line
        goto end;
    }
    
    if((to = get_token(rule->tokens, RULE_TO)) == NULL){
        rule->errors = new_error(from->pos + strlen(from->data), "Unexpected rule end");
        tokens_free(rule->tokens);
        rule->tokens = NULL;
        goto end;
    }
    if((events = get_token(rule->tokens, RULE_EVENTS)) == NULL){
        rule->errors = new_error(to->pos + strlen(to->data), "Unexpected rule end");
        tokens_free(rule->tokens);
        rule->tokens = NULL;
        goto end;
    }
    actiontype = get_token(rule->tokens, RULE_ACTION_TYPE);
    
    if((actiontype == NULL) || (strcmp(actiontype->data, "NONE") == 0)){
        if(get_token(rule->tokens, RULE_ACTION) != NULL){
            rule->errors = new_error(actiontype->pos + strlen(actiontype->data), "The rule was expected to finish here");
            tokens_free(rule->tokens);
            rule->tokens = NULL;
            goto end;
        }
        eventsl = 0;
        for(eventsp = events->down; eventsp != NULL; eventsp = eventsp->next){
            eventsl += strlen(eventsp->data);
        }
        ractiontkn = new_token(action_new(NONE), NULL, NULL, events->pos + eventsl);
    }
    else{
        if((action = get_token(rule->tokens, RULE_ACTION)) == NULL){
            rule->errors = new_error(actiontype->pos + strlen(actiontype->data), "Unexpected rule end");
            tokens_free(rule->tokens);
            rule->tokens = NULL;
            goto end;
        }
        if(strcmp(actiontype->data, "CMD") == 0){
            ractiontkn = new_token(action_new(CMD), NULL, NULL, actiontype->pos);
            action_cmd_set_cmd((struct r_action *) ractiontkn->data, strdup(action->down->data));
        }
        else if(strcmp(actiontype->data, "PROP") == 0){
            tokenchari = 0;
            tokencharp = action->down->data;
            skip_noblanks_and(tokencharp, tokenchari, ':');
            ractiontkn = new_token(action_new(PROP), NULL, NULL, actiontype->pos);
            action_prop_set_addr((struct r_action *) ractiontkn->data, strndup(action->down->data, tokenchari));
            skip_blanks_simple(tokencharp, tokenchari);
            switch( (int) ((char *) (action->down->data))[tokenchari]){
                case ':':
                    skip_blanks_simple(tokencharp, tokenchari);
                    if(((char *)action->down->data)[tokenchari] == '\0'){
                        rule->errors = new_error(action->pos + tokenchari, "Unexpected rule end");
                        tokens_free(rule->tokens);
                        rule->tokens = NULL;
                        goto end;
                    }
                    tokenchari++;
                    skip_blanks_simple(tokencharp, tokenchari);
                    // TODO If after the port number there are invalid characters return error (second parameter of strtol)
                    action_prop_set_port((struct r_action *)ractiontkn->data, 
                                         (unsigned short) strtol(&((char*)action->down->data)[tokenchari], &nnum, 10));
                    if(nnum != &((char*)action->down->data)[strlen(action->down->data)]){
                        rule->errors = new_error(action->pos + tokenchari, "The port is not a number");
                        tokens_free(rule->tokens);
                        rule->tokens = NULL;
                        goto end;
                    }
                    break;
                case '\0':
                    // Default port
                    action_prop_set_port((struct r_action *)ractiontkn->data, REACTOR_PORT);
                    break;
                default:
                    rule->errors = new_error(actiontype->pos, "Malformed address");
                    tokens_free(rule->tokens);
                    rule->tokens = NULL;
                    goto end;
            }
        }
        else{
            rule->errors = new_error(action->pos + tokenchari, "Unexpected token");
            tokens_free(rule->tokens);
            rule->tokens = NULL;
            goto end;
        }
    }
    tokens_free(actiontype);
    events->next = ractiontkn;
end:
    return rule;
}

struct r_rule* parse_rules_file(const char *filename, unsigned int uid){
    /* TODO Add the ability to have an unordered list of rules */
    FILE *f;
    size_t len;
    unsigned int linecount = 0;
    char line[LINE_SIZE];
    int *linep;
    int dummy = 0;
    
    struct r_rule   *rule = NULL,
                    *head = NULL, 
                    *tail = NULL;
    
    info("Reading '%s' as rules file", filename);
    f = fopen(filename, "r");
    if (f == NULL){
        warn("Unable to open '%s' rules file", filename);
        goto end;
    }
    
    while (fgets(line, sizeof(line), f) != NULL) {
        linep = 0;
//         /* Skip comments */
//         skip_blanks_simple(line, linep);
//         if(line[linep] == '#'){
//             linecount++;
//             continue;
//         }
//         /* Skip empty lines */
//         if(line[linep] == '\n'){
//             linecount++;
//             continue;
//         }
        len = strlen(line);
        while (line[len-2] == '\\') {
            if (fgets(&line[len-2], (sizeof(line)-len)+2, f) == NULL)
                break;
            if (strlen(&line[len-2]) < 2)
                break;
            linecount++;
            len = strlen(line);
        }
        if (len+1 >= sizeof(line)) {
            err("Line too long '%s':%u, ignored", filename, linecount);
            continue;
        }
        rule = rule_parse(line, filename, linecount, uid);
        /* Malloc error */
        if(rule == NULL){
            // TODO Error
        }        
        /* Not empty or commented line */
        if(rule->tokens != NULL || rule->errors != NULL){
            if(head == NULL || tail == NULL) {
                head = rule;
                tail = rule;
            }    
            else{
                tail->next = rule;
                tail = rule;
            }
        }
        linecount++;
    }
end:    
    return head;
}