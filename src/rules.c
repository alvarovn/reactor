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
/*
 * Restricted characters:
 * '&' and '#'
 * TODO This function needs refactor
 */
struct r_rule* rule_parse(char *rulestr, uid_t uid){
    struct r_rule * rule;
    char *token, *tkaux;
    int tokenlength;
    bool final;
    
    if((rule = (struct r_rule *)calloc(1, sizeof(struct r_rule))) == NULL){
        goto malloc_error;
    }
    
    token = rulestr;
    skip_blanks(token);
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
    skip_blanks(token);
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
    skip_blanks(token);
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
        skip_blanks(token);
        tokenlength = 0;
        skip_noblanks(token, tokenlength);
        rule->enids = reactor_slist_prepend(rule->enids, (void *) strndup(token, tokenlength));
        token += tokenlength;
        skip_blanks(token);
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
        skip_blanks(token);
        if(*token != '\0') goto syntax_error;
        rule->raction = action_new(NONE);
        goto end;
    }
    if(!strncmp("CMD", token, tokenlength)){
        token += tokenlength;
        skip_blanks(token);
        if( *token == '\0' ||
            *token == '&' ||
            *token == '#' || 
            *token == '\n' ){
               goto syntax_error; 
        }
        tokenlength = 0;
        while(token[tokenlength] != '\0' && token[tokenlength] != '#' && token[tokenlength] != '\n' ){
            tokenlength++;
            skip_blanks(token);
            skip_noblanks(token, tokenlength);
        }
        rule->raction = action_new(CMD);
        action_cmd_set_cmd(rule->raction, strndup(token, tokenlength));
        goto end;
    }
    if(!strncmp("PROP", token, tokenlength)){
        char *numend;
        int port;
        token += tokenlength;
        skip_blanks(token);
        if( *token == '\0' ||
            *token == '&' ||
            *token == '#' || 
            *token == '\n' ){
               goto syntax_error; 
        }
        tokenlength = 1;
        while(token[tokenlength] != ':' && token[tokenlength] != '\0') 
            tokenlength++;
        rule->raction = action_new(PROP);
        action_prop_set_addr(rule->raction, strndup(token, tokenlength));
        action_prop_set_enids(rule->raction, rule->enids);
        if(token[tokenlength] == '\0'){
            action_prop_set_port(rule->raction, REACTOR_PORT);
            goto end;
        }
        token += tokenlength;
        skip_blanks(token);
        tokenlength = 0;
        while(token[tokenlength] != '\0' && token[tokenlength] != ' '){
            tokenlength++;
        }
        token[tokenlength] = '\0';
        port = (int) strtol(token, &numend, 10);
        if( *numend != NULL ){
            /* Malformed port number */
            goto syntax_error;
        }
        action_prop_set_port(rule->raction, strtol(token, &numend, 10));
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
    int linecount;
    char line[LINE_SIZE];
    char *linep;
    
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
        skip_blanks(linep);
        if(*linep == '#'){
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
            warn("Syntax error '%s':%u, ignored", filename, linecount);
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
    }
end:    
    return head;
}