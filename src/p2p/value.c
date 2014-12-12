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

#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>

#include "value.h"

VALUE *val_init(void)
{
	VALUE *value = (VALUE *) myalloc(sizeof(VALUE));
	value->list = list_init();
	value->hash = hash_init(VALUE_SIZE_MAX + 1);
	return value;
}

void val_free(void)
{
	val_clean();
	list_clear(_main->value->list);
	list_free(_main->value->list);
	hash_free(_main->value->hash);
	myfree(_main->value);
}

void val_clean(void)
{
	while (_main->value->list->item != NULL) {
		val_del(_main->value->list->item);
	}
}

void val_put(UCHAR * target_id, UCHAR * node_id, int port, IP * from)
{
	TARGET_V *target = val_find(target_id);

	/* Create a new target if necessary  */
	if (target == NULL) {
		target = val_ins_sort(target_id);
	}

	/* Still NULL:
	 * That means that the target_id does not match my node_id very well. */
	if (target == NULL) {
		return;
	}

	/* Insert node into the target list */
	tgt_v_update(target, node_id, from, port);
}

TARGET_V *val_ins_sort(UCHAR * target_id)
{
	TARGET_V *old = NULL, *new = NULL;
	ITEM *i = NULL;
	int done = FALSE;

	/* Create a sorted list. The first targets are fitting nicely to my own
	 * node_id. */
	i = list_start(_main->value->list);
	while (i != NULL) {
		old = list_value(i);

		if (str_sha1_compare
		    (target_id, old->target, _main->conf->node_id) < 0) {
			new = tgt_v_init(target_id);
			list_ins(_main->value->list, i, new);
			hash_put(_main->value->hash, new->target, SHA1_SIZE,
				 new);

			done = TRUE;
			break;
		}

		i = list_next(i);
	}

	/* Last resort:
	 * Only add a new target if it has not be done before and if it will
	 * persist. */
	if (done == FALSE && list_size(_main->value->list) < VALUE_SIZE_MAX) {
		new = tgt_v_init(target_id);
		list_put(_main->value->list, new);
		hash_put(_main->value->hash, new->target, SHA1_SIZE, new);

		done = TRUE;
	}

	/* Limit reached after list_ins(). Delete the last target. */
	if (list_size(_main->value->list) > VALUE_SIZE_MAX) {
		val_del(list_stop(_main->value->list));
	}

	/* Print the database on changes */
	if (done == TRUE) {
		val_print();
	}

	return new;
}

void val_del(ITEM * i)
{
	TARGET_V *target = list_value(i);
	hash_del(_main->value->hash, target->target, SHA1_SIZE);
	list_del(_main->value->list, i);
	tgt_v_free(target);
}

void val_expire(time_t now)
{
	ITEM *i = NULL, *n = NULL;
	TARGET_V *target = NULL;
	int index = 0;

	i = list_start(_main->value->list);
	while (i != NULL) {
		n = list_next(i);
		target = list_value(i);

		/* Look at the nodes within the target */
		tgt_v_expire(target, now);

		/* The target contains no more nodes */
		/* Or the target does not match well enough */
		if (list_size(target->list) == 0) {
			val_del(i);
		}

		i = n;
		index++;
	}
}

void val_print(void)
{
	ITEM *i = NULL;
	TARGET_V *target = NULL;
	char hex[HEX_LEN];

	if (log_verbosely(_log)) {
		return;
	}

	info(_log, NULL, "Values:");
	i = list_start(_main->value->list);
	while (i != NULL) {
		target = list_value(i);

		hex_hash_encode(hex, target->target);
		info(_log, NULL, " Target: %s", hex);

		tgt_v_print(target);

		i = list_next(i);
	}
}

int val_compact_list(UCHAR * nodes_compact_list, UCHAR * target_id)
{
	UCHAR *p = nodes_compact_list;
	NODE_V *node_v = NULL;
	TARGET_V *target = NULL;
	ITEM *item = NULL;
	int j = 0;
	int size = 0;

	/* Look into the local database */
	if ((target = val_find(target_id)) == NULL) {
		return 0;
	}

	/* Walkthrough local database */
	item = list_start(target->list);
	while (item != NULL && j < 8) {
		node_v = list_value(item);

		/* IP + Port */
		memcpy(p, node_v->pair, IP_SIZE_META_PAIR);

		p += IP_SIZE_META_PAIR;
		size += IP_SIZE_META_PAIR;

		item = list_next(item);
		j++;
	}

	/* Rotate the list. This only helps Bittorrent to get fresh IP addresses
	 * after each request. */
//      list_rotate( target->list );

	return size;
}

