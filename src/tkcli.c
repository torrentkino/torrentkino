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

#include "tkcli.h"

int torrentkino_lookup( const char *handler, const char *hostname,
		const char *path, unsigned int port, int mode ) {
	UCHAR bencode[BUF_SIZE];
	int bensize = 0;
	int j = 0;
	struct sockaddr_in6 sin6;
	struct sockaddr_in sin;
	struct sockaddr_in6 sa;
	int sockfd = -1;
	socklen_t sa_size = sizeof( struct sockaddr_in6 );
	UCHAR tid[TID_SIZE];
	UCHAR nid[SHA1_SIZE];
	BEN *packet = NULL;
	BEN *values = NULL;
	BEN *ip_bin = NULL;
	ITEM *item = NULL;
	int pair_size = ( mode == 6 ) ? 18 : 6;

	/* Create transaction id */
	rand_urandom( tid, TID_SIZE );

	/* Create random node id */
	rand_urandom( nid, SHA1_SIZE );

	/* Prepare socket */
	if( !_nss_tk_socket( &sockfd, &sa, &sa_size, port, mode ) ) {
		return FALSE;
	}

	/* Send request */
	if( !_nss_tk_send_lookup( sockfd, &sa, &sa_size, nid, tid,
		(UCHAR *)hostname, strlen( hostname ) ) ) {
		return FALSE;
	}

	/* Read reply */
	bensize = _nss_tk_read_data( sockfd, &sa, &sa_size, bencode, BUF_SIZE );
	if( bensize <= 0 ) {
		return FALSE;
	}

	/* Create packet */
	if( ( packet = _nss_tk_packet( bencode, bensize ) ) == NULL ) {
		return FALSE;
	}

	/* Get values*/
	if( ( values = _nss_tk_values( packet, tid ) ) == NULL ) {
		ben_free( packet );
		return FALSE;
	}

	/* Get values */
	item = list_start( values->v.l );
	while( item != NULL && j < 8 ) {
		ip_bin = list_value( item );

		if( !ben_is_str( ip_bin ) || ben_str_i( ip_bin ) != pair_size ) {
			ben_free( packet );
			return FALSE;
		}

		if( handler != NULL && *handler != '\0' ) {
			printf( "%s://", handler );
		}

		if( mode == 6 ) {
			_nss_tk_bytes_to_sin6( &sin6, ben_str_s( ip_bin ) );
			torrenkino_print6( &sin6, handler, path );
		} else {
			_nss_tk_bytes_to_sin( &sin, ben_str_s( ip_bin ) );
			torrenkino_print( &sin, handler, path );
		}

		if( path != NULL && *path != '\0' ) {
			printf( "/%s\n", path );
		} else {
			printf( "\n" );
		}

		item = list_next(item);
		j++;
	}

	ben_free( packet );
	return TRUE;
}

void torrenkino_print6( struct sockaddr_in6 *sin, const char *handler,
		const char *path ) {
	char ip_buf[INET6_ADDRSTRLEN+1];
	memset( ip_buf, '\0', INET6_ADDRSTRLEN+1);
	printf("[%s]:%i",
			inet_ntop( AF_INET6, &sin->sin6_addr, ip_buf, INET6_ADDRSTRLEN ),
			ntohs( sin->sin6_port ) );
}

void torrenkino_print( struct sockaddr_in *sin, const char *handler,
		const char *path ) {
	char ip_buf[INET_ADDRSTRLEN+1];
	memset( ip_buf, '\0', INET_ADDRSTRLEN+1);
	printf("%s:%i",
			inet_ntop( AF_INET, &sin->sin_addr, ip_buf, INET_ADDRSTRLEN ),
			ntohs( sin->sin_port ) );
}

void torrentkino_url( char *url, char **hostname, char **handler,
	char **path ) {

	if( ( *hostname = strstr( url, "://" ) ) != NULL ) {
		*handler = url;
		**hostname = '\0';
		*hostname += 3;
	} else {
		*handler = NULL;
		*hostname = url;
	}

	if( ( *path = strchr( *hostname, '/' ) ) != NULL ) {
		**path = '\0';
		*path += 1;
	} else {
		*path = NULL;
	}
}

int main( int argc, char **argv ) {
	char *hostname = NULL;
	char *handler = NULL;
	char *path = NULL;
	unsigned int port = 0;
	int mode = 0;
	char domain[BUF_SIZE];

	if( argc != 2 ) {
		return 1;
	}
	if( argv == NULL ) {
		return 1;
	}

	torrentkino_url( argv[1], &hostname, &handler, &path );

	/* Get some hints */
	if( !_nss_tk_conf( &port, &mode, domain ) ) {
		fail( "Reading configuration failed" );
	}

	if( !str_valid_hostname( hostname, strlen( hostname ) ) ) {
		fail( "%s is not a valid hostname", hostname );
	}

	if( !str_valid_tld( hostname, strlen( hostname ), domain ) ) {
		fail( "The TLD of %s does not match .%s", hostname, domain );
	}

	if( ! torrentkino_lookup( handler, hostname, path, port, mode ) ) {
		fail( "Looking up %s failed", hostname );
	}

	return 0;
}
