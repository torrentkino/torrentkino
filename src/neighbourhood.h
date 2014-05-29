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

#ifndef NEIGHBOURHOOD_H
#define NEIGHBOURHOOD_H

#include "list.h"
#include "hash.h"
#include "node_udp.h"
#include "bucket.h"

struct obj_nbhd {
	LIST *bucket;
	HASH *hash;
};
typedef struct obj_nbhd NBHD;

NBHD *nbhd_init( void );
void nbhd_free( void );

void nbhd_put( UCHAR *id, IP *sa );
void nbhd_del( UDP_NODE *n );

void nbhd_pinged( UCHAR *id );
void nbhd_ponged( UCHAR *id, IP *from );

void nbhd_expire( time_t now );
void nbhd_split( UCHAR *target, int verbose );

int nbhd_is_empty( void );

#endif
