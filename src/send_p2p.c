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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>

#include "malloc.h"
#include "main.h"
#include "log.h"
#include "conf.h"
#include "random.h"
#include "aes.h"
#include "udp.h"
#include "str.h"
#include "list.h"
#include "ben.h"
#include "hash.h"
#include "node_p2p.h"
#include "p2p.h"
#include "bucket.h"
#include "send_p2p.h"
#include "cache.h"
#include "hex.h"

void send_ping( CIPV6 *sa, int type ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	UCHAR skey[SHA_DIGEST_LENGTH];

	/*
		1:c 20:COLLISION_ID
		1:i 20:NODE_ID
		1:k 20:SESSION_ID
		1:q 1:p
	*/

	rand_urandom( skey, SHA_DIGEST_LENGTH );
	cache_put( skey, type );

	/* Collision ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"c", 1 );
	ben_str( val, _main->conf->risk_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"i", 1 );
	ben_str( val, _main->conf->host_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Session key */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"k", 1 );
	ben_str( val, skey, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Query type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"q", 1 );
	ben_str( val,( UCHAR *)"p", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	/* Log */
	log_udp( sa, "PING" );
}

void send_pong( CIPV6 *sa, UCHAR *node_sk, int warning ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;

	/*
		1:c 20:COLLISION_ID
		1:i 20:NODE_ID
		1:k 20:SESSION_ID
		1:q 1:o
		[1:e 1:c]
	*/

	/* Collision ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"c", 1 );
	ben_str( val, _main->conf->risk_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"i", 1 );
	ben_str( val, _main->conf->host_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Session key */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"k", 1 );
	ben_str( val, node_sk, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Query Type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"q", 1 );
	ben_str( val,( UCHAR *)"o", 1 );
	ben_dict( dict, key, val );

	/* Collision detected */
	if( warning == NODE_COLLISION ) {
		/* Error */
		key = ben_init( BEN_STR );
		val = ben_init( BEN_STR );
		ben_str( key,( UCHAR *)"e", 1 );
		ben_str( val,( UCHAR *)"c", 1 );
		ben_dict( dict, key, val );
	}

	raw = ben_enc( dict );
	if( _main->conf->encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	/* Log */
	log_udp( sa, "PONG" );
}

void send_find( CIPV6 *sa, UCHAR *node_id, UCHAR *lkp_id ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	UCHAR skey[SHA_DIGEST_LENGTH];
	char buffer[MAIN_BUF+1];
	char hexbuf[HEX_LEN+1];

	/*
		1:c 20:COLLISION_ID
		1:i 20:NODE_ID
		1:k 20:SESSION_ID
		1:l 20:LOOKUP_ID
		1:f 20:FIND_ID
		1:q 1:f
	*/

	rand_urandom( skey, SHA_DIGEST_LENGTH );
	cache_put( skey, SEND_UNICAST );

	/* Collision ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"c", 1 );
	ben_str( val, _main->conf->risk_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"i", 1 );
	ben_str( val, _main->conf->host_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Session key */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"k", 1 );
	ben_str( val, skey, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Lookup ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"l", 1 );
	ben_str( val, lkp_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Target */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"f", 1 );
	ben_str( val, node_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Query Type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"q", 1 );
	ben_str( val,( UCHAR *)"f", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	/* Log */
	hex_encode( hexbuf, node_id );
	snprintf( buffer, MAIN_BUF+1, "FIND %s at", hexbuf );
	log_udp( sa, buffer );
}

