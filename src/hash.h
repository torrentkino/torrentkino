/*
Copyright 2009 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

struct obj_pair {
	UCHAR *key;
	long int keysize;
	void *value;
};

struct obj_bucket {
	unsigned int count;
	struct obj_pair *pairs;
};

struct obj_hash {
	unsigned int count;
	struct obj_bucket *buckets;
};

struct obj_hash *hash_init( unsigned int capacity );
void hash_free( struct obj_hash *map );

unsigned long hash_this( UCHAR *str, long int keysize );
struct obj_pair *hash_getpair( struct obj_bucket *bucket, UCHAR *key, long int keysize );

void *hash_get( const struct obj_hash *map, UCHAR *key, long int keysize );
int hash_put( struct obj_hash *map, UCHAR *key, long int keysize, void *value );
void hash_del( struct obj_hash *map, UCHAR *key, long int keysize );
int hash_exists( const struct obj_hash *map, UCHAR *key, long int keysize );
