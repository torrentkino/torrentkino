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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <limits.h>

#include "malloc.h"
#include "thrd.h"
#include "torrentkino.h"
#include "str.h"
#include "list.h"
#include "hash.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "unix.h"
#include "opts.h"
#include "udp.h"
#include "ben.h"
#ifdef POLARSSL
#include "aes.h"
#endif
#include "token.h"
#include "neighbourhood.h"
#include "bucket.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "send_udp.h"
#include "search.h"
#include "time.h"
#include "random.h"
#include "sha1.h"
#include "hex.h"
#include "value.h"
#include "cache.h"
#include "worker.h"

P2P *p2p_init( void ) {
	P2P *p2p = (P2P *) myalloc( sizeof(P2P) );

	p2p->time_maintainance = 0;
	p2p->time_multicast = 0;
	p2p->time_announce = 0;
	p2p->time_restart = 0;
	p2p->time_expire = 0;
	p2p->time_cache = 0;
	p2p->time_split = 0;
	p2p->time_token = 0;
	p2p->time_find = 0;
	p2p->time_ping = 0;

	gettimeofday( &p2p->time_now, NULL );

	return p2p;
}

void p2p_free( void ) {
	myfree( _main->p2p );
}

void p2p_bootstrap( void ) {
	struct addrinfo hints;
	struct addrinfo *addrinfo = NULL;
	struct addrinfo *p = NULL;
	int rc = 0;
	int i = 0;
	ITEM *ti = NULL;
	char port[6];

	snprintf( port, 6, "%i", _main->conf->bootstrap_port );

	/* Compute address of bootstrap node */
	memset( &hints, '\0', sizeof(struct addrinfo) );
	hints.ai_socktype = SOCK_STREAM;

#ifdef IPV6
	hints.ai_family = AF_INET6;
#elif IPV4
	hints.ai_family = AF_INET;
#endif
	rc = getaddrinfo( _main->conf->bootstrap_node, port,
		&hints, &addrinfo );
	if( rc != 0 ) {
		info( NULL, 0, "getaddrinfo: %s", gai_strerror( rc ) );
		return;
	}

	p = addrinfo;
	while( p != NULL && i < P2P_MAX_BOOTSTRAP_NODES ) {

		/* Send PING to a bootstrap node */
		if( strcmp( _main->conf->bootstrap_node, CONF_MULTICAST ) == 0 ) {
			ti = tdb_put( P2P_PING_MULTICAST );
			send_ping( (IP *)p->ai_addr, tdb_tid( ti ) );
		} else {
			ti = tdb_put( P2P_PING );
			send_ping( (IP *)p->ai_addr, tdb_tid( ti ) );
		}

		p = p->ai_next;	i++;
	}

	freeaddrinfo( addrinfo );
}

void p2p_cron( void ) {
	/* Tick Tock */
	gettimeofday( &_main->p2p->time_now, NULL );

	if( nbhd_is_empty() ) {

		/* Bootstrap PING */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_restart ) {
			p2p_bootstrap();
			time_add_1_min_approx( &_main->p2p->time_restart );
		}

	} else {

		/* Expire objects. Run once a minute. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_expire ) {
			tdb_expire( _main->p2p->time_now.tv_sec );
			nbhd_expire( _main->p2p->time_now.tv_sec );
			val_expire( _main->p2p->time_now.tv_sec );
			tkn_expire( _main->p2p->time_now.tv_sec );
			cache_expire( _main->p2p->time_now.tv_sec );
			time_add_1_min_approx( &_main->p2p->time_expire );
		}

		/* Split buckets. Evolve neighbourhood. Run often to evolve
		 * neighbourhood fast. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_split ) {
			nbhd_split( _main->conf->node_id, TRUE );
			time_add_5_sec_approx( &_main->p2p->time_split );
		}

		/* Find nodes every ~5 minutes. Run once a minute. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_find ) {
			p2p_cron_find_myself();
			time_add_1_min_approx( &_main->p2p->time_find );
		}

		/* Find random node every ~5 minutes for maintainance reasons. Run once
		 * a minute */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_maintainance ) {
			p2p_cron_find_random();
			time_add_1_min_approx( &_main->p2p->time_maintainance );
		}

		/* Announce my hostname every ~5 minutes. This includes a full search
		 * to get the needed tokens first. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_announce ) {
			p2p_cron_announce_start();
			time_add_5_min_approx( &_main->p2p->time_announce );
		}

		/* Ping all nodes every ~5 minutes. Run once a minute. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_ping ) {
			p2p_cron_ping();
			time_add_1_min_approx( &_main->p2p->time_ping );
		}

		/* Renew cached requests */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_cache ) {
			cache_renew( _main->p2p->time_now.tv_sec );
			time_add_1_min_approx( &_main->p2p->time_cache );
		}

		/* Create a new token every ~5 minutes */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_token ) {
			tkn_put();
			time_add_5_min_approx( &_main->p2p->time_token );
		}
	}

	/* Try to register multicast address until it works. */
	if( _main->udp->multicast == FALSE ) {
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_multicast ) {
			udp_multicast( UDP_JOIN_MCAST );
			time_add_5_min_approx( &_main->p2p->time_multicast );
		}
	}
}

