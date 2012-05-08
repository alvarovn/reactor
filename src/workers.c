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

#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdlib.h>

#include "reactor.h"

void free_infos(struct rw_info *info){
    if(info == NULL) return;
    free(info->name);
    free(info->next);
    free(info);
}

void free_worker(struct r_worker *worker){
    if(worker == NULL) return;
    dlclose(worker->modhandler);
    free(worker->pt);
    free(worker);
}

static int workers_event_handler(char *eid){
    struct reactor_d *reactor;
    
    reactor = get_reactor();
    return reactor_event_handler(reactor, eid);
}

static struct rw_services* get_worker_services(){
    struct rw_services *serv;
    if((serv = (struct rw_services *)calloc(1, sizeof(struct rw_services *))) == NULL){
        dbg_e("Error on malloc() a new worker services structure", NULL);
        return NULL;
    }
    serv->eventhandler = workers_event_handler;
    serv->version.major = RW_MAJOR_VERSION;
    serv->version.minor = RW_MINOR_VERSION;
    
    return serv;
}

int load_module(const char *modpath, RSList *workers){
    int ret = 0;
    struct r_worker *rwkr = NULL;
    void *modhandler;
    struct rw_services *serv;
    char *erro;
    RWInitFunc initfunc;
    struct rw_info  *info = NULL,
                    *infop;
        
    if((serv = get_worker_services()) == NULL){
        err("Unable to load '%s' module", modpath);
        ret = -1;
        goto end;
    }
    modhandler = dlopen(modpath, RTLD_LAZY);
    erro = dlerror();
    if(modhandler == NULL){
        warn("Cannot open the reactor worker module '%s', probably it's a permissions issue", modpath);
        dbg("dlopen() error: %s", erro);
        ret = -1;
        goto end;
    }
    (void) dlerror();
    erro = NULL;
    initfunc = (RWInitFunc)dlsym(modhandler, "rw_init_plugin");
    erro = dlerror();
    if(erro != NULL)
        goto invalid;
    if((info = initfunc(serv)) == NULL){
        goto invalid;
    }
    infop = info;
    while(infop != NULL){
//         TODO Errors here will make the function return -1 but it added new workers. Fix it.
        if(reactor_hash_table_lookup(workers, infop->name) != NULL){
            dbg("Trying to load an already load worker", NULL);
            continue;
        }
        if((rwkr = (struct r_worker *)calloc(1, sizeof(struct r_worker))) == NULL){
            dbg_e("Error on malloc() a new worker", NULL);
            err("Unable to load '%s' module", modpath);
            ret = -1;
            goto end;
        }
        rwkr->modhandler = modhandler;
        rwkr->mainfunc = infop->mainfunc;
        rwkr->initfunc = initfunc;
        rwkr->name = infop->name;
        if(pthread_create(&(rwkr->pt), NULL, rwkr->mainfunc, (void *) serv) != 0){
            warn("Cannot run the reactor worker");
            dbg_e("pthread_create() error", NULL);
            free_worker(rwkr);
            continue;
        }
        reactor_hash_table_insert(workers, info->name, rwkr);
        infop = infop->next;
    }
end:
    // TODO Those frees should be on a public free_infos() or something like that
    free_infos(info);
    return ret;
invalid:
    warn("'%s' is not a valid reactor worker module", modpath);
    dbg("dlsym() error: %s", erro);
    free_worker(rwkr);
    return NULL;
}

int load_all_modules(const char *workerdir, RSList *workers){
    DIR *dir;
    struct dirent* d;
    char abspath[PATH_MAX];
    int namel,
        ret = 0;
    
    dir = opendir(workerdir);
    
    if(dir == NULL){
        if(errno == ENOENT) 
//             The directory doesn't exist, so there are no modules
            return;
        warn("Cannot open the workers directory '%s', probably it's a permissions issue",
            workerdir);
        dbg_e("opendir() error", NULL);
        ret = -1;
        goto end;
    }
    /* TODO By now this only accepts '.so' modules. It should be able to open HP-UX '.sl'
     * if needed. 
     */
    for(;;){
        errno = 0;
        d = readdir(dir);
        if (d == NULL)
            break;
        namel = strlen(d->d_name);
        if((strcmp(&(d->d_name[namel-3]), ".so") != 0) || (d->d_type != DT_REG))
            continue;
        sprintf(&abspath, "%s/%s", workerdir, d->d_name);
        ret = load_module(&abspath, workers);
    }
end:
    return ret;
}