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
#include "p2p.h"
#include "time.h"
#include "lookup.h"
#include "announce.h"
#include "node_p2p.h"
#include "neighboorhood.h"
#include "bucket.h"
#include "send_p2p.h"

struct obj_announce *announce_init( void ) {
	struct obj_announce *announce = (struct obj_announce *) myalloc( sizeof(struct obj_announce), "announce_init" );
	announce->list = list_init();
	announce->hash = hash_init( 4096 );
	return announce;
}

void announce_free( void ) {
	list_clear( _main->announce->list );
	list_free( _main->announce->list );
	hash_free( _main->announce->hash );
	myfree( _main->announce, "announce_free" );
}

ANNOUNCE *announce_put( UCHAR *lkp_id ) {
	ITEM *i = NULL;
	ANNOUNCE *a = NULL;

	a = (ANNOUNCE *) myalloc( sizeof(ANNOUNCE), "announce_put" );

	/* Remember nodes that have been asked */
	a->list = list_init();
	a->hash = hash_init( 100 );

	/* ID */
	memcpy( a->lkp_id, lkp_id, SHA_DIGEST_LENGTH );

	/* Timings */
	a->time_find = 0;

	/* Remember lookup request */
	i = list_put( _main->announce->list, a );
	hash_put( _main->announce->hash, a->lkp_id, SHA_DIGEST_LENGTH, i );

	/* Search the requested name */
	nbhd_announce( a );

	return a;
}

void announce_del( ITEM *i ) {
	ANNOUNCE *l = i->val;

	/* Free lookup cache */
	list_clear( l->list );
	list_free( l->list );
	hash_free( l->hash );

	/* Delete lookup item */
	hash_del( _main->announce->hash, l->lkp_id, SHA_DIGEST_LENGTH );
	list_del( _main->announce->list, i );
	myfree( l, "announce_del" );
}

void announce_expire( void ) {
	ITEM *i = NULL;
	ITEM *next = NULL;
	ANNOUNCE *l = NULL;
	long int j = 0;

	i = _main->announce->list->start;
	for( j=0; j<_main->announce->list->counter; j++ ) {
		l = i->val;
		next = list_next( i );

		if( _main->p2p->time_now.tv_sec > l->time_find ) {
			announce_del( i );
		}
		i = next;
	}
}

void announce_resolve( UCHAR *lkp_id, UCHAR *node_id, IP *c_addr ) {
	ITEM *i = NULL;
	ANNOUNCE *l = NULL;

	/* Lookup the lookup ID */
	if( ( i = hash_get( _main->announce->hash, lkp_id, SHA_DIGEST_LENGTH)) == NULL ) {
		return;
	}
	l = i->val;

	/* Found the lookup ID */

	/* Now look if this node has already been asked */
	if( !hash_exists( l->hash, node_id, SHA_DIGEST_LENGTH) ) {
		
		/* Ask the node just once */
		if( !node_me( node_id) ) {
			send_announce( c_addr, lkp_id );
		}

		/* Remember that node */
		announce_remember( l, node_id );
	}
}

void announce_remember( ANNOUNCE *l, UCHAR *node_id ) {
	UCHAR *buffer = NULL;

	/* Remember that node */
	buffer = (UCHAR *) myalloc( SHA_DIGEST_LENGTH*sizeof(char), "announce_remember" );
	memcpy( buffer, node_id, SHA_DIGEST_LENGTH );
	list_put( l->list, buffer );
	hash_put( l->hash, buffer, SHA_DIGEST_LENGTH, buffer );
}
