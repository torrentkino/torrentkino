/*
Copyright 2006 Aiko Barz

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <semaphore.h>

#include "malloc.h"
#include "main.h"
#include "list.h"
#include "str.h"
#include "log.h"
#include "ben.h"

struct obj_ben *ben_init( int type ) {
	struct obj_ben *node = (struct obj_ben *) myalloc( sizeof( struct obj_ben), "ben_init" );

	node->t = type;

	switch( type ) {
		case BEN_STR:
			node->v.s = NULL;
			break;
		case BEN_INT:
			node->v.i = 0;
			break;
		case BEN_DICT:
			node->v.d = list_init();
			break;
		case BEN_LIST:
			node->v.l = list_init();
			break;
	}

	return node;
}

void ben_free( struct obj_ben *node ) {
	if( node == NULL )
		return;

	/* Delete recursively */
	ben_free_r( node );

	/* Delete the last node */
	myfree( node, "ben_free" );
}

void ben_free_r( struct obj_ben *node ) {
	ITEM *item = NULL;

	if( node == NULL )
		return;

	switch( node->t ) {
		case BEN_DICT:
			if( node->v.d != NULL ) {
				item = node->v.d->start;
				while( node->v.d->counter > 0 && item != NULL ) {
					item = ben_free_item( node, item );
				}

				list_free( node->v.d );
			}
			break;

		case BEN_LIST:
			if( node->v.l != NULL ) {
				item = node->v.l->start;
				while( node->v.l->counter > 0 && item != NULL ) {
					item = ben_free_item( node, item );
				}

				list_free( node->v.l );
			}
			break;
		case BEN_STR:
			if( node->v.s != NULL )
				str_free( node->v.s );
			break;
	}
}

ITEM *ben_free_item( struct obj_ben *node, ITEM *item ) {
	if( node == NULL || item == NULL )
		return NULL;
	if( node->t != BEN_DICT && node->t != BEN_LIST )
		return NULL;
	if( node->t == BEN_DICT && node->v.d == NULL )
		return NULL;
	if( node->t == BEN_LIST && node->v.l == NULL )
		return NULL;
	
	/* Remove key in case of BEN_DICT */
	if( node->t == BEN_DICT ) {
		tuple_free( item->val );
		item->val = NULL;
	} else {
		/* Remove ben object */
		ben_free_r( item->val );
		myfree( item->val, "ben_free_item" );
		item->val = NULL;
	}
	
	return list_del( node->v.d, item );
}

struct obj_raw *raw_init( void ) {
	struct obj_raw *raw = (struct obj_raw *) myalloc( sizeof( struct obj_raw), "raw_init" );
	raw->code = NULL;
	raw->size = 0;
	raw->p = NULL;
	return raw;
}

void raw_free( struct obj_raw *raw ) {
	myfree( raw->code, "raw_free" );
	myfree( raw, "raw_free" );
}
 

struct obj_tuple *tuple_init( struct obj_ben *key, struct obj_ben *val ) {
	struct obj_tuple *tuple = (struct obj_tuple *) myalloc( sizeof( struct obj_tuple), "tuple_init" );
	tuple->key = key;
	tuple->val = val;
	return tuple;
}

void tuple_free( struct obj_tuple *tuple ) {
	ben_free( tuple->key );
   	ben_free( tuple->val );
	myfree( tuple, "tuple_item" );
}

void ben_dict( struct obj_ben *node, struct obj_ben *key, struct obj_ben *val ) {
	struct obj_tuple *tuple = NULL;

	if( node == NULL )
		log_fail( "ben_dict( 1)" );
	if( node->t != BEN_DICT)
		log_fail( "ben_dict( 2)" );
	if( node->v.d == NULL )
		log_fail( "ben_dict( 3)" );
	if( key == NULL )
		log_fail( "ben_dict( 4)" );
	if( key->t != BEN_STR)
		log_fail( "ben_dict( 5)" );
	if( val == NULL )
		log_fail( "ben_dict( 6)" );

	tuple = tuple_init( key, val );
	
	if( list_put( node->v.d, tuple) == NULL )
		log_fail( "ben_dict( 7)" );
}

