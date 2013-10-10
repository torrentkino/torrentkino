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

#ifndef MIME_H
#define MIME_H

#include "tumbleweed.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "thrd.h"

#define MIME_KEYLEN 8
#define MIME_VALLEN 50

struct obj_mdb {
	LIST *list;
	HASH *hash;
	pthread_mutex_t *mutex;
};

struct obj_mime {
	char key[MIME_KEYLEN+1];
	char val[MIME_VALLEN+1];
};

struct obj_mdb *mime_init( void );
void mime_free( void );

struct obj_mime *mime_add( const char *key, const char *value );
void mime_load( void );
void mime_hash( void );
const char *mime_find( char *filename );
#ifdef MAGIC
void mime_magic( char *filename, char *key );
#endif
char *mime_extension( char *filename );

#endif /* MIME_H */
