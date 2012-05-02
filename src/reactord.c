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

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <event.h>
#include <pwd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "reactor.h"

struct reactor_d reactor;

// TODO signal handlers

/* libevent control socket callbacks */

// static void attend_cntrl_msg_thread(void *arg){
//     int psfd;
//     int sfd = *((int *) arg);
//     free((int *) arg);
//     struct r_msg response;
//     struct r_msg *msg;
//     void *data;
//     response.hd.mtype = ACK;
//     response.hd.size = 0;
//     response.msg = NULL;
//     psfd = accept(sfd, NULL, NULL);
//     if((msg = receive_cntrl_msg(psfd)) == NULL){
//         err("Error in the communication with 'reactorctl'");
//         goto end;
//     }
//     switch(msg->hd.mtype){
//         case EVENT:
//             reactor_event_handler(msg->msg);
//             send_cntrl_msg(psfd, &response);
//             free(msg->msg);
//             break;
//         case ADD_RULE:
//             /* TODO Change uid to the user who sent the rule */
//             data = (void *) rule_parse(msg->msg, 0);
//             response.hd.mtype = reactor_add_rule_handler((struct r_rule *) data);
//             rules_free((struct r_rule *) data);
//             send_cntrl_msg(psfd, &response);
//             free(msg->msg);
//             break;
//         case RM_TRANS:
//             response.hd.mtype = reactor_rm_trans_handler(msg->msg);
//             send_cntrl_msg(psfd, &response);
//             free(msg->msg);
//         default:
//             break;
//     }
// end:
//     close(psfd);
//     
//     free(msg);
// }

static void attend_cntrl_msg(int sfd, short ev, void *arg){
//     int s,
//         *nsfd;
//     pthread_t t1;
//     if((nsfd = malloc(sizeof(int))) == NULL){
//         dbg_e("Error on malloc() the control socket file descriptor", NULL);
//         return;
//     }
//     *nsfd = sfd;
//     s = pthread_create(&t1, NULL, attend_cntrl_msg_thread, (void *) nsfd);
//     if(s != 0)
//         dbg("Unable to create the thread to receive control messages", strerror(s));
    int psfd;
//     int sfd = *((int *) arg);
//     free((int *) arg);
    struct r_msg response;
    struct r_msg *msg;
    void *data;
    response.hd.mtype = ACK;
    response.hd.size = 0;
    response.msg = NULL;
    psfd = accept(sfd, NULL, NULL);
    if((msg = receive_cntrl_msg(psfd)) == NULL){
        err("Error in the communication with 'reactorctl'");
        goto end;
    }
    switch(msg->hd.mtype){
        case EVENT:
            reactor_event_handler(&reactor, msg->msg);
            send_cntrl_msg(psfd, &response);
            free(msg->msg);
            break;
        case ADD_RULE:
            /* TODO Change uid to the user who sent the rule */
            data = (void *) rule_parse(msg->msg, NULL, -1, 0);
            response.hd.mtype = reactor_add_rule_handler(&reactor, (struct r_rule *) data);
            r_rules_free((struct r_rule *) data);
            send_cntrl_msg(psfd, &response);
            free(msg->msg);
            break;
        case RM_TRANS:
            response.hd.mtype = reactor_rm_trans_handler(&reactor, msg->msg);
            send_cntrl_msg(psfd, &response);
            free(msg->msg);
        default:
            break;
    }
end:
    close(psfd);
    
    free(msg);
}

// static attend_remote_events_thread(void *arg){
//     struct sockaddr addr;
//     char ip[INET6_ADDRSTRLEN];
//     int sfd = *((int *) arg);
//     int psfd,
//         addrlen = sizeof(struct sockaddr);
//     RSList *eids;
//     RSList *eidsp;
//     
//     free((int *)arg);
//     ip[0] = NULL;
//     if((psfd = accept(sfd, &addr, &addrlen)) == -1){
//         dbg_e("Unable to stablish connection with remote reactord", NULL);
//     }
//     else inet_ntop(addr.sa_family, &addr, ip, INET6_ADDRSTRLEN);
//     
//     if((eids = receive_remote_events(psfd)) == NULL){
//         goto end;
//     }
//     for(eidsp = eids; eidsp != NULL; eidsp = reactor_slist_next(eidsp)){
//         reactor_event_handler(eidsp->data);
//     }
// end:
//     while(eids != NULL){
//         free(eids->data);
//         eids = reactor_slist_delete_link(eids, eids);
//     }
//     close(psfd);
// }

