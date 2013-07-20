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

#include "malloc.h"
#include "thrd.h"
#include "main.h"
#include "str.h"
#include "list.h"
#include "hash.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "unix.h"
#include "udp.h"
#include "ben.h"
#include "token.h"
#include "neighbourhood.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "bucket.h"
#include "time.h"
#include "send_p2p.h"
#include "random.h"

struct obj_token *tkn_init( void ) {
	struct obj_token *token = (struct obj_token *) myalloc( sizeof(struct obj_token), "tkn_init" );
	token->list = list_init();
	token->hash = hash_init( 10 );
	return token;
}

void tkn_free( void ) {
	list_clear( _main->token->list );
	list_free( _main->token->list );
	hash_free( _main->token->hash );
	myfree( _main->token, "tkn_free" );
}

void tkn_put( void ) {
	ITEM *item_tkn = NULL;
	struct obj_tkn *tkn = NULL;

	tkn = (struct obj_tkn *) myalloc( sizeof(struct obj_tkn), "tkn_put" );

	/* ID */
	tkn_create( tkn->id );

	/* Availability */
	time_add_30_min( &tkn->time );

	item_tkn = list_put( _main->token->list, tkn );
	hash_put( _main->token->hash, tkn->id, TOKEN_SIZE, item_tkn );
}

void tkn_del( ITEM *item_tkn ) {
	struct obj_tkn *tkn = list_value( item_tkn );
	hash_del( _main->token->hash, tkn->id, TOKEN_SIZE );
	list_del( _main->token->list, item_tkn );
	myfree( tkn, "tkn_del" );
}

void tkn_expire( void ) {
	ITEM *item_tkn = NULL;
	ITEM *next_tkn = NULL;
	struct obj_tkn *tkn = NULL;

	item_tkn = list_start( _main->token->list );
	while( item_tkn != NULL ) {
		tkn = list_value( item_tkn );
		next_tkn = list_next( item_tkn );

		/* Bad token */
		if( _main->p2p->time_now.tv_sec > tkn->time ) {
			tkn_del( item_tkn );
		}
		item_tkn = next_tkn;
	}
}

int tkn_validate( UCHAR *id ) {
	ITEM *item_tkn = NULL;

	/* Token not found */
	if( ( item_tkn = hash_get( _main->token->hash, id, TOKEN_SIZE)) == NULL ) {
		return 0;
	}

	return 1;
}

void tkn_create( UCHAR *id ) {
	int i = 0;
	int max = 1000;

	/* Create unique ID but do not try this forever */
	do {
		rand_urandom( id, TOKEN_SIZE );
		i++;
	} while( hash_exists( _main->token->hash, id, TOKEN_SIZE) && i < max );

	if( i >= max ) {
		log_info( NULL, 0, "Tokens exhausted. Giving up." );
	}
}

UCHAR *tkn_read( void ) {
	ITEM *item_tkn = list_stop( _main->token->list );
	struct obj_tkn *tkn = list_value( item_tkn );

	/* Return newest token */
	return tkn->id;
}
