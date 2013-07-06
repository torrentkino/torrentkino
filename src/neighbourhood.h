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

struct obj_nbhd {
	LIST *bucket;
	HASH *hash;
};
typedef struct obj_nbhd NBHD;

struct obj_node {
	IP c_addr;

	UCHAR id[SHA_DIGEST_LENGTH];

	UCHAR token[TOKEN_SIZE_MAX];
	int token_size;
	int token_done;

	time_t time_ping;
	time_t time_find;

	int pinged;
};
typedef struct obj_node NODE;

NBHD *nbhd_init( void );
void nbhd_free( NBHD *nbhd );

void nbhd_put( NBHD *nbhd, UCHAR *id, IP *sa );
void nbhd_del( NBHD *nbhd, NODE *n );

void nbhd_update_address( NODE *node, IP *sa );

void nbhd_pinged( UCHAR *id );
void nbhd_ponged( UCHAR *id, IP *sa );

void nbhd_expire( void );
void nbhd_split( NBHD *nbhd, UCHAR *target );

int nbhd_is_empty( NBHD *nbhd );
int nbhd_me( UCHAR *node_id );
int nbhd_equal( const UCHAR *node_a, const UCHAR *node_b );

int nbhd_conn_from_localhost( IP *from );
