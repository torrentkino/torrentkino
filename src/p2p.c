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
#include <semaphore.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "malloc.h"
#include "thrd.h"
#include "main.h"
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
#include "aes.h"
#include "token.h"
#include "neighbourhood.h"
#include "bucket.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "send_p2p.h"
#include "search.h"
#include "time.h"
#include "random.h"
#include "sha1.h"
#include "hex.h"
#include "info_hash.h"

struct obj_p2p *p2p_init( void ) {
	struct obj_p2p *p2p = (struct obj_p2p *) myalloc( sizeof(struct obj_p2p), "p2p_init" );

	p2p->time_maintainance = 0;
	p2p->time_multicast = 0;
	p2p->time_announce = 0;
	p2p->time_restart = 0;
	p2p->time_expire = 0;
	p2p->time_split = 0;
	p2p->time_token = 0;
	p2p->time_find = 0;
	p2p->time_ping = 0;

	gettimeofday( &p2p->time_now, NULL );

	p2p->mutex = mutex_init();

	return p2p;
}

void p2p_free( void ) {
	mutex_destroy( _main->p2p->mutex );
	myfree( _main->p2p, "p2p_free" );
}

void p2p_bootstrap( void ) {
	struct addrinfo hints;
	struct addrinfo *info = NULL;
	struct addrinfo *p = NULL;
	int rc = 0;
	int i = 0;
	ITEM *ti = NULL;

	log_info( NULL, 0, "Connecting to a bootstrap server" );

	/* Compute address of bootstrap node */
	memset( &hints, '\0', sizeof(struct addrinfo) );
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET6;
	rc = getaddrinfo( _main->conf->bootstrap_node, _main->conf->bootstrap_port, &hints, &info );
	if( rc != 0 ) {
		log_info( NULL, 0, "getaddrinfo: %s", gai_strerror( rc ) );
		return;
	}

	p = info;
	while( p != NULL && i < P2P_MAX_BOOTSTRAP_NODES ) {

		/* Send PING to a bootstrap node */
		if( strcmp( _main->conf->bootstrap_node, CONF_BOOTSTRAP_NODE ) == 0 ) {
			ti = tdb_put(P2P_PING_MULTICAST, NULL, NULL);
			send_ping( (IP *)p->ai_addr, tdb_tid( ti ) );
		} else {
			ti = tdb_put(P2P_PING, NULL, NULL);
			send_ping( (IP *)p->ai_addr, tdb_tid( ti ) );
		}
		
		p = p->ai_next;	i++;
	}
	
	freeaddrinfo( info );
}

void p2p_cron( void ) {
	/* Tick Tock */
	gettimeofday( &_main->p2p->time_now, NULL );

	if( nbhd_is_empty( _main->nbhd ) ) {
		
		/* Bootstrap PING */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_restart ) {
			p2p_bootstrap();
			time_add_1_min_approx( &_main->p2p->time_restart );
		}
	
	} else {

		/* Expire objects. Run once a minute. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_expire ) {
			tdb_expire();
			nbhd_expire();
			idb_expire();
			tkn_expire();
			time_add_1_min_approx( &_main->p2p->time_expire );
		}

		/* Split buckets. Evolve neighbourhood. Run once a minute. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_split ) {
			nbhd_split( _main->nbhd, _main->conf->node_id );
			time_add_1_min_approx( &_main->p2p->time_split );
		}
	
		/* Ping all nodes every ~5 minutes. Run once a minute. */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_ping ) {
			p2p_cron_ping();
			time_add_1_min_approx( &_main->p2p->time_ping );
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

		/* Create a new token every ~5 minutes */
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_token ) {
			tkn_put();
			time_add_5_min_approx( &_main->p2p->time_token );
		}
	}

	/* Try to register multicast address until it works. */
	if( _main->udp->multicast == 0 ) {
		if( _main->p2p->time_now.tv_sec > _main->p2p->time_multicast ) {
			udp_multicast();
			time_add_5_min_approx( &_main->p2p->time_multicast );
		}
	}
}

void p2p_cron_ping( void ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
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
	UCHAR node_id[SHA_DIGEST_LENGTH];
	rand_urandom( node_id, SHA_DIGEST_LENGTH );
	p2p_cron_find( node_id );
}

