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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>

#include "malloc.h"
#include "tumbleweed.h"
#include "thrd.h"
#include "str.h"
#include "conf.h"
#include "list.h"
#include "node_tcp.h"
#include "log.h"
#include "hash.h"
#include "file.h"
#include "send_tcp.h"
#include "http.h"

void send_tcp( TCP_NODE *n ) {
	switch( n->mode ) {
		case NODE_MODE_SEND_INIT:
			send_cork_start( n );

		case NODE_MODE_SEND_MEM:
			send_mem( n );

		case NODE_MODE_SEND_FILE:
			send_file( n );

		case NODE_MODE_SEND_STOP:
			send_cork_stop( n );
	}
}

void send_cork_start( TCP_NODE *n ) {
	int on = 1;

	if( setsockopt( n->connfd, IPPROTO_TCP, TCP_CORK, &on, sizeof(on)) != 0 ) {
		fail( strerror( errno ) );
	}
	
	node_status( n, NODE_MODE_SEND_MEM );
}

void send_mem( TCP_NODE *n ) {
	ssize_t bytes_sent = 0;
	int bytes_todo = 0;
	char *p = NULL;

	while( status == RUMBLE ) {
		p = n->send_buf + n->send_offset;
		bytes_todo = n->send_size - n->send_offset;
	
		bytes_sent = send( n->connfd, p, bytes_todo, 0 );
	
		if( bytes_sent < 0 ) {
			if( errno == EAGAIN || errno == EWOULDBLOCK ) {
				return;
			}
	
			/* Client closed the connection, etc... */
			node_status( n, NODE_MODE_SHUTDOWN );
			return;
		}
	
		n->send_offset += bytes_sent;
		
		/* Done */
		if( n->send_offset >= n->send_size ) {
			node_status( n, NODE_MODE_SEND_FILE );
			return;
		}
	}
}

void send_file( TCP_NODE *n ) {
	ssize_t bytes_sent = 0;
	int fh = 0;

	if( n->mode != NODE_MODE_SEND_FILE ) {
		return;
	}

	/* Nothing to do */
	if( n->filesize == 0 ) {
		node_status( n, NODE_MODE_SEND_STOP );
		return;
	}

	/* Not modified. Stop here. */
	if( n->code == 304 ) {
		node_status( n, NODE_MODE_SEND_STOP );
		return;
	}

	/* Not found */
	if( n->code == 404 ) {
		node_status( n, NODE_MODE_SEND_STOP );
		return;
	}

	/* HEAD request. Stop here. */
	if( n->type == HTTP_HEAD ) {
		node_status( n, NODE_MODE_SEND_STOP );
		return;
	}

	while( status == RUMBLE ) {
		fh = open( n->filename, O_RDONLY );	
		if( fh < 0 ) {
			info( NULL, 500, "Failed to open %s", n->filename );
			fail( strerror( errno) );
		}
	
		/* The SIGPIPE gets catched in sig.c */
#ifdef RANGE
		bytes_sent = sendfile( n->connfd, fh, &n->f_offset, n->content_length );
#else
		bytes_sent = sendfile( n->connfd, fh, &n->f_offset, n->filesize );
#endif

		if( close( fh ) != 0 ) {
			info( NULL, 500, "Failed to close %s", n->filename );
			fail( strerror( errno) );
		}

		if( bytes_sent < 0 ) {
			if( errno == EAGAIN || errno == EWOULDBLOCK ) {
				/* connfd is blocking */
				return;
			}
			
			/* Client closed the connection, etc... */
			node_status( n, NODE_MODE_SHUTDOWN );
			return;
		}
		
		/* Done */
#ifdef RANGE
		if( n->f_offset >= n->f_stop ) {
#else
		if( n->f_offset >= n->filesize ) {
#endif
			node_status( n, NODE_MODE_SEND_STOP );
			return;
		}
	}
}

void send_cork_stop( TCP_NODE *n ) {
	int off = 0;

	if( n->mode != NODE_MODE_SEND_STOP ) {
		return;
	}

	if( setsockopt( n->connfd, IPPROTO_TCP, TCP_CORK, &off, sizeof(off)) != 0 ) {
		fail( strerror( errno ) );
	}

	/* Clear input and output buffers */
	node_clearSendBuf( n );

	/* Mark TCP server ready */
	node_status( n, NODE_MODE_READY );

	/* HTTP Pipeline: There is at least one more request to parse. */
	if( n->recv_size > 0 ) {
		http_buf( n );
	}
}
