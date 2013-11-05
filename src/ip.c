/*
Copyright 2013 Aiko Barz

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

#include "ip.h"

int ip_is_localhost( IP *from ) {
#ifdef IPV6
	const UCHAR localhost[] = 
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

	if( memcmp( &from->sin6_addr.s6_addr, localhost, IP_SIZE ) == 0 ) {
		return TRUE;
	}
#elif IPV4
	const UCHAR localhost[] = 
		{ 0x7f, 0, 0, 1 };

	if( memcmp( &from->sin_addr.s_addr, localhost, IP_SIZE ) == 0 ) {
		return TRUE;
	}
#endif

	return FALSE;
}

/* In the Internet Protocol Version 6 (IPv6), the address block fe80::/10 has
 * been reserved for link-local unicast addressing.[2] The actual link local
 * addresses are assigned with the prefix fe80::/64. */
int ip_is_linklocal( IP *from ) {
#ifdef IPV6
	const UCHAR linklocal[] = 
		{ 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	if( memcmp(from->sin6_addr.s6_addr, linklocal, 8) == 0 ) {
		return TRUE;
	}
#endif

	return FALSE;
}

UCHAR *ip_bytes_to_sin( IP *sin, UCHAR *p ) {
	memset( sin, '\0', sizeof(IP) );
#ifdef IPV6
	sin->sin6_family = AF_INET6;
	memcpy( &sin->sin6_addr, p, IP_SIZE ); p += IP_SIZE;
	memcpy( &sin->sin6_port, p, 2 ); p += 2;
#elif IPV4
	sin->sin_family = AF_INET;
	memcpy( &sin->sin_addr, p, IP_SIZE ); p += IP_SIZE;
	memcpy( &sin->sin_port, p, 2 ); p += 2;
#endif

	return p;
}

UCHAR *ip_sin_to_bytes( IP *sin, UCHAR *p ) {
		/* IP + Port */
#ifdef IPV6
		memcpy( p, (UCHAR *)&sin->sin6_addr, IP_SIZE ); p += IP_SIZE;
		memcpy( p, (UCHAR *)&sin->sin6_port, 2 ); p += 2;
#elif IPV4
		memcpy( p, (UCHAR *)&sin->sin_addr, IP_SIZE ); p += IP_SIZE;
		memcpy( p, (UCHAR *)&sin->sin_port, 2 ); p += 2;
#endif

	return p;
}

void ip_merge_port_to_sin( IP *sin, int port ) {
#ifdef IPV6
	sin->sin6_port = htons( port );
#elif IPV4
	sin->sin_port = htons( port );
#endif
}
