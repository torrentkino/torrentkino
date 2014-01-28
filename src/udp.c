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

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "udp.h"

UDP *udp_init( void ) {
	UDP *udp = ( UDP * ) myalloc( sizeof( UDP ) );

	/* Init server structure */
	udp->s_addrlen = sizeof( IP );
	memset( ( char * ) &udp->s_addr, '\0', udp->s_addrlen );
	udp->sockfd = -1;

	/* Multicast listener state */
	udp->multicast = FALSE;

	return udp;
}

void udp_free( void ) {
	myfree( _main->udp );
}

void udp_start( void ) {
#ifdef IPV6
	int optval = 1;
#endif

#ifdef IPV6
	if( ( _main->udp->sockfd = socket( PF_INET6, SOCK_DGRAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	_main->udp->s_addr.sin6_family = AF_INET6;
	_main->udp->s_addr.sin6_port = htons( _main->conf->port );
	_main->udp->s_addr.sin6_addr = in6addr_any;
#elif IPV4
	if( ( _main->udp->sockfd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	_main->udp->s_addr.sin_family = AF_INET;
	_main->udp->s_addr.sin_port = htons( _main->conf->port );
	_main->udp->s_addr.sin_addr.s_addr = htonl( INADDR_ANY );
#endif

	/* Start multicast */
	udp_multicast( UDP_JOIN_MCAST );

#ifdef IPV6
	/* Disable IPv4 */
	if( setsockopt( _main->udp->sockfd, 
		IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof( int ) ) == -1 ) {
		fail( "Setting IPV6_V6ONLY failed" );
	}
#endif

	if( bind( _main->udp->sockfd,
		( struct sockaddr * ) &_main->udp->s_addr, _main->udp->s_addrlen ) ) {
		fail( "bind() to socket failed." );
	}

	if( udp_nonblocking( _main->udp->sockfd ) < 0 ) {
		fail( "udp_nonblocking( _main->udp->sockfd ) failed" );
	}

	/* Setup epoll */
	udp_event();
}

void udp_stop( void ) {
	
	/* Stop multicast */
	udp_multicast( UDP_LEAVE_MCAST );
	
	/* Close socket */
	if( close( _main->udp->sockfd ) != 0 ) {
		fail( "close() failed." );
	}

	/* Close epoll */
	if( close( _main->udp->epollfd ) != 0 ) {
		fail( "close() failed." );
	}
}

void udp_event( void ) {
	struct epoll_event ev;

	_main->udp->epollfd = epoll_create( 23 );
	if( _main->udp->epollfd == -1 ) {
		fail( "epoll_create() failed" );
	}

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.fd = _main->udp->sockfd;

	if( epoll_ctl( _main->udp->epollfd, EPOLL_CTL_ADD, _main->udp->sockfd,
		&ev ) == -1 ) {
		fail( "udp_event: epoll_ctl() failed" );
	}
}

void *udp_thread( void *arg ) {
	struct epoll_event events[CONF_EPOLL_MAX_EVENTS];
	int nfds;
	int id = 0;

	mutex_block( _main->work->mutex );
	id = _main->work->id++;
	mutex_unblock( _main->work->mutex );

	info( NULL, "UDP Thread[%i] - Max events: %i", id,
		CONF_EPOLL_MAX_EVENTS );

	while( status == RUMBLE ) {

		nfds = epoll_wait( _main->udp->epollfd, events,
			CONF_EPOLL_MAX_EVENTS, CONF_EPOLL_WAIT );

		/* Shutdown server */
		if( status != RUMBLE ) {
			break;
		}

		if( nfds == -1 ) {
			if( errno != EINTR ) {
				fail( "udp_thread: epoll_wait() failed / %s",
					strerror( errno ) );
			}
		} else if( nfds == 0 ) {
			/* Timeout wakeup */
			if( id == 0 ) {
				udp_cron();
			}
		} else if( nfds > 0 ) {
			udp_worker( events, nfds );
		}
	}

	pthread_exit( NULL );
}

void *udp_client( void *arg ) {

	if( nbhd_is_empty() ) {
		p2p_bootstrap();
		time_add_1_min_approx( &_main->p2p->time_restart );
	}

	pthread_exit( NULL );
}

void udp_worker( struct epoll_event *events, int nfds ) {
	int i;

	for( i=0; i<nfds; i++ ) {
		if( ( events[i].events & EPOLLIN ) == EPOLLIN ) {
			udp_input( events[i].data.fd );
			udp_rearm( events[i].data.fd );
		} else {
			info( NULL, "udp_worker: Unknown event" );
		}
	}
}

void udp_rearm( int sockfd ) {
	struct epoll_event ev;

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
	ev.data.fd = sockfd;

	if( epoll_ctl( _main->udp->epollfd, EPOLL_CTL_MOD, sockfd, &ev ) == -1 ) {
		fail( "udp_rearm: epoll_ctl() failed / %s", strerror( errno ) );
	}
}

int udp_nonblocking( int sock ) {
	int opts = fcntl( sock,F_GETFL );
	if( opts < 0 ) {
		return -1;
	}
	opts = opts|O_NONBLOCK;
	if( fcntl( sock,F_SETFL,opts ) < 0 ) {
		return -1;
	}

	return 1;
}

void udp_input( int sockfd ) {
	UCHAR buffer[UDP_BUF+1];
	ssize_t bytes = 0;
	IP c_addr;
	socklen_t c_addrlen = sizeof( IP );

	while( status == RUMBLE ) {
		memset( &c_addr, '\0', c_addrlen );
		memset( buffer, '\0', UDP_BUF+1 );

		bytes = recvfrom( sockfd, buffer, UDP_BUF, 0, 
			( struct sockaddr* )&c_addr, &c_addrlen );

		if( bytes < 0 ) {
			if( errno != EAGAIN && errno != EWOULDBLOCK ) {
				info( &c_addr, "UDP error while recvfrom" );
			}
			return;
		}

		if( bytes == 0 ) {
			info( &c_addr, "UDP error 0 bytes" );
			return;
		}

		/* Parse UDP packet */
		p2p_parse( buffer, bytes, &c_addr );

		/* Cron jobs */
		udp_cron();
	}
}

void udp_cron( void ) {
	mutex_block( _main->work->mutex );
	p2p_cron();
	mutex_unblock( _main->work->mutex );
}

#ifdef IPV6
void udp_multicast( int mode ) {
	struct ipv6_mreq mreq;
	int action = ( mode == UDP_JOIN_MCAST ) ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP;
	IP sin;

	memset( &sin, '\0', sizeof( IP ) );
	sin.sin6_family = AF_INET6;
	sin.sin6_port = htons( _main->conf->bootstrap_port );
	if( !inet_pton( AF_INET6, CONF_MULTICAST, &( sin.sin6_addr ) ) ) {
		return;
	}

	memset( &mreq, '\0', sizeof( mreq ) );
	memcpy( &mreq.ipv6mr_multiaddr,	&sin.sin6_addr,
		sizeof(	mreq.ipv6mr_multiaddr ) );
	mreq.ipv6mr_interface = 0;

	if( setsockopt( _main->udp->sockfd, IPPROTO_IPV6, action,
		&mreq, sizeof( mreq ) ) != 0 ) {
		info( NULL, "Joining multicast group failed: %s",
			strerror( errno ) );
		return;
	}

	_main->udp->multicast = ( mode == UDP_JOIN_MCAST ) ? TRUE : FALSE;
}
#elif IPV4
void udp_multicast( int mode ) {
	struct ip_mreq mreq;
	int action = ( mode == UDP_JOIN_MCAST ) ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
	IP sin;

	memset( &sin, '\0', sizeof( IP ) );
	sin.sin_family = AF_INET;
	sin.sin_port = htons( _main->conf->bootstrap_port );
	if( !inet_pton( AF_INET, CONF_MULTICAST, &( sin.sin_addr ) ) ) {
		return;
	}

	memset( &mreq, '\0', sizeof( mreq ) );
	memcpy( &mreq.imr_multiaddr, &sin.sin_addr,
		sizeof( mreq.imr_multiaddr ) );
	mreq.imr_interface.s_addr = INADDR_ANY;

	if( setsockopt( _main->udp->sockfd, IPPROTO_IP, action,
		&mreq, sizeof( mreq ) ) != 0 ) {
		info( NULL, "Joining multicast group failed: %s",
			strerror( errno ) );
		return;
	}

	_main->udp->multicast = ( mode == UDP_JOIN_MCAST ) ? TRUE : FALSE;
}
#endif
