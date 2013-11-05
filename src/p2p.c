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

struct obj_p2p *p2p_init( void ) {
	struct obj_p2p *p2p = (struct obj_p2p *) myalloc( sizeof(struct obj_p2p) );

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

	/* Compute address of bootstrap node */
	memset( &hints, '\0', sizeof(struct addrinfo) );
	hints.ai_socktype = SOCK_STREAM;

#ifdef IPV6
	hints.ai_family = AF_INET6;
#elif IPV4
	hints.ai_family = AF_INET;
#endif
	rc = getaddrinfo( _main->conf->bootstrap_node, _main->conf->bootstrap_port,
		&hints, &addrinfo );
	if( rc != 0 ) {
		info( NULL, 0, "getaddrinfo: %s", gai_strerror( rc ) );
		return;
	}

	p = addrinfo;
	while( p != NULL && i < P2P_MAX_BOOTSTRAP_NODES ) {

		/* Send PING to a bootstrap node */
		if( strcmp( _main->conf->bootstrap_node, CONF_MULTICAST ) == 0 ) {
			ti = tdb_put(P2P_PING_MULTICAST, NULL, NULL );
			send_ping( (IP *)p->ai_addr, tdb_tid( ti ) );
		} else {
			ti = tdb_put(P2P_PING, NULL, NULL );
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
					ti = tdb_put(P2P_PING, NULL, NULL);
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

			ti = tdb_put(P2P_FIND_NODE, NULL, NULL);
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
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket, nodes_compact_list, target );

	/* Create tid and get the lookup table */
	ti = tdb_put( P2P_ANNOUNCE_START, target, NULL );
	l = tdb_ldb( ti );

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
			t_new = tdb_put(P2P_ANNOUNCE_ENGAGE, NULL, NULL);
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
		info( from, 0, "UDP packet too small" );
		return;
	}

	/* Recursive lookup request from localhost ::1 */
	if( ip_is_localhost( from ) ) {
		p2p_localhost_get_request( bencode, bensize, from );
		return;
	}

	/* Ignore link-local address */
	if( ip_is_linklocal( from ) ) {
		info( from, 0, "DROP LINK-LOCAL" );
		return;
	}

	/* Validate bencode */
	if( !ben_validate( bencode, bensize ) ) {
		info( from, 0, "UDP packet contains broken bencode" );
		return;
	}

	/* Encrypted message or plaintext message */
#ifdef POLARSSL
	if( _main->conf->bool_encryption ) {
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
		info( NULL, 0, "Decoding AES packet failed" );
		ben_free( packet );
		return;
	}

	/* Salt */
	salt = ben_searchDictStr( packet, "s" );
	if( !ben_is_str( salt ) || ben_str_size( salt ) != AES_IV_SIZE ) {
		info( NULL, 0, "Salt missing or broken" );
		ben_free( packet );
		return;
	}

	/* Encrypted AES message */
	aes = ben_searchDictStr( packet, "a" );
	if( !ben_is_str( aes ) || ben_str_size( aes ) <= 2 ) {
		info( NULL, 0, "AES message missing or broken" );
		ben_free( packet );
		return;
	}

	/* Decrypt message */
	plain = aes_decrypt( aes->v.s->s, aes->v.s->i,
		salt->v.s->s,
		_main->conf->key, strlen( _main->conf->key ) );
	if( plain == NULL ) {
		info( NULL, 0, "Decoding AES message failed" );
		ben_free( packet );
		return;
	}

	/* AES packet too small */
	if( plain->i < SHA1_SIZE ) {
		ben_free( packet );
		str_free( plain );
		info( NULL, 0, "AES packet contains less than 20 bytes" );
		return;
	}

	/* Validate bencode */
	if( !ben_validate( plain->s, plain->i) ) {
		ben_free( packet );
		str_free( plain );
		info( NULL, 0, "AES packet contains broken bencode" );
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
		info( NULL, 0, "Decoding UDP packet failed" );
		return;
	} else if( packet->t != BEN_DICT ) {
		info( NULL, 0, "UDP packet is not a dictionary" );
		ben_free( packet );
		return;
	}

