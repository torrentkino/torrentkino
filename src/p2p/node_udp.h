/*
Copyright 2011 Aiko Barz

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

#ifndef NODE_UDP_H
#define NODE_UDP_H

#include "conf.h"
#include "token.h"

typedef struct {
	UCHAR id[SHA1_SIZE];
	IP c_addr;
	time_t time_ping;
	time_t time_find;
	int pinged;
} UDP_NODE;

UDP_NODE *node_init( UCHAR *node_id, IP *sa );
void node_free( UDP_NODE *n );

void node_update( UDP_NODE *n, IP *sa );

int node_me( UCHAR *node_id );
int node_equal( const UCHAR *node_a, const UCHAR *node_b );

int node_ok( UDP_NODE *n );
int node_bad( UDP_NODE *n );

void node_pinged( UDP_NODE *n );
void node_ponged( UDP_NODE *n, IP *from );

#endif /* NODE_UDP_H */