TARGET_V *val_find(UCHAR * target_id)
{
	return hash_get(_main->value->hash, target_id, SHA1_SIZE);
}

TARGET_V *tgt_v_init(UCHAR * target_id)
{
	TARGET_V *target = (TARGET_V *) myalloc(sizeof(TARGET_V));

	memcpy(target->target, target_id, SHA1_SIZE);
	target->list = list_init();
	target->hash = hash_init(TGT_V_SIZE_MAX + 1);

	return target;
}

void tgt_v_free(TARGET_V * target)
{
	list_clear(target->list);
	list_free(target->list);
	hash_free(target->hash);
	myfree(target);
}

void tgt_v_put(TARGET_V * target, UCHAR * node_id, IP * from, int port)
{
	NODE_V *node = NULL;
	ITEM *i = NULL;
	ITEM *s = NULL;

	node = node_v_init(node_id, from, port);
	s = list_start(target->list);
	i = list_ins(target->list, s, node);
	hash_put(target->hash, node->id, SHA1_SIZE, i);

	/* Limit reached. Delete last node */
	if (list_size(target->list) > TGT_V_SIZE_MAX) {
		s = list_stop(target->list);
		tgt_v_del(target, s);
	}
}

void tgt_v_del(TARGET_V * target, ITEM * i)
{
	NODE_V *node_v = list_value(i);
	hash_del(target->hash, node_v->id, SHA1_SIZE);
	list_del(target->list, i);
	node_v_free(node_v);
}

void tgt_v_expire(TARGET_V * target, time_t now)
{
	ITEM *i = NULL;
	ITEM *n = NULL;
	NODE_V *node = NULL;

	i = list_start(target->list);
	while (i != NULL) {
		n = list_next(i);
		node = list_value(i);

		/* Delete info_hash after 30 minutes without announcement. */
		if (now > node->eol) {
			tgt_v_del(target, i);
		}

		i = n;
	}
}

void tgt_v_print(TARGET_V * target)
{
	ITEM *i = NULL;
	NODE_V *node = NULL;
	IP sin;
	char ip_buf[IP_ADDRLEN + 1];
	USHORT port = 0;

	i = list_start(target->list);
	while (i != NULL) {
		node = list_value(i);

		ip_tuple_to_sin(&sin, node->pair);
		ip_sin_to_string(&sin, ip_buf);
		port = ip_sin_to_port(&sin);

#ifdef IPV6
		info(_log, NULL, "  [%s]:%i", ip_buf, port);
#else
		info(_log, NULL, "  %s:%i", ip_buf, port);
#endif

		i = list_next(i);
	}
}

ITEM *tgt_v_find(TARGET_V * target, UCHAR * node_id)
{
	return hash_get(target->hash, node_id, SHA1_SIZE);
}

void tgt_v_update(TARGET_V * target, UCHAR * node_id, IP * from, int port)
{
	NODE_V *node = NULL;
	ITEM *i = NULL;

	if ((i = tgt_v_find(target, node_id)) == NULL) {

		/* New node */
		tgt_v_put(target, node_id, from, port);

	} else {

		/* Update node */
		node = list_value(i);
		node_v_update(node, node_id, from, port);

		/* Put the updated node on top of the list */
		list_del(target->list, i);
		i = list_ins(target->list, list_start(target->list), node);
		hash_put(target->hash, node->id, SHA1_SIZE, i);
	}

}

NODE_V *node_v_init(UCHAR * node_id, IP * from, int port)
{
	NODE_V *node_v = (NODE_V *) myalloc(sizeof(NODE_V));
	node_v_update(node_v, node_id, from, port);
	return node_v;
}

void node_v_free(NODE_V * node_v)
{
	myfree(node_v);
}

void node_v_update(NODE_V * node_v, UCHAR * node_id, IP * from, int port)
{
	UCHAR *p = node_v->pair;

	/* Convert IP + Port */
	ip_merge_port_to_sin(from, port);
	ip_sin_to_tuple(from, p);

	memcpy(node_v->id, node_id, SHA1_SIZE);
	time_add_30_min(&node_v->eol);
}
