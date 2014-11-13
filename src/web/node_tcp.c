/*
Copyright 2010 Aiko Barz

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
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "node_tcp.h"
#include "http.h"
#include "tcp.h"
#include "worker.h"

LIST *node_init( void ) {
	return list_init();
}

void node_free( void ) {
	ITEM *i = NULL;
	TCP_NODE *n = NULL;

	while( ( i = list_start( _main->node ) ) != NULL ) {
		n = list_value( i );
		node_status( n, NODE_SHUTDOWN );
		node_shutdown( i );
	}

	list_free( _main->node );
}

ITEM *node_put( void ) {
	TCP_NODE *n = (TCP_NODE *) myalloc( sizeof(TCP_NODE) );
	ITEM *thisnode = NULL;

	/* Address information */
	n->c_addrlen = sizeof( IP );
	memset( (char *) &n->c_addr, '\0', n->c_addrlen );

	/* Receive buffer */
	memset( n->recv_buf, '\0', BUF_SIZE );
	n->recv_size = 0;

	/* Connection status */
	n->pipeline = NODE_READY;

	/* keepalive */
	n->keepalive = HTTP_UNDEF;

	/* Respnses */
	n->response = list_init();

	/* Connect node to the list */
	mutex_block( _main->work->tcp_node );
	thisnode = list_put( _main->node, n );
	mutex_unblock( _main->work->tcp_node );

	if( thisnode == NULL ) {
		list_free( n->response );
		myfree( n );
	}

	return thisnode;
}

void node_shutdown( ITEM *thisnode ) {
	TCP_NODE *n = list_value( thisnode );

	mutex_block( _main->work->tcp_node );

	/* Disconnect */
	node_disconnect( n->connfd );

	/* Clear response list */
	resp_free( n->response );

	/* Delete node */
	myfree( n );

	/* Unlink node object */
	list_del( _main->node, thisnode );

	mutex_unblock( _main->work->tcp_node );
}

void node_disconnect( int connfd ) {
	/* Remove FD from the watchlist */
	if( epoll_ctl( _main->tcp->epollfd, EPOLL_CTL_DEL, connfd, NULL ) == -1 ) {
		if( status == RUMBLE ) {
			info( NULL, strerror( errno ) );
			fail( "node_shutdown: epoll_ctl() failed" );
		}
	}

	/* Close socket */
	if( close( connfd ) != 0 ) {
		info( NULL, "close() failed" );
	}
}

void node_status( TCP_NODE *n, int status ) {
	n->pipeline = status;
}

void node_clearRecvBuf( TCP_NODE *n ) {
	memset( n->recv_buf, '\0', BUF_SIZE );
	n->recv_size = 0;
}

ssize_t node_appendBuffer( TCP_NODE *n, char *buffer, ssize_t bytes ) {
	char *p = n->recv_buf + n->recv_size;
	ssize_t buf_free = BUF_OFF1 - n->recv_size;
	ssize_t buf_copy = (bytes > buf_free) ? buf_free : bytes;

	/* No space left in buffer */
	if( buf_copy == 0 ) {
		return n->recv_size;
	}

	/* Append buffer */
	memcpy( p, buffer, buf_copy );
	n->recv_size += buf_copy;

	/* Terminate buffer( Buffer size is NODE_BUFSIZE+1) */
	n->recv_buf[n->recv_size] = '\0';

	return n->recv_size;
}
