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
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <stdarg.h>

#include "response.h"

LIST *resp_init( void ) {
	return list_init();
}

void resp_free( LIST *list ) {
	list_clear( list );
	list_free( list );
}

RESPONSE *resp_put( LIST *list, int type ) {
	RESPONSE *r = NULL;

	/* Enough */
	if( list_size( list ) > 100 ) {
		return NULL;
	}

	r = (RESPONSE *) myalloc( sizeof( RESPONSE ) );

	/* Init response */
	if( type == RESPONSE_FROM_MEMORY ) {
		r->type = RESPONSE_FROM_MEMORY;
		memset( r->data.memory.send_buf, '\0', BUF_SIZE );
		r->data.memory.send_offset = 0;
		r->data.memory.send_size = 0;
	} else {
		r->type = RESPONSE_FROM_FILE;
		memset( r->data.file.filename, '\0', BUF_SIZE );
		r->data.file.f_offset = 0;
		r->data.file.f_stop = 0;
	}

	/* Connect response to the list */
	list_put( list, r );

	return r;
}


void resp_del( LIST *list, ITEM *item ) {
	RESPONSE *r = NULL;

	if( list == NULL ) {
		return;
	}
	if( item == NULL ) {
		return;
	}

	r = list_value( item );
	myfree( r );

	list_del( list, item );
}

int resp_set_memory( RESPONSE *r, const char *format, ... ) {
	va_list vlist;

	memset( r->data.memory.send_buf, '\0', BUF_SIZE );

	va_start( vlist, format );
	vsnprintf( r->data.memory.send_buf, BUF_SIZE, format, vlist );
	va_end( vlist );

	r->data.memory.send_size = strlen( r->data.memory.send_buf );

	return r->data.memory.send_size;
}