void p2p_cron_ping( void ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	UDP_NODE *n = NULL;
	ITEM *ti = NULL;
	unsigned long int j = 0;

	/* Cycle through all the buckets */
	item_b = list_start( _main->nbhd->bucket );
	while( item_b != NULL ) {
		b = list_value( item_b );

		/* Cycle through all the nodes */
		j = 0;
		item_n = list_start( b->nodes );
		while( item_n != NULL ) {
			n = list_value( item_n );

			/* It's time for pinging */
			if( _main->p2p->time_now.tv_sec > n->time_ping ) {

				/* Ping the first 8 nodes. Ignore the rest. */
				if( j < 8 ) {
					ti = tdb_put( P2P_PING );
					send_ping( &n->c_addr, tdb_tid( ti ) );
					nbhd_pinged( n->id );
				} else {
					nbhd_pinged( n->id );
				}
			}

			item_n = list_next( item_n );
			j++;
		}

		item_b = list_next( item_b );
	}
}

void p2p_cron_find_myself( void ) {
	p2p_cron_find( _main->conf->node_id );
}

void p2p_cron_find_random( void ) {
	UCHAR node_id[SHA1_SIZE];
	rand_urandom( node_id, SHA1_SIZE );
	p2p_cron_find( node_id );
}

void p2p_cron_find( UCHAR *target ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	UDP_NODE *n = NULL;
	unsigned long int j = 0;
	ITEM *ti = NULL;

	if( ( item_b = bckt_find_any_match( _main->nbhd->bucket, target)) == NULL ) {
		return;
	} else {
		b = list_value( item_b );
	}

	j = 0;
	item_n = list_start( b->nodes );
	while( item_n != NULL && j < 8 ) {
		n = list_value( item_n );

		if( _main->p2p->time_now.tv_sec > n->time_find ) {

			ti = tdb_put( P2P_FIND_NODE );
			send_find_node_request( &n->c_addr, target, tdb_tid( ti ) );
			time_add_5_min_approx( &n->time_find );
		}

		item_n = list_next( item_n );
		j++;
	}
}

void p2p_cron_announce_start( void ) {
	LOOKUP *l = NULL;
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	UCHAR *target = _main->conf->host_id;
	ITEM *ti = NULL;
	UCHAR *p = NULL;
	int nodes_compact_size = 0;
	int j = 0;
	IP sin;
	UCHAR *id = NULL;

	/* Start the incremental remote search program */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket,
		nodes_compact_list, target );

	/* Create tid and get the lookup table */
	ti = tdb_put( P2P_ANNOUNCE_START );
	l = ldb_init( target, NULL, NULL );
	tdb_link_ldb( ti, l );

	p = nodes_compact_list;
	for( j=0; j<nodes_compact_size; j+=IP_SIZE_META_TRIPLE ) {

		/* ID */
		id = p;	p += SHA1_SIZE;

		/* IP + Port */
		p = ip_bytes_to_sin( &sin, p );

		/* Remember queried node */
		ldb_put( l, id, &sin );

		/* Query node */
		send_get_peers_request( &sin, target, tdb_tid( ti ) );
	}
}

