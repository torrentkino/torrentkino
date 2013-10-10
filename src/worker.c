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
#include <netdb.h>

#include "worker.h"

#ifdef TUMBLEWEED
#include "tcp.h"
#include "tumbleweed.h"
#endif

#ifdef TORRENTKINO
#include "udp.h"
#include "torrentkino.h"
#endif

struct obj_work *work_init( void ) {
	struct obj_work *work = (struct obj_work *) myalloc( sizeof(struct obj_work), "work_init" );
	work->mutex = mutex_init();
	work->threads = NULL;
	work->id = 0;
	work->active = 0;
#ifdef TORRENTKINO
	work->number_of_threads = _main->conf->cores + 1;
#endif
#ifdef TUMBLEWEED
	work->number_of_threads = _main->conf->cores;
	work->tcp_node = mutex_init();
#endif
	return work;
}

void work_free( void ) {
	mutex_destroy( _main->work->mutex );
#ifdef TUMBLEWEED
	mutex_destroy( _main->work->tcp_node );
#endif
	myfree( _main->work, "work_free" );
}

void work_start( void ) {
	int i = 0;

	info( NULL, 0, "Starting %i worker", _main->work->number_of_threads );

	/* Initialize and set thread detached attribute */
	pthread_attr_init( &_main->work->attr );
	pthread_attr_setdetachstate( &_main->work->attr, PTHREAD_CREATE_JOINABLE );

	/* Thread index */

#ifdef TUMBLEWEED
	_main->work->threads = (pthread_t **) myalloc( 
		_main->work->number_of_threads * sizeof(pthread_t *), "work_start" );
	while( i < _main->work->number_of_threads ) {
		_main->work->threads[i] = (pthread_t *) myalloc( sizeof(pthread_t), "work_start" );
		if( pthread_create( _main->work->threads[i], &_main->work->attr, tcp_thread, NULL ) != 0 ) {
			fail( "pthread_create()" );
		}
		i++;
	}
#endif

#ifdef TORRENTKINO
	_main->work->threads = (pthread_t **) myalloc( 
		_main->work->number_of_threads * sizeof(pthread_t *), "work_start" );
	while( i < _main->work->number_of_threads - 1 ) {
		_main->work->threads[i] = (pthread_t *) myalloc( sizeof(pthread_t), "work_start" );
		if( pthread_create( _main->work->threads[i], &_main->work->attr, udp_thread, NULL ) != 0 ) {
			fail( "pthread_create()" );
		}
		i++;
	}

	/* Send 1st request while the UDP worker is starting */
	_main->work->threads[i] = (pthread_t *) myalloc( sizeof(pthread_t), "work_start" );
	if( pthread_create( _main->work->threads[i], &_main->work->attr, udp_client, NULL ) != 0 ) {
		fail( "pthread_create()" );
	}
	i++;
#endif
}

void work_stop( void ) {
	int i = 0;

	/* Join threads */
	pthread_attr_destroy( &_main->work->attr );
	for( i=0; i < _main->work->number_of_threads; i++ ) {
		if( pthread_join( *_main->work->threads[i], NULL) != 0 ) {
			fail( "pthread_join() failed" );
		}
		myfree( _main->work->threads[i], "work_stop" );
	}
	myfree( _main->work->threads, "work_stop" );
}
