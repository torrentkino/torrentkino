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
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>

#include "malloc.h"
#include "main.h"
#include "log.h"
#include "conf.h"
#include "udp.h"
#include "str.h"
#include "list.h"
#include "ben.h"
#include "unix.h"
#include "hash.h"
#include "token.h"
#include "neighbourhood.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "bucket.h"
#include "send_p2p.h"
#include "info_hash.h"
#include "search.h"
#include "time.h"
#include "hex.h"

struct obj_infohash *idb_init( void ) {
	struct obj_infohash *infohash = (struct obj_infohash *) myalloc(sizeof(struct obj_infohash), "idb_init");
	infohash->list = list_init();
	infohash->hash = hash_init( 1000 );
	return infohash;
}

void idb_free( void ) {
	idb_clean();
	list_clear( _main->infohash->list );
	list_free( _main->infohash->list );
	hash_free( _main->infohash->hash );
	myfree( _main->infohash, "idb_free" );
}

void idb_clean( void ) {
	IHASH *ihash = NULL;

	while( _main->infohash->list->start != NULL ) {
		ihash = list_value( _main->infohash->list->start );

		while( ihash->list->start != NULL ) {
			idb_del_node( ihash, ihash->list->start );
		}

		idb_del_target( _main->infohash->list->start );
	}
}

void idb_put( UCHAR *target, int port, UCHAR *node_id, IP *sa ) {
	INODE *inode = NULL;
	IHASH *ihash = NULL;
	ITEM *i = NULL;
	char hex[HEX_LEN];

	/* ID list */
	if ( (i = idb_find_target( target )) == NULL ) {

		ihash = (IHASH *) myalloc( sizeof(IHASH), "idb_put" );

		memcpy( ihash->target, target, SHA_DIGEST_LENGTH );
		ihash->list = list_init();
		ihash->hash = hash_init( 1000 );

		i = list_put(_main->infohash->list, ihash);
		hash_put(_main->infohash->hash, ihash->target, SHA_DIGEST_LENGTH, i );

		hex_hash_encode( hex, target );
		log_info( NULL, 0, "INFO_HASH: %li (+) %s",
			_main->infohash->list->counter, hex );

	} else {

		ihash = list_value( i );

	}

	/* Node list */
	if ( (i = idb_find_node( ihash->hash, node_id )) == NULL ) {

		inode = (INODE *) myalloc( sizeof(INODE), "idb_put" );
		memcpy( inode->id, node_id, SHA_DIGEST_LENGTH );

		i = list_put( ihash->list, inode );
		hash_put( ihash->hash, inode->id, SHA_DIGEST_LENGTH, i );
	
	} else {
	
		inode = list_value( i );
	
	}

	idb_update(inode, sa, port);
}

void idb_update( INODE *db, IP *sa, int port ) {
	time_add_30_min( &db->time_anno );
	
	/* Store announcing IP address */
	memcpy(&db->c_addr, sa, sizeof(IP));

	/* Store the announced port, not the the source port of the sender */
	db->c_addr.sin6_port = htons(port);
}

void idb_expire( void ) {
	ITEM *i_id = NULL;
	ITEM *n_id = NULL;
	IHASH *ihash = NULL;
	ITEM *i_node = NULL;
	ITEM *n_node = NULL;
	INODE *inode = NULL;
	long int j = 0, k = 0;
	char hex[HEX_LEN];

	i_id = _main->infohash->list->start;
	for( j = 0; j < _main->infohash->list->counter; j++ ) {
		n_id = list_next(i_id);
		ihash = list_value( i_id );

		i_node = ihash->list->start;
		for( k = 0; k < ihash->list->counter; k++ ) {
			n_node = list_next(i_node);
			inode = list_value( i_node );

			/* Delete info_hash after 30 minutes without announcement. */
			if( _main->p2p->time_now.tv_sec > inode->time_anno ) {
				idb_del_node(ihash, i_node);
			}

			i_node = n_node;
		}
		
		if( ihash->list->counter == 0 ) {
			
			hex_hash_encode( hex, ihash->target );
			log_info( NULL, 0, "INFO_HASH: %li (-) %s",
				_main->infohash->list->counter, hex );
			
			idb_del_target(i_id);
		}

		i_id = n_id;
	}
}

ITEM *idb_find_target( UCHAR *target ) {
	return hash_get( _main->infohash->hash, target, SHA_DIGEST_LENGTH );
}

void idb_del_target( ITEM *i_id ) {
	IHASH *ihash = list_value( i_id );
	hash_del( _main->infohash->hash, ihash->target, SHA_DIGEST_LENGTH );
	list_del( _main->infohash->list, i_id );
	myfree( ihash, "idb_del" );
}

ITEM *idb_find_node( HASH *hash, UCHAR *node_id ) {
	return hash_get( hash, node_id, SHA_DIGEST_LENGTH );
}

void idb_del_node( IHASH *ihash, ITEM *i_node ) {
	INODE *inode = list_value( i_node );
	hash_del(ihash->hash, inode->id, SHA_DIGEST_LENGTH );
	list_del(ihash->list, i_node);
	myfree(inode, "idb_del");
}
