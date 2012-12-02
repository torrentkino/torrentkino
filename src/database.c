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
#include <openssl/ssl.h>

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
	struct obj_database *stor = (struct obj_database *) myalloc(sizeof(struct
			obj_database), "db_init");
	stor->list = list_init();
	return stor;
}

void db_free(void) {
	list_clear(_main->database->list);
	list_free(_main->database->list);
	myfree(_main->database, "db_free");
}


void db_put(UCHAR *host_id, IP *sa) {
	ITEM *i = NULL;
	DB *st = NULL;
	char buffer[MAIN_BUF+1];

	/* It's me */
	if( node_me( host_id ) ) {
		return;
	}

	/* Create new storage place holder if necessary */
	if ( (i = db_find_node(host_id)) != NULL ) {
		st = i->val;
	} else {
		st = (DB *) myalloc(sizeof(DB), "db_put");
		list_put(_main->database->list, st);
		snprintf(buffer, MAIN_BUF+1, "Increasing storage size to %li", _main->database->list->counter);
		log_info(buffer);
	}

	/* Update node */
	memcpy(st->host_id, host_id, SHA_DIGEST_LENGTH);

	/* Availability */
	st->time_anno = time_add_15_min();
	memcpy(&st->c_addr, sa, sizeof(IP));
}

void db_del(ITEM *i) {
	DB *st = i->val;
	myfree(st, "db_del");
	list_del(_main->database->list, i);
}

void db_expire(void) {
	ITEM *i = NULL;
	ITEM *next_st = NULL;
	DB *st = NULL;
	long int j=0;

	i = _main->database->list->start;
	for (j=0; j<_main->database->list->counter; j++) {
		st = i->val;
		next_st = list_next(i);

		/* Delete node after 15 minutes without announce. */
		if (_main->p2p->time_now.tv_sec > st->time_anno) {
			db_del(i);
		}

		i = next_st;
	}
}

ITEM *db_find_node(UCHAR *host_id) {
	ITEM *item = NULL;
	DB *st = NULL;
	long int i = 0;

	item = _main->database->list->start;
	for (i=0; i<_main->database->list->counter; i++) {
		st = item->val;

		if (memcmp(host_id, st->host_id, SHA_DIGEST_LENGTH) == 0) {
			return item;
		}

		item = list_next(item);
	}

	return NULL;
}

void db_send(IP *from, UCHAR *host_id, UCHAR *lkp_id, UCHAR *key_id) {
	ITEM *i = NULL;
	DB *st = NULL;
	
	if ( (i = db_find_node(host_id)) == NULL ) {
		return;
	}
	st = i->val;

	/* Reply the stored IP address. */
	send_value( from, &st->c_addr, key_id, lkp_id );
}

IP *db_address(UCHAR *host_id) {
	ITEM *i = NULL;
	DB *st = NULL;
	
	if ( (i = db_find_node(host_id)) == NULL ) {
		return NULL;
	}
	st = i->val;

	/* Reply the stored IP address. */
	return &st->c_addr;
}