	/* Type of message */
	y = ben_searchDictStr( packet, "y" );
	if( !ben_is_str( y ) || ben_str_size( y ) != 1 ) {
		info( NULL, 0, "Message type missing or broken" );
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
			info( NULL, 0, "Unknown message type" );
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
	q = ben_searchDictStr( packet, "q" );
	if( !ben_is_str( q ) ) {
		info( NULL, 0, "Query type missing or broken" );
		return;
	}

	/* Argument */
	a = ben_searchDictStr( packet, "a" );
	if( !ben_is_dict( a ) ) {
		info( NULL, 0, "Argument missing or broken" );
		return;
	}

	/* Node ID */
	id = ben_searchDictStr( a, "id" );
	if( !p2p_is_hash( id ) ) {
		info( NULL, 0, "Node ID missing or broken" );
		return;
	}

	/* Do not talk to myself */
	if( p2p_packet_from_myself( id->v.s->s ) ) {
		return;
	}

	/* Transaction ID */
	t = ben_searchDictStr( packet, "t" );
	if( !ben_is_str( t ) ) {
		info( NULL, 0, "Transaction ID missing or broken" );
		return;
	}
	if( ben_str_size( t ) > TID_SIZE_MAX ) {
		info( NULL, 0, "Transaction ID too big" );
		return;
	}

	/* Remember node. This does not update the IP address. */
	nbhd_put( id->v.s->s, (IP *)from );

	/* PING */
	if( ben_str_size( q ) == 4 && memcmp( q->v.s->s, "ping", 4 ) == 0 ) {
		p2p_ping( t, from );
		return;
	}

	/* FIND_NODE */
	if( ben_str_size( q ) == 9 && memcmp( q->v.s->s, "find_node", 9 ) == 0 ) {
		p2p_find_node_get_request( a, t, from );
		return;
	}

	/* GET_PEERS */
	if( ben_str_size( q ) == 9 && memcmp( q->v.s->s, "get_peers", 9 ) == 0 ) {
		p2p_get_peers_get_request( a, t, from );
		return;
	}

	/* ANNOUNCE */
	if( ben_str_size( q ) == 13 && memcmp( q->v.s->s, "announce_peer", 13 ) == 0 ) {
		p2p_announce_get_request( a, id->v.s->s, t, from );
		return;
	}
	
	info( NULL, 0, "Unknown query type" );
}

void p2p_reply( BEN *packet, IP *from ) {
	BEN *r = NULL;
	BEN *t = NULL;
	BEN *id = NULL;
	ITEM *ti = NULL;

	/* Argument */
	r = ben_searchDictStr( packet, "r" );
	if( !ben_is_dict( r ) ) {
		info( NULL, 0, "Argument missing or broken" );
		return;
	}

	/* Node ID */
	id = ben_searchDictStr( r, "id" );
	if( !p2p_is_hash( id ) ) {
		info( NULL, 0, "Node ID missing or broken" );
		return;
	}
	
	/* Do not talk to myself */
	if( p2p_packet_from_myself( id->v.s->s ) ) {
		return;
	}

	/* Transaction ID */
	t = ben_searchDictStr( packet, "t" );
	if( !ben_is_str( t ) && ben_str_size( t ) != TID_SIZE ) {
		info( NULL, 0, "Transaction ID missing or broken" );
		return;
	}

	/* Remember node. */
	nbhd_put( id->v.s->s, (IP *)from );

	ti = tdb_item( t->v.s->s );

	/* Get Query type by looking at the TDB */
	switch( tdb_type( ti ) ) {
		case P2P_PING:
		case P2P_PING_MULTICAST:
			p2p_pong( id->v.s->s, from );
			break;
		case P2P_FIND_NODE:
			p2p_find_node_get_reply( r, id->v.s->s, from );
			break;
		case P2P_GET_PEERS:
		case P2P_ANNOUNCE_START:
			p2p_get_peers_get_reply( r, id->v.s->s, ti, from );
			break;
		case P2P_ANNOUNCE_ENGAGE:
			p2p_announce_get_reply( r, id->v.s->s, ti, from );
			break;
		default:
			info( NULL, 0, "Unknown Transaction ID..." );
			return;
	}

	/* Cleanup. The TID gets reused by GET_PEERS and ANNOUNCE_PEER requests. The
	 * TID is also persistant for multicast requests since multiple answers are
	 * likely possible. */
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
	send_pong( from, tid->v.s->s, tid->v.s->i );
}

void p2p_pong( UCHAR *node_id, IP *from ) {
	nbhd_ponged( node_id, from );
}

void p2p_find_node_get_request( BEN *arg, BEN *tid, IP *from ) {
	BEN *target = NULL;
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	int nodes_compact_size = 0;

	/* Target */
	target = ben_searchDictStr( arg, "target" );
	if( !p2p_is_hash( target ) ) {
		info( NULL, 0, "Missing or broken target" );
		return;
	}

	/* Create compact node list */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket, nodes_compact_list, target->v.s->s );

