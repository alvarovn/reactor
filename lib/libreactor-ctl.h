/*
    This file is part of reactor.

    Copyright (C) 2011  Álvaro Villalba Navarro <vn.alvaro@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef LIBREACTORCTL_INCLUDED
#define LIBREACTORCTL_INCLUDED

#include <string.h>
#include <stdbool.h>


static inline bool str_eq(const char *s1, const char *s2){
    return !strcmp(s1, s2);
}

/* cntrl.c */
enum rmsg_type{
    /* to server */
    EVENT,
    ADD_RULE,
    RM_TRANS,
    EOM,
    /* from server */
    ACK,
    RULE_MULTINIT,
    ARG_MALFORMED,
    NO_TRANS
};

struct rmsg_hd{
    int size;
    enum rmsg_type mtype;
};

struct r_msg{
    struct rmsg_hd hd;
    char *msg;
};

int listen_cntrl();
int connect_cntrl();
int send_cntrl_msg(int psfd, const struct r_msg *msg);
struct r_msg* receive_cntrl_msg(int psfd);
void close_cntrl(int sfd); 
#endif