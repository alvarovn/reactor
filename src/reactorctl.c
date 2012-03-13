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

#include "reactord.h"

Cntrl *cntrl;

/* By now this program only executes tests */
int main(int argc, char *argv[]) {
    char *ens[] = { "event1",
                    NULL
    };
    CntrlMsg msgs[6];
    CntrlMsgType cmt;
        
    msgs[0].cmt = ADD_TRANSITION;
    msgs[0].cm = (void *)calloc(1, sizeof(AddTransMsg));
    ((AddTransMsg *)msgs[0].cm)->action =  "echo \"De 'init' a 'A'\" >> /home/lostevil/toma_ya";
    ((AddTransMsg *)msgs[0].cm)->enids = ens;
    ((AddTransMsg *)msgs[0].cm)->to = "A";
    ((AddTransMsg *)msgs[0].cm)->from = "";
    msgs[1].cmt = ADD_TRANSITION;
    msgs[1].cm = (void *)calloc(1, sizeof(AddTransMsg));
    ((AddTransMsg *)msgs[1].cm)->action =  "echo \"De 'A' a 'B'\" >> /home/lostevil/toma_ya";
    ((AddTransMsg *)msgs[1].cm)->enids = ens;
    ((AddTransMsg *)msgs[1].cm)->to = "B";
    ((AddTransMsg *)msgs[1].cm)->from = "A";
    msgs[2].cmt = ADD_TRANSITION;
    msgs[2].cm = (void *)calloc(1, sizeof(AddTransMsg));
    ((AddTransMsg *)msgs[2].cm)->action =  "echo \"De 'A' a 'C'\" >> /home//toma_ya";
    ((AddTransMsg *)msgs[2].cm)->enids = ens;
    ((AddTransMsg *)msgs[2].cm)->to = "C";
    ((AddTransMsg *)msgs[2].cm)->from = "A";
    msgs[3].cmt = REACTOR_EVENT;
    msgs[3].cm = (void *)calloc(1, sizeof(ReactorEventMsg));
    ((ReactorEventMsg *)msgs[3].cm)->eid = "event1";
    msgs[4].cmt = REACTOR_EVENT;
    msgs[4].cm = (void *)calloc(1, sizeof(ReactorEventMsg));
    ((ReactorEventMsg *)msgs[4].cm)->eid = "event1";
    msgs[5].cmt = REACTOR_EVENT;
    msgs[5].cm = (void *)calloc(1, sizeof(ReactorEventMsg));
    ((ReactorEventMsg *)msgs[5].cm)->eid = "event1";
    
    int i;
    for(i=0;i<6;i++){
        cntrl = cntrl_new(false);
        if(cntrl_connect(cntrl) == -1){
            return 1;
        }
        if((cmt = (CntrlMsgType)cntrl_send_msg(cntrl, &msgs[i])) != ACK){
            fprintf(stderr, "NACK! %i\n", (int)cmt);
        }
        cntrl_peer_close(cntrl);
        cntrl_free(cntrl);
    }
    
    return 0;
}