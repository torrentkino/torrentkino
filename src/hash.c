/*
Copyright 2009 Aiko Barz

This file is part of masala/tumbleweed.

masala/tumbleweed is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/tumbleweed is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/tumbleweed.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "malloc.h"
#include "main.h"
#include "hash.h"

HASH *hash_init( unsigned int capacity ) {
	HASH *map = myalloc( sizeof(HASH), "hash_init" );
	
	map->count = capacity;
	map->buckets = myalloc( map->count * sizeof(BUCKET), "hash_init" );

	return map;
}

void hash_free( HASH *map ) {
	unsigned int i;
	BUCKET *bucket = NULL;

	if( map == NULL ) {
		return;
	}

	bucket = map->buckets;
	for( i=0; i<map->count; i++ ) {
		myfree( bucket->pairs, "hash_free" );
		bucket++;
	}

	myfree( map->buckets, "hash_free" );
	myfree( map, "hash_free" );
	map = NULL;
}

int hash_exists( const HASH *map, UCHAR *key, long int keysize ) {
	if( map == NULL ) {
		return 0;
	}

	if( hash_get( map,key,keysize) != NULL ) {
		return 1;
	}
	
	return 0;
}

void *hash_get( const HASH *map, UCHAR *key, long int keysize ) {
	unsigned int index = 0;
	BUCKET *bucket = NULL;
	PAIR *pair = NULL;

	if( map == NULL || key == NULL ) {
		return NULL;
	}

	index = hash_this( key,keysize) % map->count;
	bucket = &(map->buckets[index] );
	pair = hash_getpair( bucket,key,keysize );

	if( pair == NULL ) {
		return NULL;
	}

	return pair->value;
}

int hash_put( HASH *map, UCHAR *key, long int keysize, void *value ) {
	unsigned int index = 0;
	BUCKET *bucket = NULL;
	PAIR *pair = NULL;

	if( map == NULL || key == NULL || value == NULL ) {
		return 0;
	}
	
	index = hash_this( key,keysize) % map->count;
	bucket = &(map->buckets[index] );
	
	/* Key already exists */
	if( ( pair = hash_getpair( bucket, key, keysize)) != NULL ) {
		pair->value = value;
		return 1;
	}

	/* Create new obj_pair */
	if( bucket->count == 0 ) {
		bucket->pairs = myalloc( sizeof(PAIR), "hash_put" );
		bucket->count = 1;
	} else  {
		bucket->pairs = myrealloc( bucket->pairs,( bucket->count + 1) * sizeof(PAIR), "hash_put" );
		bucket->count++;
	}
	
	/* Store key pairs */
	pair = &(bucket->pairs[bucket->count - 1] );
	pair->key = key;
	pair->keysize = keysize;
	pair->value = value;
	
	return 1;
}

void hash_del( HASH *map, UCHAR *key, long int keysize ) {
	unsigned int index = 0;
	BUCKET *bucket = NULL;
	PAIR *thispair = NULL;
	PAIR *oldpair = NULL;
	PAIR *newpair = NULL;
	PAIR *p_old = NULL;
	PAIR *p_new = NULL;
	unsigned int i = 0;

	if( map == NULL || key == NULL ) {
		return;
	}
	
	/* Compute bucket */
	index = hash_this( key,keysize) % map->count;
	bucket = &(map->buckets[index] );

	/* Not found */
	if( ( thispair = hash_getpair( bucket,key,keysize)) == NULL ) {
		return;
	}
	
	if( bucket->count == 1 ) {
		myfree( bucket->pairs, "hash_rem" );
		bucket->pairs = NULL;
		bucket->count = 0;
	} else if( bucket->count > 1 ) {
		/* Get new memory and remember the old one */
		oldpair = bucket->pairs;
		newpair = myalloc( (bucket->count - 1) * sizeof(PAIR), "hash_rem" );

		/* Copy pairs except the one to delete */
		p_old = oldpair;
		p_new = newpair;
		for( i=0; i<bucket->count; i++ ) {
			if( p_old != thispair ) {
				memcpy( p_new++, p_old, sizeof(PAIR) );
			}

			p_old++;
		}

		myfree( oldpair, "hash_rem" );
		bucket->pairs = newpair;
		bucket->count--;
	}
}

PAIR *hash_getpair( BUCKET *bucket, UCHAR *key, long int keysize ) {
	unsigned int i;
	PAIR *pair = NULL;

	if( bucket->count == 0 ) {
		return NULL;
	}

	pair = bucket->pairs;
	for( i=0; i<bucket->count; i++ ) {
		if( pair->keysize == keysize ) {
			if( pair->key != NULL && pair->value != NULL ) {
				if( memcmp( pair->key, key, keysize) == 0 ) {
					return pair;
				}
			}
		}
		pair++;
	}
	
	return NULL;
}

unsigned long hash_this( UCHAR *p, long int keysize ) {
	unsigned long result = 5381;
	long int i = 0;

	for( i=0; i<keysize; i++ ) {
		result = ((result << 5) + result) + *(p++ );
	}

	return result;
}