void p2p_cron_announce_engage( ITEM *ti ) {
	ITEM *item = NULL;
	ITEM *t_new = NULL;
	ULONG j = 0;
	TID *tid = list_value( ti );
	LOOKUP *l = tid->lookup;
	NODE_L *n = NULL;

	info( NULL, 0, "Start announcing after querying %lu nodes",
		list_size( l->list ) );

	item = list_start( l->list );
	while( item != NULL && j < 8 ) {
		n = list_value( item );

		if( n->token_size != 0 ) {
			t_new = tdb_put( P2P_ANNOUNCE_ENGAGE );
			send_announce_request( &n->c_addr, tdb_tid( t_new ), n->token, n->token_size );
			j++;
		}

		item = list_next( item );
	}
}

void p2p_parse( UCHAR *bencode, size_t bensize, IP *from ) {
	/* Tick Tock */
	mutex_block( _main->work->mutex );
	gettimeofday( &_main->p2p->time_now, NULL );
	mutex_unblock( _main->work->mutex );

	/* UDP packet too small */
	if( bensize < 1 ) {
		info( from, 0, "Zero size packet from" );
		return;
	}

	/* Ignore link-local address */
	if( ip_is_linklocal( from ) ) {
		info( from, 0, "DROP LINK-LOCAL message from" );
		return;
	}

	/* Validate bencode */
	if( !ben_validate( bencode, bensize ) ) {
		info( from, 0, "Received broken bencode from" );
		return;
	}

	/* Encrypted message or plaintext message */
#ifdef POLARSSL
	if( _main->conf->bool_encryption && !ip_is_localhost( from ) ) {
		p2p_decrypt( bencode, bensize, from );
	} else {
		p2p_decode( bencode, bensize, from );
	}
#else
	p2p_decode( bencode, bensize, from );
#endif
}

#ifdef POLARSSL
void p2p_decrypt( UCHAR *bencode, size_t bensize, IP *from ) {
	BEN *packet = NULL;
	BEN *salt = NULL;
	BEN *aes = NULL;
	struct obj_str *plain = NULL;

	/* Parse request */
	packet = ben_dec( bencode, bensize );
	if( !ben_is_dict( packet ) ) {
		info( from, 0, "Decoding AES packet failed:" );
		ben_free( packet );
		return;
	}

	/* Salt */
	salt = ben_dict_search_str( packet, "s" );
	if( !ben_is_str( salt ) || ben_str_i( salt ) != AES_IV_SIZE ) {
		info( from, 0, "Salt missing or broken:" );
		ben_free( packet );
		return;
	}

	/* Encrypted AES message */
	aes = ben_dict_search_str( packet, "a" );
	if( !ben_is_str( aes ) || ben_str_i( aes ) <= 2 ) {
		info( from, 0, "AES message missing or broken:" );
		ben_free( packet );
		return;
	}

	/* Decrypt message */
	plain = aes_decrypt( ben_str_s( aes ), ben_str_i( aes ),
		ben_str_s( salt ),
		_main->conf->key, strlen( _main->conf->key ) );
	if( plain == NULL ) {
		info( from, 0, "Decoding AES message failed:" );
		ben_free( packet );
		return;
	}

	/* AES packet too small */
	if( plain->i < SHA1_SIZE ) {
		ben_free( packet );
		str_free( plain );
		info( from, 0, "AES packet contains less than 20 bytes:" );
		return;
	}

	/* Validate bencode */
	if( !ben_validate( plain->s, plain->i) ) {
		ben_free( packet );
		str_free( plain );
		info( from, 0, "AES packet contains broken bencode:" );
		return;
	}

	/* Parse message */
	p2p_decode( plain->s, plain->i, from );

	/* Free */
	ben_free( packet );
	str_free( plain );
}
#endif

void p2p_decode( UCHAR *bencode, size_t bensize, IP *from ) {
	BEN *packet = NULL;
	BEN *y = NULL;

	/* Parse request */
	packet = ben_dec( bencode, bensize );
	if( packet == NULL ) {
		info( from, 0, "Decoding UDP packet failed:" );
		return;
	} else if( packet->t != BEN_DICT ) {
		info( from, 0, "UDP packet is not a dictionary:" );
		ben_free( packet );
		return;
	}

	/* Type of message */
	y = ben_dict_search_str( packet, "y" );
	if( !ben_is_str( y ) || ben_str_i( y ) != 1 ) {
		info( from, 0, "Message type missing or broken:" );
		ben_free( packet );
		return;
	}

	mutex_block( _main->work->mutex );

	switch( *y->v.s->s ) {

		case 'q':
			p2p_request( packet, from );
			break;
		case 'r':
			p2p_reply( packet, from );
			break;
		default:
			info( from, 0, "Unknown message type:" );
	}

	mutex_unblock( _main->work->mutex );

	ben_free( packet );
}


