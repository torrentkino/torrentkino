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

	udp->type = udp_p2p_worker;

	return udp;
}

void udp_free( UDP *udp ) {
	myfree( udp );
}

void udp_start( UDP *udp, int port, int multicast_mode ) {
#ifdef IPV6
	int optval = 1;
#endif

#ifdef IPV6
	if( ( udp->sockfd = socket( PF_INET6, SOCK_DGRAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	udp->s_addr.sin6_family = AF_INET6;
	udp->s_addr.sin6_port = htons( port );
	udp->s_addr.sin6_addr = in6addr_any;
#elif IPV4
	if( ( udp->sockfd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	udp->s_addr.sin_family = AF_INET;
	udp->s_addr.sin_port = htons( port );
	udp->s_addr.sin_addr.s_addr = htonl( INADDR_ANY );
#endif

	/* Start multicast */
	udp_multicast( udp, multicast_mode, multicast_start );

	/* Type of operation */
	udp->type = ( port == _main->conf->p2p_port ) ? udp_p2p_worker : udp_dns_worker;

#ifdef IPV6
	/* Disable IPv4 P2P when operating on IPv6 */
	/* if( udp->type == udp_p2p_worker ) { */
		if( setsockopt( udp->sockfd,
			IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof( int ) ) == -1 ) {
			fail( "Setting IPV6_V6ONLY failed" );
		}
	/* } */
#endif

	if( bind( udp->sockfd,
		( struct sockaddr * ) &udp->s_addr, udp->s_addrlen ) ) {
		fail( "bind() to socket failed." );
	}

	if( udp_nonblocking( udp->sockfd ) < 0 ) {
		fail( "udp_nonblocking() failed" );
	}

	/* Setup epoll */
	udp_event( udp );
}

void udp_stop( UDP *udp, int multicast_mode ) {

	/* Stop multicast */
	udp_multicast( udp, multicast_mode, multicast_stop );

	/* Close socket */
	if( close( udp->sockfd ) != 0 ) {
		fail( "close() failed." );
	}

	/* Close epoll */
	if( close( udp->epollfd ) != 0 ) {
		fail( "close() failed." );
	}
}

void udp_event( UDP *udp ) {
	struct epoll_event ev;

	udp->epollfd = epoll_create( 23 );
	if( udp->epollfd == -1 ) {
		fail( "epoll_create() failed" );
	}

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.fd = udp->sockfd;

	if( epoll_ctl( udp->epollfd, EPOLL_CTL_ADD, udp->sockfd,
		&ev ) == -1 ) {
		fail( "udp_event: epoll_ctl() failed" );
	}
}

void *udp_thread( void *arg ) {
	UDP *udp = arg;
	struct epoll_event events[CONF_EPOLL_MAX_EVENTS];
	int nfds;
	int id = 0;

	mutex_block( _main->work->mutex );
	id = _main->work->id++;
	mutex_unblock( _main->work->mutex );

	info( _log, NULL, "UDP Thread[%i] - Max events: %i", id,
		CONF_EPOLL_MAX_EVENTS );

	while( status == RUMBLE ) {

		nfds = epoll_wait( udp->epollfd, events,
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
			udp_cron( udp );
		} else if( nfds > 0 ) {
			udp_worker( udp, events, nfds );
		}
	}

	pthread_exit( NULL );
}

void *udp_client( void *arg ) {
	UDP *udp = arg;

	if( udp->type != udp_p2p_worker ) {
		pthread_exit( NULL );
	}

	p2p_bootstrap();
	pthread_exit( NULL );
}

void udp_worker( UDP *udp, struct epoll_event *events, int nfds ) {
	int i;

	for( i=0; i<nfds; i++ ) {
		if( ( events[i].events & EPOLLIN ) == EPOLLIN ) {
			udp_input( udp, events[i].data.fd );
			udp_rearm( udp, events[i].data.fd );
		} else {
			info( _log, NULL, "udp_worker: Unknown event" );
		}
	}
}

void udp_rearm( UDP *udp, int sockfd ) {
	struct epoll_event ev;

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
	ev.data.fd = sockfd;

	if( epoll_ctl( udp->epollfd, EPOLL_CTL_MOD, sockfd, &ev ) == -1 ) {
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

void udp_input( UDP *udp, int sockfd ) {
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
				info( _log, &c_addr, "UDP error while recvfrom" );
			}
			return;
		}

		if( bytes == 0 ) {
			info( _log, &c_addr, "UDP error 0 bytes" );
			return;
		}

		if( udp->type == udp_p2p_worker ) {
			/* Parse UDP packet */
			p2p_parse( buffer, bytes, &c_addr );
			udp_cron( udp );
		} else {
			/* Parse DNS packet */
			r_parse( buffer, bytes, &c_addr );
		}
	}
}

void udp_cron( UDP *udp ) {
	if( udp->type != udp_p2p_worker ) {
		return;
	}

	mutex_block( _main->work->mutex );
	p2p_cron();
	mutex_unblock( _main->work->mutex );
}

#ifdef IPV6
void udp_multicast( UDP *udp, int mode, int runmode ) {
	struct ipv6_mreq mreq;
	int action;
	IP sin;

	if( mode == multicast_disabled ) {
		return;
	}

	action = ( runmode == multicast_start ) ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP;

	memset( &sin, '\0', sizeof( IP ) );
	sin.sin6_family = AF_INET6;
	sin.sin6_port = htons( _main->conf->bootstrap_port );
	if( !inet_pton( AF_INET6, MULTICAST_DEFAULT, &( sin.sin6_addr ) ) ) {
		return;
	}

	memset( &mreq, '\0', sizeof( mreq ) );
	memcpy( &mreq.ipv6mr_multiaddr,	&sin.sin6_addr,
		sizeof(	mreq.ipv6mr_multiaddr ) );
	mreq.ipv6mr_interface = 0;

	if( setsockopt( udp->sockfd, IPPROTO_IPV6, action,
		&mreq, sizeof( mreq ) ) != 0 ) {
		info( _log, NULL, "Joining multicast group failed: %s",
			strerror( errno ) );
		return;
	}

	udp->multicast = ( runmode == multicast_start ) ? TRUE : FALSE;
}
#elif IPV4
void udp_multicast( UDP *udp, int mode, int runmode ) {
	struct ip_mreq mreq;
	int action;
	IP sin;

	if( mode == multicast_disabled ) {
		return;
	}

	action = ( runmode == multicast_start ) ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;

	memset( &sin, '\0', sizeof( IP ) );
	sin.sin_family = AF_INET;
	sin.sin_port = htons( _main->conf->bootstrap_port );
	if( !inet_pton( AF_INET, MULTICAST_DEFAULT, &( sin.sin_addr ) ) ) {
		return;
	}

	memset( &mreq, '\0', sizeof( mreq ) );
	memcpy( &mreq.imr_multiaddr, &sin.sin_addr,
		sizeof( mreq.imr_multiaddr ) );
	mreq.imr_interface.s_addr = INADDR_ANY;

	if( setsockopt( udp->sockfd, IPPROTO_IP, action,
		&mreq, sizeof( mreq ) ) != 0 ) {
		info( _log, NULL, "Joining multicast group failed: %s",
			strerror( errno ) );
		return;
	}

	udp->multicast = ( runmode == multicast_start ) ? TRUE : FALSE;
}
#endif
