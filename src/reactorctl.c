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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include "reactord.h"

#define TRANS_COUNT 6

/* By now this program only executes tests */
int main(int argc, char *argv[]) {
    Cntrl *cntrl;
    int opt, optindex;
    enum rmsg_type mtype;
    const struct option options[] = {
        { "event", required_argument, NULL, 'e' },
        { "rule", required_argument, NULL, 'r' }
    };
    
    const char *optstring = "e:r:";
    struct r_msg msg;
    
    cntrl = cntrl_new(false);
    if(cntrl_connect(cntrl) == -1){
        warn("reactord is not running. Start reactord.");
        return 1;
    }
    for(opt = getopt_long( argc, argv, optstring, options, &optindex );
        opt != -1;
        opt = getopt_long( argc, argv, optstring, options, &optindex )){
            switch(opt){
                case 'e':
                    msg.hd.mtype = REACTOR_EVENT;
                    break;
                case 'r':
                    msg.hd.mtype = RULE;
                    break;
            }
            msg.msg = strdup(optarg);
            msg.hd.size = strlen(msg.msg) + 1;

            mtype = cntrl_send_msg(cntrl, &msg);
            switch(mtype){
                case RULE_NOFROM:
                    warn("Origin state must exist and it doesn't. The transition won't be added.");
                    break;
                case RULE_MULTINIT:
                    warn("Trying to set multiple initial transitions to the same state machine. The transition won't be added.");
                    break;
                case RULE_MALFORMED:
                    warn("Rule malformed.");
                    break;
                case ACK:
                    break;
                default:
                    warn("reactord is not working properly.");
                    break;
            }
            cntrl_peer_close(cntrl);
            cntrl_free(cntrl);
        }
    return 0;
}