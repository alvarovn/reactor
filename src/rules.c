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
#include <stdbool.h>

#include "reactord.h"

/*
 * Restricted characters:
 * '-', '&' and '#'
 * TODO Restrict them.
 */
struct r_rule* rules_parse_one(char *rulestr){
    struct r_rule * rule;
    char *token, *tkaux;
    int tokenlength;
    bool final;
    
    if((rule = (struct r_rule *)calloc(1, sizeof(struct r_rule))) == NULL){
        goto malloc_error;
    }
    
    token = rulestr;
    skip_blanks(token);
    if(*token == '-'){
        rule->from = NULL;
        token+=sizeof(char);
        if(*token != ' ' && *token != '\t'){
            goto syntax_error;
        }
    }
    else{
        tokenlength = 0;
        skip_noblanks(token, tokenlength);
        if( token[tokenlength] == '\0' || 
            token[tokenlength] == '&' || 
            token[tokenlength] == '#' || 
            token[tokenlength] == '-'){
            goto syntax_error;
        }
        rule->from = strndup(token, tokenlength);
        token += tokenlength;
    }
    skip_blanks(token);
    tokenlength = 0;
    skip_noblanks(token, tokenlength);
    if( token[tokenlength] == '\0' || 
        token[tokenlength] == '&' || 
        token[tokenlength] == '#' || 
        token[tokenlength] == '-'){
            goto syntax_error;
    }
    rule->to = strndup(token, tokenlength);
    token += tokenlength;
    skip_blanks(token);
    if( *token == '\0' || 
        *token == '&' || 
        *token == '#' ){
            goto syntax_error;
    }
    token -= sizeof(char);
    do{
        token += sizeof(char);
        skip_blanks(token);
        tokenlength = 0;
        skip_noblanks(token, tokenlength);
        while(token[tokenlength] == '-'){
            tokenlength++;
            skip_noblanks(token, tokenlength);
        }
        rule->enids = reactor_slist_prepend(rule->enids, (void *) strndup(token, tokenlength));
        token += tokenlength;
        skip_blanks(token);
    }while(*token == '&');

    if( *token == '\0' || *token == '#' ) {
        rule->action = NULL;
        return rule;
    }
    tokenlength = 0;
    while(token[tokenlength] != '\0' && *token != '#' ){
        tokenlength++;
        skip_blanks(token);
        skip_noblanks(token, tokenlength);
    }
    rule->action = strndup(token, tokenlength);
    return rule;
syntax_error:
    /* TODO free atm */
    dbg("Rule malformed", NULL);
    return NULL;
malloc_error:
    dbg_e("Error on malloc the new parsed rule", NULL);
    return NULL;
}