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

#ifndef BUCKET_H
#define BUCKET_H

#include "../shr/ip.h"
#include "ben.h"
#include "hex.h"
#include "node_udp.h"
#include "torrentkino.h"

#define BCKT_SIZE_MAX 20

struct obj_neighboorhood_bucket {
	UCHAR id[SHA1_SIZE];
	LIST *nodes;
};
typedef struct obj_neighboorhood_bucket BUCK;

LIST *bckt_init(void);
void bckt_free(LIST * thislist);
int bckt_put(LIST * l, UDP_NODE * n);
void bckt_del(LIST * l, UDP_NODE * n);

ITEM *bckt_find_best_match(LIST * thislist, const UCHAR * id);
ITEM *bckt_find_any_match(LIST * thislist, const UCHAR * id);
ITEM *bckt_find_node(LIST * thislist, const UCHAR * id);

int bckt_split(LIST * thislist, const UCHAR * target);
void bckt_split_loop(LIST * l, UCHAR * target, int verbose);
void bckt_split_print(LIST * l);

int bckt_is_empty(LIST * l);

int bckt_compute_id(LIST * thislist, ITEM * item_b, UCHAR * id_return);
int bckt_significant_bit(const UCHAR * id);

int bckt_compact_list(LIST * l, UCHAR * nodes_compact_list, UCHAR * target);

#endif
