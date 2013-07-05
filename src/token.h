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

#define TOKEN_SIZE 8
#define TOKEN_SIZE_MAX 20

struct obj_token {
	LIST *list;
	HASH *hash;
};

struct obj_tkn {
	UCHAR id[TOKEN_SIZE];
	time_t time;
};

struct obj_token *tkn_init( void );
void tkn_free( void );

void tkn_put( void );
void tkn_del( UCHAR *id );

void tkn_create( UCHAR *id );
void tkn_expire( void );
int tkn_validate( UCHAR *id );
UCHAR *tkn_read( void );
