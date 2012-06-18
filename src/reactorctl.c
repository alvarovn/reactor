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

#include "reactor.h"

int main(int argc, char *argv[]) {
    int opt, optindex, psfd;
    enum rmsg_type mtype;
    const struct option options[] = {
        { "event", required_argument, NULL, 'e' },
        { "add-rule", required_argument, NULL, 'r' },
        { "remove-transition", required_argument, NULL, 't' }
    };
    
    const char *optstring = "e:a:r:";
    struct r_msg msg;
    
    if((opt = getopt_long( argc, argv, optstring, options, &optindex )) == -1){
        // Here must print help
        fprintf(stderr, "Write an argument\n");
        return 1;
    }
    if((psfd = connect_cntrl()) == -1){
        fprintf(stderr, "Seems that reactord is not running. Start reactord\n");
        return 1;
    }
    for(;
        opt != -1;
        opt = getopt_long( argc, argv, optstring, options, &optindex )
    ){
            switch(opt){
                case 'e':
                    msg.hd.mtype = EVENT;
                    break;
                case 'a':
                    msg.hd.mtype = ADD_RULE;
                    break;
                case 'r':
                    msg.hd.mtype = RM_TRANS;
                    break;
                default:
                    continue;
            }
            msg.msg = strdup(optarg);
            msg.hd.size = strlen(msg.msg) + 1;

            mtype = send_cntrl_msg(psfd, &msg);
            switch(mtype){
                case RULE_MULTINIT:
                    fprintf(stderr, "Trying to set multiple initial rules to the same state machine, the rule won't be added\n");
                    break;
                case ARG_MALFORMED:
                    fprintf(stderr, "Rule malformed\n");
                    break;
                case NO_TRANS:
                    fprintf(stderr, "Transition '%s' doesn't exist\n", msg.msg);
                case ACK:
//                     dbg("Message sent", NULL);
                    break;
                default:
                    fprintf(stderr, "reactord is not working properly\n");
                    break;
            }
            close(psfd);
        }
    return 0;
}