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

#define XP2P_MAX_BOOTSTRAP_NODES 20

struct obj_p2p {
	struct timeval time_now;
	time_t time_maintainance;
	time_t time_multicast;
	time_t time_announce;
	time_t time_restart;
	time_t time_expire;
	time_t time_split;
	time_t time_ping;
	time_t time_find;
	pthread_mutex_t *mutex;
};

struct obj_p2p *p2p_init( void );
void p2p_free( void );

void p2p_cron( void );
void p2p_bootstrap( void );

void p2p_parse( UCHAR *bencode, size_t bensize, IP *from );
void p2p_decrypt( UCHAR *bencode, size_t bensize, IP *from );
void p2p_decode( UCHAR *bencode, size_t bensize, IP *from );

void p2p_ping( UCHAR *node_sk, IP *from );
void p2p_find( struct obj_ben *packet, UCHAR *node_sk, IP *from );
void p2p_announce( struct obj_ben *packet, UCHAR *node_sk, IP *from );
void p2p_lookup( struct obj_ben *packet, UCHAR *node_sk, IP *from );

void p2p_pong( UCHAR *node_id, UCHAR *node_sk, IP *from );
void p2p_node_find( struct obj_ben *packet, UCHAR *node_id, UCHAR *node_sk, IP *from );
void p2p_node_announce( struct obj_ben *packet, UCHAR *node_id, UCHAR *node_sk, IP *from );
void p2p_node_lookup( struct obj_ben *packet, UCHAR *node_id, UCHAR *node_sk, IP *from );
void p2p_value( struct obj_ben *packet, UCHAR *node_id, UCHAR *node_sk, IP *from );

void p2p_lookup_nss( UCHAR *hostname, size_t size, IP *from );
void p2p_announce_myself( void );

void p2p_compute_realm_id( UCHAR *host_id, char *hostname );
