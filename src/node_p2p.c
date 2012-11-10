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
#include "node_p2p.h"
#include "bucket.h"
#include "send_p2p.h"
#include "lookup.h"
#include "neighboorhood.h"
#include "time.h"

NODES *nodes_init( void ) {
	NODES *nodes = (NODES *) myalloc( sizeof(NODES), "nodes_init" );
	nodes->list = list_init();
	nodes->hash = hash_init( 4096 );
	return nodes;
}

void nodes_free( void ) {
	list_clear( _main->nodes->list );
	list_free( _main->nodes->list );
	hash_free( _main->nodes->hash );
	myfree( _main->nodes, "nodes_free" );
}

NODE *node_put( UCHAR *id, UCHAR *risk_id, CIPV6 *sa ) {
	ITEM *i = NULL;
	NODE *n = NULL;

	/* It's me */
	if( node_me( id) ) {
		return NULL;
	}

	/* Find the node or create a new one */
	if( ( i = hash_get( _main->nodes->hash, id, SHA_DIGEST_LENGTH)) != NULL ) {
		n = i->val;
	} else {
		n = (NODE *) myalloc( sizeof(NODE), "node_put" );
		
		/* ID */
		memcpy( n->id, id, SHA_DIGEST_LENGTH );
		memcpy( n->risk_id, risk_id, SHA_DIGEST_LENGTH );

		/* Timings */
		n->time_ping = 0;
		n->time_find = 0;
		n->pinged = 0;

		/* Update IP address */
		node_update_address( n, sa );

		i = list_put( _main->nodes->list, n );
		hash_put( _main->nodes->hash, n->id, SHA_DIGEST_LENGTH, i );

		/* Send a PING */
		send_ping( &n->c_addr, SEND_UNICAST );
		node_pinged( n->id );

		/* New node: Ask for myself */
		send_find( &n->c_addr, _main->conf->host_id, _main->conf->null_id );
	}

	return n;
}

void node_del( ITEM *i ) {
	NODE *n = i->val;
	hash_del( _main->nodes->hash, n->id, SHA_DIGEST_LENGTH );
	list_del( _main->nodes->list, i );
	myfree( n, "node_del" );
}

void node_update_address( NODE *node, CIPV6 *sa ) {
	if( node == NULL ) {
		return;
	}

	/* Update address */
	if( memcmp( &node->c_addr, sa, sizeof(struct sockaddr_in6)) != 0 ) {
		memcpy( &node->c_addr, sa, sizeof(struct sockaddr_in6) );
	}
}

int node_update_risk_id( NODE *node, UCHAR *risk_id ) {
	int warning = NODE_NOERROR;

	if( node == NULL ) {
		return NODE_NOERROR;
	}

	/* Update the Collision ID. We must generate a warning too: There might be a duplicate hostname. */
	if( memcmp( node->risk_id, risk_id, SHA_DIGEST_LENGTH) != 0 ) {
		memcpy( node->risk_id, risk_id, SHA_DIGEST_LENGTH );
		warning = NODE_COLLISION;
	}

	return warning;
}

void node_pinged( UCHAR *id ) {
	ITEM *l = NULL;
	NODE *n = NULL;

	if( ( l = hash_get( _main->nodes->hash, id, SHA_DIGEST_LENGTH)) == NULL ) {
		return;
	}

	n = l->val;
	n->pinged++;
	
	/* ~5 minutes */
	n->time_ping = time_add_5_min_approx();
}

void node_ponged( UCHAR *id, CIPV6 *sa ) {
	ITEM *l = NULL;
	NODE *n = NULL;

	if( ( l = hash_get( _main->nodes->hash, id, SHA_DIGEST_LENGTH)) == NULL ) {
		return;
	}

	n = l->val;
	n->pinged = 0;

	/* ~5 minutes */
	n->time_ping = time_add_5_min_approx();
 
	memcpy( &n->c_addr, sa, sizeof(struct sockaddr_in6) );
}

void node_expire( void ) {
	ITEM *i = NULL;
	ITEM *next = NULL;
	NODE *n = NULL;
	long int j = 0;

	i = _main->nodes->list->start;
	for( j=0; j<_main->nodes->list->counter; j++ ) {
		n = i->val;
		next = list_next( i );

		/* Bad node */
		if( n->pinged >= 4 ) {
			/* Delete references */
			nbhd_del( n );

			/* Delete node */
			node_del( i );
		}
		i = next;
	}
}

long int node_counter( void ) {
	return _main->nodes->list->counter;
}

int node_me( UCHAR *node_id ) {
	if( memcmp( node_id, _main->conf->host_id, SHA_DIGEST_LENGTH) == 0 ) {
		return 1;
	}

	return 0;
}
