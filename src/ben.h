/*
Copyright 2006 Aiko Barz

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

struct obj_ben {
	int t;
	
	union  {
		unsigned long int i;
		struct obj_str *s;
		LIST *d;
		LIST *l;
	} v;
};

struct obj_tuple {
	struct obj_ben *key;
	struct obj_ben *val;
};

struct obj_raw {
	UCHAR *code;
	long int size;
	UCHAR *p;
};

struct obj_ben *ben_init( int type );
void ben_free( struct obj_ben *node );
void ben_free_r( struct obj_ben *node );
ITEM *ben_free_item( struct obj_ben *node, ITEM *item );

struct obj_raw *raw_init( void );
void raw_free( struct obj_raw *raw );

void ben_dict( struct obj_ben *node, struct obj_ben *key, struct obj_ben *val );
void ben_list( struct obj_ben *node, struct obj_ben *val );
void ben_str( struct obj_ben *node, UCHAR *str, long int len );
void ben_int( struct obj_ben *node, long int i );

struct obj_tuple *tuple_init( struct obj_ben *key, struct obj_ben *val );
void tuple_free( struct obj_tuple *tuple );

struct obj_raw *ben_enc( struct obj_ben *node );
UCHAR *ben_enc_rec( struct obj_ben *node, UCHAR *p );
long int ben_enc_size( struct obj_ben *node );

int ben_validate( UCHAR *bencode, long int bensize );
int ben_validate_r( struct obj_raw *raw );
int ben_validate_d( struct obj_raw *raw );
int ben_validate_l( struct obj_raw *raw );
int ben_validate_i( struct obj_raw *raw );
int ben_validate_s( struct obj_raw *raw );

struct obj_ben *ben_dec( UCHAR *bencode, long int bensize );
struct obj_ben *ben_dec_r( struct obj_raw *raw );
struct obj_ben *ben_dec_d( struct obj_raw *raw );
struct obj_ben *ben_dec_l( struct obj_raw *raw );
struct obj_ben *ben_dec_i( struct obj_raw *raw );
struct obj_ben *ben_dec_s( struct obj_raw *raw );

struct obj_ben *ben_searchDictKey( struct obj_ben *node, struct obj_ben *key );
struct obj_ben *ben_searchDictStr( struct obj_ben *node, const char *buffer );
void ben_sort( struct obj_ben *node );
int ben_compare( struct obj_ben *key1, struct obj_ben *key2 );
