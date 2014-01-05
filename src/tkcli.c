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
		const char *path ) {
	UCHAR buffer[BUF_SIZE];
	int n = 0, j = 0;
	UCHAR *p = NULL;
	struct sockaddr_in6 sin6;
	struct sockaddr_in sin;
	int pair_size = 0;
	int port = 0;
	int mode = 0;

	/* Load config file */
	if( !_nss_tk_conf( &port, &mode ) ) {
		return FALSE;
	}

	n = _nss_tk_connect( hostname, strlen( hostname ), buffer, BUF_SIZE, port,
		mode );
	if( n == 0 ) {
		return FALSE;
	}

	/* IPv6: 16+2 byte */
	/* IPv4:  4+2 byte */
	pair_size = ( mode == 6 ) ? 18 : 6;

	if( n % pair_size != 0 ) {
		return FALSE;
	}

	p = buffer;
	for( j=0; j<n; j+=pair_size ) {

		if( handler != NULL && *handler != '\0' ) {
			printf( "%s://", handler );
		}

		if( mode == 6 ) {
			p = _nss_tk_bytes_to_sin6( &sin6, p );
			torrenkino_print6( &sin6, handler, path );
		} else {
			p = _nss_tk_bytes_to_sin( &sin, p );
			torrenkino_print( &sin, handler, path );
		}

		if( path != NULL && *path != '\0' ) {
			printf( "/%s\n", path );
		} else {
			printf( "\n" );
		}
	}

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

	if( argc != 2 ) {
		return 1;
	}
	if( argv == NULL ) {
		return 1;
	}

	torrentkino_url( argv[1], &hostname, &handler, &path );

	if( !str_valid_hostname( hostname, strlen( hostname ) ) ) {
		fail( "%s is not a valid hostname", hostname );
	}

	if( !str_valid_tld( hostname, strlen( hostname ) ) ) {
		fail( "%s must end with .p2p", hostname );
	}

	if( ! torrentkino_lookup( handler, hostname, path ) ) {
		fail( "Looking up %s failed", hostname );
	}

	return 0;
}