void send_node( CIPV6 *sa, BUCK *b, UCHAR *node_sk, UCHAR *lkp_id, int warning ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *list_id = NULL;
	struct obj_ben *dict_node = NULL;
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	ITEM *item_n = NULL;
	struct obj_nodeItem *n = NULL;
	long int i=0;
	struct sockaddr_in6 *sin = NULL;

	/*
		1:c 20:COLLISION_ID
		1:i 20:NODE_ID
		1:k 20:SESSION_ID
		1:l 20:LOOKUP_ID
		1:n
			1:c 20:COLLISION_ID
			1:i 20:NODE_ID
			1:a 16:IP
			1:p 2:PORT
		1:q 1:n
		[1:e 1:c]
	*/

	/* Collision ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"c", 1 );
	ben_str( val, _main->conf->risk_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"i", 1 );
	ben_str( val, _main->conf->host_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Session key */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"k", 1 );
	ben_str( val, node_sk, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Lookup ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"l", 1 );
	ben_str( val, lkp_id, SHA_DIGEST_LENGTH );
	ben_dict( dict, key, val );

	/* Nodes */
	key = ben_init( BEN_STR );
	list_id = ben_init( BEN_LIST );
	ben_str( key,( UCHAR *)"n", 1 );
	ben_dict( dict, key, list_id );

	/* Insert nodes */
	item_n = b->nodes->start;
	for( i=0; i<b->nodes->counter; i++ ) {
		n = item_n->val;

		/* Do not include nodes, that are questionable */
		if( n->pinged > 0 ) {
			item_n = list_next( item_n );
			continue;
		}

		/* Network data */
		sin = (struct sockaddr_in6*)&n->c_addr;

		/* List object */
		dict_node = ben_init( BEN_DICT );
		ben_list( list_id, dict_node );

		/* Collision */
		key = ben_init( BEN_STR );
		val = ben_init( BEN_STR );
		ben_str( key,( UCHAR *)"c", 1 );
		ben_str( val, n->risk_id, SHA_DIGEST_LENGTH );
		ben_dict( dict_node, key, val );

		/* ID Dictionary */
		key = ben_init( BEN_STR );
		val = ben_init( BEN_STR );
		ben_str( key,( UCHAR *)"i", 1 );
		ben_str( val, n->id, SHA_DIGEST_LENGTH );
		ben_dict( dict_node, key, val );

		/* IP */
		key = ben_init( BEN_STR );
		val = ben_init( BEN_STR );
		ben_str( key,( UCHAR *)"a", 1 );
		ben_str( val,( UCHAR *)&sin->sin6_addr, 16 );
		ben_dict( dict_node, key, val );

		/* Port */
		key = ben_init( BEN_STR );
		val = ben_init( BEN_STR );
		ben_str( key,( UCHAR *)"p", 1 );
		ben_str( val,( UCHAR *)&sin->sin6_port, 2 );
		ben_dict( dict_node, key, val );

		item_n = list_next( item_n );
	}

	/* Query */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"q", 1 );
	ben_str( val,( UCHAR *)"n", 1 );
	ben_dict( dict, key, val );

	/* Collision detected */
	if( warning == NODE_COLLISION ) {
		/* Error */
		key = ben_init( BEN_STR );
		val = ben_init( BEN_STR );
		ben_str( key,( UCHAR *)"e", 1 );
		ben_str( val,( UCHAR *)"c", 1 );
		ben_dict( dict, key, val );
	}

	raw = ben_enc( dict );
	if( _main->conf->encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	/* Log */
	log_udp( sa, "NODE reply to" );
}

void send_aes( CIPV6 *sa, struct obj_raw *raw ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_str *aes = NULL;
	struct obj_raw *enc = NULL;
	UCHAR salt[AES_IV_SIZE];

	/*
		1:a[es] XX:LENGTH
		1:s[alt] 32:SALT
	*/

	/* Create random salt */
	rand_urandom( salt, AES_IV_SIZE );

	/* Encrypt message */
	aes = aes_encrypt( raw->code, raw->size, salt, 
			_main->conf->key, strlen( _main->conf->key) );
	if( aes == NULL ) {
		log_info( "Encoding AES message failed" );
		ben_free( dict );
		return;
	}

	/* AES */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"a", 1 );
	ben_str( val, aes->s, aes->i );
	ben_dict( dict, key, val );

	/* Salt */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"s", 1 );
	ben_str( val, salt, AES_IV_SIZE );
	ben_dict( dict, key, val );

	enc = ben_enc( dict );
	send_exec( sa, enc );
	raw_free( enc );
	ben_free( dict );
	str_free( aes );
}

void send_exec( CIPV6 *sa, struct obj_raw *raw ) {
	socklen_t addrlen = sizeof(struct sockaddr_in6 );

	if( _main->udp->sockfd < 0 ) {
		return;
	}

	sendto( _main->udp->sockfd, raw->code, raw->size, 0,( const struct sockaddr *)sa, addrlen );
}
