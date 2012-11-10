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

struct obj_neighboorhood_bucket {
	unsigned char id[SHA_DIGEST_LENGTH];
	LIST *nodes;
};
typedef struct obj_neighboorhood_bucket BUCK;

LIST *bckt_init( void );
void bckt_free( LIST *thislist );
void bckt_put( LIST *l, struct obj_nodeItem *n );
void bckt_del( LIST *l, struct obj_nodeItem *n );

ITEM *bckt_find_best_match( LIST *thislist, const unsigned char *id );
ITEM *bckt_find_any_match( LIST *thislist, const unsigned char *id );
ITEM *bckt_find_node( LIST *thislist, const unsigned char *id );

int bckt_split( LIST *thislist, const unsigned char *id );

int bckt_compute_id( LIST *thislist, ITEM *item_b, unsigned char *id_return );
int bckt_significant_bit( const unsigned char *id );
