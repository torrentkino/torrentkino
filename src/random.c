/*
Copyright 2006 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <netinet/in.h>

#include "main.h"
#include "str.h"
#include "list.h"
#include "file.h"
#include "random.h"
#include "conf.h"
#include "malloc.h"
#include "log.h"

void rand_urandom( void *buffer, size_t size ) {
	UCHAR *random = NULL;

	if( ( random = (UCHAR *)file_load( "/dev/urandom", 0, size)) == NULL ) {
		log_fail( "Failed to read /dev/urandom" );
	}

	memcpy( buffer, random, size );

	myfree( random, "rand_urandom" );
}
