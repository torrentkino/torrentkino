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

#ifndef HASH_H
#define HASH_H

struct obj_pair {
	UCHAR *key;
	long int keysize;
	void *value;
};
typedef struct obj_pair PAIR;

struct obj_bucket {
	unsigned int count;
	PAIR *pairs;
};
typedef struct obj_bucket BUCKET;

struct obj_hash {
	unsigned int count;
	BUCKET *buckets;
};
typedef struct obj_hash HASH;

HASH *hash_init( unsigned int capacity );
void hash_free( HASH *map );

unsigned long hash_this( UCHAR *str, long int keysize );
PAIR *hash_getpair( BUCKET *bucket, UCHAR *key, long int keysize );

void *hash_get( const HASH *map, UCHAR *key, long int keysize );
int hash_put( HASH *map, UCHAR *key, long int keysize, void *value );
void hash_del( HASH *map, UCHAR *key, long int keysize );
int hash_exists( const HASH *map, UCHAR *key, long int keysize );

#endif