void p2p_cron_find( UCHAR *target ) {
	ITEM *item_b = NULL;
	BUCK *b = NULL;
	ITEM *item_n = NULL;
	NODE *n = NULL;
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
	UCHAR nodes_compact_list[304]; /* 8*(20+16+2) or 8*(16+2) */
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
	for( j=0; j<nodes_compact_size; j+=38 ) {

		/* Remember node_id */
		id = p;
		p += SHA_DIGEST_LENGTH;

		/* IP + Port */
		memset( &sin, '\0', sizeof(IP) );
		sin.sin6_family = AF_INET6;
		
		memcpy( &sin.sin6_addr, p, 16 ); p += 16;
		memcpy( &sin.sin6_port, p, 2 ); p += 2;

		/* Query node */
		send_get_peers_request( (IP *)&sin, target, tdb_tid( ti ) );

		/* Remember queried node */
		nbhd_put( l->nbhd, id, (IP *)&sin );
	}
}

void p2p_cron_announce_engage( ITEM *ti ) {
	ITEM *t_new = NULL;
	unsigned long int j = 0;
	ITEM *item = NULL;
	BUCK *b = NULL;
	NODE *n = NULL;
	TID *tid = list_value( ti );
	LOOKUP *l = tid->lookup;

	/* Remove nodes, that did not reply with a token. We cannot announce to
	 * them. So sort them out. */
	nbhd_expire_nodes_with_emtpy_tokens( l->nbhd );

	/* Order the nodes within the buckets. Later, we lookup the best matching
	 * bucket. */
	nbhd_split( l->nbhd, l->target );

	/* And find matching bucket */
	if( ( item = bckt_find_any_match( l->nbhd->bucket, l->target)) == NULL ) {
		return;
	} else {
		b = list_value( item );
	}

	item = list_start( b->nodes );
	while( item != NULL && j < 8 ) {
		n = list_value( item );

		t_new = tdb_put(P2P_ANNOUNCE_ENGAGE, NULL, NULL);
		send_announce_request( &n->c_addr, tdb_tid( t_new ), n->token, n->token_size );
		
		item = list_next( item );
		j++;
	}
}

void p2p_parse( UCHAR *bencode, size_t bensize, IP *from ) {
	/* Tick Tock */
	mutex_block( _main->p2p->mutex );
	gettimeofday( &_main->p2p->time_now, NULL );
	mutex_unblock( _main->p2p->mutex );

	/* UDP packet too small */
	if( bensize < 1 ) {
		log_info( NULL, 0, "UDP packet too small" );
		return;
	}

	/* Recursive lookup request from localhost ::1 */
	if( nbhd_conn_from_localhost(from) ) {
		p2p_localhost_get_request( bencode, bensize, from );
		return;
	}

	/* Validate bencode */
	if( !ben_validate( bencode, bensize ) ) {
		log_info( NULL, 0, "UDP packet contains broken bencode" );
		return;
	}

	/* Encrypted message or plaintext message */
	if( _main->conf->bool_encryption ) {
		p2p_decrypt( bencode, bensize, from );
	} else {
		p2p_decode( bencode, bensize, from );
	}
}

void p2p_decrypt( UCHAR *bencode, size_t bensize, IP *from ) {
	BEN *packet = NULL;
	BEN *salt = NULL;
	BEN *aes = NULL;
	struct obj_str *plain = NULL;

	/* Parse request */
	packet = ben_dec( bencode, bensize );
	if( !ben_is_dict( packet ) ) {
		log_info( NULL, 0, "Decoding AES packet failed" );
		ben_free( packet );
		return;
	}

	/* Salt */
	salt = ben_searchDictStr( packet, "s" );
	if( !ben_is_str( salt ) || ben_str_size( salt ) != AES_IV_SIZE ) {
		log_info( NULL, 0, "Salt missing or broken" );
		ben_free( packet );
		return;
	}

	/* Encrypted AES message */
	aes = ben_searchDictStr( packet, "a" );
	if( !ben_is_str( aes ) || ben_str_size( aes ) <= 2 ) {
		log_info( NULL, 0, "AES message missing or broken" );
		ben_free( packet );
		return;
	}

	/* Decrypt message */
	plain = aes_decrypt( aes->v.s->s, aes->v.s->i,
		salt->v.s->s,
		_main->conf->key, strlen( _main->conf->key ) );
	if( plain == NULL ) {
		log_info( NULL, 0, "Decoding AES message failed" );
		ben_free( packet );
		return;
	}

	/* AES packet too small */
	if( plain->i < SHA_DIGEST_LENGTH ) {
		ben_free( packet );
		str_free( plain );
		log_info( NULL, 0, "AES packet contains less than 20 bytes" );
		return;
	}

	/* Validate bencode */
	if( !ben_validate( plain->s, plain->i) ) {
		ben_free( packet );
		str_free( plain );
		log_info( NULL, 0, "AES packet contains broken bencode" );
		return;
	}

	/* Parse message */
	p2p_decode( plain->s, plain->i, from );

	/* Free */
	ben_free( packet );
	str_free( plain );
}

