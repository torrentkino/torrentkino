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

#include "dns.h"

DNS *dns_init( void ) {
	DNS *dns = ( DNS * ) myalloc( sizeof( DNS ) );

	/* Init server structure */
	dns->s_addrlen = sizeof( IP );
	memset( ( char * ) &dns->s_addr, '\0', dns->s_addrlen );
	dns->sockfd = -1;

	return dns;
}

void dns_free( void ) {
	myfree( _main->dns );
}

void dns_start( void ) {
#if 0
	int optval = 1;
#endif

#ifdef IPV6
	if( ( _main->dns->sockfd = socket( PF_INET6, SOCK_DGRAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	_main->dns->s_addr.sin6_family = AF_INET6;
	_main->dns->s_addr.sin6_port = htons( 53 );
	_main->dns->s_addr.sin6_addr = in6addr_any;
#elif IPV4
	if( ( _main->dns->sockfd = socket( PF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	_main->dns->s_addr.sin_family = AF_INET;
	_main->dns->s_addr.sin_port = htons( 53 );
	_main->dns->s_addr.sin_addr.s_addr = htonl( INADDR_ANY );
#endif

#if 0
	/* Disable IPv4 */
	if( setsockopt( _main->dns->sockfd,
		IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof( int ) ) == -1 ) {
		fail( "Setting IPV6_V6ONLY failed" );
	}
#endif

	if( bind( _main->dns->sockfd,
		( struct sockaddr * ) &_main->dns->s_addr, _main->dns->s_addrlen ) ) {
		fail( "bind() to socket failed." );
	}

	if( dns_nonblocking( _main->dns->sockfd ) < 0 ) {
		fail( "dns_nonblocking( _main->dns->sockfd ) failed" );
	}

	/* Setup epoll */
	dns_event();
}

void dns_stop( void ) {

	/* Close socket */
	if( close( _main->dns->sockfd ) != 0 ) {
		fail( "close() failed." );
	}

	/* Close epoll */
	if( close( _main->dns->epollfd ) != 0 ) {
		fail( "close() failed." );
	}
}

void dns_event( void ) {
	struct epoll_event ev;

	_main->dns->epollfd = epoll_create( 23 );
	if( _main->dns->epollfd == -1 ) {
		fail( "epoll_create() failed" );
	}

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.fd = _main->dns->sockfd;

	if( epoll_ctl( _main->dns->epollfd, EPOLL_CTL_ADD, _main->dns->sockfd,
		&ev ) == -1 ) {
		fail( "dns_event: epoll_ctl() failed" );
	}
}

void *dns_thread( void *arg ) {
	struct epoll_event events[CONF_EPOLL_MAX_EVENTS];
	int nfds;
	int id = 0;

	mutex_block( _main->work->mutex );
	id = _main->work->id++;
	mutex_unblock( _main->work->mutex );

	info( NULL, "DNS Thread[%i] - Max events: %i", id,
		CONF_EPOLL_MAX_EVENTS );

	while( status == RUMBLE ) {

		nfds = epoll_wait( _main->dns->epollfd, events,
			CONF_EPOLL_MAX_EVENTS, CONF_EPOLL_WAIT );

		/* Shutdown server */
		if( status != RUMBLE ) {
			break;
		}

		if( nfds == -1 ) {
			if( errno != EINTR ) {
				fail( "dns_thread: epoll_wait() failed / %s",
					strerror( errno ) );
			}
		} else if( nfds == 0 ) {
			/* Timeout wakeup */
		} else if( nfds > 0 ) {
			dns_worker( events, nfds );
		}
	}

	pthread_exit( NULL );
}

void dns_worker( struct epoll_event *events, int nfds ) {
	int i;

	for( i=0; i<nfds; i++ ) {
		if( ( events[i].events & EPOLLIN ) == EPOLLIN ) {
			dns_input( events[i].data.fd );
			dns_rearm( events[i].data.fd );
		} else {
			info( NULL, "dns_worker: Unknown event" );
		}
	}
}

void dns_rearm( int sockfd ) {
	struct epoll_event ev;

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
	ev.data.fd = sockfd;

	if( epoll_ctl( _main->dns->epollfd, EPOLL_CTL_MOD, sockfd, &ev ) == -1 ) {
		fail( "dns_rearm: epoll_ctl() failed / %s", strerror( errno ) );
	}
}

int dns_nonblocking( int sock ) {
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

void dns_input( int sockfd ) {
	UCHAR buffer[DNS_BUF+1];
	ssize_t bytes = 0;
	IP c_addr;
	socklen_t c_addrlen = sizeof( IP );

	while( status == RUMBLE ) {
		memset( &c_addr, '\0', c_addrlen );
		memset( buffer, '\0', DNS_BUF+1 );

		bytes = recvfrom( sockfd, buffer, DNS_BUF, 0, 
			( struct sockaddr* )&c_addr, &c_addrlen );

		if( bytes < 0 ) {
			if( errno != EAGAIN && errno != EWOULDBLOCK ) {
				info( &c_addr, "DNS error while recvfrom" );
			}
			return;
		}

		if( bytes == 0 ) {
			info( &c_addr, "DNS error 0 bytes" );
			return;
		}

		/* Parse DNS packet */
		r_parse( buffer, bytes, &c_addr );
	}
}
