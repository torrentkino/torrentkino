/*
Copyright 2013 Aiko Barz

This file is part of torrentkino.

torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with torrentkino.  If not, see <http://www.gnu.org/licenses/>.
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
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "malloc.h"
#include "thrd.h"
#include "torrentkino.h"
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
#include "send_udp.h"
#include "time.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"

NBHD *nbhd_init( void ) {
	NBHD *nbhd = (NBHD *) myalloc( sizeof(NBHD) );
	nbhd->bucket = bckt_init();
	nbhd->hash = hash_init( 4096 );
	return nbhd;
}

void nbhd_free( void ) {
	bckt_free( _main->nbhd->bucket );
	hash_free( _main->nbhd->hash );
	myfree( _main->nbhd );
}

void nbhd_put( UCHAR *id, IP *sa ) {
	UDP_NODE *n = NULL;

	/* It's me */
	if( node_me( id ) ) {
		return;
	}

	/* Find the node or create a new one */
	if( ( n = hash_get( _main->nbhd->hash, id, SHA1_SIZE)) != NULL ) {
		node_update( n, sa );
		return;
	}
	
	/* New node */
	n = node_init( id, sa );

	/* Remember node */
	if( !bckt_put( _main->nbhd->bucket, n ) ) {
		node_free( n );
		return;
	}

	/* Fast access */
	hash_put( _main->nbhd->hash, n->id, SHA1_SIZE, n );
}

void nbhd_del( UDP_NODE *n ) {
	bckt_del( _main->nbhd->bucket, n );
	hash_del( _main->nbhd->hash, n->id, SHA1_SIZE );
	node_free( n );
}

void nbhd_pinged( UCHAR *id ) {
	UDP_NODE *n = hash_get( _main->nbhd->hash, id, SHA1_SIZE );

	if( n != NULL ) {
		node_pinged( n );
	}
}

void nbhd_ponged( UCHAR *id, IP *from ) {
	UDP_NODE *n = hash_get( _main->nbhd->hash, id, SHA1_SIZE );

	if( n != NULL ) {
		node_ponged( n, from );
	}
}

void nbhd_expire( time_t now ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	ITEM *item_n_next = NULL;
	UDP_NODE *n = NULL;

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
			  if( node_bad( n ) ) {
				   nbhd_del( n );
			  }
			  
			  item_n = item_n_next;
		 }

		 item_b = list_next( item_b );
	}
}

void nbhd_split( UCHAR *target, int verbose ) {
	bckt_split_loop( _main->nbhd->bucket, target, verbose );
}

int nbhd_is_empty( void ) {
	return bckt_is_empty( _main->nbhd->bucket );
}