void p2p_decode( UCHAR *bencode, size_t bensize, IP *from ) {
	BEN *packet = NULL;
	BEN *y = NULL;

	/* Parse request */
	packet = ben_dec( bencode, bensize );
	if( packet == NULL ) {
		log_info( NULL, 0, "Decoding UDP packet failed" );
		return;
	} else if( packet->t != BEN_DICT ) {
		log_info( NULL, 0, "UDP packet is not a dictionary" );
		ben_free( packet );
		return;
	}

	/* Type of message */
	y = ben_searchDictStr( packet, "y" );
	if( !ben_is_str( y ) || ben_str_size( y ) != 1 ) {
		log_info( NULL, 0, "Message type missing or broken" );
		ben_free( packet );
		return;
	}

	mutex_block( _main->p2p->mutex );

	switch( *y->v.s->s ) {

		case 'q':
			p2p_request( packet, from );
			break;
		case 'r':
			p2p_reply( packet, from );
			break;
		default:
			log_info( NULL, 0, "Unknown message type" );
	}
	
	mutex_unblock( _main->p2p->mutex );

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
		log_info( NULL, 0, "Query type missing or broken" );
		return;
	}

	/* Argument */
	a = ben_searchDictStr( packet, "a" );
	if( !ben_is_dict( a ) ) {
		log_info( NULL, 0, "Argument missing or broken" );
		return;
	}

	/* Node ID */
	id = ben_searchDictStr( a, "id" );
	if( !p2p_is_hash( id ) ) {
		log_info( NULL, 0, "Node ID missing or broken" );
		return;
	}

	/* Do not talk to myself */
	if( p2p_packet_from_myself( id->v.s->s ) ) {
		return;
	}

	/* Transaction ID */
	t = ben_searchDictStr( packet, "t" );
	if( !ben_is_str( t ) ) {
		log_info( NULL, 0, "Transaction ID missing or broken" );
		return;
	}
	if( ben_str_size( t ) > TID_SIZE_MAX ) {
		log_info( NULL, 0, "Transaction ID too big" );
		return;
	}

	/* Remember node. This does not update the IP address. */
	nbhd_put( _main->nbhd, id->v.s->s, (IP *)from );

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
	
	log_info( NULL, 0, "Unknown query type" );
}

void p2p_reply( BEN *packet, IP *from ) {
	BEN *r = NULL;
	BEN *t = NULL;
	BEN *id = NULL;
	ITEM *ti = NULL;

	/* Argument */
	r = ben_searchDictStr( packet, "r" );
	if( !ben_is_dict( r ) ) {
		log_info( NULL, 0, "Argument missing or broken" );
		return;
	}

	/* Node ID */
	id = ben_searchDictStr( r, "id" );
	if( !p2p_is_hash( id ) ) {
		log_info( NULL, 0, "Node ID missing or broken" );
		return;
	}
	
	/* Do not talk to myself */
	if( p2p_packet_from_myself( id->v.s->s ) ) {
		return;
	}

	/* Transaction ID */
	t = ben_searchDictStr( packet, "t" );
	if( !ben_is_str( t ) && ben_str_size( t ) != TID_SIZE ) {
		log_info( NULL, 0, "Transaction ID missing or broken" );
		return;
	}

	/* Remember node. */
	nbhd_put( _main->nbhd, id->v.s->s, (IP *)from );

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
			log_info( NULL, 0, "Unknown Transaction ID..." );
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
	if( nbhd_me( node_id ) ) {
		
		/* Received packet from myself 
		 * If the neighbourhood is empty, 
		 * then you may see multicast requests from yourself.
		 * Do not warn about them.
		 */
		if( !nbhd_is_empty( _main->nbhd ) ) {
			log_info( NULL, 0, "WARNING: Received a packet from myself..." );
		}

		/* Yep */
		return 1;
	}

	/* No */
	return 0;
}