void ben_list( struct obj_ben *node, struct obj_ben *val ) {
	if( node == NULL )
		log_fail( "ben_list( 1)" );
	if( node->t != BEN_LIST)
		log_fail( "ben_list( 2)" );
	if( node->v.l == NULL )
		log_fail( "ben_list( 3)" );
	if( val == NULL )
		log_fail( "ben_list( 4)" );

	if( list_put( node->v.l, val) == NULL )
		log_fail( "ben_list( 5)" );
}

void ben_str( struct obj_ben *node, UCHAR *str, long int len ) {
	if( node == NULL )
		log_fail( "ben_str( 1)" );
	if( node->t != BEN_STR)
		log_fail( "ben_str( 2)" );
	if( str == NULL )
		log_fail( "ben_str( 3)" );
	if( len <= 0)
		log_fail( "ben_str( 4)" );

	node->v.s = str_init( str, len );
}

void ben_int( struct obj_ben *node, long int i ) {
	if( node == NULL )
		log_fail( "ben_int( 1)" );
	if( node->t != BEN_INT)
		log_fail( "ben_int( 2)" );
	
	node->v.i = i;
}

long int ben_enc_size( struct obj_ben *node ) {
	ITEM *item = NULL;
	struct obj_tuple *tuple = NULL;
	char buf[MAIN_BUF+1];
	long int size = 0;

	if( node == NULL )
		return size;

	switch( node->t ) {
		case BEN_DICT:
			size += 2; /* de */
			
			if( node->v.d == NULL )
				return size;

			if( node->v.d->counter <= 0 )
				return size;

			item = node->v.d->start;
			do {
				tuple = item->val;

				if( tuple->key != NULL && tuple->val != NULL ) {
					size += ben_enc_size( tuple->key );
					size += ben_enc_size( tuple->val );
				}
				
				item = list_next( item );
				
			} while( item != node->v.d->stop->next );

			break;

		case BEN_LIST:
			size += 2; /* le */

			if( node->v.l == NULL )
				return size;

			if( node->v.l->counter <= 0 )
				return size;

			item = node->v.l->start;
			do {
				if( item->val != NULL ) {
					size += ben_enc_size( item->val );
				}
				item = list_next( item );
				
			} while( item != node->v.l->stop->next );

			break;

		case BEN_INT:
			snprintf( buf, MAIN_BUF+1, "i%lie", node->v.i );
			size += strlen( buf );
			break;

		case BEN_STR:
			snprintf( buf, MAIN_BUF+1, "%li:", node->v.s->i );
			size += strlen( buf) + node->v.s->i;
			break;

	}

	return size;
}

struct obj_raw *ben_enc( struct obj_ben *node ) {
	struct obj_raw *raw = raw_init();

	/* Calculate size of ben data */
	raw->size = ben_enc_size( node );
	if( raw->size <= 0 ) {
		raw_free( raw );
		return NULL;
	}

	/* Encode ben object */
	raw->code = (unsigned char *) myalloc( (raw->size) * sizeof( unsigned char), "ben_enc" );
	raw->p = ben_enc_rec( node,raw->code );
	if( raw->p == NULL ||( long int)(raw->p-raw->code) != raw->size ) {
		raw_free( raw );
		return NULL;
	}

	return raw;
}

