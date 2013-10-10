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

#ifndef INFO_HASH_H
#define INFO_HASH_H

#include "list.h"
#include "hash.h"
#include "hex.h"
#include "log.h"
#include "time.h"
#include "ben.h"
#include "p2p.h"
#include "torrentkino.h"

#define IDB_TARGET_SIZE_MAX 1024
#define IDB_NODES_SIZE_MAX 64

struct obj_idb {
	LIST *list;
	HASH *hash;
};

struct obj_target {
	UCHAR target[SHA1_SIZE];
	LIST *list;
	HASH *hash;
};
typedef struct obj_target TARGET;

struct obj_inode {
	UCHAR id[SHA1_SIZE];
	IP c_addr;
	time_t time_anno;
};
typedef struct obj_inode INODE;

struct obj_idb *idb_init( void );
void idb_free( void );
void idb_put( UCHAR *target_id, int port, UCHAR *node_id, IP *sa );
void idb_clean( void );
void idb_expire( time_t now );
int idb_compact_list( UCHAR *nodes_compact_list, UCHAR *target_id );

TARGET *tgt_init( UCHAR *target_id );
void tgt_free( ITEM *i );
ITEM *tgt_find( UCHAR *target );
int tgt_limit_reached( void );

INODE *inode_init( TARGET *target, UCHAR *node_id );
void inode_free( TARGET *target, ITEM *i );
ITEM *inode_find( HASH *target, UCHAR *node_id );
int inode_limit_reached( LIST *l );
void inode_update( INODE *inode, IP *sa, int port );

#endif
