/*
Copyright 2006 Aiko Barz

This file is part of masala/tumbleweed.

masala/tumbleweed is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/tumbleweed is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/tumbleweed.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/epoll.h>

#ifdef TUMBLEWEED
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "list.h"
#else
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "list.h"
#include "ben.h"
#endif

LIST *list_init( void ) {
	LIST *list = (LIST *) myalloc( sizeof(LIST), "list_init" );

	list->start = NULL;
	list->stop = NULL;
	list->counter = 0;

	return list;
}

void list_free( LIST *list ) {
	if( list != NULL ) {
		while( list->start != NULL ) {
			list_del( list, list->start );
		}
		myfree( list, "list_free" );
	}
}

void list_clear( LIST *list ) {
	ITEM *i = NULL;
	long int j = 0;

	/* Free payload */
	i = list->start;
	for( j=0; j<list->counter; j++ ) {
		myfree( i->val, "list_clear" );
		i = list_next( i );
	}
}

ITEM *list_put( LIST *list, void *payload ) {
	ITEM *newItem = NULL;

	/* Overflow */
	if( list->counter+1 <= 0 ) {
		return NULL;
	}

	/* Get memory */
	newItem = (ITEM *) myalloc( sizeof(ITEM), "list_put" );

	/* Data container */
	newItem->val = payload;

	/* First item? */
	if( list->start == NULL )
		list->start = newItem;
	if( list->stop == NULL )
		list->stop = newItem;

	/* Setup pointer for newItem */
	newItem->next = list->start;
	newItem->prev = list->stop;

	/* Update pointer for global start/stop */
	list->start->prev = newItem;
	list->stop->next  = newItem;

	/* We have a new ending */
	list->stop = newItem;

	/* Increment counter */
	list->counter++;

	/* Return pointer to the new entry */
	return newItem;
}

ITEM *list_ins( LIST *list, ITEM *here, void *payload ) {
	ITEM *new = NULL;
	ITEM *next = NULL;

	/* Overflow */
	if( list->counter+1 <= 0 ) {
		return NULL;
	}

	/* This insert is like a normal list_put */
	if( list->counter <= 1 ) {
		return list_put( list, payload );
	}

	/* Data */
	new = (ITEM *) myalloc( sizeof(ITEM), "list_app" );
	new->val = payload;

	/* Setup pointer */
	next = here->next;
	new->next = here->next;
	new->prev = here;
	here->next = new;
	next->prev = new;

	/* Fix start and stop */
	if( here == list->start && list->counter == 2 ) {
		list->stop = here->next;
	} else if( here == list->stop ) {
		list->start = here->next;
	}
	
	/* Increment counter */
	list->counter++;

	/* Return pointer to the new entry */
	return new;
}

ITEM *list_del( LIST *list, ITEM *item ) {
	/* Variables */
	ITEM *next = NULL;

	/* Check input */
	if( list == NULL )
		return NULL;
	if( item == NULL )
		return NULL;
	if( list->counter <= 0 )
		return NULL;

	/* If TRUE, there is only one item left */
	if( item == item->next && item == item->prev ) {
		list->start = NULL;
		list->stop = NULL;
	} else {
		/* Remember next->item */
		next = item->next;

		/* Set list pointer to make the item disappear */
		item->prev->next = item->next;
		item->next->prev = item->prev;

		/* If item is the stop item, set new stop item */
		if( list->stop == item ) {
			list->stop = item->prev;
		}

		/* If item is the start item, set new start item */
		if( list->start == item ) {
			list->start = item->next;
		}
	}

	/* Decrement list counter */
	list->counter--;

	/* item is not linked anymore. Free it */
	myfree( item, "list_del" );

	return next;
}

ITEM *list_next( ITEM *item ) {
	/* Variables */
	ITEM *next = NULL;

	/* Next item */
	if( item != NULL ) {
		next = (item->next != NULL ) ? item->next : NULL;
	}

	/* Return pointer to the next item */
	return next;
}

ITEM *list_prev( ITEM *item ) {
	/* Variables */
	ITEM *prev = NULL;

	/* Next item */
	if( item != NULL ) {
		prev = (item->prev != NULL ) ? item->prev : NULL;
	}

	/* Return pointer to the next item */
	return prev;
}

void list_swap( LIST *list, ITEM *item1, ITEM *item2 ) {
	ITEM *a = item1->prev;
	ITEM *b = item1;
	ITEM *c = item1->next;

	ITEM *x = item2->prev;
	ITEM *y = item2;
	ITEM *z = item2->next;

	/*
	struct obj_ben *key1 = NULL;
	struct obj_ben *key2 = NULL;
	struct obj_ben *key3 = NULL;
	*/

	/*
	key1 = b->key;
	key2 = y->key;
	printf( "#Vertausche: %s <> %s\n", key1->v.s->s, key2->v.s->s );
	*/

	/*
	key1 = b->prev->key;
	key2 = b->key;
	key3 = b->next->key;
	printf( "#Vorher: %s > %s > %s\n", key1->v.s->s, key2->v.s->s, key3->v.s->s );
	*/

	if( list->counter < 2 ) {
		return;
	} else if( list->counter == 2 ) {
		list->start = item2;
		list->stop  = item1;
	} else {
		/* item1 -> item2 */
		if( c == y && x == b ) {
			item2->prev = a;
			item2->next = item1;
			item1->prev = item2;
			item1->next = z;
			a->next = item2;
			z->prev = item1;
		}
		/* item2 -> item1 */
		else if( a == y && z == b ) {
			item2->prev = item1;
			item2->next = c;
			item1->prev = x;
			item1->next = item2;
			z->next = item2;
			c->prev = item1;
		} else {
			item1->prev = x;
			item1->next = z;
			item2->prev = a;
			item2->next = c;

			a->next = item2;
			x->next = item1;
			c->prev = item2;
			z->prev = item1;
		}

		if( item1 == list->start ) {
			list->start = item2;
			list->stop = list->start->prev;
		} else if( item1 == list->stop ) {
			list->stop = item2;
			list->start = list->stop->next;
		} else if( item2 == list->start ) {
			list->start = item1;
			list->stop = list->start->prev;
		} else if( item2 == list->stop ) {
			list->stop = item1;
			list->start = list->stop->next;
		}

		/* 
		key1 = b->prev->key;
		key2 = b->key;
		key3 = b->next->key;
		printf( "#Nachher: %s > %s > %s\n", key1->v.s->s, key2->v.s->s, key3->v.s->s );
		*/
	}
}
