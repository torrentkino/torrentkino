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
		node_status( n, NODE_MODE_SHUTDOWN );
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
	
	/* Send buffer */
	memset( n->send_buf, '\0', BUF_SIZE );
	n->send_offset = 0;
	n->send_size = 0;

	/* Entity url */
	memset( n->entity_url, '\0', BUF_SIZE );

	/* File metadata */
	memset( n->filename, '\0', BUF_SIZE );
	n->filesize = 0;
	n->f_offset = 0;

#ifdef RANGE
	n->f_stop = 0;
	n->content_length = 0;

	/* HTTP Range */
	n->range_start = 0;
	n->range_stop = 0;
#endif

	/* HTTP Keep-Alive */
	n->keepalive = FALSE;
	n->keepalive_counter = CONF_KEEPALIVE;

	/* HTTP Last-Modified */
	memset( n->lastmodified, '\0', DATE_SIZE );

	/* HTTP code */
	n->code = 0;
	
	/* HTTP request type */
	n->type = HTTP_UNKNOWN;

	/* HTTP protocol type */
	n->proto = HTTP_1_0;

	/* Connection status */
	n->mode = NODE_MODE_READY;

	/* Connect node to the list */
	mutex_block( _main->work->tcp_node );
	thisnode = list_put( _main->node, n );
	mutex_unblock( _main->work->tcp_node );

	if( thisnode == NULL ) {
		myfree( n );
	}

	return thisnode;
}

void node_shutdown( ITEM *thisnode ) {
	TCP_NODE *n = list_value( thisnode );

	/* Close socket */
	node_disconnect( n->connfd );

	/* Clear buffer */
	node_clearRecvBuf( n );
	node_clearSendBuf( n );

	/* Unlink node object */
	mutex_block( _main->work->tcp_node );
	myfree( thisnode->val );
	list_del( _main->node, thisnode );
	mutex_unblock( _main->work->tcp_node );
}

void node_disconnect( int connfd ) {
	/* Remove FD from the watchlist */
	if( epoll_ctl( _main->tcp->epollfd, EPOLL_CTL_DEL, connfd, NULL) == -1 ) {
		if( status == RUMBLE ) {
			info( NULL, 500, strerror( errno ) );
			fail( "node_shutdown: epoll_ctl() failed" );
		}
	}

	/* Close socket */
	if( close( connfd ) != 0 ) {
		info( NULL, 500, "close() failed" );
	}
}

void node_status( TCP_NODE *n, int status ) {
	n->mode = status;
}

void node_activity( TCP_NODE *n ) {
	/* Reset timeout counter */
	n->keepalive_counter = CONF_KEEPALIVE;
}

void node_clearRecvBuf( TCP_NODE *n ) {
	memset( n->recv_buf, '\0', BUF_SIZE );
	n->recv_size = 0;
}

void node_clearSendBuf( TCP_NODE *n ) {
	memset( n->send_buf, '\0', BUF_SIZE );
	n->send_size = 0;
	n->send_offset = 0;

	memset( n->filename, '\0', BUF_SIZE );
	memset( n->lastmodified, '\0', DATE_SIZE );
	memset( n->entity_url, '\0', BUF_SIZE );
	n->filesize = 0;
	n->f_offset = 0;
#ifdef RANGE
	n->f_stop = 0;
	n->content_length = 0;
	n->range_start = 0;
	n->range_stop = 0;
#endif
	n->keepalive = FALSE;
	n->code = 0;
	n->type = HTTP_UNKNOWN;
	n->proto = HTTP_1_0;
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

void node_cleanup( void ) {
	ITEM *thisnode = NULL;
	ITEM *nextnode = NULL;
	TCP_NODE *n = NULL;
	
	if( list_size( _main->node ) == 0 ) {
		return;
	}

	thisnode = list_start( _main->node );
	while( thisnode != NULL ) {

		nextnode = list_next( thisnode );

		/* node_cleanup gets called every 1000ms if no Worker Thread is running.
		 * A counter, starting at 5, gets decreased every run if there is no
		 * activity. The counter will be reset to 5 if there is new activity
		 * within this timespan. */

		n = list_value( thisnode );
		if( n->keepalive_counter <= 0 ) {
			node_status( n, NODE_MODE_SHUTDOWN );
			node_shutdown( thisnode );
		} else {
			n->keepalive_counter--;
		}
		
		thisnode = nextnode;
	}
}
