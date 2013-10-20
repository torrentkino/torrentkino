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

int torrentkino_lookup( const char *hostname, int size ) {
	UCHAR buffer[BUF_SIZE];
	int n = 0, j = 0;
	UCHAR *p = NULL;
	BEN *conf = NULL;
	struct sockaddr_in6 sin6;
	struct sockaddr_in sin;
	int pair_size = 0;
	int port = 0;
	int mode = 0;

	/* Load port from config file */
	if( ( conf = _nss_tk_conf() ) == NULL ) {
		fail( "Loading configuration failed" );
	}
	port = _nss_tk_port( conf );
	if( port == -1 ) {
		fail( "Invalid port number" );
	}
	mode = _nss_tk_mode( conf );
	if( mode == -1 ) {
		fail( "Invalid IP version" );
	}
	ben_free( conf );

	n = _nss_tk_connect( hostname, size, buffer, BUF_SIZE, port, mode );
	if( n == 0 ) {
		fail( "Talking to the daemon failed" );
	}
	
	/* IPv6: 16+2 byte */
	/* IPv4:  4+2 byte */
	pair_size = ( mode == 6 ) ? 18 : 6;

	if( n % pair_size != 0 ) {
		fail( "Broken data from server" );
	}

	p = buffer;
	for( j=0; j<n; j+=pair_size ) {
		if( mode == 6 ) {
			p = _nss_tk_convert_to_sin6( &sin6, p );
			torrenkino_print6( &sin6 );
		} else {
			p = _nss_tk_convert_to_sin( &sin, p );
			torrenkino_print( &sin );
		}
	}

	return 1;
}

void torrenkino_print6( struct sockaddr_in6 *sin ) {
	char ip_buf[INET6_ADDRSTRLEN+1];
	memset( ip_buf, '\0', INET6_ADDRSTRLEN+1);
	printf("%s %i\n", 
			inet_ntop( AF_INET6, &sin->sin6_addr, ip_buf, INET6_ADDRSTRLEN ),
			ntohs( sin->sin6_port ) );
}

void torrenkino_print( struct sockaddr_in *sin ) {
	char ip_buf[INET_ADDRSTRLEN+1];
	memset( ip_buf, '\0', INET_ADDRSTRLEN+1);
	printf("%s %i\n", 
			inet_ntop( AF_INET, &sin->sin_addr, ip_buf, INET_ADDRSTRLEN ), 
			ntohs( sin->sin_port ) );
}

int main( int argc, char **argv ) {
	if( argc < 2 ) {
		return 1;
	}
	if( argv == NULL ) {
		return 1;
	}

	if( !str_isValidHostname( argv[1], strlen( argv[1] ) ) ) {
		fail( "%s is not a valid hostname", argv[1]);
	}

	if( ! torrentkino_lookup( argv[1], strlen( argv[1] ) ) ) {
		fail( "Looking up %s failed", argv[1]);
	}

	return 0;
}
