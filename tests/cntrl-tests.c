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
#include <arpa/inet.h>

#include "tests.h"
#include "cntrl.c"
#include "libreactor.h"

#define EID             "ev1"
#define READ_CORRECT    0
#define READ_SHORT      1
#define READ_ERROR      2

#define WRITE_CORRECT   3
#define WRITE_SHORT     4
#define WRITE_ERROR     5


struct rmsg_event{
    int size;
    int mtype;
    char msg[4];
};
struct rmsg_ack{
    int size;
    int mtype;
    char msg[1];
};

struct rmsg_event eventmsg;
struct rmsg_event badeventmsg;
struct rmsg_ack ackmsg;

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
/*
 * read() mock
 */
ssize_t reactor_read(int fd, void *buf, size_t count){
    struct r_msg *msg = NULL;
    int msgsize = 0;
    int leastsize = 0;
    switch(fd){
        case READ_CORRECT:
            msg = &eventmsg;
            break;
        case READ_SHORT:
            msg = &badeventmsg;
            break;
        case READ_ERROR:
            return -1;
        default:
            msg = &ackmsg;
    }
    if(msgp == NULL) 
        msgp = (void *) msg;
    msgsize = sizeof(*msg);
    leastsize = msgsize - (int) (msgp - (void *) msg);
    if(count > leastsize) 
        count = leastsize;
    memcpy(buf, msgp, count);
    msgp+= count;
    return count;
}

/*
 * write() mock
 */
ssize_t reactor_write(int fd, void *buf, size_t count){
    switch(fd){
        case WRITE_CORRECT:
            break;
        case WRITE_SHORT:
            count = count > 0 ? count -1 : count;
        case WRITE_ERROR:
            return -1;
    }
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
    rmsg = receive_cntrl_msg(READ_CORRECT);
    fail_unless( strcmp(rmsg->msg, EID) == 0 );
    if(rmsg != NULL){
        free(rmsg->msg);
        free(rmsg);
    }
}
END_TEST

START_TEST(test_receive_short){
    struct r_msg *rmsg;
    
    msgp = NULL;
    rmsg = receive_cntrl_msg(READ_SHORT);
    fail_unless(rmsg == NULL);
    if(rmsg != NULL){
        free(rmsg->msg);
        free(rmsg);
    }
}
END_TEST

START_TEST(test_receive_error){
    struct r_msg *rmsg;
    
    msgp = NULL;
    rmsg = receive_cntrl_msg(READ_ERROR);
    fail_unless(rmsg == NULL);
    if(rmsg != NULL){
        free(rmsg->msg);
        free(rmsg);
    }
}
END_TEST


Suite* make_cntrl_suite(void){
    int msgsize;
    int msgtype;
    Suite *s = suite_create("Cntrl");

    /* Core test case */


    msgsize = htonl((u_int32_t) 4);
    msgtype = htonl((u_int32_t) EVENT);
    eventmsg.size = msgsize;
    eventmsg.mtype = msgtype;
    strcpy(eventmsg.msg, strdup(EID));

    msgsize = htonl((u_int32_t) 5);
    badeventmsg.size = msgsize;
    badeventmsg.mtype = msgtype;
    strcpy(badeventmsg.msg, EID);
    
    msgsize = htonl((u_int32_t) 0);
    msgtype = htonl((u_int32_t) ACK);
    ackmsg.size = msgsize;
    ackmsg.mtype = msgtype;
    ackmsg.msg[0] = '\0';

    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_connection);
    tcase_add_test(tc_core, test_receive_normal);
    tcase_add_test(tc_core, test_receive_short);
    
    suite_add_tcase(s, tc_core);

  return s;
}