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

#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#include <sys/types.h>

#define SOCK_PATH "/tmp/rcrtlsock"

typedef struct _addtransmsg{
    char *action;
    char **enids; 
    char *to; 
    char *from;
}AddTransMsg;

typedef struct _reactoreventmsg{
    char *eid;
    int uid;
    /* In the future this field may contain more information about the font 
     * than the pid.
     */ 
    pid_t fontpid;  
} ReactorEventMsg;

/* cntrl.c */
typedef enum _cntrlmsgtype{
    REACTOR_EVENT,
    ADD_TRANSITION
}CntrlMsgType;

typedef struct _cntrlheader{
    int size;
    CntrlMsgType cmt;
}CntrlHeader;

typedef struct _cntrlmsg{
    CntrlMsgType cmt;
    void *cm;
}CntrlMsg;

typedef struct _cntrl Cntrl;

CntrlMsg* cntrl_get_msg(Cntrl *cntrl);
void cntrl_free(Cntrl *cntrl);
Cntrl* cntrl_new();
int cntrl_listen(Cntrl* cntrl);
int cntrl_get_fd(Cntrl *cntrl);
void cntrl_atm_free(AddTransMsg *atm);
void cntrl_rem_free(ReactorEventMsg *rem);

/* user.c */
typedef struct _user User;
struct _user{
    char *name;
    User *next;
};

User* load_users(const char*);
void free_users(User*);

#endif