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

#ifndef BEN_H
#define BEN_H

#include "main.h"
#include "list.h"
#include "fail.h"

#define BEN_STR  0
#define BEN_INT  1
#define BEN_LIST 2
#define BEN_DICT 3

#define BEN_STR_MAXLEN 10
#define BEN_STR_MAXSIZE 33554432
#ifdef __i386__
#define BEN_INT_MAXLEN 10
#define BEN_INT_MAXSIZE 2147483647
#else
#define BEN_INT_MAXLEN 19
#define BEN_INT_MAXSIZE 9223372036854775807
#endif

#ifdef NSS
#define ben_init _nss_tk_ben_init
#define ben_free _nss_tk_ben_free
#define ben_free_r _nss_tk_ben_free_r
#define ben_free_item _nss_tk_ben_free_item
#define raw_init _nss_tk_raw_init
#define raw_free _nss_tk_raw_free
#define ben_dict _nss_tk_ben_dict
#define ben_list _nss_tk_ben_list
#define ben_str _nss_tk_ben_str
#define ben_int _nss_tk_ben_int
#define tuple_init _nss_tk_tuple_init
#define tuple_free _nss_tk_tuple_free
#define ben_enc _nss_tk_ben_enc
#define ben_enc_rec _nss_tk_ben_enc_rec
#define ben_enc_size _nss_tk_ben_enc_size
#define ben_validate _nss_tk_ben_validate
#define ben_validate_r _nss_tk_ben_validate_r
#define ben_validate_d _nss_tk_ben_validate_d
#define ben_validate_l _nss_tk_ben_validate_l
#define ben_validate_i _nss_tk_ben_validate_i
#define ben_validate_s _nss_tk_ben_validate_s
#define ben_dec _nss_tk_ben_dec
#define ben_dec_r _nss_tk_ben_dec_r
#define ben_dec_d _nss_tk_ben_dec_d
#define ben_dec_l _nss_tk_ben_dec_l
#define ben_dec_i _nss_tk_ben_dec_i
#define ben_dec_s _nss_tk_ben_dec_s
#define ben_is_dict _nss_tk_ben_is_dict
#define ben_is_list _nss_tk_ben_is_list
#define ben_is_str _nss_tk_ben_is_str
#define ben_is_int _nss_tk_ben_is_int
#define ben_dict_search_key _nss_tk_ben_dict_search_key
#define ben_dict_search_str _nss_tk_ben_dict_search_str
#define ben_compare _nss_tk_ben_compare
#define ben_str_s _nss_tk_ben_str_s
#define ben_str_i _nss_tk_ben_str_i
#define str_init _nss_tk_str_init
#define str_free _nss_tk_str_free
#endif

typedef struct {
	UCHAR *s;
	long int i;
} STR;

typedef struct {
	int t;

	union  {
		unsigned long int i;
		STR *s;
		LIST *d;
		LIST *l;
	} v;
} BEN;

typedef struct {
	BEN *key;
	BEN *val;
} TUPLE;

typedef struct {
	UCHAR *code;
	long int size;
	UCHAR *p;
} RAW;

BEN *ben_init( int type );
void ben_free( BEN *node );
void ben_free_r( BEN *node );
ITEM *ben_free_item( BEN *node, ITEM *item );

RAW *raw_init( void );
void raw_free( RAW *raw );

void ben_dict( BEN *node, BEN *key, BEN *val );
void ben_list( BEN *node, BEN *val );
void ben_str( BEN *node, UCHAR *str, long int len );
void ben_int( BEN *node, long int i );

TUPLE *tuple_init( BEN *key, BEN *val );
void tuple_free( TUPLE *tuple );

RAW *ben_enc( BEN *node );
UCHAR *ben_enc_rec( BEN *node, UCHAR *p );
long int ben_enc_size( BEN *node );

int ben_validate( UCHAR *bencode, long int bensize );
int ben_validate_r( RAW *raw );
int ben_validate_d( RAW *raw );
int ben_validate_l( RAW *raw );
int ben_validate_i( RAW *raw );
int ben_validate_s( RAW *raw );

BEN *ben_dec( UCHAR *bencode, long int bensize );
BEN *ben_dec_r( RAW *raw );
BEN *ben_dec_d( RAW *raw );
BEN *ben_dec_l( RAW *raw );
BEN *ben_dec_i( RAW *raw );
BEN *ben_dec_s( RAW *raw );

int ben_is_dict( BEN *node );
int ben_is_list( BEN *node );
int ben_is_str( BEN *node );
int ben_is_int( BEN *node );

BEN *ben_dict_search_key( BEN *node, BEN *key );
BEN *ben_dict_search_str( BEN *node, const char *buffer );

int ben_compare( BEN *key1, BEN *key2 );

UCHAR *ben_str_s( BEN *node );
long int ben_str_i( BEN *node );

STR *str_init( UCHAR *buf, long int size );
void str_free( STR *str );

#endif
