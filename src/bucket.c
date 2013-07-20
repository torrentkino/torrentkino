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
#include "token.h"
#include "neighbourhood.h"
#include "bucket.h"
#include "hex.h"

LIST *bckt_init( void ) {
	BUCK *b = (BUCK *) myalloc( sizeof(BUCK), "bckt_init" );
	LIST *l = list_init();
	
	/* First bucket */
	memset( b->id, '\0', SHA_DIGEST_LENGTH );
	b->nodes = list_init();

	/* Connect bucket */
	list_put( l, b );
	
	return l;
}

void bckt_free( LIST *thislist ) {
	ITEM *i = NULL;
	BUCK *b = NULL;

	i = list_start( thislist );
	while( i != NULL ) {
		b = list_value( i );

		list_clear( b->nodes );
		list_free( b->nodes );

		i = list_next( i );
	}
	list_clear( thislist );
	list_free( thislist );
}

void bckt_put( LIST *l, NODE *n ) {
	ITEM *i = NULL;
	BUCK *b = NULL;

	if( n == NULL ) {
		return;
	}

	if( ( i = bckt_find_best_match( l, n->id)) == NULL ) {
		log_fail( "Something is terribly broken: No appropriate bucket found for ID" );
		return;
	}
	b = list_value( i );

	if( bckt_find_node( l, n->id ) != NULL ) {
		/* Node found */
		return;
	}

	list_put( b->nodes, n );
}

void bckt_del( LIST *l, NODE *n ) {
	ITEM *item_b = NULL;
	ITEM *item_n = NULL;
	BUCK *b = NULL;

	if( n == NULL ) {
		return;
	}

	if( ( item_b = bckt_find_best_match( l, n->id ) ) == NULL ) {
		log_fail( "Something is terribly broken: No appropriate bucket found for ID" );
		return;
	}
	b = list_value( item_b );

	if( ( item_n = bckt_find_node( l, n->id ) ) == NULL ) {
		/* Node node found */
		return;
	}

	/* Delete reference to node */
	list_del( b->nodes, item_n );
}

ITEM *bckt_find_best_match( LIST *thislist, const UCHAR *id ) {
	ITEM *item = NULL;
	BUCK *b = NULL;

	item = list_start( thislist );
	while( item->next != NULL ) {
		b = list_value( list_next( item ) );

		/* Does this bucket fits better than the next one? */
		if( memcmp( id, b->id, SHA_DIGEST_LENGTH) < 0 ) {
			return item;
		}

		item = list_next( item );
	}

	/* No bucket found, so return the last one */
	return list_stop( thislist );
}

ITEM *bckt_find_any_match( LIST *thislist, const UCHAR *id ) {
	ITEM *i = NULL;
	BUCK *b = NULL;

	i = bckt_find_best_match( thislist, id );
	b = list_value( i );

	/* Success, */
	if( b->nodes->item != NULL ) {
		return i;
	}

	/* This bucket is empty: Find another one. */
	while( i != NULL ) {

		b = list_value( i );
		if( b->nodes->item != NULL ) {
			return i;
		}
		i = list_prev( i );
	}

	return NULL;
}

ITEM *bckt_find_node( LIST *thislist, const UCHAR *id ) {
	ITEM *item_b = NULL;
	ITEM *item_n = NULL;
	BUCK *b = NULL;
	LIST *list_n = NULL;
	NODE *n = NULL;

	if( ( item_b = bckt_find_best_match( thislist, id)) == NULL ) {
		return NULL;
	}
	b = list_value( item_b );

	list_n = b->nodes;
	item_n = list_start( list_n );
	while( item_n != NULL ) {
		n = list_value( item_n );
		if( nbhd_equal( n->id, id ) ) {
			return item_n;
		}
		item_n = list_next( item_n );
	}

	return NULL;
}