UCHAR *ben_enc_rec( struct obj_ben *node, UCHAR *p ) {
	ITEM *item = NULL;
	struct obj_tuple *tuple = NULL;
	char buf[MAIN_BUF+1];
	long int len = 0;

	if( node == NULL || p == NULL ) {
		return NULL;
	}
		
	switch( node->t ) {
		case BEN_DICT:
			*p++ = 'd';
 
			if( node->v.d != NULL && node->v.d->counter > 0 ) {
				item = node->v.d->start;
				do {
					tuple = item->val;

					if( tuple->key != NULL && tuple->val != NULL ) {
						if( ( p = ben_enc_rec( tuple->key, p)) == NULL )
							return NULL;
						
						if( ( p = ben_enc_rec( tuple->val, p)) == NULL )
							return NULL;
					}
					
					item = list_next( item );
					
				} while( item != node->v.d->stop->next );
			}
			
			*p++ = 'e';
			break;

		case BEN_LIST:
			*p++ = 'l';
			
			if( node->v.l != NULL && node->v.l->counter > 0 ) {
				item = node->v.l->start;
				do {
					if( ( p = ben_enc_rec( item->val, p)) == NULL )
						return NULL;
					
					item = list_next( item );
					
				} while( item != node->v.l->stop->next );
			}

			*p++ = 'e';
			break;

		case BEN_INT:
			snprintf( buf, MAIN_BUF+1, "i%lie", node->v.i );
			len = strlen( buf );
			memcpy( p, buf, len );
			p += len;
			break;

		case BEN_STR:
			/* Meta */
			snprintf( buf, MAIN_BUF+1, "%li:", node->v.s->i );
			len = strlen( buf );
			memcpy( p, buf, len );
			p += len;
			/* Data */
			memcpy( p, node->v.s->s, node->v.s->i );
			p += node->v.s->i;
			break;

	}

	return p;
}

struct obj_ben *ben_dec( UCHAR *bencode, long int bensize ) {
	struct obj_raw raw;

	raw.code = (UCHAR *)bencode;
	raw.size = bensize;
	raw.p = (UCHAR *)bencode;

	return ben_dec_r( &raw );
}

struct obj_ben *ben_dec_r( struct obj_raw *raw ) {
	struct obj_ben *node = NULL;

	switch( *raw->p ) {
		case 'd':
			node = ben_dec_d( raw );
			break;

		case 'l':
			node = ben_dec_l( raw );
			break;

		case 'i':
			node = ben_dec_i( raw );
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			node = ben_dec_s( raw );
			break;
	}
	
	return node;
}

struct obj_ben *ben_dec_d( struct obj_raw *raw ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *val = NULL;
	struct obj_ben *key = NULL;
	
	raw->p++;
	while( *raw->p != 'e' ) {
		key = ben_dec_s( raw );
		val = ben_dec_r( raw );
		ben_dict( dict, key, val );
	}
	++raw->p;

	return dict;
}

struct obj_ben *ben_dec_l( struct obj_raw *raw ) {
	struct obj_ben *list = ben_init( BEN_LIST );
	struct obj_ben *val = NULL;
	
	raw->p++;
	while( *raw->p != 'e' ) {
		val = ben_dec_r( raw );
		ben_list( list, val );
	}
	++raw->p;
	
	return list;
}

struct obj_ben *ben_dec_s( struct obj_raw *raw ) {
	struct obj_ben *node = ben_init( BEN_STR );
	long int i = 0;
	long int l = 0;
	unsigned char *start = raw->p;
	unsigned char *buf = NULL;
	int run = 1;

	while( run ) {
		switch( *raw->p ) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i++;
				raw->p++;
				break;
			case ':':
				buf = (unsigned char *) myalloc( ( i+1) * sizeof( unsigned char), "ben_dec_s" );
				memcpy( buf,start,i );
				l = atol( (char *)buf );
				myfree( buf, "ben_dec_s" );
	
				raw->p += 1;
				ben_str( node,raw->p,l );
				raw->p += l;
				
				run = 0;
				break;
		}
	}
   
	return node;
}

struct obj_ben *ben_dec_i( struct obj_raw *raw ) {
	struct obj_ben *node = ben_init( BEN_INT );
	long int i = 0;
	unsigned char *start = NULL;
	unsigned char *buf = NULL;
	int run = 1;
	long int prefix = 1;
	long int result = 0;

	start = ++raw->p;
	if( *raw->p == '-' ) {
		prefix = -1;
		start = ++raw->p;
	}

