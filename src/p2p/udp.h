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
#include "worker.h"

#define UDP_BUF 1460

enum {
	multicast_enabled = 0,
	multicast_disabled = 1,
	multicast_start = 2,
	multicast_stop = 3,
};

enum {
	udp_dns_worker = 0,
	udp_p2p_worker = 1,
};

struct obj_udp {
	/* Socket data */
	IP s_addr;
	socklen_t s_addrlen;
	int sockfd;

	/* Epoll */
	int epollfd;

	/* Listen to multicast address */
	int multicast;

	/* Type of operation */
	int type;
};
typedef struct obj_udp UDP;

#include "p2p.h"

UDP *udp_init( void );
void udp_free( UDP *udp );

void udp_start( UDP *udp, int port, int multicast_mode );
void udp_stop( UDP *udp, int multicast_mode );

int udp_nonblocking( int sock );
void udp_event( UDP *udp );

void *udp_thread( void *arg );
void *udp_client( void *arg );
void udp_worker( UDP *udp, struct epoll_event *events, int nfds );
void udp_rearm( UDP *udp, int sockfd );

void udp_input( UDP *udp, int sockfd );
void udp_cron( UDP *udp );

void udp_multicast( UDP *udp, int mode, int runmode );

#endif
