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

#include "reactor.h"
#include "reactord.h"

typedef struct _reactorevent{
    char *msg;
    int uid;
    /* In the future this field may contain more information about the font 
     * than the pid.
     */ 
    pid_t fontpid;  
} ReactorEvent;

RHashTable *eventnotices;
RHashTable *states;

// TODO signal handlers

void notice_event(const ReactorEvent* revent){
    
}

char* get_states_representation(int){
    
}

int add_trans(char* action, char* enids, char* to, char* from){
    
}

int add_init_trans(char* action, char* enids, char* to){
    
}

int main_loop(){
    
}

int main(int argc, char *argv[]) {
    
    pid_t pid, sid;
    
    // TODO root user check
    // TODO change ruid to euid (root)
    // TODO help
    // TODO version
    // TODO daemon lock file
    // TODO trap signals
    
    /* daemonize */
    pid = fork();
    switch(pid){
        case 0:
            break;
        case -1:
            err("Unable to daemonize");
            fprintf(stderr, "Unable to daemonize\n");
        default:
            goto exit;
    }
    
    /* redirect standard files to /dev/null */
    freopen( "/dev/null", "r", stdin);
    freopen( "/dev/null", "w", stdout);
    freopen( "/dev/null", "w", stderr);
    
    // TODO cancel child signals ?
    
    sid = setsid();
    if (sid < 0) {
        err("Unable to create a new session");
        goto exit;
    }
    
    /* change current directory */
    if ((chdir("/")) < 0) {
        err("Unable to change current directory to /");
        goto exit;
    }

    // TODO open an event-listener socket
    // TODO enqueue events
    // TODO load users state machines
    // TODO open a control socket
exit:
}
