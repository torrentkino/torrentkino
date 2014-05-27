/*
Copyright 2014 Aiko Barz

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

#ifndef DNS_H
#define DNS_H

#include "resolver.h"
#include "torrentkino.h"
#include "worker.h"

#define DNS_BUF 1460

struct obj_dns {
	/* Socket data */
	IP s_addr;
	socklen_t s_addrlen;
	int sockfd;

	/* Epoll */
	int epollfd;
};
typedef struct obj_dns DNS;

DNS *dns_init( void );
void dns_free( void );

void dns_start( void );
void dns_stop( void );

int dns_nonblocking( int sock );
void dns_event( void );

void *dns_thread( void *arg );
void *dns_client( void *arg );
void dns_worker( struct epoll_event *events, int nfds );
void dns_rearm( int sockfd );

void dns_input( int sockfd );

#endif
