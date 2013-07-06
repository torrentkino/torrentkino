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
#include "database.h"
#include "search.h"
#include "time.h"
#include "hex.h"

struct obj_database *db_init(void) {
	struct obj_database *database = (struct obj_database *) myalloc(sizeof(struct obj_database), "db_init");
	database->list = list_init();
	database->hash = hash_init( 100 );
	return database;
}

void db_free(void) {
	db_clean();
	list_clear( _main->database->list );
	list_free( _main->database->list );
	hash_free( _main->database->hash );
	myfree( _main->database, "db_free" );
}

void db_clean(void) {
	DB_ID *db_id = NULL;

	while( _main->database->list->start != NULL ) {
		db_id = list_value( _main->database->list->start );

		while( db_id->list->start != NULL ) {
			db_del_node( db_id, db_id->list->start );
		}

		db_del_id( _main->database->list->start );
	}
}

void db_put(UCHAR *target, int port, UCHAR *node_id, IP *sa) {
	DB_NODE *db_node = NULL;
	DB_ID *db_id = NULL;
	ITEM *i = NULL;
	char hex[HEX_LEN];

	/* ID list */
	if ( (i = db_find_id( target )) == NULL ) {

		db_id = (DB_ID *) myalloc(sizeof(DB_ID), "db_put");
		memcpy(db_id->id, target, SHA_DIGEST_LENGTH);
		db_id->list = list_init();
		db_id->hash = hash_init( 100 );

		i = list_put(_main->database->list, db_id);
		hash_put(_main->database->hash, db_id->id, SHA_DIGEST_LENGTH, i );

		hex_hash_encode( hex, target );
		log_info( NULL, 0, "INFO_HASH: %li (+) %s",
			_main->database->list->counter, hex );

	} else {
		db_id = list_value( i );
	}

	/* Node list */
	if ( (i = db_find_node( db_id->hash, node_id )) == NULL ) {

		db_node = (DB_NODE *) myalloc(sizeof(DB_NODE), "db_put");
		memcpy(db_node->id, node_id, SHA_DIGEST_LENGTH);

		i = list_put(db_id->list, db_node);
		hash_put(db_id->hash, db_node->id, SHA_DIGEST_LENGTH, i );
	} else {
		db_node = list_value( i );
	}

	db_update(db_node, sa, port);
}

void db_update(DB_NODE *db, IP *sa, int port) {
	time_add_30_min( &db->time_anno );
	
	/* Store announcing IP address */
	memcpy(&db->c_addr, sa, sizeof(IP));

	/* Store the announced port, not the the source port of the sender */
	db->c_addr.sin6_port = htons(port);
}

void db_del_id(ITEM *i_id) {
	DB_ID *db_id = list_value( i_id );
	hash_del(_main->database->hash, db_id->id, SHA_DIGEST_LENGTH );
	list_del(_main->database->list, i_id);
	myfree(db_id, "db_del");
}

void db_del_node(DB_ID *db_id, ITEM *i_node) {
	DB_NODE *db_node = list_value( i_node );
	hash_del(db_id->hash, db_node->id, SHA_DIGEST_LENGTH );
	list_del(db_id->list, i_node);
	myfree(db_node, "db_del");
}

void db_expire(void) {
	ITEM *i_id = NULL;
	ITEM *n_id = NULL;
	DB_ID *db_id = NULL;
	ITEM *i_node = NULL;
	ITEM *n_node = NULL;
	DB_NODE *db_node = NULL;
	long int j = 0, k = 0;
	char hex[HEX_LEN];

	i_id = _main->database->list->start;
	for( j = 0; j < _main->database->list->counter; j++ ) {
		n_id = list_next(i_id);
		db_id = list_value( i_id );

		i_node = db_id->list->start;
		for( k = 0; k < db_id->list->counter; k++ ) {
			n_node = list_next(i_node);
			db_node = list_value( i_node );

			/* Delete info_hash after 30 minutes without announcement. */
			if( _main->p2p->time_now.tv_sec > db_node->time_anno ) {
				db_del_node(db_id, i_node);
			}

			i_node = n_node;
		}
		
		if( db_id->list->counter == 0 ) {
			db_del_id(i_id);

			hex_hash_encode( hex, db_id->id );
			log_info( NULL, 0, "INFO_HASH: %li (-) %s",
				_main->database->list->counter, hex );
		}

		i_id = n_id;
	}
}

ITEM *db_find_id(UCHAR *target) {
	return hash_get( _main->database->hash, target, SHA_DIGEST_LENGTH );
}

ITEM *db_find_node(HASH *hash, UCHAR *node_id) {
	return hash_get( hash, node_id, SHA_DIGEST_LENGTH );
}
