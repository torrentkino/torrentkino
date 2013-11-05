/*
Copyright 2010 Aiko Barz

This file is part of torrentkino.

torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with torrentkino.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAIN_H
#define MAIN_H

#include "main.h"

struct obj_main {
	char **argv;
	int argc;

	struct obj_conf	*conf;
	struct obj_work	*work;

#ifdef TORRENTKINO
	struct obj_transaction *transaction;
	struct obj_cache *cache;
	struct obj_token *token;
	struct obj_nbhd *nbhd;
	struct obj_udp *udp;
	struct obj_p2p *p2p;
	struct obj_val *value;
#endif
	
#ifdef TUMBLEWEED
	struct obj_list *node;
	struct obj_mdb *mime;
	struct obj_tcp *tcp;
#endif

};

extern struct obj_main *_main;

struct obj_main *main_init( int argc, char **argv );
void main_free( void );

#endif
