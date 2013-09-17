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

#include "send_p2p.h"

/*
	{
	"t": "aa",
	"y": "q",
	"q": "ping",
	"a": {
		"id": "abcdefghij0123456789"
		}
	}
*/

void send_ping( IP *sa, UCHAR *tid ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"a", 1 );
	ben_dict( dict, key, arg );

	/* Query type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"q", 1 );
	ben_str( val, (UCHAR *)"ping", 4 );
	ben_dict( dict, key, val );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, TID_SIZE );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"q", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "PING" );
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

void send_pong( IP *sa, UCHAR *tid, int tid_size ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"r", 1 );
	ben_dict( dict, key, arg );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"t", 1 );
	ben_str( val, tid, tid_size );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"r", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "PONG" );
}

/*
	{
	"t": "aa",
	"y": "q",
	"q": "find_node",
	"a": {
		"id": "abcdefghij0123456789",
		"target": "mnopqrstuvwxyz123456"
		}
	}
*/

void send_find_node_request( IP *sa, UCHAR *node_id, UCHAR *tid ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );
	char hexbuf[HEX_LEN];

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Target */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"target", 6 );
	ben_str( val, node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"a", 1 );
	ben_dict( dict, key, arg );

	/* Query type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"q", 1 );
	ben_str( val, (UCHAR *)"find_node", 9 );
	ben_dict( dict, key, val );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, TID_SIZE );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"q", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	hex_hash_encode( hexbuf, node_id );
	info( sa, 0, "FIND %s at", hexbuf );
}

/*
	{
	"t": "aa",
	"y": "r",
	"r": {
		"id":"0123456789abcdefghij",
		"nodes": "def456..."
		}
	}
*/

void send_find_node_reply( IP *sa, UCHAR *nodes_compact_list, int nodes_compact_size, UCHAR *tid, int tid_size ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Nodes */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"nodes6", 6 );
	ben_str( val, nodes_compact_list, nodes_compact_size );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"r", 1 );
	ben_dict( dict, key, arg );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, tid_size );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"r", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "NODES via FIND_NODE to");
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

void send_get_peers_request( IP *sa, UCHAR *node_id, UCHAR *tid ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );
	char hexbuf[HEX_LEN];

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* info_hash */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"info_hash", 9 );
	ben_str( val, node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"a", 1 );
	ben_dict( dict, key, arg );

	/* Query type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"q", 1 );
	ben_str( val, (UCHAR *)"get_peers", 9 );
	ben_dict( dict, key, val );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, TID_SIZE );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"q", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	hex_hash_encode( hexbuf, node_id );
	info( sa, 0, "GET_PEERS %s at", hexbuf );
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

void send_get_peers_nodes( IP *sa, UCHAR *nodes_compact_list, int nodes_compact_size, UCHAR *tid, int tid_size ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Nodes */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"nodes6", 6 );
	ben_str( val, nodes_compact_list, nodes_compact_size );
	ben_dict( arg, key, val );

	/* Token */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"token", 5 );
	ben_str( val, tkn_read(), TOKEN_SIZE );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"r", 1 );
	ben_dict( dict, key, arg );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, tid_size );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"r", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "NODES via GET_PEERS to");
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

void send_get_peers_values( IP *sa, UCHAR *nodes_compact_list, int nodes_compact_size, UCHAR *tid, int tid_size ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *list = ben_init( BEN_LIST );
	struct obj_ben *arg = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	UCHAR *p = nodes_compact_list;
	int j = 0;

	/* Values list */
	for( j=0; j<nodes_compact_size; j+=18 ) {
		val = ben_init( BEN_STR );
		ben_str( val, p, 18 );
		ben_list( list, val );
		p += 18;
	}

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Token */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"token", 5 );
	ben_str( val, tkn_read(), TOKEN_SIZE );
	ben_dict( arg, key, val );

	/* Values */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"values", 6 );
	ben_dict( arg, key, list );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"r", 1 );
	ben_dict( dict, key, arg );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, tid_size );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"r", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "VALUES via GET_PEERS to" );
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

void send_announce_request( IP *sa, UCHAR *tid, UCHAR *token, int token_size ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* My host ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"info_hash", 9 );
	ben_str( val, _main->conf->host_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Port */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_INT );
	ben_str( key,( UCHAR *)"port", 4 );
	ben_int( val, _main->conf->announce_port );
	ben_dict( arg, key, val );

	/* Token */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"token", 5 );
	ben_str( val, token, token_size );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"a", 1 );
	ben_dict( dict, key, arg );

	/* Query type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"q", 1 );
	ben_str( val, (UCHAR *)"announce_peer", 13 );
	ben_dict( dict, key, val );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, TID_SIZE );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"q", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "ANNOUNCE_PEER to" );
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

void send_announce_reply( IP *sa, UCHAR *tid, int tid_size ) {
	struct obj_ben *dict = ben_init( BEN_DICT );
	struct obj_ben *key = NULL;
	struct obj_ben *val = NULL;
	struct obj_raw *raw = NULL;
	struct obj_ben *arg = ben_init( BEN_DICT );

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key,( UCHAR *)"id", 2 );
	ben_str( val, _main->conf->node_id, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"r", 1 );
	ben_dict( dict, key, arg );

	/* Transaction ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"t", 1 );
	ben_str( val, tid, tid_size );
	ben_dict( dict, key, val );

	/* Type of message */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"y", 1 );
	ben_str( val, (UCHAR *)"r", 1 );
	ben_dict( dict, key, val );

	raw = ben_enc( dict );
	if( _main->conf->bool_encryption ) {
		send_aes( sa, raw );
	} else {
		send_exec( sa, raw );
	}
	raw_free( raw );
	ben_free( dict );

	info( sa, 0, "ANNOUNCE_PEER SUCCESS to" );
}

void send_aes( IP *sa, struct obj_raw *raw ) {
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
		info( NULL, 0, "Encoding AES message failed" );
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

void send_exec( IP *sa, struct obj_raw *raw ) {
	socklen_t addrlen = sizeof(IP );

	if( _main->udp->sockfd < 0 ) {
		return;
	}

	sendto( _main->udp->sockfd, raw->code, raw->size, 0,( const struct sockaddr *)sa, addrlen );
}
