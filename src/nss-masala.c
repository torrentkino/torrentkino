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
#include <errno.h>
#include <nss.h>
#include <netdb.h> 
#include <sys/socket.h>
#include <arpa/inet.h>

#include "nss-masala.h"

enum nss_status _nss_masala_gethostbyname_r( const char *hostname, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop ) {

	return _nss_masala_gethostbyname3_r( hostname, AF_UNSPEC, host,
			buffer, buflen,
			errnop, h_errnop,
			NULL, NULL );
}

enum nss_status _nss_masala_gethostbyname2_r( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop ) {

	return _nss_masala_gethostbyname3_r( hostname, af, host,
			buffer, buflen,
			errnop, h_errnop,
			NULL, NULL );
}

enum nss_status _nss_masala_gethostbyname3_r( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp ) {
	
	int size = strlen( hostname );

	af = (af == AF_UNSPEC) ? AF_INET6 : af;

	if( af != AF_INET6 ) {
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	if( !_nss_masala_valid_hostname( hostname, size ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	if( !_nss_masala_valid_tld( hostname, size ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	return _nss_masala_hostent( hostname, size, af, host, 
			buffer, buflen,
			errnop, h_errnop,
			ttlp, canonp );
}

enum nss_status _nss_masala_gethostbyname4_r( const char *hostname,
		struct gaih_addrtuple **pat,
		char *buffer, size_t buflen,
		int *errnop, int *h_errnop,
		int32_t *ttlp ) {

	int size = strlen( hostname );

	if( !_nss_masala_valid_hostname( hostname, size ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	if( !_nss_masala_valid_tld( hostname, size ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	return _nss_masala_gaih_tuple( hostname, size, pat, buffer,
			buflen, errnop, 
			h_errnop, ttlp );
}

enum nss_status _nss_masala_hostent( const char *hostname, int size, int af,
		struct hostent *host, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp ) {

	UCHAR address[sizeof(struct in6_addr)];
	char *p_addr = NULL;
	char *p_name = NULL;
	char *p_aliases = NULL;
	char *p_addr_list = NULL;
	char *p_idx = NULL;
	size_t s_total = 0;

	s_total = size + 1 + sizeof(char*) + sizeof(struct in6_addr) + 2 * sizeof(char*);
	if( buflen < s_total ) {
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	if( !_nss_masala_lookup( hostname, size, address) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	memset( buffer, '\0', buflen );
	p_name = buffer;
	memcpy( p_name, hostname, size );
	p_idx = p_name + size + 1;

	p_aliases = p_idx;
	*(char**) p_aliases = NULL;
	p_idx += sizeof(char* );

	p_addr = p_idx;
	memcpy( p_addr, address, sizeof(struct in6_addr) );
	p_idx += sizeof(struct in6_addr );

	p_addr_list = p_idx;
	((char**) p_addr_list)[0] = p_addr;
	((char**) p_addr_list)[1] = NULL;
	p_idx += 2*sizeof(char* );

	host->h_name = p_name;
	host->h_aliases = (char**) p_aliases;
	host->h_addrtype = af;
	host->h_length = sizeof(struct in6_addr );
	host->h_addr_list = (char**) p_addr_list;

	if( ttlp != NULL ) {
		*ttlp = 0;
	}

	if( canonp != NULL ) {
		*canonp = p_name;
	}

	return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_masala_gaih_tuple( const char *hostname, int size, struct
		gaih_addrtuple **pat, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp ) {

	UCHAR address[sizeof(struct in6_addr)];
	char *p_name = NULL;
	char *p_idx = NULL;
	struct gaih_addrtuple *p_tuple;
	size_t s_total = 0;

	s_total = size + 1 + sizeof(struct gaih_addrtuple );
	if( buflen < s_total ) {
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	if( !_nss_masala_lookup( hostname, size, address) ) {
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	memset( buffer, '\0', buflen );
	p_name = buffer;
	memcpy( p_name, hostname, size );

	p_idx = p_name + size + 1;
	p_tuple = (struct gaih_addrtuple*) p_idx;
	p_tuple->next = NULL;
	p_tuple->name = p_name;
	p_tuple->family = AF_INET6;
	memcpy( p_tuple->addr, address, sizeof(struct in6_addr) );
	p_tuple->scopeid = 0;

	*pat = p_tuple;

	if( ttlp != NULL ) {
		*ttlp = 0;
	}

	return NSS_STATUS_SUCCESS;
}

int _nss_masala_valid_hostname( const char *hostname, int size ) {

	int i = 0;
	
	for( i=0; i<size; i++ ) {
		if( hostname[i] >= '0' && hostname[i] <= '9' ) {
			continue;
		} else if( hostname[i] >= 'A' && hostname[i] <= 'Z' ) {
			continue;
		} else if( hostname[i] >= 'a' && hostname[i] <= 'z' ) {
			continue;
		} else if( hostname[i] == '-' ) {
			continue;
		} else if( hostname[i] == '_' ) {
			continue;
		} else if( hostname[i] == '.' ) {
			continue;
		} else {
			return 0;
		}
	}

	return 1;
}

int _nss_masala_valid_tld( const char *hostname, int size ) {

	const char *p0 = NULL;
	const char *p1 = NULL;

	/* "x.p2p" */
	if( size < 5 ) {
		return 0;
	}

	/* Jump to the last '.' */
	p0 = hostname;
	while( ( p1 = strchr( p0, '.')) != NULL ) {
		p0 = p1+1;
	}

	/* TLD must be ".p2p" */
	if( strcmp( p0,"p2p") != 0 ) {
		return 0;
	}

	return 1;
}

int _nss_masala_lookup( const char *hostname, int size, UCHAR *address ) {

	IP sa;
	socklen_t salen = sizeof(IP );
	char buffer[MAIN_BUF+1];
	int sockfd = -1;
	int n = 0;
	struct timeval tv;

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
	setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO,( char *)&tv, sizeof(struct timeval) );

	/* Setup IPv6 */
	sa.sin6_family = AF_INET6;
	sa.sin6_port = htons( BOOTSTRAP_PORT );
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

	/* Copy IPv6 address */
	memcpy( address, buffer, 16 );

	return 1;
}
