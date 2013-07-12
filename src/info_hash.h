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

struct obj_infohash {
	LIST *list;
	HASH *hash;
};

struct obj_ihash {
	UCHAR target[SHA_DIGEST_LENGTH];
	LIST *list;
	HASH *hash;
};
typedef struct obj_ihash IHASH;

struct obj_inode {
	UCHAR id[SHA_DIGEST_LENGTH];
	IP c_addr;
	time_t time_anno;
};
typedef struct obj_inode INODE;

struct obj_infohash *idb_init( void );
void idb_free( void );

void idb_put( UCHAR *target, int port, UCHAR *node_id, IP *sa );
void idb_del_node( IHASH *ihash, ITEM *i_node );

void idb_clean( void );
void idb_expire( void );

void idb_update( INODE *db, IP *sa, int port );

ITEM *idb_find_target( UCHAR *target );
void idb_del_target( ITEM *i_id );

ITEM *idb_find_node( HASH *hash, UCHAR *node_id );
IP *idb_address( UCHAR *target );
