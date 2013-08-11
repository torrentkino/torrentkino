/*
Copyright 2010 Aiko Barz

This file is part of masala/tumbleweed.

masala/tumbleweed is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/tumbleweed is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/tumbleweed.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "masala-cli.h"

int masala_lookup( const char *hostname, int size ) {
	IP sa;
	socklen_t salen = sizeof( IP );
	UCHAR buffer[MAIN_BUF+1];
	int sockfd = -1;
	int n = 0;
	struct timeval tv;
	UCHAR *p = NULL;
	int j = 0;
	IP sin;
	char ip_buf[INET6_ADDRSTRLEN+1];

	memset( &sa, '\0', salen );
	memset( buffer, '\0', MAIN_BUF+1 );

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
	sa.sin6_port = htons( CONF_PORT );
	if( !inet_pton( AF_INET6, "::1", &(sa.sin6_addr)) ) {
		return 0;
	}

	n = sendto( sockfd, hostname, size, 0, (struct sockaddr *)&sa, salen );
	if( n != size ) {
		return 0;
	}

	n = recvfrom( sockfd, buffer, MAIN_BUF, 0, (struct sockaddr *)&sa, &salen );
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

int main( int argc, char **argv ) {

	if( argc < 2 ) {
		return 1;
	}
	if( argv == NULL ) {
		return 1;
	}

	if( !str_isValidHostname( argv[1], strlen( argv[1] ) ) ) {
		fprintf( stderr, "%s is not a valid hostname\n", argv[1]);
	}

	if( ! masala_lookup( argv[1], strlen( argv[1] ) ) ) {
		fprintf( stderr, "Looking up %s failed\n", argv[1]);
	}
	
	return 0;
}
