/*
Copyright 2011 Aiko Barz

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

#ifndef UDP_H
#define UDP_H

#include "torrentkino.h"
#include "p2p.h"
#include "worker.h"

#define UDP_BUF 1460

#define UDP_JOIN_MCAST 0
#define UDP_LEAVE_MCAST 1

struct obj_udp {
	/* Socket data */
	IP s_addr;
	socklen_t s_addrlen;
	int sockfd;

	/* Epoll */
	int epollfd;

	/* Listen to multicast address */
	int multicast;
};
typedef struct obj_udp UDP;

UDP *udp_init( void );
void udp_free( void );

void udp_start( void );
void udp_stop( void );

int udp_nonblocking( int sock );
void udp_event( void );

void *udp_thread( void *arg );
void *udp_client( void *arg );
void udp_worker( struct epoll_event *events, int nfds );
void udp_rearm( int sockfd );

void udp_input( int sockfd );
void udp_cron( void );

void udp_multicast( int mode );

#endif
