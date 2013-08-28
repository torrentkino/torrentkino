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

#ifndef CACHE_H
#define CACHE_H

#define CACHE_SIZE 100

#include "random.h"
#include "log.h"
#include "time.h"
#include "hash.h"
#include "ben.h"

struct obj_cache {
	LIST *list;
	HASH *hash;
};

struct obj_cache_entry {
	UCHAR target[SHA_DIGEST_LENGTH];
	UCHAR nodes_compact_list[144]; /* 8*(16+2) */
	int nodes_compact_size;
	
	time_t lifetime;
	time_t renew;
};
typedef struct obj_cache_entry CACHE;

struct obj_cache *cache_init( void );
void cache_free( void );

void cache_put( UCHAR *target, UCHAR *nodes_compact_list, int nodes_compact_size );
void cache_del( ITEM *item );

void cache_expire( time_t now );
void cache_renew( time_t now );

void cache_update( CACHE *cache, UCHAR *target, UCHAR *nodes_compact_list, int nodes_compact_size );
CACHE *cache_find( UCHAR *target );
int cache_lookup( UCHAR *target, UCHAR *nodes_compact_list );

#endif
