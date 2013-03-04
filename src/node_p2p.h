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

struct obj_nodes {
	LIST *list;
	HASH *hash;
};
typedef struct obj_nodes NODES;

struct obj_node {
	IP c_addr;

	UCHAR id[SHA_DIGEST_LENGTH];

	time_t time_ping;
	time_t time_find;
	int pinged;
};
typedef struct obj_node NODE;

NODES *nodes_init( void );
void nodes_free( void );

NODE *node_put( UCHAR *id, IP *sa );
void node_del( ITEM *i );

void node_update_address( NODE *node, IP *sa );

void node_pinged( UCHAR *id );
void node_ponged( UCHAR *id, IP *sa );

void node_expire( void );
long int node_counter( void );

int node_me( UCHAR *node_id );
int node_equal( const UCHAR *node_a, const UCHAR *node_b );
