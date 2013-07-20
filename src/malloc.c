/*
Copyright 2006 Aiko Barz

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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#include "main.h"
#include "malloc.h"

#ifdef DEBUG
unsigned long int myalloc_counter = 0;
unsigned long int myfree_counter = 0;

void mem_init( void ) {
	myalloc_counter = 0;
	myfree_counter = 0;
}

void mem_print( const char *caller ) {
	printf( "%s: malloc: %lu, free: %lu\n", caller, myalloc_counter, myfree_counter );
}
#endif

void *myalloc( long int size, const char *caller ) {
	void *memory = NULL;

	if( size == 0 ) {
		fprintf( stderr, "myalloc(%s): Zero size?!", caller );
	} else if( size < 0 ) {
		fprintf( stderr, "myalloc(%s): Negative size?!", caller );
	}

	memory = (void *) malloc( size );
	
	if( memory == NULL ) {
		fprintf( stderr, "malloc(%s) failed.", caller );
	}

	memset( memory, '\0', size );

#ifdef DEBUG
	myalloc_counter++;
#endif

	return memory;
}

void *myrealloc( void *arg, long int size, const char *caller ) {
	if( size == 0 ) {
		fprintf( stderr, "myrealloc(%s): Zero size?!", caller );
	} else if( size < 0 ) {
		fprintf( stderr, "myrealloc(%s): Negative size?!", caller );
	}

	arg = (void *) realloc( arg, size );
	
	if( arg == NULL ) {
		fprintf( stderr, "realloc(%s) failed.", caller );
	}

	return arg;
}

void myfree( void *arg, const char *caller ) {
	if( arg == NULL ) {
		return;
	}
	
	free( arg );

#ifdef DEBUG
	myfree_counter++;
#endif
}
