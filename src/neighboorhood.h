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

LIST *nbhd_init( void );
void nbhd_free( void );

void nbhd_put( NODE *n );
void nbhd_del( NODE *n );

void nbhd_split( void );
void nbhd_ping( void );

void nbhd_find_myself( void );
void nbhd_find_random( void );
void nbhd_find( UCHAR *find_id );
void nbhd_lookup( LOOKUP *l );
void nbhd_announce( ANNOUNCE *a );

void nbhd_send( IP *sa, UCHAR *node_id, UCHAR *lkp_id, UCHAR *node_sk, UCHAR *reply_type );
void nbhd_print( void );
