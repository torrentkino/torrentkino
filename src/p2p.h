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

#define XP2P_MAX_BOOTSTRAP_NODES 20

#define XP2P_PING 1
#define XP2P_PONG 2
#define XP2P_FIND 3
#define XP2P_NODE 4

struct obj_p2p {
	struct timeval time_now;
	time_t time_expire;
	time_t time_split;
	time_t time_ping;
	time_t time_find;
	time_t time_srch;
	time_t time_restart;
	time_t time_multicast;
	time_t time_maintainance;

	pthread_mutex_t *mutex;
};

struct obj_p2p *p2p_init( void );
void p2p_free( void );

void p2p_cron( void );
void p2p_bootstrap( void );

void p2p_parse( UCHAR *bencode, size_t bensize, CIPV6 *from );
void p2p_decrypt( UCHAR *bencode, size_t bensize, CIPV6 *from );
void p2p_decode( UCHAR *bencode, size_t bensize, CIPV6 *from );

void p2p_ping( UCHAR *node_sk, CIPV6 *from, int warning );
void p2p_pong( UCHAR *node_id, UCHAR *node_sk, CIPV6 *from );
void p2p_find( struct obj_ben *packet, UCHAR *node_sk, CIPV6 *from, int warning );
void p2p_node( struct obj_ben *packet, UCHAR *node_id, UCHAR *node_sk, CIPV6 *from );

void p2p_lookup( UCHAR *find_id, size_t size, CIPV6 *from );
