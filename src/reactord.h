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

#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#include "reactor.h"

#include <sys/types.h>

/* TODO Socket must be changed to a secure path */

#define SOCK_PATH "/tmp/rctlsock"

#define skip_blanks(c) \
            while (*c == '\t' || *c == ' ') \
                c+=sizeof(char);
            
#define skip_noblanks(c, i) \
            while (c[i] != '\t' && c[i] != ' ' && c[i] != '&' && c[i] != '#' && c[i] != '-' && c[i] != '\0') \
                i++;

struct r_rule{
    char *action;
    RSList *enids; 
    char *to; 
    char *from;
};

struct r_event{
    char *eid;
//     int uid;
    /* In the future this field may contain more information about the font 
     * than the pid.
     */ 
//     pid_t fontpid;  
};

enum rmsg_type{
    /* to server */
    REACTOR_EVENT,
    RULE,
    /* from server */
    ACK,
    RULE_NOFROM,
    RULE_MULTINIT,
    RULE_MALFORMED
};

struct rmsg_hd{
    int size;
    enum rmsg_type mtype;
};

struct r_msg{
    struct rmsg_hd hd;
    char *msg;
};

/* cntrl.c */
typedef struct _cntrl Cntrl;

int cntrl_send_msg(Cntrl *cntrl, const struct r_msg *msg);
void cntrl_peer_close(Cntrl *cntrl);
struct r_msg* cntrl_receive_msg(Cntrl *cntrl);
void cntrl_free(Cntrl *cntrl);
Cntrl* cntrl_new(bool server);
int cntrl_listen(Cntrl* cntrl);
int cntrl_connect(Cntrl *cntrl);
int cntrl_get_fd(Cntrl *cntrl);


/* user.c */
typedef struct _user User;
struct _user{
    char *name;
    User *next;
};

User* load_users(const char*);
void free_users(User*);

/* rules.c */
struct r_rule* rules_parse_one(char *rulestr);
#endif