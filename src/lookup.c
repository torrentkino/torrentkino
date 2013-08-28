/*
Copyright 2011 Aiko Barz

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
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "lookup.h"

LOOKUP *ldb_init( UCHAR *target, IP *from ) {
	LOOKUP *ldb = (LOOKUP *) myalloc( sizeof(LOOKUP), "ldb_init" );

	memcpy( ldb->target, target, SHA_DIGEST_LENGTH );
	
	if( from == NULL ) {
		ldb->send_reply = FALSE;
		memset( &ldb->c_addr, '\0', sizeof( IP ) );
	} else {
		ldb->send_reply = TRUE;
		memcpy( &ldb->c_addr, from, sizeof( IP ) );
	}

	ldb->nbhd = nbhd_init();
	
	return ldb;
}

void ldb_free( LOOKUP *ldb ) {
	if( ldb == NULL ) {
		return;
	}

	nbhd_free( ldb->nbhd );
	myfree( ldb, "ldb_free" );
}

int ldb_contacted_node( LOOKUP *ldb, UCHAR *node_id ) {
	return hash_exists( ldb->nbhd->hash, node_id, SHA_DIGEST_LENGTH);
}

void ldb_update_token( LOOKUP *ldb, UCHAR *node_id, BEN *token, IP *from ) {
	NODE *n = NULL;

	if( ( n = hash_get( ldb->nbhd->hash, node_id, SHA_DIGEST_LENGTH) ) == NULL ) {
		return;
	}
	
	memcpy( &n->c_addr, from, sizeof( IP ) );
	memcpy( &n->token, token->v.s->s, token->v.s->i );
	n->token_size = token->v.s->i;
}
