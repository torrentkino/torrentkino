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
#include "storage.h"
#include "search.h"
#include "time.h"


struct obj_store *stor_init(void) {
	struct obj_store *stor = (struct obj_store *) myalloc(sizeof(struct obj_store), "stor_init");
	stor->list = list_init();
	return stor;
}


void stor_free(void) {
	struct obj_item *item_st = NULL;
	struct obj_stnode *st = NULL;
	long int i = 0;

	item_st = _main->stor->list->start;
	for (i=0; i<_main->stor->list->counter; i++) {
		st = item_st->val;
		myfree(st, "stor_free");
		item_st = list_next(item_st);
	}
	list_free(_main->stor->list);
	
	myfree(_main->stor, "stor_free");
}


void stor_put(UCHAR *node_id, IP *sa) {
	struct obj_item *item_st = NULL;
	struct obj_stnode *st = NULL;
	char buffer[MAIN_BUF+1];

	/* It's me */
	if( node_me( node_id ) ) {
		return;
	}

	/* Create new storage place holder if necessary */
	if ( (item_st = stor_find_node(node_id)) != NULL ) {
		st = item_st->val;
	} else {
		st = (struct obj_stnode *) myalloc(sizeof(struct obj_stnode), "stor_put");
		list_put(_main->stor->list, st);
		snprintf(buffer, MAIN_BUF+1, "Increasing storage size to %li", _main->stor->list->counter);
		log_info(buffer);
	}

	/* Update node */
	memcpy(st->node_id, node_id, SHA_DIGEST_LENGTH);

	/* Availability */
	st->time_anno = time_add_15_min();
	memcpy(&st->c_addr, sa, sizeof(IP));
}


void stor_del(struct obj_item *item_st) {
	struct obj_stnode *st = item_st->val;
	myfree(st, "stor_del");
	list_del(_main->stor->list, item_st);
}


void stor_expire(void) {
	struct obj_item *item_st = NULL;
	struct obj_item *next_st = NULL;
	struct obj_stnode *st = NULL;
	long int i=0;

	item_st = _main->stor->list->start;
	for (i=0; i<_main->stor->list->counter; i++) {
		st = item_st->val;
		next_st = list_next(item_st);

		/* Delete node after 15 minutes without announce. */
		if (_main->p2p->time_now.tv_sec > st->time_anno) {
			stor_del(item_st);
		}

		item_st = next_st;
	}
}


struct obj_item *stor_find_node(UCHAR *node_id) {
	struct obj_item *item = NULL;
	struct obj_stnode *st = NULL;
	long int i = 0;

	item = _main->stor->list->start;
	for (i=0; i<_main->stor->list->counter; i++) {
		st = item->val;

		if (memcmp(node_id, st->node_id, SHA_DIGEST_LENGTH) == 0) {
			return item;
		}

		item = list_next(item);
	}

	return NULL;
}

void stor_send(IP *from, UCHAR *node_id, UCHAR *lkp_id, UCHAR *key_id) {
	struct obj_item *item_st = NULL;
	struct obj_stnode *st = NULL;
	
	if ( (item_st = stor_find_node(node_id)) == NULL ) {
		return;
	}
	st = item_st->val;

	/* Reply the stored IP address. */
	send_value( from, &st->c_addr, key_id, lkp_id );
}

IP *stor_address(UCHAR *node_id) {
	struct obj_item *item_st = NULL;
	struct obj_stnode *st = NULL;
	
	if ( (item_st = stor_find_node(node_id)) == NULL ) {
		return NULL;
	}
	st = item_st->val;

	/* Reply the stored IP address. */
	return &st->c_addr;
}
