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

void rules_free(struct r_rule* rule){
    struct r_rule *aux;
    
    for(; rule != NULL; ){
//         reactor_slist_free_full(rule->enids, free);
        free(rule->from);
        free(rule->to);
//         action_free(rule->raction);
        aux = rule->next;
        free(rule);
        rule = aux;
    }
}

struct rr_token* new_token(char *data, struct rr_token *next, struct rr_token *down, int pos){
    struct rr_token *token;
    if((token = calloc(1, sizeof(struct token))) == NULL){
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

struct rr_error* new_error(int pos, char *msg){
    struct rr_error *error;
    
    if((error = calloc(1, sizeof(struct error))) == NULL){
        dbg_e("Error on malloc the new error", NULL);
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

/* TODO Very dirty tokenizer. Refactor. */
static int tokenize(char *line, struct rr_expr *expr, struct rr_token **tokens, struct rr_error **errors){
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
    exprp = expr->innerexpr;
    nextexpr = (exprp == NULL) ? 0 : exprp->exprnum;
    bool finished = false;
    
    linep = line;
    *errors = NULL;
    errorsp = errors;
    *tokens = NULL;
    tokenp = tokens;
    
    skip_blanks(linep, tokenlastchar, spacelength, expr->trim);
    if(*linep == '\0' || *linep == '#') goto end;
        
    exprcount = 1;
    
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
            *tokenp = new_token(strndup(linep, tokenlastchar - spacelength),
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
            exprcount++;
            linep += tokenlastchar;
            tokenlastchar = 0;

            if(linep == exprp->end)
                finished = true;
            else
                linep++;
            exprp = exprp->next;
            nextexpr = (exprp == NULL) ? 0 : exprp->exprnum;
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
        *tokenp = new_token(strndup(linep, tokenlastchar - spacelength),
                    NULL, NULL, linep-line);
        tokenp = &(*tokenp)->next;
        spacelength = 0;
    }
    else if(expr->tokensep != ' ' && !finished){
        // If there is separator at the end of the expression
        errmsg = "Unexpected expression end";
        goto error;
    }
    linep += tokenlastchar;
    if(*linep == '\0' && (expr->end != '\0')){
        errmsg = "Unexpected rule end";
        goto error;
    }
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

void tokenize_rule(struct rule *rule){
    int aux = 0;
    
    if(rule == NULL) return;
    
    rule->errors = NULL;
    tokenize(rule->line, rule->expr, &rule->tokens, &rule->errors);
}

struct r_rule* rule_parse(char *rulestr, uid_t uid){
    struct r_rule * rule;
    char *token, *tkaux;
    int tokenlength,
        dummy = 0;
    bool final;
    
    if((rule = (struct r_rule *)calloc(1, sizeof(struct r_rule))) == NULL){
        goto malloc_error;
    }
    
    token = rulestr;
    skip_blanks(token, dummy, dummy, true);
    /* From state */
    tokenlength = 0;
    skip_noblanks(token, tokenlength);
    if( token[tokenlength] == '\0' || 
        token[tokenlength] == '&' || 
        token[tokenlength] == '#' ||
        token[tokenlength] == '\n'){
            goto syntax_error;
    }
    rule->from = strndup(token, tokenlength);
    token += tokenlength;
    /* To state */
    skip_blanks(token, dummy, dummy, true);
    tokenlength = 0;
    skip_noblanks(token, tokenlength);
    if( token[tokenlength] == '\0' || 
        token[tokenlength] == '&' || 
        token[tokenlength] == '#' ||
        token[tokenlength] == '\n'){
            goto syntax_error;
    }
    rule->to = strndup(token, tokenlength);
    token += tokenlength;
    skip_blanks(token, dummy, dummy, true);
    if( *token == '\0' || 
        *token == '&' || 
        *token == '#' ||
        *token == '\n'){
            goto syntax_error;
    }
    /* Event notice ids */
    token -= sizeof(char);
    do{
        token += sizeof(char);
        skip_blanks(token, dummy, dummy, true);
        tokenlength = 0;
        skip_noblanks(token, tokenlength);
        rule->enids = reactor_slist_prepend(rule->enids, (void *) strndup(token, tokenlength));
        token += tokenlength;
        skip_blanks(token, dummy, dummy, true);
    }while(*token == '&');
    /* Action */
    if( *token == '\0' || 
        *token == '&' || 
        *token == '#' ||
        *token == '\n'){
            goto syntax_error;
    }
    tokenlength = 0;
    skip_noblanks(token, tokenlength);
    if(!strncmp("NONE", token, tokenlength)){
        token += tokenlength;
        skip_blanks(token, dummy, dummy, true);
        if(*token != '\0') goto syntax_error;
        rule->raction = action_new(NONE);
        goto end;
    }
    if(!strncmp("CMD", token, tokenlength)){
        token += tokenlength;
        skip_blanks(token, dummy, dummy, true);
        if( *token == '\0' ||
            *token == '&' ||
            *token == '#' || 
            *token == '\n' ){
               goto syntax_error; 
        }
        tokenlength = 0;
        while(token[tokenlength] != '\0' && token[tokenlength] != '#' && token[tokenlength] != '\n' ){
            tokenlength++;
            skip_blanks(token, dummy, dummy, true);
            skip_noblanks(token, tokenlength);
        }
        rule->raction = action_new(CMD);
        action_cmd_set_cmd(rule->raction, strndup(token, tokenlength));
        goto end;
    }
    if(!strncmp("PROP", token, tokenlength)){
        char *numend;
        unsigned short port;
        token += tokenlength;
        skip_blanks(token, dummy, dummy, true);
        if( *token == '\0' ||
            *token == '&' ||
            *token == '#' || 
            *token == '\n' ){
               goto syntax_error; 
        }
        tokenlength = 0;
        while(token[tokenlength] != ':' && token[tokenlength] != '\0') 
            tokenlength++;
        rule->raction = action_new(PROP);
        action_prop_set_addr(rule->raction, strndup(token, tokenlength));
        action_prop_set_enids(rule->raction, rule->enids);
        if(token[tokenlength] == '\0'){
            action_prop_set_port(rule->raction, REACTOR_PORT);
            goto end;
        }
        token += tokenlength+1;
        skip_blanks(token, dummy, dummy, true);
        tokenlength = 0;
        while(token[tokenlength] != '\0' && token[tokenlength] != ' '){
            tokenlength++;
        }
        token[tokenlength] = '\0';
        port = (int) strtol(token, &numend, 10);
        if( *numend != NULL && *numend != '\n'){
            /* Malformed port number */
            goto syntax_error;
        }
        action_prop_set_port(rule->raction, (unsigned short) strtol(token, &numend, 10));
        goto end;
    }
end:    
    return rule;
syntax_error:
    rules_free(rule);
    return NULL;
malloc_error:
    dbg_e("Error on malloc the new parsed rule", NULL);
    return NULL;
}

struct r_rule* parse_rules_file(const char *filename, unsigned int uid){
    /* TODO Add the ability to have an unordered list of rules */
    FILE *f;
    size_t len;
    unsigned int linecount = 0;
    char line[LINE_SIZE];
    char *linep;
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
        /* Skip comments */
        linep = &line[0];
        skip_blanks(linep, dummy, dummy, true);
        if(*linep == '#'){
            linecount++;
            continue;
        }
        /* Skip empty lines */
        if(*linep == '\n'){
            linecount++;
            continue;
        }
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
        rule = rule_parse(line, uid);
        /* Syntax error */
        if(rule == NULL){
            warn("Syntax error %s:%u, ignored", filename, linecount);
            continue;
        }
        /* Commented line */
        if(head == NULL || tail == NULL) {
            head = rule;
            tail = rule;
        }    
        else{
            tail->next = rule;
            tail = rule;
        }
        linecount++;
    }
end:    
    return head;
}