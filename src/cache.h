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

#ifndef CACHE_H
#define CACHE_H

#include "list.h"
#include "hash.h"
#include "hex.h"
#include "log.h"
#include "time.h"
#include "ben.h"
#include "p2p.h"
#include "sha1.h"
#include "torrentkino.h"

#define CACHE_SIZE_MAX 50
#define TGT_C_SIZE_MAX 10

struct obj_cache {
	LIST *list;
	HASH *hash;
};
typedef struct obj_cache CACHE;

typedef struct {
	UCHAR target[SHA1_SIZE];
	LIST *list;
	HASH *hash;
	time_t lifetime;
	time_t refresh;
} TARGET_C;

typedef struct {
	UCHAR pair[IP_SIZE_META_PAIR];
	time_t eol;
} NODE_C;

CACHE *cache_init( void );
void cache_free( void );
void cache_clean( void );
void cache_put( UCHAR *target_id, UCHAR *nodes_compact_list, int nodes_compact_size );
void cache_del( ITEM *i );
void cache_expire( time_t now );
void cache_renew( time_t now );
void cache_print( void );
int cache_compact_list( UCHAR *nodes_compact_list, UCHAR *target_id );
TARGET_C *cache_find( UCHAR *target_id );

TARGET_C *tgt_c_init( UCHAR *target_id );
void tgt_c_free( TARGET_C *target );
void tgt_c_put( TARGET_C *target, UCHAR *pair );
void tgt_c_del( TARGET_C *target, ITEM *i );
void tgt_c_expire( TARGET_C *target, time_t now );
void tgt_c_print( TARGET_C *target );
ITEM *tgt_c_find( TARGET_C *target, UCHAR *pair );
void tgt_c_update( TARGET_C *target, UCHAR *pair );

NODE_C *node_c_init( UCHAR *pair );
void node_c_free( NODE_C *node_c );
void node_c_update( NODE_C *node_c, UCHAR *pair );

#endif
