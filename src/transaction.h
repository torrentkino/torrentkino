/*
Copyright 2011 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#define TID_SIZE 4
#define TID_SIZE_MAX 20

struct obj_transaction {
	LIST *list;
	HASH *hash;
};

struct obj_tid {
	UCHAR id[TID_SIZE];
	time_t time;
	int type;
	int mode;
	LOOKUP *lookup;
};
typedef struct obj_tid TID;

struct obj_transaction *tdb_init( void );
void tdb_free( void );

ITEM *tdb_put( int type, UCHAR *target, IP *from );
void tdb_del( ITEM *i );

void tdb_create_random_id( UCHAR *id );
void tdb_expire( void );
ITEM *tdb_item( UCHAR *id );

int tdb_type( ITEM *i );
LOOKUP *tdb_ldb( ITEM *i );
UCHAR *tdb_tid( ITEM *i );
