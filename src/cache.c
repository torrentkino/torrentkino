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

#include "cache.h"

CACHE *cache_init( void ) {
	CACHE *cache = ( CACHE * ) myalloc( sizeof( CACHE ) );
	cache->list = list_init();
	cache->hash = hash_init( CACHE_SIZE_MAX + 1 );
	return cache;
}

void cache_free( void ) {
	cache_clean();
	list_clear( _main->cache->list );
	list_free( _main->cache->list );
	hash_free( _main->cache->hash );
	myfree( _main->cache );
}

void cache_clean( void ) {
	while( _main->cache->list->item != NULL ) {
		cache_del( _main->cache->list->item );
	}
}

void cache_put( UCHAR *target_id, UCHAR *nodes_compact_list, int nodes_compact_size ) {
	TARGET_C *target = NULL;
	UCHAR *pair = NULL;
	int j = 0;

	target = cache_find( target_id );

	/* Create a new target if necessary  */
	if ( target == NULL ) {
		target = tgt_c_init( target_id );
		list_ins( _main->cache->list, list_start( _main->cache->list ), target );
		hash_put( _main->cache->hash, target->target, SHA1_SIZE, target );
	}

	/* Update target list */
	pair = nodes_compact_list;
	for( j=0; j<nodes_compact_size; j+=IP_SIZE_META_PAIR ) {
		tgt_c_update( target, pair );
		pair += IP_SIZE_META_PAIR;
	}

	/* Limit reached after list_ins(). Delete the last target. */
	if( list_size( _main->cache->list ) > CACHE_SIZE_MAX ) {
		cache_del( list_stop( _main->cache->list ) );
	}
}

void cache_del( ITEM *i ) {
	TARGET_C *target = list_value( i );
	hash_del( _main->cache->hash, target->target, SHA1_SIZE );
	list_del( _main->cache->list, i );
	tgt_c_free( target );
}

void cache_expire( time_t now ) {
	ITEM *i = NULL, *n = NULL;
	TARGET_C *target = NULL;
	int index = 0;

	i = list_start( _main->cache->list );
	while( i != NULL ) {
		n = list_next( i );
		target = list_value( i );

		/* Look at the nodes within the target */
		tgt_c_expire( target, now );

		/* 30 minutes without activity. Kill it. */
		if( now > target->lifetime ) {
			cache_del( i );
		}

		i = n;
		index++;
	}
}

void cache_renew( time_t now ) {
	ITEM *i = NULL;
	TARGET_C *t = NULL;

	i = list_start( _main->cache->list );
	while( i != NULL ) {

		/* Lookup target on my own every 5 minutes */
		t = list_value( i );
		if( now > t->refresh ) {
			p2p_localhost_lookup_remote( t->target, NULL );
			time_add_5_min_approx( &t->refresh );
			cache_print();
		}

		i = list_next( i );
	}
}

void cache_print( void ) {
	ITEM *i = NULL;
	TARGET_C *t = NULL;
	char hex[HEX_LEN];

	if( conf_verbosity() != CONF_VERBOSE ) {
		return;
	}

	info( NULL, 0, "Cache:" );
	i = list_start( _main->cache->list );
	while( i != NULL ) {
		t = list_value( i );

		hex_hash_encode( hex, t->target );
		info( NULL, 0, " Target: %s", hex );

		tgt_c_print( t );

		i = list_next( i );
	}
}

int cache_compact_list( UCHAR *nodes_compact_list, UCHAR *target_id ) {
	UCHAR *p = nodes_compact_list;
	NODE_C *node_c = NULL;
	TARGET_C *target = NULL;
	ITEM *item = NULL;
	ULONG j = 0;
	int size = 0;

	/* Look into the local database */
	if( ( target = cache_find( target_id ) ) == NULL ) {
		return 0;
	}

	/* Walkthrough local database */
	item = list_start( target->list );
	while( item != NULL && j < 8 ) {
		node_c = list_value( item );

		/* IP + Port */
		memcpy( p, node_c->pair, IP_SIZE_META_PAIR );

		p += IP_SIZE_META_PAIR;
		size += IP_SIZE_META_PAIR;

		item = list_next( item );
		j++;
	}

	/* Extend valid lifetime: This request has been made by a client. So keep
	 * this cache item alive. */
	time_add_30_min( &target->lifetime );

	return size;
}