void p2p_ping( BEN *tid, IP *from ) {
	send_pong( from, tid->v.s->s, tid->v.s->i );
}

void p2p_pong( UCHAR *node_id, IP *from ) {
	nbhd_ponged( node_id, from );
}

void p2p_find_node_get_request( BEN *arg, BEN *tid, IP *from ) {
	BEN *target = NULL;
	UCHAR nodes_compact_list[304]; /* 8 * (20+16+2) */
	int nodes_compact_size = 0;

	/* Target */
	target = ben_searchDictStr( arg, "target" );
	if( !p2p_is_hash( target ) ) {
		log_info( NULL, 0, "Missing or broken target" );
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

	nodes = ben_searchDictStr( arg, "nodes6" );
	if( !ben_is_str( nodes ) ) {
		log_info( NULL, 0, "nodes6 key missing" );
		return;
	}

	if( ben_str_size( nodes ) % 38 != 0 ) {
		log_info( NULL, 0, "nodes6 key broken" );
		return;
	}

	p = nodes->v.s->s;
	for( i=0; i<nodes->v.s->i; i+=38 ) {
		memset( &sin, '\0', sizeof(IP) );

		/* ID */
		id = p;
		p += SHA_DIGEST_LENGTH;

		/* IP */
		sin.sin6_family = AF_INET6;
		memcpy( &sin.sin6_addr, p, 16 );
		p += 16;

		/* Port */
		memcpy( &sin.sin6_port, p, 2 );
		p += 2;

		/* Store node */
		if( !nbhd_me( id ) ) {
			nbhd_put( _main->nbhd, id, ( IP *)&sin );
		}
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
	UCHAR nodes_compact_list[304]; /* 8*(20+16+2) or 8*(16+2) */
	int nodes_compact_size = 0;

	/* info_hash */
	info_hash = ben_searchDictStr( arg, "info_hash" );
	if( !p2p_is_hash( info_hash ) ) {
		log_info( NULL, 0, "Missing or broken info_hash" );
		return;
	}

	/* Look at the database */
	nodes_compact_size = p2p_value_compact_list( nodes_compact_list, info_hash->v.s->s );

	if( nodes_compact_size > 0 ) {
		send_get_peers_values( from, nodes_compact_list, nodes_compact_size,
			tid->v.s->s, tid->v.s->i );
		return;
	}

	/* Look at the routing table */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket, nodes_compact_list, info_hash->v.s->s );

	/* Send reply */
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
		log_info( NULL, 0, "token key missing or broken" );
		return;
	} else if( ben_str_size( token ) > TOKEN_SIZE_MAX ) {
		log_info( NULL, 0, "Token key too big" );
		return;
	} else if( ben_str_size( token ) <= 0 ) {
		log_info( NULL, 0, "Token key too small" );
		return;
	}

	values = ben_searchDictStr( arg, "values" );
	nodes = ben_searchDictStr( arg, "nodes6" );

	if( values != NULL ) {
		if( !ben_is_list( values ) ) {
			log_info( NULL, 0, "values key missing or broken" );
			return;
		} else {
			p2p_get_peers_get_values( values, node_id, ti, token, from );
			return;
		}
	}
	
	if( nodes != NULL ) {
		if( !ben_is_str( nodes ) || ben_str_size( nodes ) % 38 != 0 ) {
			log_info( NULL, 0, "nodes6 key missing or broken" );
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

	ldb_update_token( l, node_id, token, from );

	p = nodes->v.s->s;
	for( i=0; i<nodes->v.s->i; i+=38 ) {
		memset( &sin, '\0', sizeof(IP) );

		/* ID */
		id = p;
		p += SHA_DIGEST_LENGTH;

		/* IP */
		sin.sin6_family = AF_INET6;
		memcpy( &sin.sin6_addr, p, 16 );
		p += 16;

		/* Port */
		memcpy( &sin.sin6_port, p, 2 );
		p += 2;

		/* Store node */
		if( !nbhd_me( id ) ) {
			nbhd_put( _main->nbhd, id, (IP *)&sin );
		}

		/* Start new lookup */
		if( !nbhd_me( id ) ) {
			if( !ldb_contacted_node( l, id ) ) {

				/* Query node */
				send_get_peers_request( (IP *)&sin, target, tdb_tid( ti ) );

				/* Remember queried node */
				nbhd_put( l->nbhd, id, (IP *)&sin );
			}
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
		"values": ["axje.u", "idhtnm"]
		}
	}
*/

void p2p_get_peers_get_values( BEN *values, UCHAR *node_id, ITEM *ti, BEN *token, IP *from ) {
	UCHAR nodes_compact_list[144]; /* 8*(16+2) */
	UCHAR *p = nodes_compact_list;
	LOOKUP *l = tdb_ldb( ti );
	int nodes_compact_size = 0;
	BEN *val = NULL;
	ITEM *item = NULL;
	long int j = 0;
	int type = tdb_type( ti );

	if( l == NULL ) {
		return;
	}

	ldb_update_token( l, node_id, token, from );

	/* Get values */
	item = list_start( values->v.l );
	while( item != NULL && j < 8 ) {
		val = list_value( item );

		if( !ben_is_str( val ) || ben_str_size( val ) != 18 ) {
			log_info( NULL, 0, "Values list broken" );
			return;
		}

		memcpy( p, val->v.s->s, val->v.s->i );
		nodes_compact_size += 18;
		p += 18;

		item = list_next(item);
		j++;
	}

	/* Send reply */
	if( type == P2P_GET_PEERS && nodes_compact_size > 0 ) {
		sendto( _main->udp->sockfd, nodes_compact_list, nodes_compact_size, 0,
			(const struct sockaddr *)&l->c_addr, sizeof( IP ) );
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
		log_info( NULL, 0, "Missing or broken info_hash" );
		return;
	}

	/* Token */
	token = ben_searchDictStr( arg, "token" );
	if( !ben_is_str( token ) || ben_str_size( token ) > TOKEN_SIZE_MAX ) {
		log_info( NULL, 0, "token key missing or broken" );
		return;
	}

	if( !tkn_validate( token->v.s->s ) ) {
		log_info( NULL, 0, "Invalid token" );
		return;
	}

	/* Port */
	port = ben_searchDictStr( arg, "port" );
	if( !ben_is_int( port ) ) {
		log_info( NULL, 0, "Missing or broken port" );
		return;
	}

	if( port->v.i < 1 || port->v.i > 65535 ) {
		log_info( NULL, 0, "Invalid port number" );
		return;
	}

	/* Store info_hash */
	idb_put( info_hash->v.s->s, port->v.i, node_id, from );

	/* Send ack */
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
	UCHAR target[SHA_DIGEST_LENGTH];
	UCHAR nodes_compact_list[304]; /* 8*(20+16+2) or 8*(16+2) */
	int nodes_compact_size = 0;
	IP sin;
	UCHAR *p = NULL;
	UCHAR *id = NULL;
	int j = 0;
	ITEM *ti = NULL;
	LOOKUP *l = NULL;

	/* Validate hostname */
	if ( !str_isValidHostname( (char *)hostname, size ) ) {
		log_info( NULL, 0, "LOOKUP %s (Invalid hostname)", hostname );
		return;
	}

	/* Compute lookup key */
	p2p_compute_realm_id( target, (char *)hostname );

	/* Check my own DB for that target. */
	mutex_block( _main->p2p->mutex );
	nodes_compact_size = p2p_value_compact_list( nodes_compact_list, target );
	mutex_unblock( _main->p2p->mutex );

	/* Direct reply if possible */
	if( nodes_compact_size > 0 ) {
		log_info( NULL, 0, "LOOKUP %s (Local)", hostname );
		sendto( _main->udp->sockfd, nodes_compact_list, nodes_compact_size, 0,
			(const struct sockaddr *)from, sizeof( IP ) );
		return;
	}

	/* Start the incremental remote search program */
	log_info( NULL, 0, "LOOKUP %s (Remote)", hostname );
	mutex_block( _main->p2p->mutex );
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket, nodes_compact_list, target );
	mutex_unblock( _main->p2p->mutex );

	/* Create tid and get the lookup table */
	ti = tdb_put(P2P_GET_PEERS, target, from );
	l = tdb_ldb( ti );

	p = nodes_compact_list;
	for( j=0; j<nodes_compact_size; j+=38 ) {
		memset( &sin, '\0', sizeof(IP) );

		/* ID */
		id = p;
		p += SHA_DIGEST_LENGTH;

		/* IP */
		sin.sin6_family = AF_INET6;
		memcpy( &sin.sin6_addr, p, 16 );
		p += 16;

		/* Port */
		memcpy( &sin.sin6_port, p, 2 );
		p += 2;

		/* Query node */
		send_get_peers_request( (IP *)&sin, target, tdb_tid( ti ) );

		/* Remember queried node */
		nbhd_put( l->nbhd, id, (IP *)&sin );
	}
}

/* FIXME: XOR */
void p2p_compute_realm_id( UCHAR *host_id, char *hostname ) {
	char buffer[MAIN_BUF+1];

	/* The realm influences the way, the lookup hash gets computed */
	if( _main->conf->bool_realm == TRUE ) {
		snprintf(buffer, MAIN_BUF+1, "%s.%s", hostname, _main->conf->realm);
		sha1_hash( host_id, buffer, strlen(buffer) );
	} else {
		sha1_hash( host_id, hostname, strlen(hostname) );
	}
}

int bckt_compact_list( LIST *l, UCHAR *nodes_compact_list, UCHAR *target ) {
	UCHAR *p = nodes_compact_list;
	ITEM *item = NULL;
	BUCK *b = NULL;
	NODE *n = NULL;
	unsigned long int j = 0;
	IP *sin = NULL;
	int size = 0;

	/* Find matching bucket */
	if( ( item = bckt_find_any_match( l, target)) == NULL ) {
		return 0;
	} else {
		b = list_value( item );
	}

/* FIXME: Always send 8 nodes, even if there are bad ones in between */

	/* Walkthrough bucket */
	item = list_start( b->nodes );
	while( item != NULL && j < 8 ) {
		n = list_value( item );
		
		/* Do not include nodes, that are questionable */
		if( n->pinged > 0 ) {
			item = list_next( item );
			continue;
		}

		/* Network data */
		sin = (IP*)&n->c_addr;

		/* Node ID */
		memcpy( p, n->id, SHA_DIGEST_LENGTH );
		p += SHA_DIGEST_LENGTH;

		/* IP */
		memcpy( p, (UCHAR *)&sin->sin6_addr, 16 );
		p += 16;

		/* Port */
		memcpy( p, (UCHAR *)&sin->sin6_port, 2 );
		p += 2;

		size += 38;

		item = list_next( item );
		j++;
	}
	
	return size;
}

int p2p_value_compact_list( UCHAR *nodes_compact_list, UCHAR *target ) {
	UCHAR *p = nodes_compact_list;
	INODE *inode = NULL;
	IHASH *ihash = NULL;
	ITEM *item = NULL;
	unsigned long int j = 0;
	IP *sin = NULL;
	int size = 0;

	/* Look into the local database */
	if( ( item = idb_find_target( target ) ) == NULL ) {
		return 0;
	} else {
		ihash = list_value( item );
	}

	/* Walkthrough local database */
	item = list_start( ihash->list );
	while( item != NULL && j < 8 ) {
		inode = list_value( item );

		/* Network data */
		sin = (IP*)&inode->c_addr;

		/* IP */
		memcpy( p, (UCHAR *)&sin->sin6_addr, 16 );
		p += 16;

		/* Port */
		memcpy( p, (UCHAR *)&sin->sin6_port, 2 );
		p += 2;

		size += 18;

		item = list_next( item );
		j++;
	}
	
	return size;
}

int p2p_is_hash( BEN *node ) {
	if( node == NULL ) {
		return 0;
	}
	if( !ben_is_str( node ) ) {
		return 0;
	}
	if( ben_str_size( node ) != SHA_DIGEST_LENGTH ) {
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
	if( ben_str_size( node ) != 16 ) {
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