int bckt_split( LIST *thislist, const UCHAR *target ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	LIST *list_n = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	BUCK *b_new = NULL;
	UCHAR id_new[SHA_DIGEST_LENGTH];

	/* Search bucket we want to evolve */
	if( ( item_b = bckt_find_best_match( thislist, target)) == NULL ) {
		return FALSE;
	}
	b = list_value( item_b );

	/* Split whenever there are more than 8 nodes within a bucket */
	if( list_size( b->nodes ) <= 8 ) {
		return FALSE;
	}

	/* Compute id of the new bucket */
	if( bckt_compute_id( thislist, item_b, id_new) < 0 ) {
		return FALSE;
	}
	
	/* Create new bucket */
	b_new = (BUCK *) myalloc( sizeof(BUCK), "split_bucket" );
	memcpy( b_new->id, id_new, SHA_DIGEST_LENGTH );
	b_new->nodes = list_init();
	
	/* Insert new bucket */
	list_join( thislist, item_b, b_new );

	/* Remember nodes */
	list_n = b->nodes;

	/* Create new node list */
	b->nodes = list_init();

	/* Walk through the existing nodes and find an adequate bucket */
	item_n = list_start( list_n );
	while( item_n != NULL ) {
		n = list_value( item_n );
		item_b = bckt_find_best_match( thislist, n->id );

		b = list_value( item_b );
		list_put( b->nodes, n );

		item_n = list_next( item_n );
	}

	/* Delete the old list but not its payload */
	list_free( list_n );

	/* Bucket successfully split */
	return TRUE;
}

void bckt_split_loop( LIST *l, UCHAR *target ) {
/*	int change = FALSE; */

	/* Do as many splits as neccessary */
	for( ;; ) {
		if( !bckt_split( l, target) ) {
			break;
/*			change = TRUE; */
		}
	}

/*	if( change == TRUE ) {
		bckt_split_print( l );
	} */
}

void bckt_split_print( LIST *l ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	char hex[HEX_LEN];

	log_info( NULL, 0, "Bucket split:" );

	/* Cycle through all the buckets */
	item_b = list_start( l );
	while( item_b != NULL ) {
		b = list_value( item_b );

		hex_hash_encode( hex, b->id );
		log_info( NULL, 0, " Bucket: %s", hex );

		/* Cycle through all the nodes */
		item_n = list_start( b->nodes );
		while( item_n != NULL ) {
			n = list_value( item_n );

			hex_hash_encode( hex, n->id );
			log_info( NULL, 0, "  Node: %s", hex );

			item_n = list_next( item_n );
		}

		item_b = list_next( item_b );
	}
}

int bckt_is_empty( LIST *l ) {
	ITEM *i = NULL;
	BUCK *b = NULL;

	/* Cycle through all the buckets */
	i = list_start( l );
	while( i != NULL ) {
		b = list_value( i );

		if( b->nodes->item != NULL ) {
			return FALSE;
		}

		i = list_next( i );
	}

	return TRUE;
}

int bckt_compute_id( LIST *thislist, ITEM *item_b, UCHAR *id ) {
	BUCK *b = list_value( item_b );
	ITEM *item_next = NULL;
	BUCK *b_next = NULL;
	int bit1 = 0;
	int bit2 = 0;
	int bit = 0;

	/* Is there a container next to this one? */
	if( item_b->next != NULL ) {
		item_next = list_next( item_b );
		b_next = list_value( item_next );
	}

	bit1 = bckt_significant_bit( b->id );
	bit2 = ( b_next != NULL ) ? bckt_significant_bit( b_next->id ) : -1;
	bit = ( bit1 >= bit2 ) ? bit1 : bit2; bit++;

	if( bit >= 160 ) {
		return -1;
	}

	memcpy( id, b->id, SHA_DIGEST_LENGTH );
	id[bit / 8] |= ( 0x80 >> ( bit % 8 ) );

	return 1;
}

int bckt_significant_bit( const UCHAR *id ) {
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
		if( ( id[i] & ( 0x80 >> j ) ) != 0 ) {
			break;
		}
	}

	return 8 * i + j;
}