	while( run ) {
		switch( *raw->p ) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i++;
				raw->p++;
				break;
			case 'e':
				buf = (unsigned char *) myalloc( ( i+1) * sizeof( unsigned char), "ben_dec_i" );
				memcpy( buf,start,i );
				result = atol( (char *)buf );
				myfree( buf, "ben_dec_i" );

				raw->p++;
				run = 0;
				break;
		}
	}

	result = prefix * result;

	ben_int( node, result );
	
	return node;
}

int ben_validate( UCHAR *bencode, long int bensize ) {
	struct obj_raw raw;

	raw.code = (UCHAR *)bencode;
	raw.size = bensize;
	raw.p = (UCHAR *)bencode;

	return ben_validate_r( &raw );
}

int ben_validate_r( struct obj_raw *raw ) {
	if( raw == NULL )
		return 0;

	if( raw->code == NULL || raw->p == NULL || raw->size < 1 )
		return 0;

	if( ( long int)( raw->p - raw->code) >= raw->size )
		return 0;

	switch( *raw->p ) {
		case 'd':
			if( !ben_validate_d( raw) )
				return 0;
			break;

		case 'l':
			if( !ben_validate_l( raw) )
				return 0;
			break;

		case 'i':
			if( !ben_validate_i( raw) )
				return 0;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if( !ben_validate_s( raw) )
				return 0;
			break;

		default:
			return 0;
	}

	return 1;
}

int ben_validate_d( struct obj_raw *raw ) {
	if( ( long int)( ++raw->p - raw->code) >= raw->size )
		return 0;
	
	while( *raw->p != 'e' ) {
		if( !ben_validate_s( raw) )
			return 0;
		if( !ben_validate_r( raw) )
			return 0;
		if( ( long int)( raw->p - raw->code) >= raw->size )
			return 0;
	}

	if( ( long int)( ++raw->p - raw->code) > raw->size )
		return 0;
	
	return 1;
}

int ben_validate_l( struct obj_raw *raw ) {
	if( ( long int)( ++raw->p - raw->code) >= raw->size )
		return 0;
	
	while( *raw->p != 'e' ) {
		if( !ben_validate_r( raw) )
			return 0;
		if( ( long int)( raw->p - raw->code) >= raw->size )
			return 0;
	}

	if( ( long int)( ++raw->p - raw->code) > raw->size )
		return 0;
	
	return 1;
}

int ben_validate_s( struct obj_raw *raw ) {
	long int i = 0;
	unsigned char *start = raw->p;
	unsigned char *buf = NULL;
	int run = 1;

	if( ( long int)( raw->p - raw->code) >= raw->size )
		return 0;
	
	while( ( long int)( raw->p - raw->code) < raw->size && run == 1 ) {
		switch( *raw->p ) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i++;
				raw->p++;
				break;
			case ':':
				/* String length limitation */
				if( i <= 0 || i > BEN_STR_MAXLEN )
					return 0;

				buf = (unsigned char *) myalloc( ( i+1) * sizeof( unsigned char), "ben_validate_s" );
				memcpy( buf,start,i );
				i = atol( (char *)buf );
				myfree( buf, "ben_validate_s" );

				/* i <= 0 makes no sense */
				if( i <= 0 || i > BEN_STR_MAXSIZE )
					return 0;

				raw->p += i+1;
				run = 0;
				break;
			default:
				return 0;
		}
	}

	if( ( long int)( raw->p - raw->code) > raw->size )
		return 0;
	
	return 1;
}

int ben_validate_i( struct obj_raw *raw ) {
	long int i = 0;
	unsigned char *start = NULL;
	unsigned char *buf = NULL;
	int run = 1;
	long int result = 0;

	if( ( long int)( ++raw->p - raw->code) >= raw->size )
		return 0;
	
	start = raw->p;
	if( *raw->p == '-' ) {
		start = ++raw->p;
		
		if( ( long int)( raw->p - raw->code) >= raw->size )
			return 0;
	}

	while( ( long int)( raw->p - raw->code) < raw->size && run == 1 ) {
		switch( *raw->p ) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				i++;
				raw->p++;
				break;
			case 'e':
				if( i <= 0 || i > BEN_INT_MAXLEN )
					return 0;

				buf = (unsigned char *) myalloc( ( i+1) * sizeof( unsigned char), "ben_validate_i" );
				memcpy( buf,start,i );
				result = atol( (char *)buf );
				myfree( buf, "ben_validate_i" );

				if( result < 0 || result > BEN_INT_MAXSIZE )
					return 0;

				raw->p++;
				run = 0;
				break;
			default:
				return 0;
		}
	}

	if( ( long int)( raw->p - raw->code) > raw->size )
		return 0;
   
	return 1;
}

