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

#define SEND_UNICAST 0
#define SEND_MULTICAST 1

void send_ping( IP *sa, int type );
void send_pong( IP *sa, UCHAR *node_sk );

void send_announce( IP *sa, UCHAR *lkp_id );
void send_find( IP *sa, UCHAR *node_id );
void send_lookup( IP *sa, UCHAR *node_id, UCHAR *lkp_id );

void send_node( IP *sa, BUCK *b, UCHAR *node_sk, UCHAR *lkp_id, UCHAR *reply_type );
void send_value( IP *sa, IP *value, UCHAR *node_sk, UCHAR *lkp_id );

void send_aes( IP *sa, struct obj_raw *raw );
void send_exec( IP *sa, struct obj_raw *raw );