	/* Send reply */
	if( nodes_compact_size > 0 ) {
		send_find_node_reply( from, nodes_compact_list, nodes_compact_size,
			tid->v.s->s, tid->v.s->i );
	}
}

void p2p_find_node_get_reply( BEN *arg, UCHAR *node_id, IP *from ) {
	BEN *nodes = NULL;
	UCHAR *id = NULL;
	UCHAR *p = NULL;
	long int i = 0;
	IP sin;

#ifdef IPV6
	nodes = ben_searchDictStr( arg, "nodes6" );
#elif IPV4
	nodes = ben_searchDictStr( arg, "nodes" );
#endif
	if( !ben_is_str( nodes ) ) {
		info( NULL, 0, "nodes key missing" );
		return;
	}

	if( ben_str_size( nodes ) % IP_SIZE_META_TRIPLE != 0 ) {
		info( NULL, 0, "nodes key broken" );
		return;
	}

	p = nodes->v.s->s;
	for( i=0; i<nodes->v.s->i; i+=IP_SIZE_META_TRIPLE ) {

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
	info_hash = ben_searchDictStr( arg, "info_hash" );
	if( !p2p_is_hash( info_hash ) ) {
		info( NULL, 0, "Missing or broken info_hash" );
		return;
	}

	/* Look at the database */
	nodes_compact_size = val_compact_list( nodes_compact_list, info_hash->v.s->s );

	/* Send values */
	if( nodes_compact_size > 0 ) {
		send_get_peers_values( from, nodes_compact_list, nodes_compact_size,
			tid->v.s->s, tid->v.s->i );
		return;
	}

	/* Look at the routing table */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket, nodes_compact_list,
		info_hash->v.s->s );

	/* Send nodes */
	if( nodes_compact_size > 0 ) {
		send_get_peers_nodes( from, nodes_compact_list, nodes_compact_size,
			tid->v.s->s, tid->v.s->i );
	}
}

