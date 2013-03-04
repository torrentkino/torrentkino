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

struct obj_announce {
	LIST *list;
	HASH *hash;
};

struct obj_node_announce {
	LIST *list;
	HASH *hash;

	UCHAR lkp_id[SHA_DIGEST_LENGTH+1];

	IP c_addr;
	time_t time_find;

};
typedef struct obj_node_announce ANNOUNCE;

struct obj_announce *announce_init( void );
void announce_free( void );

ANNOUNCE *announce_put( UCHAR *lkp_id );
void announce_del( ITEM *i );

void announce_expire( void );

void announce_resolve( UCHAR *lkp_id, UCHAR *node_id, IP *c_addr );
void announce_remember( ANNOUNCE *a, UCHAR *node_id );
