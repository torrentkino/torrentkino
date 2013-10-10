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

#ifndef LOOKUP_H
#define LOOKUP_H

#include "main.h"
#include "malloc.h"
#include "neighbourhood.h"

struct obj_lookup {

	/* What are we looking for */
    UCHAR target[SHA1_SIZE];

	LIST *list;
	HASH *hash;

	/* Caller */
	IP c_addr;
	int send_reply;
};
typedef struct obj_lookup LOOKUP;

struct obj_lnode {
	UCHAR id[SHA1_SIZE];
	IP c_addr;
	UCHAR token[TOKEN_SIZE_MAX];
	int token_size;
};
typedef struct obj_lnode LNODE;

LOOKUP *ldb_init( UCHAR *target, IP *from );
void ldb_free( LOOKUP *l );

void ldb_put( LOOKUP *l, UCHAR *node_id, IP *from );

LNODE *ldb_find( LOOKUP *l, UCHAR *node_id );
void ldb_update( LOOKUP *l, UCHAR *node_id, BEN *token, IP *from );
int ldb_compare(UCHAR *id1, UCHAR *id2, UCHAR *target );
#endif