void p2p_request( BEN *packet, IP *from ) {
	BEN *q = NULL;
	BEN *a = NULL;
	BEN *t = NULL;
	BEN *id = NULL;

	/* Query Type */
	q = ben_dict_search_str( packet, "q" );
	if( !ben_is_str( q ) ) {
		info( from, 0, "Query type missing or broken:" );
		return;
	}

	/* Argument */
	a = ben_dict_search_str( packet, "a" );
	if( !ben_is_dict( a ) ) {
		info( from, 0, "Argument missing or broken:" );
		return;
	}

	/* Node ID */
	id = ben_dict_search_str( a, "id" );
	if( !p2p_is_hash( id ) ) {
		info( from, 0, "Node ID missing or broken:" );
		return;
	}

	/* Do not talk to myself */
	if( p2p_packet_from_myself( ben_str_s( id )  ) ) {
		return;
	}

	/* Transaction ID */
	t = ben_dict_search_str( packet, "t" );
	if( !ben_is_str( t ) ) {
		info( from, 0, "Transaction ID missing or broken:" );
		return;
	}
	if( ben_str_i( t ) > TID_SIZE_MAX ) {
		info( from, 0, "Transaction ID too big:" );
		return;
	}

	/* Remember node. This does not update the IP address. */
	nbhd_put( ben_str_s( id ), from );

	/* PING */
	if( ben_str_i( q ) == 4 && memcmp( ben_str_s( q ), "ping", 4 ) == 0 ) {
		p2p_ping( t, from );
		return;
	}

	/* FIND_NODE */
	if( ben_str_i( q ) == 9 && memcmp( ben_str_s( q ), "find_node", 9 ) == 0 ) {
		p2p_find_node_get_request( a, t, from );
		return;
	}

	/* GET_PEERS */
	if( ben_str_i( q ) == 9 && memcmp( ben_str_s( q ), "get_peers", 9 ) == 0 ) {
		p2p_get_peers_get_request( a, t, from );
		return;
	}

	/* ANNOUNCE */
	if( ben_str_i( q ) == 13 && memcmp( ben_str_s( q ), "announce_peer", 13 ) == 0 ) {
		p2p_announce_get_request( a, ben_str_s( id ), t, from );
		return;
	}

	/* LOOKUP */
	if( ben_str_i( q ) == 6 && memcmp( ben_str_s( q ), "lookup", 6 ) == 0 ) {
		p2p_localhost_get_request( a, t, from );
		return;
	}

	/* VOTE (utorrent?) */
	if( ben_str_i( q ) == 4 && memcmp( ben_str_s( q ), "vote", 4 ) == 0 ) {
		info( from, 0, "Drop RPC VOTE message from" );
		return;
	}

	info( from, 0, "Invalid query type from" );
}

