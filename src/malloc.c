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
	if( arg != NULL ) {
		free( arg );
	}
}
