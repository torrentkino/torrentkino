/*
Copyright 2009 Aiko Barz

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
#include <limits.h>

#include "hash.h"

HASH *hash_init( ULONG capacity ) {
	HASH *map = myalloc( sizeof( HASH ) );
	
	map->size = capacity;
	map->buckets = myalloc( map->size * sizeof( BUCKET ) );

	return map;
}

void hash_free( HASH *map ) {
	ULONG i = 0;
	BUCKET *bucket = NULL;

	if( map == NULL ) {
		return;
	}

	bucket = map->buckets;
	for( i=0; i<map->size; i++ ) {
		myfree( bucket->pairs );
		bucket++;
	}

	myfree( map->buckets );
	myfree( map );
	map = NULL;
}

int hash_exists( const HASH *map, UCHAR *key, LONG size ) {
	if( map == NULL ) {
		return FALSE;
	}

	if( hash_get( map, key, size ) != NULL ) {
		return TRUE;
	}
	
	return FALSE;
}

void *hash_get( const HASH *map, UCHAR *key, LONG size ) {
	ULONG index = 0;
	BUCKET *bucket = NULL;
	PAIR *pair = NULL;

	if( map == NULL || key == NULL ) {
		return NULL;
	}

	index = hash_this( key, size ) % map->size;
	bucket = &( map->buckets[index] );
	pair = hash_getpair( bucket, key, size );

	if( pair == NULL ) {
		return NULL;
	}

	return pair->value;
}

int hash_put( HASH *map, UCHAR *key, LONG size, void *value ) {
	ULONG index = 0;
	BUCKET *bucket = NULL;
	PAIR *pair = NULL;

	if( map == NULL || key == NULL || value == NULL ) {
		return FALSE;
	}

	index = hash_this( key, size ) % map->size;
	bucket = &( map->buckets[index] );

	/* Key already exists */
	if( ( pair = hash_getpair( bucket, key, size ) ) != NULL ) {
		pair->value = value;
		return TRUE;
	}

	/* Create new obj_pair */
	if( bucket->size == 0 ) {
		bucket->pairs = myalloc( sizeof( PAIR ) );
		bucket->size = 1;
	} else if( bucket->size == ULONG_MAX ) {
		/* Overflow */
		return FALSE;
	} else {
		bucket->pairs = myrealloc( bucket->pairs, ( bucket->size + 1 ) * sizeof( PAIR ) );
		bucket->size++;
	}
	
	/* Store key pairs */
	pair = &( bucket->pairs[bucket->size - 1] );
	pair->key = key;
	pair->size = size;
	pair->value = value;
	
	return TRUE;
}

void hash_del( HASH *map, UCHAR *key, LONG size ) {
	BUCKET *bucket = NULL;
	PAIR *thispair = NULL;
	PAIR *oldpair = NULL;
	PAIR *newpair = NULL;
	PAIR *p_old = NULL;
	PAIR *p_new = NULL;
	ULONG index = 0;
	ULONG i = 0;

	if( map == NULL || key == NULL ) {
		return;
	}
	
	/* Compute bucket */
	index = hash_this( key, size ) % map->size;
	bucket = &( map->buckets[index] );

	/* Not found */
	if( ( thispair = hash_getpair( bucket, key, size ) ) == NULL ) {
		return;
	}
	
	if( bucket->size == 1 ) {
		myfree( bucket->pairs );
		bucket->pairs = NULL;
		bucket->size = 0;
	} else if( bucket->size > 1 ) {
		/* Get new memory and remember the old one */
		oldpair = bucket->pairs;
		newpair = myalloc( ( bucket->size - 1 ) * sizeof( PAIR ) );

		/* Copy pairs except the one to delete */
		p_old = oldpair;
		p_new = newpair;
		for( i=0; i<bucket->size; i++ ) {
			if( p_old != thispair ) {
				memcpy( p_new++, p_old, sizeof( PAIR ) );
			}

			p_old++;
		}

		myfree( oldpair );
		bucket->pairs = newpair;
		bucket->size--;
	}
}

PAIR *hash_getpair( BUCKET *bucket, UCHAR *key, LONG size ) {
	ULONG i = 0;
	PAIR *pair = NULL;

	if( bucket->size == 0 ) {
		return NULL;
	}

	pair = bucket->pairs;
	for( i=0; i<bucket->size; i++ ) {
		if( pair->size == size ) {
			if( pair->key != NULL && pair->value != NULL ) {
				if( memcmp( pair->key, key, size ) == 0 ) {
					return pair;
				}
			}
		}
		pair++;
	}
	
	return NULL;
}

ULONG hash_this( UCHAR *key, LONG size ) {
	ULONG result = 5381;
	LONG i = 0;

	for( i=0; i<size; i++ ) {
		result = ( ( result << 5 ) + result ) + *( key++ );
	}

	return result;
}