void p2p_reply( BEN *packet, IP *from ) {
	BEN *r = NULL;
	BEN *t = NULL;
	BEN *id = NULL;
	ITEM *ti = NULL;

	/* Argument */
	r = ben_dict_search_str( packet, "r" );
	if( !ben_is_dict( r ) ) {
		info( from, 0, "Argument missing or broken:" );
		return;
	}

	/* Node ID */
	id = ben_dict_search_str( r, "id" );
	if( !p2p_is_hash( id ) ) {
		info( from, 0, "Node ID missing or broken:" );
		return;
	}

	/* Do not talk to myself */
	if( p2p_packet_from_myself( ben_str_s( id ) ) ) {
		return;
	}

	/* Transaction ID */
	t = ben_dict_search_str( packet, "t" );
	if( !ben_is_str( t ) && ben_str_i( t ) != TID_SIZE ) {
		info( from, 0, "Transaction ID missing or broken:" );
		return;
	}

	/* Remember node. */
	nbhd_put( ben_str_s( id ), (IP *)from );

	ti = tdb_item( ben_str_s( t ) );

	/* Get Query type by looking at the TDB */
	switch( tdb_type( ti ) ) {
		case P2P_PING:
		case P2P_PING_MULTICAST:
			p2p_pong( ben_str_s( id ), from );
			break;
		case P2P_FIND_NODE:
			p2p_find_node_get_reply( r, ben_str_s( id ), from );
			break;
		case P2P_GET_PEERS:
		case P2P_ANNOUNCE_START:
			p2p_get_peers_get_reply( r, ben_str_s( id ), ti, from );
			break;
		case P2P_ANNOUNCE_ENGAGE:
			p2p_announce_get_reply( r, ben_str_s( id ), ti, from );
			break;
		default:
			info( from, 0, "Invalid Transaction ID from" );
			return;
	}

	/* Cleanup. The TID gets reused by GET_PEERS and ANNOUNCE_PEER requests.
	 * The TID is also persistant for multicast requests since multiple
	 * answers are likely possible. */
	switch( tdb_type( ti ) ) {
		case P2P_PING:
		case P2P_FIND_NODE:
		case P2P_ANNOUNCE_ENGAGE:
			tdb_del( ti );
			break;
	}
}

int p2p_packet_from_myself( UCHAR *node_id ) {
	if( node_me( node_id ) ) {

		/* Received packet from myself:
		 * You may see multicast requests from yourself
		 * if the neighbourhood is empty.
		 * Do not warn about them.
		 */
		if( !nbhd_is_empty() ) {
			info( NULL, 0, "WARNING: Received a packet from myself..." );
		}

		return TRUE;
	}

	return FALSE;
}

void p2p_ping( BEN *tid, IP *from ) {
	send_pong( from, ben_str_s( tid ), ben_str_i( tid ) );
}

void p2p_pong( UCHAR *node_id, IP *from ) {
	nbhd_ponged( node_id, from );
}

void p2p_find_node_get_request( BEN *arg, BEN *tid, IP *from ) {
	BEN *target = NULL;
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	int nodes_compact_size = 0;

	/* Target */
	target = ben_dict_search_str( arg, "target" );
	if( !p2p_is_hash( target ) ) {
		info( NULL, 0, "Missing or broken target" );
		return;
	}

	/* Create compact node list */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket,
			nodes_compact_list, ben_str_s( target ) );

	/* Send reply */
	if( nodes_compact_size > 0 ) {
		send_find_node_reply( from, nodes_compact_list, nodes_compact_size,
			ben_str_s( tid ), ben_str_i( tid ) );
	}
}

void p2p_find_node_get_reply( BEN *arg, UCHAR *node_id, IP *from ) {
	BEN *nodes = NULL;
	UCHAR *id = NULL;
	UCHAR *p = NULL;
	long int i = 0;
	IP sin;

#ifdef IPV6
	nodes = ben_dict_search_str( arg, "nodes6" );
#elif IPV4
	nodes = ben_dict_search_str( arg, "nodes" );
#endif
	if( !ben_is_str( nodes ) ) {
		info( NULL, 0, "nodes key missing" );
		return;
	}

	if( ben_str_i( nodes ) % IP_SIZE_META_TRIPLE != 0 ) {
		info( NULL, 0, "nodes key broken" );
		return;
	}

	p = ben_str_s( nodes );
	for( i=0; i< ben_str_i( nodes ); i+=IP_SIZE_META_TRIPLE ) {

		/* ID */
		id = p;	p += SHA1_SIZE;

		/* IP + Port */
		p = ip_bytes_to_sin( &sin, p );

		/* Ignore myself */
		if( node_me( id ) ) {
			continue;
		}

		/* Ignore link-local address */
		if( ip_is_linklocal( &sin ) ) {
			continue;
		}

		/* Store node */
		nbhd_put( id, &sin );
	}
}

/*
	{
	"t": "aa",
	"y": "q",
	"q": "get_peers",
	"a": {
		"id": "abcdefghij0123456789",
		"info_hash": "mnopqrstuvwxyz123456"
		}
	}
*/

