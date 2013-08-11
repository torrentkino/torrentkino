/*
Copyright 2011 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <netdb.h>

#include "malloc.h"
#include "thrd.h"
#include "masala-srv.h"
#include "str.h"
#include "list.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "hash.h"
#include "udp.h"
#include "unix.h"
#include "ben.h"
#include "token.h"
#include "neighbourhood.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "bucket.h"
#include "time.h"

struct obj_udp *udp_init( void ) {
	struct obj_udp *udp = (struct obj_udp *) myalloc( sizeof(struct obj_udp), "udp_init" );

	/* Init server structure */
	udp->s_addrlen = sizeof(IP );
	memset( (char *) &udp->s_addr, '\0', udp->s_addrlen );
	udp->sockfd = -1;

	/* Workers running */
	udp->id = 0;

	/* Listen to multicast */
	udp->multicast = 0;

	/* Worker */
	udp->threads = NULL;

	return udp;
}

void udp_free( void ) {
	myfree( _main->udp, "udp_free" );
}

void udp_start( void ) {
	int optval = 1;

	if( ( _main->udp->sockfd = socket( PF_INET6, SOCK_DGRAM, 0)) < 0 ) {
		log_fail( "Creating socket failed." );
	}
	_main->udp->s_addr.sin6_family = AF_INET6;
	_main->udp->s_addr.sin6_port = htons( _main->conf->port );
	_main->udp->s_addr.sin6_addr = in6addr_any;

	/* Listen to ff0e::1 */
	udp_multicast();

	/* Listen to IPv6 only */
	if( setsockopt( _main->udp->sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(int)) == -1 ) {
		log_fail( "Setting IPV6_V6ONLY failed" );
	}

	if( bind( _main->udp->sockfd,( struct sockaddr *) &_main->udp->s_addr, _main->udp->s_addrlen) ) {
		log_fail( "bind() to socket failed." );
	}

	if( udp_nonblocking( _main->udp->sockfd) < 0 ) {
		log_fail( "udp_nonblocking( _main->udp->sockfd) failed" );
	}
	
	/* Setup epoll */
	udp_event();
	
	/* Drop privileges */
	unix_dropuid0();

	/* Create worker */
	udp_pool();
}

void udp_stop( void ) {
	int i = 0;

	/* Join threads */
	pthread_attr_destroy( &_main->udp->attr );
	for( i=0; i <= _main->conf->cores; i++ ) {
		if( pthread_join( *_main->udp->threads[i], NULL) != 0 ) {
			log_fail( "pthread_join() failed" );
		}
		myfree( _main->udp->threads[i], "udp_pool" );
	}
	myfree( _main->udp->threads, "udp_pool" );

	/* Close socket */
	if( close( _main->udp->sockfd) != 0 ) {
		log_fail( "close() failed." );
	}

	/* Close epoll */
	if( close( _main->udp->epollfd) != 0 ) {
		log_fail( "close() failed." );
	}
}

