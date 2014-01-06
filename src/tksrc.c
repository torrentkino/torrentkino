/*
Copyright 2010 Aiko Barz

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
#include <string.h>
#include <arpa/inet.h>

#include "tksrc.h"

BEN *_nss_tk_load( void ) {
	char filename[BUF_SIZE];
	int filesize = 0;
	UCHAR *fbuf = NULL;
	BEN *conf = NULL;

	if( ( getenv( "HOME")) == NULL ) {
		goto end;
	}

	/* 1st: $HOME/.torrentkino.conf */
	snprintf( filename, BUF_SIZE, "%s/.%s", getenv( "HOME" ), CONF_FILE );
	if( !file_isreg( filename ) ) {

		/* 2nd: /etc/torrentkino.conf */
		snprintf( filename, BUF_SIZE, "/etc/%s", CONF_FILE );
		if( !file_isreg( filename ) ) {
			goto end;
		}
	}

	filesize = file_size( filename );
	if( filesize <= 0 ) {
		goto end;
	}

	if( ( fbuf = (UCHAR *) file_load( filename, 0, filesize ) ) == NULL ) {
		goto end;
	}

	if( !ben_validate( fbuf, filesize ) ) {
		goto end;
	}

	/* Parse request */
	conf = ben_dec( fbuf, filesize );
	if( conf == NULL ) {
		goto end;
	}
	if( conf->t != BEN_DICT ) {
		goto end;
	}

	myfree( fbuf );
	return conf;

end:
	ben_free( conf );
	myfree( fbuf );
	return NULL;
}

int _nss_tk_conf( int *port, int *mode ) {
	BEN *conf = NULL;

	/* Load config file */
	if( ( conf = _nss_tk_load() ) == NULL ) {
		return FALSE;
	}

	/* Load port */
	*port = _nss_tk_port( conf );
	if( *port == -1 ) {
		ben_free( conf );
		return FALSE;
	}

	/* Load mode */
	*mode = _nss_tk_mode( conf );
	if( *mode == -1 ) {
		ben_free( conf );
		return FALSE;
	}

	ben_free( conf );
	return TRUE;
}

int _nss_tk_port( BEN *conf ) {
	BEN *port = NULL;

	port = ben_dict_search_str( conf, "port" );
	if( !ben_is_int( port ) ) {
		return -1;
	}

	if( port->v.i < 1 || port->v.i > 65535 ) {
		return -1;
	}

	return port->v.i;
}

int _nss_tk_mode( BEN *conf ) {
	BEN *ip_version;

	ip_version = ben_dict_search_str( conf, "ip_version" );
	if( !ben_is_int( ip_version ) ) {
		return -1;
	}

	if( ip_version->v.i != 4 && ip_version->v.i != 6 ) {
		return -1;
	}

	return ip_version->v.i;
}

int _nss_tk_socket( int *sockfd, struct sockaddr_in6 *sa, socklen_t *sa_size,
	int port, int mode ) {
	struct timeval tv;

	memset( sa, '\0', *sa_size );

	/* Setup UDP */
	*sockfd = socket( AF_INET6, SOCK_DGRAM, 0 );
	if( *sockfd < 0 ) {
		return FALSE;
	}

	/* Set receive timeout */
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt( *sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
			sizeof( struct timeval ) );

	/* Setup target */
	sa->sin6_family = AF_INET6;
	sa->sin6_port = htons( port );
	if( mode == 6 ) {
		if( !inet_pton( AF_INET6, "::1", &(sa->sin6_addr)) ) {
			return FALSE;
		}
	} else {
		if( !inet_pton( AF_INET6, "::FFFF:127.0.0.1", &(sa->sin6_addr)) ) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
	{
	"t": "aa",
	"y": "q",
	"q": "name",
	"a": {
		"id": "mnopqrstuvwxyz123456"
		"name": "owncloud.p2p",
		}
	}
*/

int _nss_tk_send_name( int sockfd,
	struct sockaddr_in6 *sa, socklen_t *sa_size,
	UCHAR *nid, UCHAR *tid, UCHAR *name, int name_size ) {

	BEN *dict = ben_init( BEN_DICT );
	BEN *key = NULL;
	BEN *val = NULL;
	RAW *raw = NULL;
	BEN *arg = ben_init( BEN_DICT );
	ssize_t n = 0;

	/* Node ID */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, ( UCHAR *)"id", 2 );
	ben_str( val, nid, SHA1_SIZE );
	ben_dict( arg, key, val );

	/* Name */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"name", 4 );
	ben_str( val, name, name_size );
	ben_dict( arg, key, val );

	/* Argument */
	key = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"a", 1 );
	ben_dict( dict, key, arg );

	/* Query type */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"q", 1 );
	ben_str( val, (UCHAR *)"name", 4 );
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

	n = sendto( sockfd, raw->code, raw->size, 0,
		( const struct sockaddr * )sa, *sa_size );

	raw_free( raw );
	ben_free( dict );

	if( n != raw->size ) {
		return FALSE;
	}

	return TRUE;
}

