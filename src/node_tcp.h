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

#ifndef NODE_TCP_H
#define NODE_TCP_H

#include "tumbleweed.h"
#include "thrd.h"
#include "hash.h"
#include "list.h"
#include "log.h"

#define NODE_MODE_READY   	1
#define NODE_MODE_SEND_INIT 3
#define NODE_MODE_SEND_MEM  4
#define NODE_MODE_SEND_FILE 5
#define NODE_MODE_SEND_STOP 6
#define NODE_MODE_SHUTDOWN  7

#define NODE_HANDSHAKE_READY 0
#define NODE_HANDSHAKE_ESTABLISHED 1

typedef struct {
	int connfd;
	IP c_addr;
	socklen_t c_addrlen;

	/* Receive buffer */
	char recv_buf[BUF_SIZE];
	ssize_t recv_size;

	/* Send buffer */
	char send_buf[BUF_SIZE];
	int send_size;
	int send_offset;

	/* URL */
	char entity_url[BUF_SIZE];

	/* File meta data */
	char filename[BUF_SIZE];
	size_t filesize;
	off_t f_offset;
#ifdef RANGE
	off_t f_stop;
	size_t content_length;

	/* HTTP Range */
	off_t range_start;
	off_t range_stop;
#endif

	/* HTTP Keep-Alive */
	int keepalive;
	int keepalive_counter;

	/* HTTP Last-Modified */
	char lastmodified[DATE_SIZE];

	/* HTTP code */
	int code;

	/* HTTP type */
	int type;

	/* HTTP protocol */
	int proto;

	int mode;
} TCP_NODE;

LIST *node_init( void );
void node_free( void );

ITEM *node_put( void );
void node_disconnect( int connfd );
void node_shutdown( ITEM *thisnode );
void node_status( TCP_NODE *n, int status );
void node_activity( TCP_NODE *n );

ssize_t node_appendBuffer( TCP_NODE *n, char *buffer, ssize_t bytes );
void node_clearSendBuf( TCP_NODE *n );
void node_clearRecvBuf( TCP_NODE *n );

void node_cleanup( void );

#endif /* NODE_TCP_H */
