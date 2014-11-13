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

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "lookup.h"
#include "p2p.h"

struct obj_transaction {
	LIST *list;
	HASH *hash;
};

struct obj_tid {
	UCHAR id[TID_SIZE];
	time_t time;
	int type;
	LOOKUP *lookup;
};
typedef struct obj_tid TID;

struct obj_transaction *tdb_init( void );
void tdb_free( void );

ITEM *tdb_put( int type );
void tdb_del( ITEM *i );

void tdb_clean( void );
void tdb_expire( time_t now );

void tdb_link_ldb( ITEM *i, LOOKUP *l );

void tdb_create_random_id( UCHAR *id );
ITEM *tdb_item( UCHAR *id );
int tdb_type( ITEM *i );
LOOKUP *tdb_ldb( ITEM *i );
UCHAR *tdb_tid( ITEM *i );

#endif
