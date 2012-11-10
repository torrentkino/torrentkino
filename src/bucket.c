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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "malloc.h"
#include "thrd.h"
#include "main.h"
#include "str.h"
#include "list.h"
#include "hash.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "unix.h"
#include "udp.h"
#include "ben.h"
#include "node_p2p.h"
#include "bucket.h"

LIST *bckt_init( void ) {
	struct obj_bckt *b = (struct obj_bckt *) myalloc( sizeof(struct obj_bckt), "bckt_init" );
	LIST *l = (LIST *) list_init();
	
	/* First bucket */
	memset( b->id, '\0', SHA_DIGEST_LENGTH );
	b->nodes = list_init();

	/* Connect bucket */
	list_put( l, b );
	
	return l;
}

void bckt_free( LIST *thislist ) {
	ITEM *i = NULL;
	struct obj_bckt *b = NULL;
	long int j = 0;

	i = thislist->start;
	for( j=0; j<thislist->counter; j++ ) {
		b = i->val;

		/* Delete node references */
		list_free( b->nodes );

		/* Delete bucket */
		myfree( b, "bckt_free" );

		i = list_next( i );
	}
	list_free( thislist );
}

void bckt_put( LIST *l, struct obj_nodeItem *n ) {
	ITEM *i = NULL;
	struct obj_bckt *b = NULL;

	if( n == NULL ) {
		return;
	}

	if( ( i = bckt_find_best_match( l, n->id)) == NULL ) {
		log_fail( "Something is terribly broken: No appropriate bucket found for ID" );
		return;
	}
	b = i->val;

	if( bckt_find_node( l, n->id) != NULL ) {
		/* Node node found */
		return;
	}

	list_put( b->nodes, n );
}

void bckt_del( LIST *l, struct obj_nodeItem *n ) {
	ITEM *item_b = NULL;
	ITEM *item_n = NULL;
	struct obj_bckt *b = NULL;

	if( n == NULL ) {
		return;
	}

	if( ( item_b = bckt_find_best_match( l, n->id)) == NULL ) {
		log_fail( "Something is terribly broken: No appropriate bucket found for ID" );
		return;
	}
	b = item_b->val;

	if( ( item_n = bckt_find_node( l, n->id)) == NULL ) {
		/* Node node found */
		return;
	}

	/* Delete reference to node */
	list_del( b->nodes, item_n );
}

ITEM *bckt_find_best_match( LIST *thislist, const unsigned char *id ) {
	ITEM *item = NULL;
	ITEM *next = NULL;
	struct obj_bckt *b = NULL;
	long int i = 0;

	item = thislist->start;
	for( i=0; i<thislist->counter-1; i++ ) {
		next = list_next( item );
		b = next->val;

		/* Does this bucket fits better than the next one? */
		if( memcmp( id, b->id, SHA_DIGEST_LENGTH) < 0 ) {
			return item;
		}

		item = next;
	}

	/* No bucket found, so return the last one */
	return thislist->stop;
}

ITEM *bckt_find_any_match( LIST *thislist, const unsigned char *id ) {
	ITEM *i = NULL;
	struct obj_bckt *b = NULL;
	long int j=0;

	i = bckt_find_best_match( thislist, id );
	b = i->val;

	/* Success, */
	if( b->nodes->counter > 0 ) {
		return i;
	}

	/* This bucket is empty: Find another one. */
	for( j=0; j<thislist->counter; j++ ) {

		b = i->val;
		if( b->nodes->counter > 0 ) {
			return i;
		}
		i = list_prev( i );
	}

	return NULL;
}

ITEM *bckt_find_node( LIST *thislist, const unsigned char *id ) {
	ITEM *item_b = NULL;
	ITEM *item_n = NULL;
	struct obj_bckt *b = NULL;
	LIST *list_n = NULL;
	struct obj_nodeItem *n = NULL;
	long int i = 0;

	if( ( item_b = bckt_find_best_match( thislist, id)) == NULL ) {
		return NULL;
	}
	b = item_b->val;

	list_n = b->nodes;
	item_n = list_n->start;
	for( i=0; i<list_n->counter; i++ ) {
		n = item_n->val;
		if( memcmp( n->id, id, SHA_DIGEST_LENGTH) == 0 ) {
			return item_n;
		}
		item_n = list_next( item_n );
	}

	return NULL;
}

int bckt_split( LIST *thislist, const unsigned char *id ) {
	ITEM *item_b = NULL;
	struct obj_bckt *b = NULL;
	LIST *list_n = NULL;
	ITEM *item_n = NULL;
	struct obj_nodeItem *n = NULL;
	ITEM *item_s = NULL;
	struct obj_bckt *s = NULL;
	struct obj_bckt *b_new = NULL;
	unsigned char id_new[SHA_DIGEST_LENGTH];
	long int i = 0;

	/* Search bucket we want to evolve */
	if( ( item_b = bckt_find_best_match( thislist, id)) == NULL ) {
		return 0;
	}
	b = item_b->val;

	/* Split whenever there are more than 8 nodes within a bucket */
	if( b->nodes->counter <= 8 ) {
		return 0;
	}

	/* Compute id of the new bucket */
	if( bckt_compute_id( thislist, item_b, id_new) < 0 ) {
		return 0;
	}
	
	/* Create new bucket */
	b_new = (struct obj_bckt *) myalloc( sizeof(struct obj_bckt), "split_bucket" );
	memcpy( b_new->id, id_new, SHA_DIGEST_LENGTH );
	b_new->nodes = list_init();
	
	/* Insert new bucket */
	list_ins( thislist, item_b, b_new );

	/* Remember nodes */
	list_n = b->nodes;

	/* Create new node list */
	b->nodes = list_init();

	/* Walk through the existing nodes and find an adequate bucket */
	item_n = list_n->start;
	for( i=0; i<list_n->counter; i++ ) {
		n = item_n->val;
		item_s = bckt_find_best_match( thislist, n->id );

		s = item_s->val;
		list_put( s->nodes, n );

		item_n = list_next( item_n );
	}

	/* Delete the old list but not its payload */
	list_free( list_n );

	/* Bucket successfully split */
	return 1;
}

int bckt_compute_id( LIST *thislist, ITEM *item_b, unsigned char *id_return ) {
	struct obj_bckt *b = item_b->val;
	ITEM *item_next = NULL;
	struct obj_bckt *b_next = NULL;
	int bit1 = 0;
	int bit2 = 0;
	int bit = 0;

	/* Is there a container next to this one? */
	if( item_b != thislist->stop ) {
		item_next = list_next( item_b );
		b_next = item_next->val;
	}

	bit1 = bckt_significant_bit( b->id );
	bit2 = (b_next != NULL) ? bckt_significant_bit( b_next->id) : -1;
	bit = (bit1 >= bit2) ? bit1 : bit2; bit++;

	if( bit >= 160 ) {
		return -1;
	}

	memcpy( id_return, b->id, 20 );
	id_return[bit / 8] |= (0x80 >>( bit % 8) );

	return 1;
}

int bckt_significant_bit( const unsigned char *id ) {
	int i=0, j=0;

	for( i = 19; i >= 0; i-- ) {
		if( id[i] != 0 ) {
			break;
		}
	}

	if( i < 0 ) {
		return -1;
	}

	for( j = 7; j >= 0; j-- ) {
		if( ( id[i] &( 0x80 >> j)) != 0 ) {
			break;
		}
	}

	return 8 * i + j;
}