void p2p_get_peers_get_request( BEN *arg, BEN *tid, IP *from ) {
	BEN *info_hash = NULL;
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	int nodes_compact_size = 0;

	/* info_hash */
	info_hash = ben_dict_search_str( arg, "info_hash" );
	if( !p2p_is_hash( info_hash ) ) {
		info( NULL, 0, "Missing or broken info_hash" );
		return;
	}

	/* Look at the database */
	nodes_compact_size = val_compact_list( nodes_compact_list,
		ben_str_s( info_hash ) );

	/* Send values */
	if( nodes_compact_size > 0 ) {
		send_get_peers_values( from, nodes_compact_list, nodes_compact_size,
			ben_str_s( tid ), ben_str_i( tid ) );
		return;
	}

	/* Look at the routing table */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket,
		nodes_compact_list, ben_str_s( info_hash ) );

	/* Send nodes */
	if( nodes_compact_size > 0 ) {
		send_get_peers_nodes( from, nodes_compact_list, nodes_compact_size,
			ben_str_s( tid ), ben_str_i( tid ) );
	}
}

void p2p_get_peers_get_reply( BEN *arg, UCHAR *node_id, ITEM *ti, IP *from ) {
	BEN *token = NULL;
	BEN *nodes = NULL;
	BEN *values = NULL;

	token = ben_dict_search_str( arg, "token" );
	if( !ben_is_str( token ) ) {
		info( from, 0, "Missing or broken token from" );
		return;
	} else if( ben_str_i( token ) > TOKEN_SIZE_MAX ) {
		info( from, 0, "Token key too big from" );
		return;
	} else if( ben_str_i( token ) <= 0 ) {
		info( from, 0, "Invalid token from" );
		return;
	}

	values = ben_dict_search_str( arg, "values" );
#ifdef IPV6
	nodes = ben_dict_search_str( arg, "nodes6" );
#elif IPV4
	nodes = ben_dict_search_str( arg, "nodes" );
#endif

	if( values != NULL ) {
		if( !ben_is_list( values ) ) {
			info( NULL, 0, "values key missing or broken" );
			return;
		} else {
			p2p_get_peers_get_values( values, node_id, ti, token, from );
			return;
		}
	}

	if( nodes != NULL ) {
		if( !ben_is_str( nodes ) || ben_str_i( nodes ) % IP_SIZE_META_TRIPLE != 0 ) {
			info( NULL, 0, "nodes key missing or broken" );
			return;
		} else {
			p2p_get_peers_get_nodes( nodes, node_id, ti, token, from );
			return;
		}
	}
}

/*
	{
	"t": "aa",
	"y": "r",
	"r": {
		"id":"abcdefghij0123456789",
		"token":"aoeusnth",
		"nodes": "def456..."
		}
	}
*/

void p2p_get_peers_get_nodes( BEN *nodes, UCHAR *node_id, ITEM *ti,
	BEN *token, IP *from ) {

	LOOKUP *l = tdb_ldb( ti );
	UCHAR *target = NULL;
	UCHAR *id = NULL;
	UCHAR *p = NULL;
	long int i = 0;
	IP sin;

	if( l == NULL ) {
		return;
	} else {
		target = l->target;
	}

	ldb_update( l, node_id, token, from );

	p = ben_str_s( nodes );
	for( i=0; i< ben_str_i( nodes ); i+=IP_SIZE_META_TRIPLE ) {

		/* ID */
		id = p;	p += SHA1_SIZE;

		/* IP + Port */
		p = ip_bytes_to_sin( &sin, p );

		/* Ignore myself */
		if( node_me( id ) ) {
			continue;
		}

		/* Ignore link-local address */
		if( ip_is_linklocal( &sin ) ) {
			continue;
		}

		nbhd_put( id, &sin );

		/* Node known. Do not send requests twice. Stop here. */
		if( ldb_find( l, id ) != NULL ) {
			continue;
		}

		/* Add this node to a sorted list. And only send a new lookup request
		 * to this node if it gets inserted on top of the sorted list. */
		if( ldb_put( l, id, (IP *)&sin ) >= 8 ) {
			continue;
		}

		/* Send a new lookup request. */
		send_get_peers_request( (IP *)&sin, target, tdb_tid( ti ) );
	}
}

/*
	{
	"t": "aa",
	"y": "r",
	"r": {
		"id":"abcdefghij0123456789",
		"token":"aoeusnth",
		"values": ["axje.u", "idhtnm"]
		}
	}
*/

