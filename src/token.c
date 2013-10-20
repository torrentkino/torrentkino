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
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "token.h"
#include "torrentkino.h"
#include "info_hash.h"

struct obj_token *tkn_init( void ) {
	struct obj_token *token = (struct obj_token *) myalloc( sizeof(struct obj_token) );
	token->list = list_init();
	token->hash = hash_init( 10 );
	memset( token->null, '\0', TOKEN_SIZE );
	return token;
}

void tkn_free( void ) {
	list_clear( _main->token->list );
	list_free( _main->token->list );
	hash_free( _main->token->hash );
	myfree( _main->token );
}

void tkn_put( void ) {
	ITEM *item_tkn = NULL;
	struct obj_tkn *tkn = NULL;

	tkn = (struct obj_tkn *) myalloc( sizeof(struct obj_tkn) );

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
	myfree( tkn );
}

void tkn_expire( time_t now ) {
	ITEM *item_tkn = NULL;
	ITEM *next_tkn = NULL;
	struct obj_tkn *tkn = NULL;

	item_tkn = list_start( _main->token->list );
	while( item_tkn != NULL ) {
		tkn = list_value( item_tkn );
		next_tkn = list_next( item_tkn );

		/* Bad token */
		if( now > tkn->time ) {
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
		info( NULL, 0, "Tokens exhausted. Giving up." );
	}
}

UCHAR *tkn_read( void ) {
	ITEM *item_tkn = list_stop( _main->token->list );
	struct obj_tkn *tkn = list_value( item_tkn );

	/* Storage is full. Return null token. */
	if( tgt_limit_reached() ) {
		return _main->token->null;
	}

	/* Return newest token */
	return tkn->id;
}
