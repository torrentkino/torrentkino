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

struct obj_lkps {
	LIST *list;
	struct obj_hash *hash;
};

struct obj_lkp {
	LIST *list;
	struct obj_hash *hash;

	unsigned char find_id[SHA_DIGEST_LENGTH+1];
	unsigned char lkp_id[SHA_DIGEST_LENGTH+1];

	struct sockaddr_in6 c_addr;
	time_t time_find;

};

struct obj_lkps *lkp_init( void );
void lkp_free( void );

struct obj_lkp *lkp_put( UCHAR *find_id, UCHAR *lkp_id, CIPV6 *from );
void lkp_del( ITEM *i );

void lkp_expire( void );

void lkp_resolve( UCHAR *lkp_id, UCHAR *node_id, CIPV6 *c_addr );
void lkp_remember( struct obj_lkp *l, UCHAR *node_id );
