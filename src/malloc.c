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
#include "list.h"
#ifdef TUMBLEWEED
#include "node_web.h"
#endif
#include "log.h"

void *myalloc( long int size, const char *caller ) {
	void *memory = NULL;

	if( size == 0 ) {
		log_memfail( "myalloc(): Zero size?!", caller );
	} else if( size < 0 ) {
		log_memfail( "myalloc(): Negative size?!", caller );
	}

	memory = (void *) malloc( size );
	
	if( memory == NULL ) {
		log_memfail( "malloc() failed.", caller );
	}

	memset( memory, '\0', size );

	/*
	printf( "malloc( %s)\n", caller );
	*/

	return memory;
}

void *myrealloc( void *arg, long int size, const char *caller ) {
	if( size == 0 ) {
		log_memfail( "myrealloc(): Zero size?!", caller );
	} else if( size < 0 ) {
		log_memfail( "myrealloc(): Negative size?!", caller );
	}

	arg = (void *) realloc( arg, size );
	
	if( arg == NULL ) {
		log_memfail( "realloc() failed.", caller );
	}

	return arg;
}

void myfree( void *arg, const char *caller ) {
	if( arg != NULL ) {
		free( arg );
		/*
		printf( "free( %s)\n", caller );
		*/
	}
}
