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

#include "info_hash.h"

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
	IHASH *target = NULL;

	while( _main->infohash->list->item != NULL ) {
		target = list_value( _main->infohash->list->item );

		while( target->list->item != NULL ) {
			idb_del_node( target, target->list->item );
		}

		idb_del_target( _main->infohash->list->item );
	}
}

void idb_put( UCHAR *target_id, int port, UCHAR *node_id, IP *sa ) {
	INODE *inode = NULL;
	IHASH *target = NULL;
	ITEM *i = NULL;
	char hex[HEX_LEN];

	/* ID list */
	if ( (i = idb_find_target( target_id )) == NULL ) {

		target = (IHASH *) myalloc( sizeof(IHASH), "idb_put" );

		memcpy( target->target, target_id, SHA_DIGEST_LENGTH );
		target->list = list_init();
		target->hash = hash_init( 1000 );

		i = list_put(_main->infohash->list, target);
		hash_put(_main->infohash->hash, target->target, SHA_DIGEST_LENGTH, i );

		hex_hash_encode( hex, target_id );
		info( NULL, 0, "INFO_HASH: %s (%lu)",
			hex, list_size( _main->infohash->list ) );

	} else {

		target = list_value( i );

	}

	/* Node list */
	if ( (i = idb_find_node( target->hash, node_id )) == NULL ) {

		inode = (INODE *) myalloc( sizeof(INODE), "idb_put" );
		memcpy( inode->id, node_id, SHA_DIGEST_LENGTH );

		i = list_put( target->list, inode );
		hash_put( target->hash, inode->id, SHA_DIGEST_LENGTH, i );
	
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

void idb_expire( time_t now ) {
	ITEM *i_target = NULL;
	ITEM *n_target = NULL;
	IHASH *target = NULL;
	ITEM *i_node = NULL;
	ITEM *n_node = NULL;
	INODE *inode = NULL;
	char hex[HEX_LEN];

	i_target = list_start( _main->infohash->list );
	while( i_target != NULL ) {
		n_target = list_next(i_target);
		target = list_value( i_target );

		i_node = list_start( target->list );
		while( i_node != NULL ) {
			n_node = list_next(i_node);
			inode = list_value( i_node );

			/* Delete info_hash after 30 minutes without announcement. */
			if( now > inode->time_anno ) {
				idb_del_node(target, i_node);
			}

			i_node = n_node;
		}
		
		if( target->list->item == NULL ) {
			
			hex_hash_encode( hex, target->target );
			info( NULL, 0, "INFO_HASH: %s (%lu)",
				hex, list_size( _main->infohash->list ) );
			
			idb_del_target(i_target);
		}

		i_target = n_target;
	}
}

ITEM *idb_find_target( UCHAR *target ) {
	return hash_get( _main->infohash->hash, target, SHA_DIGEST_LENGTH );
}

void idb_del_target( ITEM *i_target ) {
	IHASH *target = list_value( i_target );

	list_clear( target->list );
	list_free( target->list );
	hash_free( target->hash );

	hash_del( _main->infohash->hash, target->target, SHA_DIGEST_LENGTH );
	list_del( _main->infohash->list, i_target );

	myfree( target, "idb_del" );
}

ITEM *idb_find_node( HASH *hash, UCHAR *node_id ) {
	return hash_get( hash, node_id, SHA_DIGEST_LENGTH );
}

void idb_del_node( IHASH *target, ITEM *i_node ) {
	INODE *inode = list_value( i_node );
	hash_del(target->hash, inode->id, SHA_DIGEST_LENGTH );
	list_del(target->list, i_node);
	myfree(inode, "idb_del");
}
