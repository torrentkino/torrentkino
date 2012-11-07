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

#define SEND_UNICAST 0
#define SEND_MULTICAST 1

void send_ping( CIPV6 *sa, int type );
void send_pong( CIPV6 *sa, UCHAR *node_sk, int warning );

void send_find( CIPV6 *sa, UCHAR *node_id, UCHAR *lkp_id );
void send_node( CIPV6 *sa, struct obj_bckt *b, UCHAR *node_sk, UCHAR *lkp_id, int warning );

void send_aes( CIPV6 *sa, struct obj_raw *raw );
void send_exec( CIPV6 *sa, struct obj_raw *raw );
