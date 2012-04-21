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

#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "tests.h"
#include "cntrl.c"
#include "libreactor.h"

#define EID             "e1"
#define READ_EVENT      0
#define READ_BAD_EVENT  1
#define READ_ERROR      2

struct r_msg eventmsg = {{3, EVENT}, EID};
struct r_msg badeventmsg = {{4, EVENT}, EID};
void *msgp;
// int sfd;
// 
// void setup(void){
//     sfd = listen_cntrl();
// }
// 
// void teardown(void){  
//     close_cntrl(sfd);
// }

ssize_t reactor_read(int fd, void *buf, size_t count){
    struct r_msg *msg = NULL;
    int msgsize = 0;
    int leastsize = 0;
    switch(fd){
        case READ_EVENT:
            msg = &eventmsg;
            break;
        case READ_BAD_EVENT:
            msg = &badeventmsg;
            break;
        case READ_ERROR:
            return -1;
    }
    if(msgp = NULL) 
        msgp = (void *) msg;
    msgsize = sizeof(*msg);
    leastsize = msgsize - ((int) msgp - (int) msg);
    if(count > leastsize) 
        count = leastsize;
    memcpy(buf, msgp, count);
    return count;
}

START_TEST(test_connection){
    int sfd = listen_cntrl();
    int psfd;
    fail_unless(sfd >= 0); 
    psfd = connect_cntrl();
    fail_unless(psfd >= 0);
    close(psfd);
    close_cntrl(sfd);
}
END_TEST

START_TEST(test_receive_normal){
    struct r_msg *rmsg;
    
    msgp = NULL;
    rmsg = receive_cntrl_msg(READ_EVENT);
    fail_unless(strcmp(rmsg->msg, EID));
    free(rmsg->msg);
    free(rmsg);
}
END_TEST

START_TEST(test_receive_short){
    struct r_msg *rmsg;
    
    msgp = NULL;
    rmsg = receive_cntrl_msg(READ_EVENT);
    fail_unless(rmsg != NULL);
    free(rmsg->msg);
    free(rmsg);
}
END_TEST

START_TEST(test_receive_error){
    struct r_msg *rmsg;
    
    msgp = NULL;
    rmsg = receive_cntrl_msg(READ_ERROR);
    fail_unless(rmsg != NULL);
    free(rmsg->msg);
    free(rmsg);
}
END_TEST

Suite* make_cntrl_suite(void){
    Suite *s = suite_create("Cntrl");

    /* Core test case */
    TCase *tc_core = tcase_create("Core");
//   tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_connection);
    tcase_add_test(tc_core, test_receive_normal);
    tcase_add_test(tc_core, test_receive_short);
    suite_add_tcase(s, tc_core);

  return s;
}