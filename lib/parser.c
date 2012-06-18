/*
    This file is part of reactor.

    Copyright (C) 2011  Álvaro Villalba Navarro <vn.alvaro@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "libreactor-private.h"
#include "libreactor-parser.h"

R_EXPORT struct rr_obj* new_rule_obj(void *data, struct rr_obj *next, struct rr_obj *down, int pos){
    struct rr_obj *obj;
    if((obj = calloc(1, sizeof(struct rr_obj))) == NULL){
        dbg_e("Error on malloc the new token", NULL);
        return NULL;
    }
    obj->data = data;
    obj->next = next;
    obj->down = down;
    obj->pos = pos;
    obj->freefunc = NULL;
    return obj;
}

R_EXPORT void rule_objs_free(struct rr_obj *objs){
    if(objs == NULL) return;
    rule_objs_free(objs->next);
    rule_objs_free(objs->down);
    if(objs->freefunc != NULL){
        objs->freefunc(objs->data);
    }
    free(objs);
}

R_EXPORT struct rr_obj* get_rule_obj(struct rr_obj *obj, unsigned int tnum){
    int i = 0;
    while((i != tnum) && (obj != NULL)){
        obj = obj->next;
        i++;
    }
    return obj;
}

R_EXPORT struct rr_error* new_rule_error(int pos, char *msg){
    struct rr_error *error;
    
    if((error = calloc(1, sizeof(struct rr_error))) == NULL){
        dbg_e("Error on malloc() the new error", NULL);
        return NULL;
    }
    error->pos = pos;
    error->msg = strdup(msg);
    
    return error;
}

R_EXPORT void rule_errors_free(struct rr_error *errors){
    if(errors == NULL) return;
    rule_errors_free(errors->next);
    free(errors->msg);
    free(errors);
}

R_EXPORT struct rr_expr* rule_expr_new(int exprnum, char *tokensep, char *end, bool trim){
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

R_EXPORT void rule_exprs_free(struct rr_expr *expr){
    if(expr == NULL) return;
    rule_exprs_free(expr->next);
    rule_exprs_free(expr->subexpr);
    free(expr);
}

R_EXPORT void rules_free(struct r_rule *rrule){
    if(rrule == NULL) return;
    rules_free(rrule->next);
    rule_errors_free(rrule->errors);
    rule_objs_free(rrule->objs);
    free(rrule->line);
    free(rrule->file);
    free(rrule);
}

/* TODO Very dirty tokenizer. Refactor. */
static int tokenize(char *line, const struct rr_expr *expr, 
                    struct rr_obj **objs, 
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
    struct rr_obj **objp;
    
    if( line == NULL ||
        expr == NULL ||
        objs == NULL ||
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
    *objs = NULL;
    objp = objs;
    
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
            *objp = new_rule_obj((void *)strndup(linep, tokenlastchar - spacelength),
                                NULL, NULL, linep-line);
            (*objp)->freefunc = free;
            objp = &(*objp)->next;
            exprcount++;
            linep += tokenlastchar;
            linep++;
            tokenlastchar = 0;
            spacelength = 0;
        }
        else if(exprcount == nextexpr){
            /* Expression */
            *objp = new_rule_obj(NULL, NULL, NULL, linep-line);
            linep += tokenlastchar;
            tokenlastchar = tokenize(linep, exprp, &((*objp)->down), errorsp);
            objp = &(*objp)->next;
            if(*errorsp != NULL){
                while(*errorsp != NULL){
                    (*errorsp)->pos += linep - line;
                    errorsp = &(*errorsp)->next;
                }
                rule_objs_free(*objs);
                *objs = NULL;
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
        *objp = new_rule_obj((void *)strndup(linep, tokenlastchar - spacelength),
                    NULL, NULL, linep-line);
        (*objp)->freefunc = free;
        objp = &(*objp)->next;
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
    *errorsp = new_rule_error(linep-line-1, errmsg);
    errorsp = &(*errorsp)->next;
    linep = line;
    rule_objs_free(*objs);
    *objs = NULL;
    return 0;
}

R_EXPORT struct r_rule* tokenize_rule(const char *line, const struct rr_expr *expr){
    int aux = 0;
    struct r_rule *rule = NULL;
    
    if((rule = calloc(1, sizeof(struct r_rule))) == NULL){
        dbg_e("Error on malloc() a rule", NULL);
        rule = NULL;
        goto end;
    }
    
    rule->line = strdup(line);
    
    rule->errors = NULL;
    rule->next = NULL;
    tokenize(rule->line, expr, &rule->objs, &rule->errors);
end:
    return rule;
}

R_EXPORT struct r_rule* parse_rules_file(const char *filename, RRParseLineFunc parseline){
    FILE *f;
    size_t len;
    unsigned int linecount = 1;
    char line[LINE_MAX];
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
        rule = parseline(line);
        /* Malloc error */
        if(rule == NULL){
            // TODO Error
        }        
        /* Not empty or commented line */
        rule->linen = linecount;
        rule->file = (filename == NULL) ? NULL : strdup(filename);
        if(rule->objs != NULL || rule->errors != NULL){
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