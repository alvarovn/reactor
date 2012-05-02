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

#ifndef LIBREACTOR_PRIVATE_INCLUDED
#define LIBREACTOR_PRIVATE_INCLUDED

#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <glib.h>
#include <sys/un.h>

#include "libreactor.h"

#define R_EXPORT __attribute__ ((visibility("default")))

#ifdef DEBUG
// A place where the permissions are not a problem for users
#define SOCK_PATH "/tmp/rctl.sock"
#else
#define SOCK_PATH "/var/run/rctl.sock"
#endif

#define MAX_ZERO_WRITES 5

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

inline static bool reactor_hash_table_remove(RHashTable *hash_table, const void *key){
    return (bool) g_hash_table_remove((GHashTable*) hash_table, (gconstpointer) key);
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

inline static RSList* reactor_slist_delete_link(RSList *list, RSList *link_){
    return g_slist_delete_link(list, link_);
}

inline static void reactor_slist_free_full(RSList *rsl, RDestroyNotify free_func){
    while(rsl != NULL){
        free_func(rsl->data);
        rsl = reactor_slist_delete_link(rsl, rsl);
    }
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

/* utils.c */
// syscall wrappers for unit testing
// TODO Can this be done inline?
ssize_t reactor_read(int fd, void *buf, size_t count);
ssize_t reactor_write(int fd, void *buf, size_t count);

#endif