void udp_event( void ) {
	struct epoll_event ev;


	_main->udp->epollfd = epoll_create( 23 );
	if( _main->udp->epollfd == -1 ) {
		log_fail( "epoll_create() failed" );
	}

	memset(&ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.fd = _main->udp->sockfd;
	
	if( epoll_ctl( _main->udp->epollfd, EPOLL_CTL_ADD, _main->udp->sockfd, &ev) == -1 ) {
		log_fail( "udp_event: epoll_ctl() failed" );
	}
}

void udp_pool( void ) {
	int i = 0;
	
	/* Initialize and set thread detached attribute */
	pthread_attr_init( &_main->udp->attr );
	pthread_attr_setdetachstate( &_main->udp->attr, PTHREAD_CREATE_JOINABLE );

	/* Create worker threads */
	_main->udp->threads = (pthread_t **) myalloc( (_main->conf->cores+1) * sizeof(pthread_t *), "udp_pool" );
	for( i=0; i < _main->conf->cores; i++ ) {
		_main->udp->threads[i] = (pthread_t *) myalloc( sizeof(pthread_t), "udp_pool" );
		if( pthread_create( _main->udp->threads[i], &_main->udp->attr, udp_thread, NULL) != 0 ) {
			log_fail( "pthread_create()" );
		}
	}

	/* Send 1st request while the workers are starting */
	_main->udp->threads[_main->conf->cores] = (pthread_t *) myalloc( sizeof(pthread_t), "udp_pool" );
	if( pthread_create( _main->udp->threads[_main->conf->cores], NULL, udp_client, NULL) != 0 ) {
		log_fail( "pthread_create()" );
	}
}

void *udp_thread( void *arg ) {
	struct epoll_event events[UDP_MAX_EVENTS];
	int nfds;
	int id = 0;

	mutex_block( _main->p2p->mutex );
	id = _main->udp->id++;
	mutex_unblock( _main->p2p->mutex );

	log_info( NULL, 0, "UDP Thread[%i] - Max events: %i", id, UDP_MAX_EVENTS );

	while( _main->status == RUMBLE ) {

		nfds = epoll_wait( _main->udp->epollfd, events, UDP_MAX_EVENTS, CONF_EPOLL_WAIT );

		/* Shutdown server */
		if( _main->status != RUMBLE ) {
			break;
		}

		if( nfds == -1 ) {
			if( errno != EINTR ) {
				log_fail( "udp_thread: epoll_wait() failed / %s", strerror( errno ) );
			}
		} else if( nfds == 0 ) {
			/* Timeout wakeup */
			if( id == 0 ) {
				udp_cron();
			}
		} else if( nfds > 0 ) {
			udp_worker( events, nfds, id );
		}
	}

	pthread_exit( NULL );
}

void *udp_client( void *arg ) {
	
	if( nbhd_is_empty( _main->nbhd ) ) {
		p2p_bootstrap();
		time_add_1_min_approx( &_main->p2p->time_restart );
	}

	pthread_exit( NULL );
}

void udp_worker( struct epoll_event *events, int nfds, int thrd_id ) {
	int i;

	for( i=0; i<nfds; i++ ) {
		if( ( events[i].events & EPOLLIN) == EPOLLIN ) {
			udp_input( events[i].data.fd );
			udp_rearm( events[i].data.fd );
		} else {
			log_info( NULL, 0, "udp_worker: Unknown event" );
		}
	}
}

void udp_rearm( int sockfd ) {
	struct epoll_event ev;

	memset(&ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
	ev.data.fd = sockfd;

	if( epoll_ctl( _main->udp->epollfd, EPOLL_CTL_MOD, sockfd, &ev) == -1 ) {
		log_fail( "udp_rearm: epoll_ctl() failed / %s", strerror( errno ) );
	}
}

int udp_nonblocking( int sock ) {
	int opts = fcntl( sock,F_GETFL );
	if( opts < 0 ) {
		return -1;
	}
	opts = opts|O_NONBLOCK;
	if( fcntl( sock,F_SETFL,opts) < 0 ) {
		return -1;
	}

	return 1;
}

void udp_input( int sockfd ) {
	UCHAR buffer[UDP_BUF+1];
	ssize_t bytes = 0;
	IP c_addr;
	socklen_t c_addrlen = sizeof(IP );

	while( _main->status == RUMBLE ) {
		memset( &c_addr, '\0', c_addrlen );
		memset( buffer, '\0', UDP_BUF+1 );

		bytes = recvfrom( sockfd, buffer, UDP_BUF, 0, 
			(struct sockaddr*)&c_addr, &c_addrlen );

		if( bytes < 0 ) {
			if( errno != EAGAIN && errno != EWOULDBLOCK ) {
				log_info( NULL, 0, "UDP error while recvfrom" );
			}
			return;
		}

		if( bytes == 0 ) {
			log_info( NULL, 0, "UDP error 0 bytes" );
			return;
		}
		
		p2p_parse( buffer, bytes, &c_addr );
		udp_cron();
	}
}

void udp_cron( void ) {
	mutex_block( _main->p2p->mutex );
	p2p_cron();
	mutex_unblock( _main->p2p->mutex );
}

void udp_multicast( void ) {
	struct addrinfo *multicast = NULL;
	struct addrinfo hints;
	struct ipv6_mreq mreq;

	/* Listen to ff0e::1 */
	memset( &hints, '\0', sizeof(hints) );
	if( getaddrinfo( "ff0e::1", _main->conf->bootstrap_port, &hints, &multicast) != 0 ) {
		log_fail( "getaddrinfo failed" );
	}
	memset( &mreq, '\0', sizeof(mreq) );
	memcpy( &mreq.ipv6mr_multiaddr, &((IP *) multicast->ai_addr)->sin6_addr, sizeof(mreq.ipv6mr_multiaddr) );
	mreq.ipv6mr_interface = 0;
	if( setsockopt( _main->udp->sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq)) == 0 ) {
		_main->udp->multicast = 1;
	} else {
		log_info( NULL, 0, "Trying to register multicast address failed: I will retry it later." );
	}
	freeaddrinfo( multicast );
}
