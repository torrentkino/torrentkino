/*
Copyright 2009 Aiko Barz

This file is part of masala/tumbleweed.

masala/tumbleweed is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/tumbleweed is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/tumbleweed.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <signal.h>

#ifdef TUMBLEWEED
#include "main.h"
#include "list.h"
#include "node_web.h"
#include "log.h"
#include "malloc.h"
#else
#include "main.h"
#include "list.h"
#include "log.h"
#include "malloc.h"
#endif

#include "thrd.h"

sem_t *thrd_init( const char *semname ) {
	sem_t *mutex = NULL;

#ifdef __APPLE__
	if( ( mutex = sem_open( semname, O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED )
#else
	mutex = (sem_t *) myalloc( sizeof(sem_t), "thrd_init" );
	if( ( sem_init( mutex, 0, 1)) == -1 )
#endif
		fail( "Initialisation of a semaphore failed." );

	return mutex;
}

void thrd_destroy( sem_t *mutex ) {
	if( mutex != NULL ) {
#ifdef __APPLE__
		sem_close( mutex );
#else
		sem_destroy( mutex );
		myfree( mutex, "thrd_destroy" );
#endif
	}
}

void thrd_block( sem_t *mutex ) {
	sem_wait( mutex );
}

void thrd_unblock( sem_t *mutex ) {
	sem_post( mutex );
}

pthread_mutex_t *mutex_init( void ) {
	pthread_mutex_t *mutex = (pthread_mutex_t *) myalloc( sizeof(pthread_mutex_t), "mutex_init" );
	
	if( pthread_mutex_init( mutex, NULL) != 0 )
		fail( "pthread_mutex_init() failed." );

	return mutex;
}

void mutex_destroy( pthread_mutex_t *mutex ) {
	if( mutex != NULL ) {
		pthread_mutex_destroy( mutex );
		myfree( mutex, "mutex_destroy" );
	}
}

void mutex_block( pthread_mutex_t *mutex ) {
	pthread_mutex_lock( mutex );
}

void mutex_unblock( pthread_mutex_t *mutex ) {
	pthread_mutex_unlock( mutex );
}

pthread_cond_t *cond_init( void ) {
	pthread_cond_t *cond = (pthread_cond_t *) myalloc( sizeof(pthread_cond_t), "cond_init" );

	if( pthread_cond_init( cond, NULL) != 0 ) {
		fail( "pthread_cond_init() failed." );
	}

	return cond;
}

void cond_destroy( pthread_cond_t *cond ) {
	if( cond != NULL ) {
		pthread_cond_destroy( cond );
		myfree( cond, "cond_destroy" );
	}
}