struct obj_ben *ben_searchDictKey( struct obj_ben *node, struct obj_ben *key ) {
	ITEM *item = NULL;
	struct obj_ben *thiskey = NULL;
	struct obj_tuple *tuple = NULL;
	
	/* Tests */
	if( node == NULL )
		return NULL;
	if( node->t != BEN_DICT )
		return NULL;
	if( key == NULL )
		return NULL;
	if( key->t != BEN_STR )
		return NULL;
	if( node->v.d == NULL )
		return NULL;
	if( node->v.d->counter <= 0 )
		return NULL;

	item = node->v.d->start;

	do {
		tuple = item->val;
		thiskey = tuple->key;
		if( thiskey->v.s->i == key->v.s->i && memcmp( thiskey->v.s->s, key->v.s->s, key->v.s->i) == 0 ) {
			return tuple->val;
		}
		item = list_next( item );
		
	} while( item != node->v.d->stop->next );

	return NULL;
}

struct obj_ben *ben_searchDictStr( struct obj_ben *node, const char *buffer ) {
	struct obj_ben *result = NULL;
	struct obj_ben *key = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)buffer, strlen( buffer) );
	result = ben_searchDictKey( node, key );
	ben_free( key );
	return result;
}

void ben_sort( struct obj_ben *node ) {
	ITEM *item = NULL;
	ITEM *next = NULL;
	struct obj_ben *key1 = NULL;
	struct obj_ben *key2 = NULL;
	struct obj_tuple *tuple_this = NULL;
	struct obj_tuple *tuple_next = NULL;
	long int switchcounter = 0;
	int result = 0;
	
	if( node == NULL )
		log_fail( "ben_sort( 1)" );
	if( node->t != BEN_DICT)
		log_fail( "ben_sort( 2)" );
	if( node->v.d == NULL )
		return;
	if( node->v.d->counter < 2 )
		return;

	item = node->v.d->start;

	while( 1 ) {
		next = list_next( item );

		/* Reached the end */
		if( next == node->v.d->start ) {
			if( switchcounter == 0 ) {
				/* The list is sorted now */
				break;
			}

			/* Reset switchcounter ... */
			switchcounter = 0;

			/* ... and start again */
			item = node->v.d->start;
			next = list_next( item );
		}

		tuple_this = item->val;
		tuple_next = next->val;
		key1 = tuple_this->key;
		key2 = tuple_next->key;

		result = ben_compare( key1, key2 );
		if( result > 0 ) {
			list_swap( node->v.d, item, next );
			switchcounter++;
			
			/* Continue moving up until start is reached */
			if( next != node->v.d->start ) {
				item = list_prev( next );
			}
		} else {
			/* Move down */
			item = next;
		}
	}
}

int ben_compare( struct obj_ben *key1, struct obj_ben *key2 ) {
	long int length = (key1->v.s->i > key2->v.s->i) ? key1->v.s->i : key2->v.s->i;
	long int i = 0;

	for( i=0; i<length; i++ ) {
		if( key1->v.s->s[i] > key2->v.s->s[i] ) {
			return 1;
		} else if( key1->v.s->s[i] < key2->v.s->s[i] ) {
			return -1;
		}
	}

	/* Strings are equal for "length" characters */
	if( key1->v.s->i > key2->v.s->i ) {
		return 1;
	} else if( key1->v.s->i < key2->v.s->i ) {
		return -1;
	}

	/* Both strings are completely equal */
	return 0;
}