static void attend_remote_events(int sfd, short ev, void *arg){
//     int s,
//         *nsfd;
//     pthread_t t1;
//     if((nsfd = malloc(sizeof(int))) == NULL){
//         dbg_e("Error on malloc() the remote socket file descriptor", NULL);
//         return;
//     }
//     *nsfd = sfd;
//     s = pthread_create(&t1, NULL, attend_remote_events_thread, (void *) nsfd);
//     if(s != 0)
//         dbg("Unable to create the thread to receive the events", strerror(s));
    struct sockaddr addr;
    char ip[INET6_ADDRSTRLEN];
    int psfd,
        addrlen = sizeof(struct sockaddr);
    RSList *eids;
    RSList *eidsp;
    
    ip[0] = NULL;
    if((psfd = accept(sfd, &addr, &addrlen)) == -1){
        dbg_e("Unable to stablish connection with remote reactord", NULL);
    }
    else inet_ntop(addr.sa_family, &addr, ip, INET6_ADDRSTRLEN);
    
    if((eids = receive_remote_events(psfd)) == NULL){
        goto end;
    }
    for(eidsp = eids; eidsp != NULL; eidsp = reactor_slist_next(eidsp)){
        reactor_event_handler(&reactor, eidsp->data);
    }
end:
    while(eids != NULL){
        free(eids->data);
        eids = reactor_slist_delete_link(eids, eids);
    }
    close(psfd);
}

/* TODO We need a way to distinguish states with the same name but from different users */
/* TODO Main admin rules file */
static void init_rules(){
    struct r_user *users;
    struct r_rule *rules;
    char filename[PATH_MAX];
    unsigned int fnln;
    
    for(users = load_users(R_GRP); users != NULL; users = users->next){
        strcpy(filename, users->pw->pw_dir);
        fnln = strlen(users->pw->pw_dir);
        strcpy(&filename[fnln], "/.");
        fnln += strlen("/.");
        strcpy(&filename[fnln], RULES_FILE);
        for(rules = parse_rules_file(filename, users->pw->pw_uid);
            rules != NULL;
            rules = rules->next){
                reactor_add_rule_handler(&reactor, rules);
        }
        r_rules_free(rules);
    }
    
}

int main(int argc, char *argv[]) {
    int error, cntrlsfd, remotesfd;
    struct event cntrlev, remoteev;
    pid_t pid, sid;
    
    error = 0;
    info("Starting the reactor...");
    dbg("...freeing Mars.", NULL);
    // TODO root user check
    // TODO change ruid to euid (root)
    // TODO help
    // TODO version
    // TODO daemon lock file (?)
    // TODO trap signals
    
    /* daemonize */
#ifndef DEBUG
    pid = fork();
    switch(pid){
        case 0:
            /* child */
            dbg("Forked child running.", NULL);
            break;
        case -1:
            err("Unable to daemonize");
            fprintf(stderr, "Unable to daemonize\n");
        default:
            /* parent */
            goto exit;
    }
#endif
    /* redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
    
    // TODO cancel child signals ?
    
//     sid = setsid();
//     if (sid < 0) {
//         error = 1;
//         dbg_e("setsid() returned with errors.", NULL);
//         err("Unable to create a new session.");
//         goto exit;
//     }
    
    /* change current directory */
    if ((chdir("/")) < 0) {
        error = 1;
        err("Unable to change current directory to '/'.");
        goto exit;
    }

    // TODO open an event-listener socket
    // TODO enqueue events
    // TODO open a control socket
    
    reactor.eventnotices = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    reactor.states = reactor_hash_table_new((RHashFunc) reactor_str_hash, (REqualFunc) str_eq);
    init_rules();
    /* sockets setup and poll */
    event_init();
    if((cntrlsfd = listen_cntrl()) == -1){
        err("Unable to create the control socket and bind it to '%s'. Probably a permissions issue.", SOCK_PATH);
        goto exit;
    }
    if((remotesfd = listen_remote()) == -1){
        err("Unable to create the remote socket.");
        goto exit;
    }
    event_set(&cntrlev, cntrlsfd, EV_READ | EV_PERSIST, &attend_cntrl_msg, NULL);
    event_add(&cntrlev, NULL);
    event_set(&remoteev, remotesfd, EV_READ | EV_PERSIST, &attend_remote_events, NULL);
    event_add(&remoteev, NULL);
    event_dispatch();

    
exit:
    close_cntrl(cntrlsfd);
    close(remotesfd);
    return error;
}

