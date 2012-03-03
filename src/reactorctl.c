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

int send_atm(char *action, char **enids, char *to, char *from){
   int error;
   error = 0;
   return error;
}

int send_rem(char *eid){
    int error;
    error = 0;
    return error;
}

/* By now this program only executes tests */

int main(int argc, char *argv[]) {
    char *ens[] = { "event1",
                     "event2",
                     "event3",
                     "event4",
                     NULL
    };
                   
    send_atm("echo \"De 'A' a 'B'\" >> /home/lostevil/toma_ya", ens, "A", "B");
    send_rem("event1");
    send_rem("event14");
    send_rem("event2");
    send_rem("event3");
    send_rem("event4");
    send_rem("event5");
}