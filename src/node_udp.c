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

#include "node_udp.h"

UDP_NODE *node_init( UCHAR *node_id, IP *sa ) {
	UDP_NODE *n = (UDP_NODE *) myalloc( sizeof(UDP_NODE) );

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

void node_free( UDP_NODE *n ) {
	myfree( n );
}

void node_update( UDP_NODE *n, IP *sa ) {
	if( n == NULL ) {
		return;
	}

	/* Update address */
	if( memcmp( &n->c_addr, sa, sizeof(IP)) != 0 ) {
		memcpy( &n->c_addr, sa, sizeof(IP) );
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

int node_ok( UDP_NODE *n ) {
	return ( n->pinged <= 1 ) ? TRUE : FALSE;
}

int node_bad( UDP_NODE *n ) {
	return ( n->pinged >= 4 ) ? TRUE : FALSE;
}

void node_pinged( UDP_NODE *n ) {
	/* Remember no of pings */
	n->pinged++;
	
	/* Try again in ~5 minutes */
	time_add_5_min_approx( &n->time_ping );
}

void node_ponged( UDP_NODE *n, IP *from ) {
	/* Reset no of pings */
	n->pinged = 0;

	/* Try again in ~5 minutes */
	time_add_5_min_approx( &n->time_ping );

	/* Update IP */
	node_update( n, from );
}
