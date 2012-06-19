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
#ifndef LIBREACTORPARSER_INCLUDED
#define LIBREACTORPARSER_INCLUDED

// #define skip_blanks(c) \
//             while (*c == '\t' || *c == ' ') \
//                 c+=sizeof(char);
#define skip_blanks(c, tl, sl, trim) \
            if(trim){ \
                while(c[tl] == '\t' || c[tl] == ' ' || c[tl] == '\n'){ \
                    if(tl == 0) c+=sizeof(char); \
                    else if(tl > 0){ \
                        tl+=sizeof(char); \
                        sl+=sizeof(char); \
                    } \
                } \
            }

#define skip_blanks_simple(c, i) \
            while(c[i] == '\t' || c[i] == ' ' || c[i] == '\n'){ \
                i+=sizeof(char); \
            }
                
// #define skip_noblanks(c, i) \
//             while (c[i] != '\n' && c[i] != '\t' && c[i] != ' ' && c[i] != '&' && c[i] != '#' && c[i] != '\0') \
//                 i++;

#define skip_noblanks_and(c, i, d) \
            while(  (c[i] != '\n') && \
                    (c[i] != '\t') && \
                    (c[i] != ' ') && \
                    (c[i] != '\0') && \
                    (c[i] != d) \
            ){ \
                i++; \
            }

struct rr_error{
    int pos;
    char *msg;
    struct rr_error *next;
};

typedef void *(*RRObjFreeFunc)(void *);
struct rr_obj{
    void *data;
    RRObjFreeFunc freefunc;
    struct rr_obj *next;
    struct rr_obj *down;
    int pos;
};
struct rr_expr{
    int exprnum;
    char tokensep;
    char end;
    bool trim;
    struct rr_expr *subexpr;
    struct rr_expr *next;
};
typedef struct rr_expr *(*RRParseLineFunc)(char *line);
struct r_rule{
    char *line;
    int linen;
    char *file;
    struct rr_obj *objs;
    struct rr_error *errors;
    struct r_rule *next;
};

void rule_objs_free(struct rr_obj *objs);
struct rr_obj* get_rule_obj(struct rr_obj *objs, unsigned int tnum);
struct rr_error* new_rule_error(int pos, char *msg);
void rule_errors_free(struct rr_error *errors);
struct rr_expr* rule_expr_new(int exprnum, char *tokensep, char *end, bool trim);
void rule_exprs_free(struct rr_expr *expr);
void rules_free(struct r_rule *rrule);
struct r_rule* tokenize_rule(const char *line, const struct rr_expr *expr);
struct r_rule* parse_rules_file(const char *filename, RRParseLineFunc parseline);



#endif