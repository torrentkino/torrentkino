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
#include "p2p.h"
#include "node_p2p.h"
#include "bucket.h"
#include "send_p2p.h"
#include "lookup.h"
#include "announce.h"
#include "neighboorhood.h"
#include "time.h"
#include "hex.h"
#include "random.h"

LIST *nbhd_init( void ) {
	return bckt_init();
}

void nbhd_free( void ) {
	bckt_free( _main->nbhd );
}

void nbhd_put( NODE *n ) {
	bckt_put( _main->nbhd, n );
}

void nbhd_del( NODE *n ) {
	bckt_del( _main->nbhd, n );
}

void nbhd_split( void ) {
	/* Do as many splits as neccessary */
	for( ;; ) {
		if( !bckt_split( _main->nbhd, _main->conf->node_id) ) {
			return;
		}
		nbhd_print();
	}
}

void nbhd_send( IP *sa, UCHAR *node_id, UCHAR *lkp_id, UCHAR *node_sk, UCHAR *reply_type ) {
	ITEM *i = NULL;
	BUCK *b = NULL;

	if( ( i = bckt_find_any_match( _main->nbhd, node_id)) == NULL ) {
		return;
	}
	b = i->val;

	send_node( sa, b, node_sk, lkp_id, reply_type );
}

void nbhd_ping( void ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	long int j = 0, k = 0;

	/* Cycle through all the buckets */
	item_b = _main->nbhd->start;
	for( k=0; k<_main->nbhd->counter; k++ ) {
		b = item_b->val;

		/* Cycle through all the nodes */
		item_n = b->nodes->start;
		for( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			/* It's time for pinging */
			if( _main->p2p->time_now.tv_sec > n->time_ping ) {

				/* Ping the first 8 nodes. Sort out the rest. */
				if( j < 8 ) {
					send_ping( &n->c_addr, SEND_UNICAST );
					node_pinged( n->id );
				} else {
					node_pinged( n->id );
				}
			}

			item_n = list_next( item_n );
		}

		item_b = list_next( item_b );
	}
}

void nbhd_find_myself( void ) {
	nbhd_find( _main->conf->node_id );
}

void nbhd_find_random( void ) {
	UCHAR node_id[SHA_DIGEST_LENGTH];

	rand_urandom( node_id, SHA_DIGEST_LENGTH );
	nbhd_find( node_id );
}

void nbhd_find( UCHAR *find_id ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	long int j = 0;

	if( ( item_b = bckt_find_any_match( _main->nbhd, find_id)) != NULL ) {
		b = item_b->val;

		item_n = b->nodes->start;
		for( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			/* Maintainance search */
			if( _main->p2p->time_now.tv_sec > n->time_find ) {

				send_find( &n->c_addr, find_id, _main->conf->null_id );
				n->time_find = time_add_5_min_approx();
			}

			item_n = list_next( item_n );
		}
	}
}

void nbhd_lookup( LOOKUP *l ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	long int j = 0;
	long int max = 0;

	/* Find a matching bucket */
	if( ( item_b = bckt_find_any_match( _main->nbhd, l->find_id)) == NULL ) {
		return;
	}

	/* Ask the first 8 nodes for the requested node */
	b = item_b->val;
	item_n = b->nodes->start;
	max = ( b->nodes->counter < 8 ) ? b->nodes->counter : 8;
	for( j = 0; j < max; j++ ) {
		n = item_n->val;

		/* Remember node */
		lkp_remember( l, n->id );

		/* Direct lookup */
		send_lookup( &n->c_addr, l->find_id, l->lkp_id );

		item_n = list_next( item_n );
	}
}

void nbhd_announce( ANNOUNCE *a ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	long int j = 0;
	long int max = 0;

	/* Find a matching bucket */
	if( (item_b = bckt_find_any_match( _main->nbhd, _main->conf->host_id )) == NULL ) {
		return;
	}

	/* Ask the first 8 nodes */
	b = item_b->val;
	item_n = b->nodes->start;
	max = ( b->nodes->counter < 8 ) ? b->nodes->counter : 8;
	for( j = 0; j < max; j++ ) {
		n = item_n->val;

		/* Remember node */
		announce_remember( a, n->id );

		/* Send announcement */
		send_announce( &n->c_addr, a->lkp_id );

		item_n = list_next( item_n );
	}
}

void nbhd_print( void ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
	long int j = 0, k = 0;
	char hex[HEX_LEN+1];
	char buf[MAIN_BUF+1];

	log_info( "Bucket split:" );

	/* Cycle through all the buckets */
	item_b = _main->nbhd->start;
	for( k=0; k<_main->nbhd->counter; k++ ) {
		b = item_b->val;

		hex_encode( hex, b->id );
		snprintf( buf, MAIN_BUF+1, " Bucket: %s", hex );
		log_info( buf );

		/* Cycle through all the nodes */
		item_n = b->nodes->start;
		for( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			hex_encode( hex, n->id );
			snprintf( buf, MAIN_BUF+1, "  Node: %s", hex );
			log_info( buf );

			item_n = list_next( item_n );
		}

		item_b = list_next( item_b );
	}
}
