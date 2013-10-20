/*
Copyright 2006 Aiko Barz

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

#include "malloc.h"

void *myalloc( long int size ) {
	void *memory = NULL;

	if( size == 0 ) {
		fprintf( stderr, "myalloc(): Zero size?!" );
	} else if( size < 0 ) {
		fprintf( stderr, "myalloc(): Negative size?!" );
	}

	memory = (void *) malloc( size );
	
	if( memory == NULL ) {
		fprintf( stderr, "malloc() failed." );
	}

	memset( memory, '\0', size );

	return memory;
}

void *myrealloc( void *arg, long int size ) {
	if( size == 0 ) {
		fprintf( stderr, "myrealloc(): Zero size?!" );
	} else if( size < 0 ) {
		fprintf( stderr, "myrealloc(): Negative size?!" );
	}

	arg = (void *) realloc( arg, size );
	
	if( arg == NULL ) {
		fprintf( stderr, "realloc() failed." );
	}

	return arg;
}

void myfree( void *arg ) {
	if( arg != NULL ) {
		free( arg );
	}
}
