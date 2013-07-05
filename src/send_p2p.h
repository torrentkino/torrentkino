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

void send_ping( IP *sa, UCHAR *tid );
void send_pong( IP *sa, UCHAR *tid, int tid_size );

void send_find_node_request( IP *sa, UCHAR *node_id, UCHAR *tid );
void send_find_node_reply( IP *sa, UCHAR *nodes_compact_list, int nodes_compact_size, UCHAR *tid, int tid_size );

void send_get_peers_request( IP *sa, UCHAR *node_id, UCHAR *tid );
void send_get_peers_nodes( IP *sa, UCHAR *nodes_compact_list, int nodes_compact_size, UCHAR *tid, int tid_size );
void send_get_peers_values( IP *sa, UCHAR *nodes_compact_list, int nodes_compact_size, UCHAR *tid, int tid_size );

void send_announce_request( IP *sa, UCHAR *tid, UCHAR *token, int token_size );
void send_announce_reply( IP *sa, UCHAR *tid, int tid_size );

void send_aes( IP *sa, struct obj_raw *raw );
void send_exec( IP *sa, struct obj_raw *raw );
