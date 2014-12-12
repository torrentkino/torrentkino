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

#ifndef INFO_HASH_H
#define INFO_HASH_H

#include "../shr/list.h"
#include "../shr/hash.h"
#include "hex.h"
#include "../shr/log.h"
#include "time.h"
#include "ben.h"
#include "p2p.h"
#include "sha1.h"
#include "torrentkino.h"

#define VALUE_SIZE_MAX 50
#define TGT_V_SIZE_MAX 10

struct obj_val {
	LIST *list;
	HASH *hash;
};
typedef struct obj_val VALUE;

typedef struct {
	UCHAR target[SHA1_SIZE];
	LIST *list;
	HASH *hash;
} TARGET_V;

typedef struct {
	UCHAR id[SHA1_SIZE];
	UCHAR pair[IP_SIZE_META_PAIR];
	time_t eol;
} NODE_V;

VALUE *val_init(void);
void val_free(void);
void val_clean(void);
void val_put(UCHAR * target_id, UCHAR * node_id, int port, IP * from);
void val_del(ITEM * i);
TARGET_V *val_ins_sort(UCHAR * target_id);
void val_expire(time_t now);
void val_print(void);
int val_compact_list(UCHAR * nodes_compact_list, UCHAR * target_id);
TARGET_V *val_find(UCHAR * target_id);

TARGET_V *tgt_v_init(UCHAR * target_id);
void tgt_v_free(TARGET_V * target);
void tgt_v_put(TARGET_V * target, UCHAR * node_id, IP * from, int port);
void tgt_v_del(TARGET_V * target, ITEM * i);
void tgt_v_expire(TARGET_V * target, time_t now);
void tgt_v_print(TARGET_V * target);
ITEM *tgt_v_find(TARGET_V * target, UCHAR * pair);
void tgt_v_update(TARGET_V * target, UCHAR * node_id, IP * from, int port);

NODE_V *node_v_init(UCHAR * node_id, IP * from, int port);
void node_v_free(NODE_V * node_v);
void node_v_update(NODE_V * node_v, UCHAR * node_id, IP * from, int port);

#endif
