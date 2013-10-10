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
#include "malloc.h"
#include "file.h"
#include "ben.h"

int torrentkino_lookup( const char *hostname, int size ) {
	IP sa;
	socklen_t salen = sizeof( IP );
	UCHAR buffer[BUF_SIZE];
	int sockfd = -1;
	int n = 0;
	struct timeval tv;
	UCHAR *p = NULL;
	int j = 0;
	IP sin;
	char ip_buf[INET6_ADDRSTRLEN+1];
	int port = 0;

	/* Load port from config file */
	port = torrentkino_port();

	memset( &sa, '\0', salen );
	memset( buffer, '\0', BUF_SIZE );

	/* Setup UDP */
	sockfd = socket( AF_INET6, SOCK_DGRAM, 0 );
	if( sockfd < 0 ) {
		return 0;
	}

	/* Set receive timeout */
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof( struct timeval ) );

	/* Setup IPv6 */
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons( port );
	if( !inet_pton( AF_INET6, "::1", &(sa.sin6_addr)) ) {
		return 0;
	}

	n = sendto( sockfd, hostname, size, 0, (struct sockaddr *)&sa, salen );
	if( n != size ) {
		return 0;
	}

	n = recvfrom( sockfd, buffer, BUF_OFF1, 0, (struct sockaddr *)&sa, &salen );
	if( n % 18 != 0 ) {
		return 0;
	}

	p = buffer;
	for( j=0; j<n; j+=18 ) {
		memset( &sin, '\0', sizeof(IP) );
		sin.sin6_family = AF_INET6;

		/* IP */
		memcpy( &sin.sin6_addr, p, 16 );
		p += 16;

		/* Port */
		memcpy( &sin.sin6_port, p, 2 );
		p += 2;

		printf("%s %i\n", inet_ntop( AF_INET6, &sin.sin6_addr, ip_buf,
			INET6_ADDRSTRLEN), ntohs(sin.sin6_port) );
	}

	return 1;
}

int torrentkino_port( void ) {
	char filename[BUF_SIZE];
	int filesize = 0;
	UCHAR *fbuf = NULL;
	BEN *ben = NULL;
	BEN *port = NULL;
	int port_number = 0;

	if( ( getenv( "HOME")) == NULL ) {
		fail("Looking up $HOME failed");
	}

	snprintf( filename, BUF_SIZE, "%s/%s", getenv( "HOME" ), ".torrentkino.conf" );

	if( !file_isreg( filename ) ) {
		fail("%s not found", filename );
	}

	filesize = file_size( filename );
	if( filesize <= 0 ) {
		fail("Config file %s is empty", filename );
	}

	if( ( fbuf = (UCHAR *) file_load( filename, 0, filesize ) ) == NULL ) {
		fail("Reading %s failed", filename );
	}

	if( !ben_validate( fbuf, filesize ) ) {
		fail( "Broken config in %s", filename );
	}

	/* Parse request */
	ben = ben_dec( fbuf, filesize );
	if( ben == NULL ) {
		fail( "Decoding %s failed", filename );
	}
	if( ben->t != BEN_DICT ) {
		fail( "%s does not contain a dictionary", filename );
	}

	port = ben_searchDictStr( ben, "port" );
	if( !ben_is_int( port ) ) {
		fail( "%s does not contain a port number" );
	}

	if( port->v.i < 1 || port->v.i > 65535 ) {
		fail( "%s contains an invalid port number" );
	}

	port_number = port->v.i;

	ben_free( ben );
	myfree( fbuf, "torrentkino_port" );

	return port_number;
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
