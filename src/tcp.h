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

#ifndef TCP_H
#define TCP_H

#include "list.h"
#include "node_tcp.h"

#define TCP_INPUT 0
#define TCP_OUTPUT 1

struct obj_tcp {
	/* Socket data */
	IP s_addr;
	socklen_t s_addrlen;
	int sockfd;

	/* Epoll */
	int epollfd;
};

struct obj_tcp *tcp_init( void );
void tcp_free( void );

void tcp_start( void );
void tcp_stop( void );

int tcp_nonblocking( int sock );
void tcp_event( void );
void tcp_cron( void );

void *tcp_thread( void *arg );
void tcp_worker( struct epoll_event *events, int nfds, int thrd_id );

void tcp_newconn( void );
void tcp_output( ITEM *listItem );
void tcp_input( ITEM *listItem );
void tcp_gate( ITEM *listItem );
void tcp_rearm( ITEM *listItem, int mode );

void tcp_buffer( TCP_NODE *n, char *buffer, ssize_t bytes );

#endif /* TCP_H */
