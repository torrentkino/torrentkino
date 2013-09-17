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

struct obj_idb *idb_init( void ) {
	struct obj_idb *idb = (struct obj_idb *) myalloc(sizeof(struct obj_idb), "idb_init");
	idb->list = list_init();
	idb->hash = hash_init( IDB_TARGET_SIZE_MAX );
	return idb;
}

void idb_free( void ) {
	idb_clean();
	list_clear( _main->idb->list );
	list_free( _main->idb->list );
	hash_free( _main->idb->hash );
	myfree( _main->idb, "idb_free" );
}

void idb_put( UCHAR *target_id, int port, UCHAR *node_id, IP *sa ) {
	TARGET *target = NULL;
	INODE *inode = NULL;
	ITEM *i = NULL;
	char hex[HEX_LEN];

	/* Enough... */
	if( tgt_limit_reached() ) {
		hex_hash_encode( hex, target_id );
		info( NULL, 0, "INFO_HASH LIMIT (%i) REACHED. NOT STORING %s...",
			IDB_TARGET_SIZE_MAX, hex );
		return;
	}

	/* ID list */
	if ( (i = tgt_find( target_id )) == NULL ) {
		target = tgt_init( target_id );
	} else {
		target = list_value( i );
	}

	/* Enough... */
	if( inode_limit_reached( target->list ) ) {
		hex_hash_encode( hex, target_id );
		info( NULL, 0, "NODE LIMIT (%i) REACHED FOR INFO_HASH %s...",
			IDB_NODES_SIZE_MAX, hex );
		return;
	}

	/* Node list */
	if ( (i = inode_find( target->hash, node_id )) == NULL ) {
		inode = inode_init( target, node_id );
	} else {
		inode = list_value( i );
	}

	inode_update( inode, sa, port );
}

void idb_clean( void ) {
	TARGET *target = NULL;

	while( _main->idb->list->item != NULL ) {
		target = list_value( _main->idb->list->item );

		while( target->list->item != NULL ) {
			inode_free( target, target->list->item );
		}

		tgt_free( _main->idb->list->item );
	}
}

void idb_expire( time_t now ) {
	ITEM *i_target = NULL;
	ITEM *n_target = NULL;
	TARGET *target = NULL;
	ITEM *i_node = NULL;
	ITEM *n_node = NULL;
	INODE *inode = NULL;

	i_target = list_start( _main->idb->list );
	while( i_target != NULL ) {
		n_target = list_next( i_target );
		target = list_value( i_target );

		i_node = list_start( target->list );
		while( i_node != NULL ) {
			n_node = list_next( i_node );
			inode = list_value( i_node );

			/* Delete info_hash after 30 minutes without announcement. */
			if( now > inode->time_anno ) {
				inode_free( target, i_node );
			}

			i_node = n_node;
		}
		
		if( list_size( target->list ) == 0 ) {
			tgt_free( i_target );
		}

		i_target = n_target;
	}
}

int idb_compact_list( UCHAR *nodes_compact_list, UCHAR *target_id ) {
	UCHAR *p = nodes_compact_list;
	INODE *inode = NULL;
	TARGET *target = NULL;
	ITEM *item = NULL;
	unsigned long int j = 0;
	IP *sin = NULL;
	int size = 0;

	/* Look into the local database */
	if( ( item = tgt_find( target_id ) ) == NULL ) {
		return 0;
	} else {
		target = list_value( item );
	}

	/* Walkthrough local database */
	item = list_start( target->list );
	while( item != NULL && j < 8 ) {
		inode = list_value( item );

		/* Network data */
		sin = (IP*)&inode->c_addr;

		/* IP */
		memcpy( p, (UCHAR *)&sin->sin6_addr, 16 );
		p += 16;

		/* Port */
		memcpy( p, (UCHAR *)&sin->sin6_port, 2 );
		p += 2;

		size += 18;

		item = list_next( item );
		j++;
	}

	/* Rotate the list. This only helps Bittorrent to get fresh IP addresses
	 * after each request. */
	list_rotate( target->list );

	return size;
}

TARGET *tgt_init( UCHAR *target_id ) {
	TARGET *target = NULL;
	ITEM *i = NULL;
	char hex[HEX_LEN];

	target = (TARGET *) myalloc( sizeof(TARGET), "tgt_init" );

	memcpy( target->target, target_id, SHA1_SIZE );
	target->list = list_init();
	target->hash = hash_init( IDB_NODES_SIZE_MAX );

	i = list_put(_main->idb->list, target);
	hash_put(_main->idb->hash, target->target, SHA1_SIZE, i );

	hex_hash_encode( hex, target_id );
	info( NULL, 0, "INFO_HASH: %s (%lu)",
		hex, list_size( _main->idb->list ) );

	return target;
}

void tgt_free( ITEM *i ) {
	TARGET *target = list_value( i );

	list_clear( target->list );
	list_free( target->list );
	hash_free( target->hash );

	hash_del( _main->idb->hash, target->target, SHA1_SIZE );
	list_del( _main->idb->list, i );

	myfree( target, "tgt_free" );
}

ITEM *tgt_find( UCHAR *target ) {
	return hash_get( _main->idb->hash, target, SHA1_SIZE );
}

int tgt_limit_reached( void ) {
	if( list_size( _main->idb->list ) < IDB_TARGET_SIZE_MAX ) {
		return FALSE;
	}
	return TRUE;
}

INODE *inode_init( TARGET *target, UCHAR *node_id ) {
	INODE *inode = NULL;
	ITEM *i = NULL;
	
	inode = (INODE *) myalloc( sizeof(INODE), "inode_init" );
	
	memcpy( inode->id, node_id, SHA1_SIZE );

	i = list_put( target->list, inode );
	hash_put( target->hash, inode->id, SHA1_SIZE, i );
	
	return inode;
}

void inode_free( TARGET *target, ITEM *i ) {
	INODE *inode = list_value( i );
	hash_del(target->hash, inode->id, SHA1_SIZE );
	list_del(target->list, i );
	myfree(inode, "inode_free");
}

ITEM *inode_find( HASH *target, UCHAR *node_id ) {
	return hash_get( target, node_id, SHA1_SIZE );
}

int inode_limit_reached( LIST *l ) {
	if( list_size( l ) < IDB_NODES_SIZE_MAX ) {
		return FALSE;
	}
	return TRUE;
}

void inode_update( INODE *inode, IP *sa, int port ) {
	time_add_30_min( &inode->time_anno );
	
	/* Store announcing IP address */
	memcpy(&inode->c_addr, sa, sizeof(IP));

	/* Store the announced port, not the the source port of the sender */
	inode->c_addr.sin6_port = htons(port);
}
