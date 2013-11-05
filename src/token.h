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

#ifndef TOKEN_H
#define TOKEN_H

#include "random.h"
#include "log.h"
#include "time.h"
#include "hash.h"
#include "ben.h"

#define TOKEN_SIZE 8
#define TOKEN_SIZE_MAX 20

struct obj_token {
	LIST *list;
	HASH *hash;
	/* UCHAR null[TOKEN_SIZE]; */
};

struct obj_tkn {
	UCHAR id[TOKEN_SIZE];
	time_t time;
};

struct obj_token *tkn_init( void );
void tkn_free( void );

void tkn_put( void );
void tkn_del( ITEM *item_tkn );

void tkn_create( UCHAR *id );
void tkn_expire( time_t now );
int tkn_validate( UCHAR *id );
UCHAR *tkn_read( void );

#endif