TARGET_C *cache_find( UCHAR *target_id ) {
	return hash_get( _main->cache->hash, target_id, SHA1_SIZE );
}

TARGET_C *tgt_c_init( UCHAR *target_id ) {
	TARGET_C *target = ( TARGET_C * ) myalloc( sizeof( TARGET_C ) );

	memcpy( target->target, target_id, SHA1_SIZE );
	target->list = list_init();
	target->hash = hash_init( TGT_C_SIZE_MAX + 1 );

	return target;
}

void tgt_c_free( TARGET_C *target ) {
	list_clear( target->list );
	list_free( target->list );
	hash_free( target->hash );
	myfree( target );
}

void tgt_c_put( TARGET_C *target, UCHAR *pair ) {
	NODE_C *node = NULL;
	ITEM *i = NULL;
	ITEM *s = NULL;

	node = node_c_init( pair );
	s = list_start( target->list );
	i = list_ins( target->list, s, node );
	hash_put( target->hash, node->pair, IP_SIZE_META_PAIR, i );

	/* Lifetime + Refresh timer */
	time_add_30_min( &target->lifetime );
	time_add_5_min_approx( &target->refresh );

	/* Limit reached. Delete last node */
	if( list_size( target->list ) > TGT_C_SIZE_MAX ) {
		s = list_stop( target->list );
		tgt_c_del( target, s );
	}
}

void tgt_c_del( TARGET_C *target, ITEM *i ) {
	NODE_C *node_c = list_value( i );
	hash_del( target->hash, node_c->pair, IP_SIZE_META_PAIR );
	list_del( target->list, i );
	node_c_free( node_c );
}

void tgt_c_expire( TARGET_C *target, time_t now ) {
	ITEM *i = NULL;
	ITEM *n = NULL;
	NODE_C *node = NULL;

	i = list_start( target->list );
	while( i != NULL ) {
		n = list_next( i );
		node = list_value( i );

		/* Delete info_hash after 30 minutes without announcement. */
		if( now > node->eol ) {
			tgt_c_del( target, i );
		}

		i = n;
	}
}

void tgt_c_print( TARGET_C *target ) {
	ITEM *i = NULL;
	NODE_C *node = NULL;
	IP sin;

	i = list_start( target->list );
	while( i != NULL ) {
		node = list_value( i );

		ip_bytes_to_sin( &sin, node->pair );
		info( &sin, 0, "  IP:");

		i = list_next( i );
	}
}

ITEM *tgt_c_find( TARGET_C *target, UCHAR *pair ) {
	return hash_get( target->hash, pair, IP_SIZE_META_PAIR );
}

void tgt_c_update( TARGET_C *target, UCHAR *pair ) {
	NODE_C *node = NULL;
	ITEM *i = NULL;

	if ( ( i = tgt_c_find( target, pair ) ) == NULL ) {

		/* New node */
		tgt_c_put( target, pair );

	} else {

		/* Update node */
		node = list_value( i );
		node_c_update( node, pair );

		/* Put the updated node on top of the list */
		list_del( target->list, i );
		i = list_ins( target->list, list_start( target->list ), node );
		hash_put( target->hash, node->pair, IP_SIZE_META_PAIR, i );
	}
}

NODE_C *node_c_init( UCHAR *pair ) {
	NODE_C *node_c = ( NODE_C * ) myalloc( sizeof( NODE_C ) );
	node_c_update( node_c, pair );
	return node_c;
}

void node_c_free( NODE_C *node_c ) {
	myfree( node_c );
}

void node_c_update( NODE_C *node_c, UCHAR *pair ) {
	memcpy( node_c->pair, pair, IP_SIZE_META_PAIR );
	time_add_30_min( &node_c->eol );
}
