/*
Copyright 2011 Aiko Barz

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

struct obj_cache {
	LIST *list;
	HASH *hash;
};

struct obj_key {
	UCHAR id[SHA_DIGEST_LENGTH];
	time_t time;
	int type;
};

struct obj_cache *cache_init( void );
void cache_free( void );

void cache_put( UCHAR *id, int type );
void cache_del( UCHAR *id );

void cache_expire( void );
int cache_validate( UCHAR *id );
