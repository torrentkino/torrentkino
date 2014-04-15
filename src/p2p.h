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

#ifndef P2P_H
#define P2P_H

#include "ip.h"
#include "ben.h"
#include "transaction.h"
#include "token.h"

#define P2P_MAX_BOOTSTRAP_NODES 20

#define P2P_TYPE_UNKNOWN 0
#define P2P_PING 1
#define P2P_PING_MULTICAST 2
#define P2P_FIND_NODE 3
#define P2P_GET_PEERS 4
#define P2P_ANNOUNCE_START 5
#define P2P_ANNOUNCE_ENGAGE 6

struct obj_p2p {
	struct timeval time_now;
	time_t time_maintainance;
	time_t time_multicast;
	time_t time_announce_host;
	time_t time_restart;
	time_t time_expire;
	time_t time_cache;
	time_t time_split;
	time_t time_token;
	time_t time_ping;
	time_t time_find;
};
typedef struct obj_p2p P2P;

P2P *p2p_init( void );
void p2p_free( void );

void p2p_bootstrap( void );

void p2p_cron( void );
void p2p_cron_ping( void );
void p2p_cron_find_myself( void );
void p2p_cron_find_random( void );
void p2p_cron_find( UCHAR *target );
void p2p_cron_announce( ITEM *ti );

void p2p_parse( UCHAR *bencode, size_t bensize, IP *from );
#ifdef POLARSSL
void p2p_decrypt( UCHAR *bencode, size_t bensize, IP *from );
#endif
void p2p_decode( UCHAR *bencode, size_t bensize, IP *from );

void p2p_request( BEN *packet, IP *from );
void p2p_reply( BEN *packet, IP *from );
void p2p_error( BEN *packet, IP *from );

void p2p_ping( BEN *tid, IP *from );
void p2p_pong( UCHAR *node_id, IP *from );

void p2p_find_node_get_request( BEN *arg, BEN *tid, IP *from );
void p2p_find_node_get_reply( BEN *arg, UCHAR *node_id, IP *from );

void p2p_get_peers_get_request( BEN *arg, BEN *tid, IP *from );
void p2p_get_peers_get_reply( BEN *arg, UCHAR *node_id, ITEM *ti, IP *from );
void p2p_get_peers_get_nodes( BEN *nodes, UCHAR *node_id, ITEM *ti, BEN *token, IP *from );
void p2p_get_peers_get_values( BEN *values, UCHAR *node_id, ITEM *ti, BEN *token, IP *from );

void p2p_announce_get_request( BEN *arg, UCHAR *node_id, BEN *tid, IP *from );
void p2p_announce_get_reply( BEN *arg, UCHAR *node_id, ITEM *ti, IP *from );

void p2p_localhost_get_request( BEN *arg, BEN *tid, IP *from );
int p2p_localhost_lookup_cache( UCHAR *target, BEN *tid, IP *from );
int p2p_localhost_lookup_local( UCHAR *target, BEN *tid, IP *from );
void p2p_localhost_lookup_remote( UCHAR *target, int type, BEN *tid, IP *from );

int p2p_packet_from_myself( UCHAR *node_id );

int p2p_is_hash( BEN *node );
int p2p_is_ip( BEN *node );
int p2p_is_port( BEN *node );

#endif
