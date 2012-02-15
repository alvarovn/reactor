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

#include <glib.h>
#include <string.h>

#define EXIT_FAILURE 1

static inline bool str_eq(const char *s1, const char *s2){
    return !strcmp(s1, s2);
}

/* basic third party data structures wrappers */

typedef GHashTable RHashTable;

static inline unsigned int reactor_str_hash(const char *str){
    return g_str_hash(str);
}

static inline RHashTable* reactor_hash_table_new(   unsigned int (*hash)(const void*), 
                                                    bool (*equal)(const void*, const void*)){
    return g_hash_table_new(hash, equal);   
}

static inline void reactor_hash_table_insert(RHashTable *ht, void *key, void *value){
    g_hash_table_insert(ht, key, value);
}

static inline void reactor_hash_table_destroy(RHashTable *ht){
    g_hash_table_destroy(ht);
}

static inline unsigned int reactor_hash_table_size(RHashTable *ht){
    return g_hash_table_size(ht);
}


typedef GSList RSList;

static inline RSList* reactor_slist_prepend(RSList *rsl, void *data){
    return g_slist_prepend(rsl, data);
}

static inline void reactor_slist_free(RSList *rsl){
    g_slist_free(rsl);
}

static inline void reactor_slist_free_full(RSList *rsl, void (*free_func)(void *)){
    g_slist_free_full(rsl, free_func);
}

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

typedef EventNotice;

static inline void en_add_curr_trans(EventNotice *en, Transition *trans){
    en->currtrans = reactor_slist_prepend(en->currtrans, trans);
}

static inline void en_clear_curr_trans(EventNotice *en){
    reactor_slist_free(en->currtrans);
}

static inline const char* en_get_id(EventNotice *en){
    return en->id;
}

/* transition.c */

typedef Transition;

typedef enum _actiontypes{
    CMD_ACTION
}ActionTypes;

static inline void trans_add_requisite(Transition *trans, EventNotice *en){
    reactor_hash_table_insert(trans->enrequisites, en_get_id(en), en);
}

/* state.c */

typedef State;

static inline void state_add_trans(State *ste, Transition *trans){
    ste->currtrans = reactor_slist_prepend(ste->currtrans, trans);
}

static inline const char* state_get_id(State *ste){
    return ste->id;
}

/* cfg.c */

typedef Cfg;


#endif