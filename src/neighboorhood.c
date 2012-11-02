/*
Copyright 2011 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
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
#include <openssl/ssl.h>

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
#include "p2p.h"
#include "node_p2p.h"
#include "bucket.h"
#include "send_p2p.h"
#include "lookup.h"
#include "neighboorhood.h"
#include "time.h"
#include "hex.h"
#include "random.h"

LIST *nbhd_init(void ) {
	return bckt_init();
}

void nbhd_free(void ) {
	bckt_free(_main->nbhd);
}

void nbhd_put(struct obj_nodeItem *n ) {
	bckt_put(_main->nbhd, n);
}

void nbhd_del(struct obj_nodeItem *n ) {
	bckt_del(_main->nbhd, n);
}

void nbhd_split(void ) {
	/* Do as many splits as neccessary */
	for ( ;; ) {
		if ( !bckt_split(_main->nbhd, _main->conf->host_id) ) {
			return;
		}
		nbhd_print();
	}
}

void nbhd_send(CIPV6 *sa, UCHAR *node_id, UCHAR *lkp_id, UCHAR *node_sk, int warning ) {
	ITEM *i = NULL;
	struct obj_bckt *b = NULL;

	if ( (i = bckt_find_any_match(_main->nbhd, node_id)) == NULL ) {
		return;
	}
	b = i->val;

	send_node(sa, b, node_sk, lkp_id, warning);
}

void nbhd_ping(void ) {
	ITEM *item_b = NULL;
	struct obj_bckt *b = NULL;
	ITEM *item_n = NULL;
	struct obj_nodeItem *n = NULL;
	long int j = 0, k = 0;

	/* Cycle through all the buckets */
	item_b = _main->nbhd->start;
	for ( k=0; k<_main->nbhd->counter; k++ ) {
		b = item_b->val;

		/* Cycle through all the nodes */
		item_n = b->nodes->start;
		for ( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			/* It's time for pinging */
			if ( _main->p2p->time_now.tv_sec > n->time_ping ) {

				/* Ping the first 8 nodes. Sort out the rest. */
				if ( j < 8 ) {
					send_ping(&n->c_addr, SEND_UNICAST);
					node_pinged(n->id);
				} else {
					node_pinged(n->id);
				}
			}

			item_n = list_next(item_n);
		}

		item_b = list_next(item_b);
	}
}

void nbhd_find_myself(void ) {
	nbhd_find(_main->conf->host_id);
}

void nbhd_find_random(void ) {
	UCHAR node_id[SHA_DIGEST_LENGTH];

	rand_urandom(node_id, SHA_DIGEST_LENGTH);
	nbhd_find(node_id);
}

void nbhd_find(UCHAR *find_id ) {
	ITEM *item_b = NULL;
	struct obj_bckt *b = NULL;
	ITEM *item_n = NULL;
	struct obj_nodeItem *n = NULL;
	long int j = 0;

	if ( (item_b = bckt_find_any_match(_main->nbhd, find_id)) != NULL ) {
		b = item_b->val;

		item_n = b->nodes->start;
		for ( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			/* Maintainance search */
			if ( _main->p2p->time_now.tv_sec > n->time_find ) {

				send_find(&n->c_addr, find_id, _main->conf->null_id);
				n->time_find = time_add_5_min_approx();
			}

			item_n = list_next(item_n);
		}
	}
}

void nbhd_lookup(struct obj_lkp *l ) {
	ITEM *item_b = NULL;
	struct obj_bckt *b = NULL;
	ITEM *item_n = NULL;
	struct obj_nodeItem *n = NULL;
	long int j = 0;

	if ( (item_b = bckt_find_any_match(_main->nbhd, l->find_id)) != NULL ) {
		b = item_b->val;

		item_n = b->nodes->start;
		for ( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			/* Remember node */
			lkp_remember(l, n->id);

			/* Direct lookup */
			send_find(&n->c_addr, l->find_id, l->lkp_id);

			item_n = list_next(item_n);
		}
	}
}

void nbhd_print(void ) {
	ITEM *item_b = NULL;
	struct obj_bckt *b = NULL;
	ITEM *item_n = NULL;
	struct obj_nodeItem *n = NULL;
	long int j = 0, k = 0;
	char hex[HEX_LEN+1];
	char buf[MAIN_BUF+1];

	log_info("Bucket split:");

	/* Cycle through all the buckets */
	item_b = _main->nbhd->start;
	for ( k=0; k<_main->nbhd->counter; k++ ) {
		b = item_b->val;

		hex_encode(hex, b->id);
		snprintf(buf, MAIN_BUF+1, " Bucket: %s", hex);
		log_info(buf);

		/* Cycle through all the nodes */
		item_n = b->nodes->start;
		for ( j=0; j<b->nodes->counter; j++ ) {
			n = item_n->val;

			hex_encode(hex, n->id);
			snprintf(buf, MAIN_BUF+1, "  Node: %s", hex);
			log_info(buf);

			item_n = list_next(item_n);
		}

		item_b = list_next(item_b);
	}
}
