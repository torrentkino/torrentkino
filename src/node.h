/*
Copyright 2011 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NODE_H
#define NODE_H

#include "token.h"

struct obj_node {
	IP c_addr;

	UCHAR id[SHA_DIGEST_LENGTH];

	UCHAR token[TOKEN_SIZE_MAX];
	int token_size;

	time_t time_ping;
	time_t time_find;

	int pinged;
};
typedef struct obj_node NODE;

NODE *node_init( UCHAR *node_id, IP *sa );
void node_free( NODE *n );

void node_update( NODE *node, IP *sa );

int node_me( UCHAR *node_id );
int node_equal( const UCHAR *node_a, const UCHAR *node_b );

#endif
