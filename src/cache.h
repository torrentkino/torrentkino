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

#include <string.h>
#include <netdb.h>
#include <signal.h>

#include "torrentkino.h"
#include "p2p.h"
#include "time.h"
#include "hash.h"

#define CACHE_SIZE 100

struct obj_cache {
	LIST *list;
	HASH *hash;
};
typedef struct obj_cache CACHE;

typedef struct {
	UCHAR target[SHA1_SIZE];
	UCHAR nodes[IP_SIZE_META_PAIR8];
	int size;
	time_t lifetime;
	time_t refresh;
} TARGET_C;

CACHE *cache_init( void );
void cache_free( void );

void cache_put( UCHAR *target, UCHAR *nodes, int size );
void cache_del( ITEM *i );

void cache_expire( time_t now );
void cache_renew( time_t now );

void cache_update( TARGET_C *tc, UCHAR *target, UCHAR *nodes, int size );
TARGET_C *cache_find( UCHAR *target );

int cache_compact_list( UCHAR *nodes, UCHAR *target );

#endif
