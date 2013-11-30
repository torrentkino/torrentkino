/*
Copyright 2006 Aiko Barz

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

#ifndef LIST_H
#define LIST_H

#include "malloc.h"

#ifdef NSS
#define list_add _nss_tk_list_add
#define list_clear _nss_tk_list_clear
#define list_del _nss_tk_list_del
#define list_free _nss_tk_list_free
#define list_init _nss_tk_list_init
#define list_ins _nss_tk_list_ins
#define list_next _nss_tk_list_next
#define list_prev _nss_tk_list_prev
#define list_put _nss_tk_list_put
#define list_rotate _nss_tk_list_rotate
#define list_size _nss_tk_list_size
#define list_start _nss_tk_list_start
#define list_stop _nss_tk_list_stop
#define list_value _nss_tk_list_value
#endif

struct obj_list {	
	struct obj_item *item;
	ULONG size;
};
typedef struct obj_list LIST;

struct obj_item {	
	void *val;
	struct obj_item *next;
	struct obj_item *prev;
};
typedef struct obj_item ITEM;

LIST *list_init( void );
void list_free( LIST *list );
void list_clear( LIST *list );

ITEM *list_start( LIST *list );
ITEM *list_stop( LIST *list );
ULONG list_size( LIST *list );

ITEM *list_put( LIST *list, void *payload );
ITEM *list_del( LIST *list, ITEM *item );

ITEM *list_ins( LIST *list, ITEM *here, void *payload );
ITEM *list_add( LIST *list, ITEM *here, void *payload );

ITEM *list_next( ITEM *item );
ITEM *list_prev( ITEM *item );

void list_rotate( LIST *list );

void *list_value( ITEM *item );

#endif
