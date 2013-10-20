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
#include <signal.h>
#include <sys/epoll.h>
#include <sys/resource.h>

#include "malloc.h"
#include "thrd.h"
#include "tumbleweed.h"
#include "str.h"
#include "list.h"
#include "node_tcp.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "send_tcp.h"
#include "hash.h"
#include "mime.h"
#include "tcp.h"
#include "unix.h"
#include "http.h"
#include "worker.h"

struct obj_tcp *tcp_init( void ) {
	struct obj_tcp *tcp = ( struct obj_tcp * ) myalloc( sizeof( struct obj_tcp ) );

	/* Init server structure */
	tcp->s_addrlen = sizeof( IP );
	memset( ( char * ) &tcp->s_addr, '\0', tcp->s_addrlen );
	tcp->sockfd = -1;

	return tcp;
}

void tcp_free( void ) {
	myfree( _main->tcp );
}

void tcp_start( void ) {
	int listen_queue_length = SOMAXCONN * 8;
	int optval = 1;

	if( ( _main->tcp->sockfd = socket( PF_INET6, SOCK_STREAM, 0 ) ) < 0 ) {
		fail( "Creating socket failed." );
	}
	
	_main->tcp->s_addr.sin6_family = AF_INET6;
	_main->tcp->s_addr.sin6_port = htons( _main->conf->port );
	_main->tcp->s_addr.sin6_addr = in6addr_any;
	
	if( setsockopt( _main->tcp->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof( int ) ) == -1 ) {
		fail( "Setting SO_REUSEADDR failed" );
	}

	/* IP version */
#if 0
		info( NULL, 0, "IPv4 disabled" );
		if( setsockopt( _main->tcp->sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof( int ) ) == -1 ) {
			fail( "Setting IPV6_V6ONLY failed" );
		}
#endif

	if( bind( _main->tcp->sockfd, ( struct sockaddr * ) &_main->tcp->s_addr, _main->tcp->s_addrlen ) ) {
		fail( "bind() to socket failed." );
	}

	if( tcp_nonblocking( _main->tcp->sockfd ) < 0 ) {
		fail( "tcp_nonblocking( _main->tcp->sockfd ) failed" );
	}
	
	if( listen( _main->tcp->sockfd, listen_queue_length ) ) {
		fail( "listen() to socket failed." );
	} else {
		info( NULL, 0, "Listen queue length: %i", listen_queue_length );
	}

	/* Setup epoll */
	tcp_event();
}

void tcp_stop( void ) {
	/* Shutdown socket */
	if( shutdown( _main->tcp->sockfd, SHUT_RDWR ) != 0 ) {
		fail( "shutdown() failed." );
	}

	/* Close socket */
	if( close( _main->tcp->sockfd ) != 0 ) {
		fail( "close() failed." );
	}

	/* Close epoll */
	if( close( _main->tcp->epollfd ) != 0 ) {
		fail( "close() failed." );
	}
}

void tcp_event( void ) {
	struct epoll_event ev;
	
	_main->tcp->epollfd = epoll_create( 23 );
	if( _main->tcp->epollfd == -1 ) {
		fail( "epoll_create() failed" );
	}

	memset( &ev, '\0', sizeof( struct epoll_event ) );
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = _main->tcp->sockfd;
	if( epoll_ctl( _main->tcp->epollfd, EPOLL_CTL_ADD, _main->tcp->sockfd, &ev ) == -1 ) {
		fail( "tcp_event: epoll_ctl() failed" );
	}
}

void *tcp_thread( void *arg ) {
	struct epoll_event events[CONF_EPOLL_MAX_EVENTS];
	int nfds;
	int id = 0;

	mutex_block( _main->work->mutex );
	id = _main->work->id++;
	mutex_unblock( _main->work->mutex );
	
	info( NULL, 0, "Thread[%i] - Max events: %i", id, CONF_EPOLL_MAX_EVENTS );

	for( ;; ) {
		nfds = epoll_wait( _main->tcp->epollfd, events, CONF_EPOLL_MAX_EVENTS, CONF_EPOLL_WAIT );

		if( status == RUMBLE && nfds == -1 ) {
			if( errno != EINTR ) {
				info( NULL, 500, "epoll_wait() failed" );
				fail( strerror( errno ) );
			}
		} else if( status == RUMBLE && nfds == 0 ) {
			/* Timeout wakeup */
			if( id == 0 ) {
				tcp_cron();
			}
		} else if( status == RUMBLE && nfds > 0 ) {
			tcp_worker( events, nfds, id );
		} else {
			/* Shutdown server */
			break;
		}
	}

	pthread_exit( NULL );
}

void tcp_worker( struct epoll_event *events, int nfds, int thrd_id ) {
	ITEM *listItem = NULL;
	int i;

	mutex_block( _main->work->mutex );
	_main->work->active++;
	mutex_unblock( _main->work->mutex );

	for( i=0; i<nfds; i++ ) {
		if( events[i].data.fd == _main->tcp->sockfd ) {
			tcp_newconn();
		} else {
			listItem = events[i].data.ptr;
	
			if( events[i].events & EPOLLIN ) {
				tcp_input( listItem );
			} else if( events[i].events & EPOLLOUT ) {
				tcp_output( listItem );
			}

			/* Close, Input or Output next? */
			tcp_gate( listItem );
		}
	}

	mutex_block( _main->work->mutex );
	_main->work->active--;
	mutex_unblock( _main->work->mutex );
}

