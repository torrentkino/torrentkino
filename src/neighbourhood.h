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

#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

#include "list.h"
#include "hash.h"
#include "node.h"

struct obj_nbhd {
	LIST *bucket;
	HASH *hash;
};
typedef struct obj_nbhd NBHD;

NBHD *nbhd_init( void );
void nbhd_free( void );

void nbhd_put( UCHAR *id, IP *sa );
void nbhd_del( NODE *n );

void nbhd_pinged( UCHAR *id );
void nbhd_ponged( UCHAR *id, IP *sa );

void nbhd_expire( time_t now );
void nbhd_split( UCHAR *target, int verbose );

int nbhd_is_empty( void );

#endif
