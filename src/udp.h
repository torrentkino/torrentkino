/*
Copyright 2011 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

#define UDP_MAX_EVENTS 32
#define UDP_BUF 1460

struct obj_udp {
	/* Socket data */
	struct sockaddr_in6 s_addr;
	socklen_t s_addrlen;
	int sockfd;

	/* Epoll */
	int epollfd;

	/* Listen to multicast address */
	int multicast;

	/* Worker index */
	int id;

	/* Worker */
	pthread_t **threads;
	pthread_attr_t attr;
};

struct obj_udp *udp_init( void );
void udp_free( void );

void udp_start( void );
void udp_stop( void );

int udp_nonblocking( int sock );
void udp_event( void );

void udp_pool( void );
void *udp_thread( void *arg );
void *udp_client( void *arg );
void udp_worker( struct epoll_event *events, int nfds, int thrd_id );
void udp_rearm( int sockfd );

void udp_input( int sockfd );
void udp_cron( void );

void udp_multicast( void );
