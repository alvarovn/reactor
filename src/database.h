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

#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#include <glib.h>

#include "eventnotice.h"

GHashTable* eventnotices;
GHashTable* states;

void notice_event(const char* id);
char* get_db_representation(int);
int add_trans(char* action, char* enids, char* to, char* from);
int add_init_trans(char* action, char* enids, char* to);

#endif