void p2p_get_peers_get_values( BEN *values, UCHAR *node_id, ITEM *ti,
	BEN *token, IP *from ) {

	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	UCHAR *p = nodes_compact_list;
	LOOKUP *l = tdb_ldb( ti );
	int nodes_compact_size = 0;
	BEN *val = NULL;
	ITEM *item = NULL;
	long int j = 0;
	int type = tdb_type( ti );
	char hex[HEX_LEN];

	if( l == NULL ) {
		return;
	}

	ldb_update( l, node_id, token, from );

	/* Get values */
	item = list_start( values->v.l );
	while( item != NULL && j < 8 ) {
		val = list_value( item );

		if( !ben_is_str( val ) || ben_str_i( val ) != IP_SIZE_META_PAIR ) {
			info( NULL, 0, "Values list broken" );
			return;
		}

		memcpy( p, ben_str_s( val ), ben_str_i( val ) );
		nodes_compact_size += IP_SIZE_META_PAIR;
		p += IP_SIZE_META_PAIR;

		item = list_next(item);
		j++;
	}

	/* Lookup request? */
	if( type == P2P_GET_PEERS && nodes_compact_size > 0 ) {

		/* Cache result */
		cache_put( l->target, nodes_compact_list, nodes_compact_size );

		/* Send reply to tknss or tkcli */
		if( l->send_reply == TRUE ) {
			send_get_peers_values( &l->c_addr,
				nodes_compact_list, nodes_compact_size,
				l->tid, l->tid_size );
		}

		/* Debugging */
		hex_hash_encode( hex, l->target );
		info( from, 0, "Found %s at", hex );
	}
}

/*
{
"t": "aa",
"y": "q",
"q": "announce_peer",
"a": {
	"id": "abcdefghij0123456789",
	"info_hash": "mnopqrstuvwxyz123456",
	"port": 6881,
	"token": "aoeusnth"
	}
}
*/

void p2p_announce_get_request( BEN *arg, UCHAR *node_id, BEN *tid, IP *from ) {
	BEN *info_hash = NULL;
	BEN *token = NULL;
	BEN *port = NULL;

	/* info_hash */
	info_hash = ben_dict_search_str( arg, "info_hash" );
	if( !p2p_is_hash( info_hash ) ) {
		info( from, 0, "Missing or broken info_hash from" );
		return;
	}

	/* Token */
	token = ben_dict_search_str( arg, "token" );
	if( !ben_is_str( token ) || ben_str_i( token ) > TOKEN_SIZE_MAX ) {
		info( from, 0, "Missing or broken token from" );
		return;
	}

	if( !tkn_validate( ben_str_s( token ) ) ) {
		info( from, 0, "Invalid token from" );
		return;
	}

	/* Port */
	port = ben_dict_search_str( arg, "port" );
	if( !ben_is_int( port ) ) {
		info( from, 0, "Missing or broken port from" );
		return;
	}

	if( port->v.i < 1 || port->v.i > 65535 ) {
		info( from, 0, "Invalid port number from" );
		return;
	}

	/* Store info_hash */
	val_put( ben_str_s( info_hash ), node_id, port->v.i, from );

	/* Send success message */
	send_announce_reply( from, ben_str_s( tid ), ben_str_i( tid ) );
}

/*
	{
	"t": "aa",
	"y": "r",
	"r": {
		"id": "mnopqrstuvwxyz123456"
		}
	}
*/

void p2p_announce_get_reply( BEN *arg, UCHAR *node_id,
	ITEM *ti, IP *from ) {
	/* Nothing to do */
}

/*
	{
	"t": "aa",
	"y": "q",
	"q": "lookup",
	"a": {
		"id": "mnopqrstuvwxyz123456"
		"hostname": "owncloud.p2p",
		}
	}
*/

