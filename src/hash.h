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

#ifndef HASH_H
#define HASH_H

#include "malloc.h"

typedef struct {
	UCHAR *key;
	LONG size;
	void *value;
} PAIR;

typedef struct {
	PAIR *pairs;
	ULONG size;
} BUCKET;

typedef struct {
	BUCKET *buckets;
	ULONG size;
} HASH;

HASH *hash_init( ULONG capacity );
void hash_free( HASH *map );

ULONG hash_this( UCHAR *key, LONG size );
PAIR *hash_getpair( BUCKET *bucket, UCHAR *key, LONG size );

void *hash_get( const HASH *map, UCHAR *key, LONG size );
int hash_put( HASH *map, UCHAR *key, LONG size, void *value );
void hash_del( HASH *map, UCHAR *key, LONG size );
int hash_exists( const HASH *map, UCHAR *key, LONG size );

#endif