void p2p_get_peers_get_reply( BEN *arg, UCHAR *node_id, ITEM *ti, IP *from ) {
	BEN *token = NULL;
	BEN *nodes = NULL;
	BEN *values = NULL;

	token = ben_searchDictStr( arg, "token" );
	if( !ben_is_str( token ) ) {
		info( NULL, 0, "token key missing or broken" );
		return;
	} else if( ben_str_size( token ) > TOKEN_SIZE_MAX ) {
		info( NULL, 0, "Token key too big" );
		return;
	} else if( ben_str_size( token ) <= 0 ) {
		info( NULL, 0, "Token key too small" );
		return;
	}

	values = ben_searchDictStr( arg, "values" );
#ifdef IPV6
	nodes = ben_searchDictStr( arg, "nodes6" );
#elif IPV4
	nodes = ben_searchDictStr( arg, "nodes" );
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
		if( !ben_is_str( nodes ) || ben_str_size( nodes ) % IP_SIZE_META_TRIPLE != 0 ) {
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

void p2p_get_peers_get_nodes( BEN *nodes, UCHAR *node_id, ITEM *ti, BEN *token, IP *from ) {
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

	p = nodes->v.s->s;
	for( i=0; i<nodes->v.s->i; i+=IP_SIZE_META_TRIPLE ) {

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

		/* Do not send requests twice */
		if( ldb_find( l, id ) != NULL ) {
			continue;
		}

		/* Only send a request to this node if it is one of the top matching
		 * nodes. */
		if( ldb_put( l, id, (IP *)&sin ) >= 8 ) {
			continue;
		}
		
		/* Query node */
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

void p2p_get_peers_get_values( BEN *values, UCHAR *node_id, ITEM *ti, BEN *token, IP *from ) {
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

		if( !ben_is_str( val ) || ben_str_size( val ) != IP_SIZE_META_PAIR ) {
			info( NULL, 0, "Values list broken" );
			return;
		}

		memcpy( p, val->v.s->s, val->v.s->i );
		nodes_compact_size += IP_SIZE_META_PAIR;
		p += IP_SIZE_META_PAIR;

		item = list_next(item);
		j++;
	}

	/* Lookup request? */
	if( type == P2P_GET_PEERS && nodes_compact_size > 0 ) {

		/* Cache result */
		cache_put( l->target, nodes_compact_list, nodes_compact_size );

		/* Send reply */
		if( l->send_reply == TRUE ) {
			sendto( _main->udp->sockfd, nodes_compact_list, nodes_compact_size, 0,
				(const struct sockaddr *)&l->c_addr, sizeof( IP ) );
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
	info_hash = ben_searchDictStr( arg, "info_hash" );
	if( !p2p_is_hash( info_hash ) ) {
		info( NULL, 0, "Missing or broken info_hash" );
		return;
	}

	/* Token */
	token = ben_searchDictStr( arg, "token" );
	if( !ben_is_str( token ) || ben_str_size( token ) > TOKEN_SIZE_MAX ) {
		info( NULL, 0, "token key missing or broken" );
		return;
	}

	if( !tkn_validate( token->v.s->s ) ) {
		info( NULL, 0, "Invalid token" );
		return;
	}

	/* Port */
	port = ben_searchDictStr( arg, "port" );
	if( !ben_is_int( port ) ) {
		info( NULL, 0, "Missing or broken port" );
		return;
	}

	if( port->v.i < 1 || port->v.i > 65535 ) {
		info( NULL, 0, "Invalid port number" );
		return;
	}

	/* Store info_hash */
	val_put( info_hash->v.s->s, node_id, port->v.i, from );

	/* Send success message */
	send_announce_reply( from, tid->v.s->s, tid->v.s->i );
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

void p2p_localhost_get_request( UCHAR *hostname, size_t size, IP *from ) {
	UCHAR target[SHA1_SIZE];
	int result = FALSE;

	/* Validate hostname */
	if ( !str_isValidHostname( (char *)hostname, size ) ) {
		info( NULL, 0, "LOOKUP %s (Invalid hostname)", hostname );
		return;
	}

	/* Compute lookup key */
	conf_hostid( target, (char *)hostname,
		_main->conf->realm, _main->conf->bool_realm );

	/* Check local cache */
	mutex_block( _main->work->mutex );
	result = p2p_localhost_lookup_cache( target, from );
	mutex_unblock( _main->work->mutex );

	if( result == TRUE ) {
		info( NULL, 0, "LOOKUP %s (cached)", hostname );
		return;
	}

	/* Check local database */
	mutex_block( _main->work->mutex );
	result = p2p_localhost_lookup_local( target, from );
	mutex_unblock( _main->work->mutex );

	if( result == TRUE ) {
		info( NULL, 0, "LOOKUP %s (local)", hostname );
		return;
	}

	/* Start remote search */
	mutex_block( _main->work->mutex );
	result = p2p_localhost_lookup_remote( target, from );
	mutex_unblock( _main->work->mutex );

	if( result == TRUE ) {
		info( NULL, 0, "LOOKUP %s (remote)", hostname );
		return;
	}
}

int p2p_localhost_lookup_cache( UCHAR *target, IP *from ) {
	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	int nodes_compact_size = 0;

	/* Check cache for hostname */
	nodes_compact_size = cache_compact_list( nodes_compact_list, target );
	if( nodes_compact_size <= 0 ) {
		return FALSE;
	}

	sendto( _main->udp->sockfd, nodes_compact_list, nodes_compact_size, 0,
		(const struct sockaddr *)from, sizeof( IP ) );

	return TRUE;
}

/* Use local info_hash database for lookups too. This is nessecary if only 2
 * nodes are active: Node A announces its name to node B. But Node B cannot talk
 * to itself to lookup A. So, it must use its database directly. */
int p2p_localhost_lookup_local( UCHAR *target, IP *from ) {
	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	int nodes_compact_size = 0;

	/* Check cache for hostname */
	nodes_compact_size = val_compact_list( nodes_compact_list, target );
	if( nodes_compact_size <= 0 ) {
		return FALSE;
	}

	sendto( _main->udp->sockfd, nodes_compact_list, nodes_compact_size, 0,
		(const struct sockaddr *)from, sizeof( IP ) );

	return TRUE;
}

int p2p_localhost_lookup_remote( UCHAR *target, IP *from ) {
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	int nodes_compact_size = 0;
	IP sin;
	UCHAR *p = NULL;
	UCHAR *id = NULL;
	int j = 0;
	ITEM *ti = NULL;
	LOOKUP *l = NULL;

	/* Start the incremental remote search program */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket, nodes_compact_list, target );

	/* Create tid and get the lookup table */
	ti = tdb_put(P2P_GET_PEERS, target, from );
	l = tdb_ldb( ti );

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
	if( ben_str_size( node ) != SHA1_SIZE ) {
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
	if( ben_str_size( node ) != IP_SIZE ) {
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
	if( ben_str_size( node ) != 2 ) {
		return 0;
	}
	return 1;
}
