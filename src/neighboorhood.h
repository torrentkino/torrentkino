/*
Copyright 2011 Aiko Barz

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

LIST *nbhd_init( void );
void nbhd_free( void );

void nbhd_put( struct obj_nodeItem *n );
void nbhd_del( struct obj_nodeItem *n );

void nbhd_split( void );
void nbhd_ping( void );

void nbhd_find_myself( void );
void nbhd_find_random( void );
void nbhd_find( UCHAR *find_id );
void nbhd_lookup( struct obj_lkp *l );

void nbhd_send( CIPV6 *sa, UCHAR *node_id, UCHAR *lkp_id, UCHAR *node_sk, int warning );
void nbhd_print( void );
