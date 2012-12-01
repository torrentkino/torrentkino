/*
Copyright 2011 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

struct obj_lookups {
	LIST *list;
	HASH *hash;
};
typedef struct obj_lookups LOOKUPS;

struct obj_lookup {
	LIST *list;
	HASH *hash;

	UCHAR find_id[SHA_DIGEST_LENGTH+1];
	UCHAR lkp_id[SHA_DIGEST_LENGTH+1];

	IP c_addr;
	time_t time_find;

};
typedef struct obj_lookup LOOKUP;

LOOKUPS *lkp_init( void );
void lkp_free( void );

LOOKUP *lkp_put( UCHAR *find_id, UCHAR *lkp_id, IP *from );
void lkp_del( ITEM *i );

void lkp_expire( void );

void lkp_resolve( UCHAR *lkp_id, UCHAR *node_id, IP *c_addr );
void lkp_success( UCHAR *lkp_id, UCHAR *address );
void lkp_local( IP *address, IP *from );
void lkp_remember( LOOKUP *l, UCHAR *node_id );
