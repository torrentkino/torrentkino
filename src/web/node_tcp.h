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
#include "../shr/thrd.h"
#include "../shr/hash.h"
#include "../shr/list.h"
#include "../shr/log.h"
#include "response.h"

#define NODE_READY		1
#define NODE_SEND_INIT	2
#define NODE_SEND_DATA	3
#define NODE_SEND_STOP	4
#define NODE_SHUTDOWN 	5

typedef struct {
	int connfd;
	IP c_addr;
	socklen_t c_addrlen;

	/* Receive buffer */
	char recv_buf[BUF_SIZE];
	ssize_t recv_size;

	/* keepalive */
	int keepalive;

	/* Pipeline */
	int pipeline;

	/* Response list */
	LIST *response;

} TCP_NODE;

LIST *node_init( void );
void node_free( void );

ITEM *node_put( void );
void node_disconnect( int connfd );
void node_shutdown( ITEM *thisnode );
void node_status( TCP_NODE *n, int status );

ssize_t node_appendBuffer( TCP_NODE *n, char *buffer, ssize_t bytes );
void node_clearRecvBuf( TCP_NODE *n );

#endif /* NODE_TCP_H */