void tcp_gate( ITEM *listItem ) {
	TCP_NODE *n = list_value( listItem );

	switch( n->mode ) {
		case NODE_MODE_SHUTDOWN:
			node_shutdown( listItem );
			break;
		case NODE_MODE_READY:
			tcp_rearm( listItem, TCP_INPUT );
			break;
		case NODE_MODE_SEND_INIT:
		case NODE_MODE_SEND_MEM:
		case NODE_MODE_SEND_FILE:
		case NODE_MODE_SEND_STOP:
			tcp_rearm( listItem, TCP_OUTPUT );
			break;
		default:
			fail( "No shit" );
	}
}

void tcp_rearm( ITEM *listItem, int mode ) {
	TCP_NODE *n = list_value( listItem );
	struct epoll_event ev;

	memset( &ev, '\0', sizeof( struct epoll_event ) );

	if( mode == TCP_INPUT ) {
		ev.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
	} else {
		ev.events = EPOLLET | EPOLLOUT | EPOLLONESHOT;
	}
	
	ev.data.ptr = listItem;

	if( epoll_ctl( _main->tcp->epollfd, EPOLL_CTL_MOD, n->connfd, &ev ) == -1 ) {
		info( NULL, 500, strerror( errno ) );
		fail( "tcp_rearm: epoll_ctl() failed" );
	}
}

int tcp_nonblocking( int sock ) {
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

void tcp_newconn( void ) {
	struct epoll_event ev;
	ITEM *listItem = NULL;
	TCP_NODE *n = NULL;
	int connfd;
	IP c_addr;
	socklen_t c_addrlen = sizeof( IP );

	for( ;; ) {
		/* Prepare incoming connection */
		memset( ( char * ) &c_addr, '\0', c_addrlen );

		/* Accept */
		connfd = accept( _main->tcp->sockfd, ( struct sockaddr * ) &c_addr, &c_addrlen );
		if( connfd < 0 ) {
			if( errno == EAGAIN || errno == EWOULDBLOCK ) {
				break;
			} else {
				fail( strerror( errno ) );
			}
		}

		/* New connection: Create node object */
		if( ( listItem = node_put() ) == NULL ) {
			info( NULL, 500, "The linked list reached its limits" );
			node_disconnect( connfd );
			break;
		}
		
		/* Store data */
		n = list_value( listItem );
		n->connfd = connfd;
		memcpy( &n->c_addr, &c_addr, c_addrlen );
		n->c_addrlen = c_addrlen;

		/* Non blocking */
		if( tcp_nonblocking( n->connfd ) < 0 ) {
			fail( "tcp_nonblocking() failed" );
		}

		ev.events = EPOLLET | EPOLLIN | EPOLLONESHOT;
		ev.data.ptr = listItem;
		if( epoll_ctl( _main->tcp->epollfd, EPOLL_CTL_ADD, n->connfd, &ev ) == -1 ) {
			info( NULL, 500, strerror( errno ) );
			fail( "tcp_newconn: epoll_ctl( ) failed" );
		}
	}
}

void tcp_output( ITEM *listItem ) {
	TCP_NODE *n = list_value( listItem );
	
	switch( n->mode ) {
		case NODE_MODE_SEND_INIT:
		case NODE_MODE_SEND_MEM:
		case NODE_MODE_SEND_FILE:
		case NODE_MODE_SEND_STOP:
			
			/* Reset timeout counter */
			node_activity( n );
			
			/* Send file */
			send_tcp( n );
			break;
	}
}

void tcp_input( ITEM *listItem ) {
	TCP_NODE *n = list_value( listItem );
	char buffer[BUF_SIZE];
	ssize_t bytes = 0;
	
	while( status == RUMBLE ) {

		/* Reset timeout counter */
		node_activity( n );
		
		/* Get data */
		bytes = recv( n->connfd, buffer, BUF_OFF1, 0 );

		if( bytes < 0 ) {
			if( errno == EAGAIN || errno == EWOULDBLOCK ) {
				return;
			} else if( errno == ECONNRESET ) {
				/*
				 * Very common behaviour
				 * info( &n->c_addr, 0, "Connection reset by peer" );
				 */
				node_status( n, NODE_MODE_SHUTDOWN );
				return;
			} else {
				info( &n->c_addr, 0, "recv() failed:" );
				info( &n->c_addr, 0, strerror( errno ) );
				return;
			}
		} else if( bytes == 0 ) {
			/* Regular shutdown */
			node_status( n, NODE_MODE_SHUTDOWN );
			return;
		} else {
			/* Read */
			tcp_buffer( n, buffer, bytes );
			return;
		}
	}
}

void tcp_buffer( TCP_NODE *n, char *buffer, ssize_t bytes ) {
	/* Append buffer */
	node_appendBuffer( n, buffer, bytes );

	if( n->mode != NODE_MODE_READY ) {
		fail( "FIXME tcp_buffer..." );
	}

	/* Overflow? */
	if( n->recv_size >= BUF_OFF1 ) {
		info( &n->c_addr, 500, "Max head buffer exceeded..." );
		node_status( n, NODE_MODE_SHUTDOWN );
		return;
	}

	http_buf( n );
}

void tcp_cron( void ) {
	mutex_block( _main->work->mutex );

	/* Do some jobs while no worker is running */
	if( _main->work->active == 0 ) {
		node_cleanup();
	}
	mutex_unblock( _main->work->mutex );
}
