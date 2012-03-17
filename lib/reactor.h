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

#ifndef REACTOR_INCLUDED
#define REACTOR_INCLUDED

#include <string.h>
#include <stdbool.h>
#include <glib.h>

typedef struct _transition Transition;

static inline bool str_eq(const char *s1, const char *s2){
    return !strcmp(s1, s2);
}

/* basic third party data structures wrappers */

typedef GDestroyNotify RDestroyNotify;
typedef GEqualFunc REqualFunc;
typedef GHashFunc RHashFunc;
typedef GFunc RFunc;

typedef GHashTable RHashTable;

inline static unsigned int reactor_str_hash(const char *str){
    return g_str_hash(str);
}

inline static RHashTable* reactor_hash_table_new(   RHashFunc hash, 
                                                    REqualFunc equal){
    return g_hash_table_new((GHashFunc) hash, (GEqualFunc) equal);   
}

inline static void reactor_hash_table_insert(RHashTable *ht, void *key, void *value){
    g_hash_table_insert(ht, key, value);
}

inline static void reactor_hash_table_destroy(RHashTable *ht){
    g_hash_table_destroy((GHashTable *) ht);
}

inline static unsigned int reactor_hash_table_size(RHashTable *ht){
    return g_hash_table_size(ht);
}

inline static void* reactor_hash_table_lookup(RHashTable *ht, const void *key){
    return g_hash_table_lookup(ht, key);
}

typedef GSList RSList;

inline static RSList* reactor_slist_prepend(RSList *rsl, void *data){
    return g_slist_prepend(rsl, data);
}

inline static void reactor_slist_free(RSList *rsl){
    g_slist_free(rsl);
}

inline static void reactor_slist_free_full(RSList *rsl, RDestroyNotify free_func){
    g_slist_free_full(rsl, (RDestroyNotify) free_func);
}

inline static void reactor_slist_foreach(RSList *rsl, RFunc func, void *user_data){
    g_slist_foreach(rsl, func, user_data);
}

inline static RSList* reactor_slist_next(RSList *rsl){
    return g_slist_next(rsl);
}

inline static RSList* reactor_slist_remove(RSList *rsl, void *data){
    return g_slist_remove(rsl, data);
}

inline static RSList* reactor_slist_remove_all(RSList *rsl, void *data){
    return g_slist_remove_all(rsl, data);
}
// typedef GList RList;
// 
// inline static RList* reactorslist_prepend(RList *rl, void *data){
//     return g_list_prepend(rl, data);
// }

/* log */

#define FACILITY LOG_DAEMON
#define MAX_LENGTH 256

#ifdef __GNUC__
#define PRINTF_ATTR(n, m) __attribute__ ((format (printf, n, m)))
   
#else /* !__GNUC__ */
#define PRINTF_ATTR(n, m)

#endif /* !__GNUC__ */

void info(const char *format, ...) PRINTF_ATTR(1, 2);
void warn(const char *format, ...) PRINTF_ATTR(1, 2);
void err(const char *format, ...) PRINTF_ATTR(1, 2);
void die(const char *format, ...) PRINTF_ATTR(1, 2);
void close_log(void);

#ifdef DEBUG

void l_debug(const char *format, ...) PRINTF_ATTR(1, 2);
void l_debug_e(const char *format, ...) PRINTF_ATTR(1, 2);

#define dbg(format, ...) l_debug("%s:%d:%s(): " format, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__)
#define dbg_e(format, ...) l_debug_e("%s:%d:%s(): " format, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__)

#else/* !DEBUG */
#define dbg(format, ...) (void)(0)
#define dbg_e(format, ...) (void)(0)

#endif/* !DEBUG */

/* eventnotice.c */

typedef struct _eventnotice EventNotice;

EventNotice* en_new(const char* id);
bool en_free(EventNotice *en);
void en_add_curr_trans(EventNotice *en, Transition *trans);
void en_clear_curr_trans(EventNotice *en);
const char* en_get_id(EventNotice *en);
void en_add_transpointer(EventNotice *en);
const RSList* en_get_currtrans(EventNotice *en);
void en_remove_one_curr_trans(EventNotice *en, Transition *trans);
/* state.c */

typedef struct _state State;

State* state_new(const char* id);
bool state_free(State *ste);
void state_add_trans(State *ste, Transition *trans);
const char* state_get_id(State *ste);
void state_add_transpointer(State *ste);
Transition* state_get_trans(State *ste);
void state_set_fsminitial(State *ste, Transition *fsminitial);
Transition* state_get_fsminitial(State *ste);
/* transition.c */

typedef enum _actiontypes{
    CMD_ACTION,
    CMD_NONE
}ActionTypes;
typedef struct _cmdaction CmdAction;

Transition* trans_new(State *dest);
bool trans_set_cmd_action(Transition *trans, const char *cmd, const char *shell, int uid);
void trans_set_cmd_none(Transition *trans);
void trans_free(Transition *trans);
bool trans_notice_event(Transition *trans);
void trans_add_requisite(Transition *trans, EventNotice *en);
const State* trans_get_dest(Transition *trans);
const RSList* trans_get_enrequisites(Transition *trans);
Transition* trans_clist_merge(Transition* clist1, Transition* clist2);
Transition* trans_clist_remove_link(Transition* trans);
void trans_clist_free_full(Transition* trans);
Transition* trans_clist_next(Transition *clist);
void trans_clist_clear_curr_trans(Transition *clist);

#endif