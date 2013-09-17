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
#include "masala-srv.h"
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

NODE *node_init( UCHAR *node_id, IP *sa ) {
	NODE *n = (NODE *) myalloc( sizeof(NODE), "node_init" );
		
	/* ID */
	memcpy( n->id, node_id, SHA1_SIZE );

	/* Timings */
	n->time_ping = 0;
	n->time_find = 0;
	n->pinged = 0;

	/* Update IP address */
	node_update( n, sa );

	return n;
}

void node_free( NODE *n ) {
	myfree( n, "node_free" );
}

void node_update( NODE *node, IP *sa ) {
	if( node == NULL ) {
		return;
	}

	/* Update address */
	if( memcmp( &node->c_addr, sa, sizeof(IP)) != 0 ) {
		memcpy( &node->c_addr, sa, sizeof(IP) );
	}
}

int node_me( UCHAR *node_id ) {
	if( node_equal( node_id, _main->conf->node_id ) ) {
		 return 1;
	}
	return 0;
}

int node_equal( const UCHAR *node_a, const UCHAR *node_b ) {
	if( memcmp( node_a, node_b, SHA1_SIZE) == 0 ) {
		 return 1;
	}
	return 0;
}

int node_localhost( IP *from ) {
	const UCHAR localhost[] = 
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

	if( memcmp(from->sin6_addr.s6_addr, localhost, 16) == 0 ) {
		return TRUE;
	}

	return FALSE;
}

/*
int node_teredo( IP *from ) {
	const UCHAR teredo[] = 
		{ 0x20, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	if( memcmp(from->sin6_addr.s6_addr, teredo, 4) == 0 ) {
		log_info( from, 0, "Teredo access denied (2001:0::/32) from" );
		return TRUE;
	}

	return FALSE;
}
*/
