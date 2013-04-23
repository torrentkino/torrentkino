/*
Copyright 2011 Aiko Barz

This file is part of Torrentkino.

Torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Torrentkino.  If not, see <http://www.gnu.org/licenses/>.
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
#include "node_p2p.h"
#include "p2p.h"
#include "bucket.h"
#include "send_p2p.h"
#include "database.h"
#include "search.h"
#include "time.h"


struct obj_database *db_init(void) {
	struct obj_database *database = (struct obj_database *) myalloc(sizeof(struct obj_database), "db_init");
	database->list = list_init();
	database->hash = hash_init( 4096 );
	return database;
}

void db_free(void) {
	list_clear(_main->database->list);
	list_free(_main->database->list);
	hash_free( _main->database->hash );
	myfree(_main->database, "db_free");
}

void db_put(UCHAR *host_id, IP *sa) {
	ITEM *i = NULL;
	DB *db = NULL;

	/* It's me */
	if( node_me( host_id ) ) {
		return;
	}

	/* Create new storage place holder if necessary */
	if ( (i = db_find( host_id )) == NULL ) {

		db = (DB *) myalloc(sizeof(DB), "db_put");
		memcpy(db->host_id, host_id, SHA_DIGEST_LENGTH);
		db_update(db, sa);

		i = list_put(_main->database->list, db);
		hash_put(_main->database->hash, db->host_id, SHA_DIGEST_LENGTH, i );

		log_info( NULL, 0, "Database size: %li (+1)", _main->database->list->counter);

	} else {
		db = i->val;
	}

	db_update(db, sa);
}

void db_update(DB *db, IP *sa) {
	db->time_anno = time_add_15_min();
	memcpy(&db->c_addr, sa, sizeof(IP));
}

void db_del(ITEM *i) {
	DB *db = i->val;
	hash_del(_main->database->hash, db->host_id, SHA_DIGEST_LENGTH );
	list_del(_main->database->list, i);
	myfree(db, "db_del");
}

void db_expire(void) {
	ITEM *i = NULL;
	ITEM *n = NULL;
	DB *db = NULL;
	long int j=0;

	i = _main->database->list->start;
	for (j=0; j<_main->database->list->counter; j++) {
		db = i->val;
		n = list_next(i);

		/* Delete node after 15 minutes without announcement. */
		if (_main->p2p->time_now.tv_sec > db->time_anno) {
			db_del(i);

			log_info( NULL, 0, "Database size: %li (-1)",
				_main->database->list->counter);
		}

		i = n;
	}
}

ITEM *db_find(UCHAR *host_id) {
	ITEM *i = NULL;

	if ( (i = hash_get( _main->database->hash, host_id, SHA_DIGEST_LENGTH )) != NULL ) {
		return i;
	}

	return NULL;
}

int db_send(IP *from, UCHAR *host_id, UCHAR *lkp_id, UCHAR *key_id) {
	ITEM *i = NULL;
	DB *db = NULL;
	
	if ( (i = db_find(host_id)) == NULL ) {
		return 0;
	}
	db = i->val;

	/* Reply the stored IP address. */
	send_value( from, &db->c_addr, key_id, lkp_id );

	return 1;
}

IP *db_address(UCHAR *host_id) {
	ITEM *i = NULL;
	DB *db = NULL;
	
	if ( (i = db_find(host_id)) == NULL ) {
		return NULL;
	}
	db = i->val;

	/* Reply the stored IP address. */
	return &db->c_addr;
}
