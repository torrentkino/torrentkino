/*
Copyright 2013 Aiko Barz

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
#include "send_p2p.h"
#include "time.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"

NBHD *nbhd_init( void ) {
	NBHD *nbhd = (NBHD *) myalloc( sizeof(NBHD), "nbhd_init" );
	nbhd->bucket = bckt_init();
	nbhd->hash = hash_init( 4096 );
	return nbhd;
}

void nbhd_free( NBHD *nbhd ) {
	if( nbhd == NULL ) {
		return;
	}
	bckt_free( nbhd->bucket );
	hash_free( nbhd->hash );
	myfree( nbhd, "nbhd_free" );
}

void nbhd_put( NBHD *nbhd, UCHAR *id, IP *sa ) {
	NODE *n = NULL;

	/* It's me */
	if( nbhd_me( id ) ) {
		return;
	}

	/* Find the node or create a new one */
	if( ( n = hash_get( nbhd->hash, id, SHA_DIGEST_LENGTH)) != NULL ) {
		nbhd_update_address( n, sa );
	} else {
		n = (NODE *) myalloc( sizeof(NODE), "nbhd_put" );
		
		/* ID */
		memcpy( n->id, id, SHA_DIGEST_LENGTH );

		/* Token */
		memset( n->token, '\0', TOKEN_SIZE_MAX );
		n->token_size = 0;

		/* Timings */
		n->time_ping = 0;
		n->time_find = 0;
		n->pinged = 0;

		/* Update IP address */
		nbhd_update_address( n, sa );

		/* Store node */
		bckt_put( nbhd->bucket, n );
		hash_put( nbhd->hash, n->id, SHA_DIGEST_LENGTH, n );
	}
}

void nbhd_del( NBHD *nbhd, NODE *n ) {
	bckt_del( nbhd->bucket, n );
	hash_del( nbhd->hash, n->id, SHA_DIGEST_LENGTH );
	myfree( n, "nbhd_del" );
}

void nbhd_update_address( NODE *node, IP *sa ) {
	if( node == NULL ) {
		return;
	}

	/* Update address */
	if( memcmp( &node->c_addr, sa, sizeof(IP)) != 0 ) {
		memcpy( &node->c_addr, sa, sizeof(IP) );
	}
}

void nbhd_pinged( UCHAR *id ) {
	NODE *n = NULL;

	if( ( n = hash_get( _main->nbhd->hash, id, SHA_DIGEST_LENGTH)) == NULL ) {
		return;
	}

	n->pinged++;
	
	/* ~5 minutes */
	time_add_5_min_approx( &n->time_ping );
}

void nbhd_ponged( UCHAR *id, IP *sa ) {
	NODE *n = NULL;

	if( ( n = hash_get( _main->nbhd->hash, id, SHA_DIGEST_LENGTH)) == NULL ) {
		return;
	}

	n->pinged = 0;

	/* ~5 minutes */
	time_add_5_min_approx( &n->time_ping );
 
	memcpy( &n->c_addr, sa, sizeof(IP) );
}

void nbhd_expire( void ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	ITEM *item_n_next = NULL;
	NODE *n = NULL;

	/* Cycle through all the buckets */
	item_b = list_start( _main->nbhd->bucket );
	while( item_b != NULL ) {
		 b = list_value( item_b );

		 /* Cycle through all the nodes */
		 item_n = list_start( b->nodes );
		 while( item_n != NULL ) {
			  n = list_value( item_n );

			  item_n_next = list_next( item_n );

			  /* Bad node */
			  if( n->pinged >= 4 ) {
				   nbhd_del( _main->nbhd, n );
			  }
			  
			  item_n = item_n_next;
		 }

		 item_b = list_next( item_b );
	}
}

void nbhd_expire_nodes_with_emtpy_tokens( NBHD *nbhd ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	ITEM *item_n_next = NULL;
	NODE *n = NULL;

	/* Cycle through all the buckets */
	item_b = list_start( nbhd->bucket );
	while( item_b != NULL ) {
		 b = list_value( item_b );

		 /* Cycle through all the nodes */
		 item_n = list_start( b->nodes );
		 while( item_n != NULL ) {
			  n = list_value( item_n );

			  item_n_next = list_next( item_n );

			  /* Bad node */
			  if( n->token_size == 0 ) {
				   nbhd_del( nbhd, n );
			  }
			  
			  item_n = item_n_next;
		 }

		 item_b = list_next( item_b );
	}
}

void nbhd_split( NBHD *nbhd, UCHAR *target ) {
	bckt_split_loop( nbhd->bucket, target );
}

int nbhd_is_empty( NBHD *nbhd ) {
	return bckt_is_empty( nbhd->bucket );
}

int nbhd_me( UCHAR *node_id ) {
	if( nbhd_equal( node_id, _main->conf->node_id ) ) {
		 return 1;
	}
	return 0;
}

int nbhd_equal( const UCHAR *node_a, const UCHAR *node_b ) {
	if( memcmp( node_a, node_b, SHA_DIGEST_LENGTH) == 0 ) {
		 return 1;
	}
	return 0;
}

int nbhd_conn_from_localhost( IP *from ) {
	const UCHAR localhost[] = 
		 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

	if( memcmp(from->sin6_addr.s6_addr, localhost, 16) == 0 ) {
		 return TRUE;
	}

	return FALSE;
}