void p2p_localhost_get_request( BEN *arg, BEN *tid, IP *from ) {
	UCHAR target[SHA1_SIZE];
	int result = FALSE;
	char *hostname = NULL;
	int hostsize = 0;
	BEN *entity = NULL;

	/* Limit lookups to localhost */
	if( !ip_is_localhost( from ) ) {
		return;
	}

	/* Get hostname */
	entity = ben_dict_search_str( arg, "hostname" );
	if( !ben_is_str( entity ) || ben_str_i( entity ) <= 0 ) {
		info( from, 0, "Missing or broken hostname from" );
		return;
	}
	if( ben_str_i( entity ) > 256 ) {
		info( from, 0, "Name too long from" );
		return;
	}

	hostname = (char *) ben_str_s( entity );
	hostsize = ben_str_i( entity );

	/* Validate hostname */
	if ( !str_valid_hostname( hostname, hostsize ) ) {
		info( NULL, 0, "LOOKUP %s (Invalid hostname)", hostname );
		return;
	}

	/* Compute lookup key */
	conf_hostid( target, hostname,
		_main->conf->realm, _main->conf->bool_realm );

	/* Check local cache */
	result = p2p_localhost_lookup_cache( target, tid, from );
	if( result == TRUE ) {
		info( NULL, 0, "LOOKUP %s (cached)", hostname );
		return;
	}

	/* Check local database */
	result = p2p_localhost_lookup_local( target, tid, from );
	if( result == TRUE ) {
		info( NULL, 0, "LOOKUP %s (local)", hostname );
		return;
	}

	/* Start remote search */
	result = p2p_localhost_lookup_remote( target, tid, from );
	if( result == TRUE ) {
		info( NULL, 0, "LOOKUP %s (remote)", hostname );
		return;
	}
}

int p2p_localhost_lookup_cache( UCHAR *target, BEN *tid, IP *from ) {
	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	int nodes_compact_size = 0;

	/* Check cache for hostname */
	nodes_compact_size = cache_compact_list( nodes_compact_list, target );
	if( nodes_compact_size <= 0 ) {
		return FALSE;
	}

	send_get_peers_values( from, nodes_compact_list, nodes_compact_size,
		ben_str_s( tid ), ben_str_i( tid ) );

	return TRUE;
}

/* Use local info_hash database for lookups too. This is nessecary if only 2
 * nodes are active: Node A announces its name to node B. But Node B cannot
 * talk to itself to lookup A. So, it must use its database directly. */
int p2p_localhost_lookup_local( UCHAR *target, BEN *tid, IP *from ) {
	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	int nodes_compact_size = 0;

	/* Check cache for hostname */
	nodes_compact_size = val_compact_list( nodes_compact_list, target );
	if( nodes_compact_size <= 0 ) {
		return FALSE;
	}

	send_get_peers_values( from, nodes_compact_list, nodes_compact_size,
		ben_str_s( tid ), ben_str_i( tid ) );

	return TRUE;
}

int p2p_localhost_lookup_remote( UCHAR *target, BEN *tid, IP *from ) {
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	int nodes_compact_size = 0;
	IP sin;
	UCHAR *p = NULL;
	UCHAR *id = NULL;
	int j = 0;
	ITEM *ti = NULL;
	LOOKUP *l = NULL;

	/* Start the incremental remote search program */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket,
		nodes_compact_list, target );

	/* Create tid and get the lookup table */
	ti = tdb_put( P2P_GET_PEERS );
	l = ldb_init( target, from, tid );
	tdb_link_ldb( ti, l );

	p = nodes_compact_list;
	for( j=0; j<nodes_compact_size; j+=IP_SIZE_META_TRIPLE ) {

		/* ID */
		id = p;	p += SHA1_SIZE;

		/* IP + Port */
		p = ip_bytes_to_sin( &sin, p );

		/* Remember queried node */
		ldb_put( l, id, &sin );

		/* Query node */
		send_get_peers_request( &sin, target, tdb_tid( ti ) );
	}

	return TRUE;
}

int p2p_is_hash( BEN *node ) {
	if( node == NULL ) {
		return 0;
	}
	if( !ben_is_str( node ) ) {
		return 0;
	}
	if( ben_str_i( node ) != SHA1_SIZE ) {
		return 0;
	}
	return 1;
}

int p2p_is_ip( BEN *node ) {
	if( node == NULL ) {
		return 0;
	}
	if( !ben_is_str( node ) ) {
		return 0;
	}
	if( ben_str_i( node ) != IP_SIZE ) {
		return 0;
	}
	return 1;
}

int p2p_is_port( BEN *node ) {
	if( node == NULL ) {
		return 0;
	}
	if( !ben_is_str( node ) ) {
		return 0;
	}
	if( ben_str_i( node ) != 2 ) {
		return 0;
	}
	return 1;
}