ssize_t _nss_tk_read_ip( int sockfd,
	struct sockaddr_in6 *sa, socklen_t *sa_size,
	UCHAR *buffer, int bufsize ) {

	ssize_t n = 0;

	memset( buffer, '\0', bufsize );

	n = recvfrom( sockfd, buffer, bufsize-1, 0,
			( struct sockaddr * )sa, sa_size );

	if( n <= 0 ) {
		return 0;
	}

	return n;
}

int _nss_tk_connect( const char *hostname, int hostsize,
	UCHAR *buffer, int bufsize, int port, int mode ) {

	int sockfd = -1;
	int n = 0;
	struct timeval tv;
	struct sockaddr_in6 sa6;
	socklen_t sin_size = sizeof( struct sockaddr_in6 );

	memset( &sa6, '\0', sin_size );
	memset( buffer, '\0', bufsize );

	/* Setup UDP */
	sockfd = socket( AF_INET6, SOCK_DGRAM, 0 );
	if( sockfd < 0 ) {
		return 0;
	}

	/* Set receive timeout */
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
			sizeof( struct timeval ) );

	/* Setup target */
	sa6.sin6_family = AF_INET6;
	sa6.sin6_port = htons( port );
	if( mode == 6 ) {
		if( !inet_pton( AF_INET6, "::1", &(sa6.sin6_addr)) ) {
			return 0;
		}
	} else {
		if( !inet_pton( AF_INET6, "::FFFF:127.0.0.1", &(sa6.sin6_addr)) ) {
			return 0;
		}
	}

	n = sendto( sockfd, hostname, hostsize, 0,
			(struct sockaddr *)&sa6, sin_size );
	if( n != hostsize ) {
		return 0;
	}

	n = recvfrom( sockfd, buffer, bufsize-1, 0,
			(struct sockaddr *)&sa6, &sin_size );

	if( n < 0 ) {
		return 0;
	}

	return n;
}

BEN *_nss_tk_packet( UCHAR *bencode, int bensize ) {
	BEN *packet = NULL;

	/* Validate bencode */
	if( !ben_validate( bencode, bensize ) ) {
		return NULL;
	}

	/* Parse request */
	packet = ben_dec( bencode, bensize );
	if( packet == NULL ) {
		return NULL;
	} else if( packet->t != BEN_DICT ) {
		ben_free( packet );
		return NULL;
	}

	return packet;
}

BEN *_nss_tk_values( BEN *packet, UCHAR *tid ) {
	BEN *y = NULL;
	BEN *r = NULL;
	BEN *t = NULL;
	BEN *v = NULL;
	BEN *id = NULL;

	/* Type of message */
	y = ben_dict_search_str( packet, "y" );
	if( !ben_is_str( y ) || ben_str_i( y ) != 1 ) {
		return NULL;
	}
	if( *y->v.s->s != 'r' ) {
		return NULL;
	}

	/* Argument */
	r = ben_dict_search_str( packet, "r" );
	if( !ben_is_dict( r ) ) {
		return NULL;
	}

	/* Node ID */
	id = ben_dict_search_str( r, "id" );
	if( !ben_is_str( id ) || ben_str_i( id ) != SHA1_SIZE ) {
		return NULL;
	}

	/* Transaction ID */
	t = ben_dict_search_str( packet, "t" );
	if( !ben_is_str( t ) && ben_str_i( t ) != TID_SIZE ) {
		return NULL;
	}
	if( memcmp( t->v.s->s, tid, TID_SIZE ) != 0 ) {
		return NULL;
	}

	/* Values */
	v = ben_dict_search_str( r, "values" );
	if( !ben_is_list( v ) ) {
		return NULL;
	}

	return v;
}

UCHAR *_nss_tk_bytes_to_sin6( struct sockaddr_in6 *sin, UCHAR *p ) {
	socklen_t sin_size = sizeof( struct sockaddr_in6 );
	memset( sin, '\0', sin_size );
	sin->sin6_family = AF_INET6;
	memcpy( &sin->sin6_addr, p, 16 ); p += 16;
	memcpy( &sin->sin6_port, p, 2 ); p += 2;
	return p;
}

UCHAR *_nss_tk_bytes_to_sin( struct sockaddr_in *sin, UCHAR *p ) {
	socklen_t sin_size = sizeof( struct sockaddr_in );
	memset( sin, '\0', sin_size );
	sin->sin_family = AF_INET;
	memcpy( &sin->sin_addr, p, 4 ); p += 4;
	memcpy( &sin->sin_port, p, 2 ); p += 2;
	return p;
}
