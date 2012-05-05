/*
    This file is part of reactor->

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

#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>

#include "reactor.h"

/* TODO By now this only accepts '.so' modules. It should be able to open HP-UX '.sl'
 * if needed. 
 */

void free_worker(struct r_worker *worker){
    if(worker == NULL) return;
    dlclose(worker->modhandlr);
    free(worker);
}

static void init_worker(struct reactor_d* reactor, const char *modpath){
    struct r_worker *rwkr;
    char *erro;
    
    if((rwkr = (struct r_worker *)calloc(1, sizeof(struct r_worker))) == NULL){
        dbg_e("Error on malloc() a new worker", NULL);
        err("Unable to load '%s' worker", modpath);
        goto end;
    }
    
    rwkr->modhandlr = dlopen(modpath, RTLD_LAZY);
    if(rwkr->modhandlr == NULL){
        warn("Cannot open the reactor worker module '%s', probably it's a permissions issue", modpath);
        dbg("dlopen() error: %s", dlerror());
        goto end;
    }
    (void) dlerror();
    rwkr->wkrname = (char *)dlsym(rwkr->modhandlr, WORKER_NAME);
    erro = dlerror();
    if(erro != NULL)
        goto invalid;
    if(reactor_hash_table_lookup(reactor->workers, rwkr->wkrname) != NULL){
        dbg("Trying to initialize an already initialized worker", NULL);
        free_worker(rwkr);
        goto end;
    }
    rwkr->wkr_main_thread = (WkrMainThreadFunc) dlsym(rwkr->modhandlr, WORKER_MAIN_THREAD);
    if(erro != NULL)
        goto invalid;
    if(pthread_create(rwkr->wkrpt, NULL, rwkr->wkr_main_thread, NULL) != 0){
         warn("Cannot run the reactor worker");
         dbg_e("pthread_create() error", NULL);
         free_worker(rwkr);
         goto end;
     }
     reactor_hash_table_insert(reactor->workers, rwkr->wkrname, (void *)rwkr);
end:
    return;
invalid:
    free_worker(rwkr);
    warn("'%s' is not a valid reactor worker module", modpath);
    dbg("dlsym() error: %s", err);
    return;
}

void init_workers(struct reactor_d *reactor){
    DIR *dir;
    struct dirent* d;
    char abspath[PATH_MAX];
    int namel;
    
    dir = opendir(PKGLIBDIR);
    
    if(dir == NULL){
        warn("Cannot open the workers directory '%s', probably it's a permissions issue",
            PKGLIBDIR);
        dbg_e("opendir() error", NULL);
        return;
    }
    
    for(;;){
        errno = 0;
        d = readdir(dir);
        if (d == NULL)
            break;
        namel = strlen(d->d_name);
        if((strcmp(&(d->d_name[namel-3]), ".so") != 0) && (d->d_type != DT_REG))
            continue;
        sprintf(&abspath, "%s/%s", PKGLIBDIR, d->d_name);
        init_worker(reactor, &abspath);
    }
}