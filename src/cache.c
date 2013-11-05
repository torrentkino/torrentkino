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

#include "cache.h"

CACHE *cache_init( void ) {
	CACHE *cache = ( CACHE * ) myalloc( sizeof( CACHE ) );
	cache->list = list_init();
	cache->hash = hash_init( CACHE_SIZE );
	return cache;
}

void cache_free( void ) {
	list_clear( _main->cache->list );
	list_free( _main->cache->list );
	hash_free( _main->cache->hash );
	myfree( _main->cache );
}

void cache_put( UCHAR *target, UCHAR *nodes, int size ) {
	ITEM *i = NULL;
	TARGET_C *tc = cache_find( target );

	/* Found in cache, update cache, done */
	if( tc != NULL ) {
		cache_update( tc, target, nodes, size );
		return;
	}

	tc = ( TARGET_C * ) myalloc( sizeof( TARGET_C ) );

	cache_update( tc, target, nodes, size );
	time_add_30_min( &tc->lifetime );
	time_add_5_min_approx( &tc->refresh );

	i = list_put( _main->cache->list, tc );
	hash_put( _main->cache->hash, tc->target, SHA1_SIZE, i );
}

void cache_del( ITEM *i ) {
	TARGET_C *tc = list_value( i );
	hash_del( _main->cache->hash, tc->target, SHA1_SIZE );
	list_del( _main->cache->list, i );
	myfree( tc );
}

void cache_expire( time_t now ) {
	ITEM *i = NULL;
	ITEM *n = NULL;
	TARGET_C *tc = NULL;

	i = list_start( _main->cache->list );
	while( i != NULL ) {
		n = list_next( i );

		/* 30 minutes without activity. Kill it. */
		tc = list_value( i );
		if( now > tc->lifetime ) {
			cache_del( i );
		}

		i = n;
	}
}

void cache_renew( time_t now ) {
	ITEM *i = NULL;
	TARGET_C *tc = NULL;

	i = list_start( _main->cache->list );
	while( i != NULL ) {

		/* Lookup target on my own every 5 minutes */
		tc = list_value( i );
		if( now > tc->refresh ) {
			p2p_localhost_lookup_remote( tc->target, NULL );
			time_add_5_min_approx( &tc->refresh );
		}

		i = list_next( i );
	}
}

void cache_update( TARGET_C *tc, UCHAR *target, UCHAR *nodes, int size ) {
	memcpy( tc->target, target, SHA1_SIZE );
	memcpy( tc->nodes, nodes, size );
	tc->size = size;
}

TARGET_C *cache_find( UCHAR *target ) {
	ITEM *i = NULL;

	if( ( i = hash_get( _main->cache->hash, target, SHA1_SIZE ) ) == NULL ) {
		return NULL;
	}

	return list_value( i );
}

int cache_compact_list( UCHAR *nodes, UCHAR *target ) {
	TARGET_C *tc = cache_find( target );

	/* Not found */
	if( tc == NULL ) {
		return 0;
	}

	/* Success */
	memcpy( nodes, tc->nodes, tc->size );

	/* Extend valid lifetime */
	time_add_30_min( &tc->lifetime );

	return tc->size;
}
