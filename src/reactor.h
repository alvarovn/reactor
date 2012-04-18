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

#include "libreactor.h"
#include "libreactor-private.h"

#include <sys/types.h>

/* TODO Group should be defined on a configuration file */

#define REACTOR_PORT 6500

#define R_GRP "events"

#define RULES_FILE "reactor.rules"
#define PATH_MAX 8192

#define skip_blanks(c) \
            while (*c == '\t' || *c == ' ') \
                c+=sizeof(char);
            
#define skip_noblanks(c, i) \
            while (c[i] != '\n' && c[i] != '\t' && c[i] != ' ' && c[i] != '&' && c[i] != '#' && c[i] != '\0') \
                i++;
            
struct reactor_d{
    GHashTable *eventnotices;
    GHashTable *states;
};
enum a_types{
    NONE,
    CMD,
    PROP
};
struct r_action{
  enum a_types atype;
  void *action;
};
	    
/* Action types */
struct cmd_action{
    uid_t uid;
    char *shell;
    char *cmd;
};
struct prop_action{
    char *addr;
    unsigned int  port;
    RSList *enids;
};

struct r_rule{
    struct r_action *raction;
    RSList *enids; 
    char *to; 
    char *from;
    unsigned int line;
    /* Linked list */
    struct r_rule *next;
};

struct r_event{
    char *eid;
//     int uid;
    /* In the future this field may contain more information about the font 
     * than the pid.
     */ 
//     pid_t fontpid;  
};
typedef struct _transition Transition;
typedef struct _remote Remote;

/* user.c */
struct r_user{
    struct passwd *pw;
    struct r_user *next;
};

struct r_user* load_users(const char*);
void free_users(struct r_user*);

/* rules.c */
#define LINE_SIZE 16384
struct r_rule* rule_parse(char *rulestr, uid_t uid);
void rules_free(struct r_rule *rule);
struct r_rule* parse_rules_file(const char *filename, unsigned int uid);
/* eventnotice.c */

typedef struct _eventnotice EventNotice;

EventNotice* en_new(struct reactor_d *reactor, const char* id);
void en_unref(struct reactor_d *reactor, EventNotice *en);
void en_add_curr_trans(EventNotice *en, Transition *trans);
void en_clear_curr_trans(EventNotice *en);
const char* en_get_id(EventNotice *en);
void en_ref(EventNotice *en);
const RSList** en_get_currtrans_ref(EventNotice *en);
void en_remove_one_curr_trans(EventNotice *en, Transition *trans);
/* state.c */

typedef struct _state State;

State* state_new(struct reactor_d *reactor, const char* id);
void state_unref(struct reactor_d *reactor, State *ste);
void state_add_trans(State *ste, Transition *trans);
const char* state_get_id(State *ste);
void state_ref(State *ste);
Transition* state_get_trans(State *ste);
void state_set_fsminitial(State *ste, State *fsminitial);
State* state_get_fsminitial(State *ste);

/* transition.c */
typedef struct cmd_action;

Transition* trans_new(State *dest);
bool trans_set_action(Transition *trans, struct r_action *action);
Transition* trans_clist_free(struct reactor_d *reactor, Transition *trans);
bool trans_notice_event(Transition *trans);
void trans_add_requisite(Transition *trans, EventNotice *en);
const State* trans_get_dest(Transition *trans);
const RSList* trans_get_enrequisites(Transition *trans);
Transition* trans_clist_merge(Transition* clist1, Transition* clist2);
Transition* trans_clist_remove_link(Transition* trans);
// void trans_clist_free_full(struct reactor_d *reactor, Transition* trans);
Transition* trans_clist_next(Transition *clist);
void trans_clist_clear_curr_trans(Transition *clist);
void state_set_trans(State *ste, Transition *trans);
/* action.c */

struct r_action* action_new(enum a_types atype);
void action_free(struct r_action *raction);
void action_do(struct r_action *raction);
void action_cmd_set_cmd(struct r_action *raction, char *cmd);
void action_prop_set_port(struct r_action *raction, unsigned int port);
void action_prop_set_addr(struct r_action *raction, char *addr);
void action_prop_set_enids(struct r_action *raction, RSList *enids);
#endif
