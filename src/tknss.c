/*
Copyright 2011 Aiko Barz

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
#include <errno.h>
#include <nss.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "tknss.h"

enum nss_status _nss_tk_gethostbyname_r( const char *hostname,
		struct hostent *host, char *buffer, size_t buflen, int *errnop,
		int *h_errnop ) {

	return _nss_tk_gethostbyname3_r( hostname, AF_UNSPEC, host,
			buffer, buflen, errnop, h_errnop, NULL, NULL );
}

enum nss_status _nss_tk_gethostbyname2_r( const char *hostname, int af,
		struct hostent *host, char *buffer, size_t buflen, int *errnop,
		int *h_errnop ) {

	return _nss_tk_gethostbyname3_r( hostname, af, host,
			buffer, buflen, errnop, h_errnop, NULL, NULL );
}

enum nss_status _nss_tk_gethostbyname3_r( const char *hostname, int af,
		struct hostent *host, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp ) {

	return _nss_tk_hostent( hostname, strlen( hostname ), af, host,
			buffer, buflen, errnop, h_errnop, ttlp, canonp );
}

enum nss_status _nss_tk_gethostbyname4_r( const char *hostname,
		struct gaih_addrtuple **pat,
		char *buffer, size_t buflen,
		int *errnop, int *h_errnop,
		int32_t *ttlp ) {

	return _nss_tk_gaih_tuple( hostname, strlen( hostname ), pat,
			buffer, buflen,
			errnop, h_errnop,
			ttlp );
}

enum nss_status _nss_tk_hostent( const char *hostname, int hostsize, int af,
		struct hostent *host, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp ) {

	UCHAR *address = NULL;
	char *p_addr = NULL;
	char *p_name = NULL;
	char *p_aliases = NULL;
	char *p_addr_list = NULL;
	char *p_idx = NULL;
	size_t s_total = 0;
	int port = 0;
	int mode = 0;
	int in_addr_size = 0;
	int result = 0;
	int j = 0;

	/* Check */
	if( !_nss_tk_str_valid_hostname( hostname, hostsize ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	if( !_nss_tk_str_valid_tld( hostname, hostsize ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	/* Load config file */
	if( !_nss_tk_conf( &port, &mode ) ) {
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	af = ( mode == 6 ) ? AF_INET6 : AF_INET;

	in_addr_size = (mode == 6) ?
			sizeof( struct in6_addr ) : sizeof( struct in_addr );

	/* 8 addresses max */
	address = myalloc( 8 * in_addr_size * sizeof( char ) );

	/* Ask daemon */
	result = _nss_tk_lookup( hostname, hostsize, address, in_addr_size, port, mode );
	if( result == 0 ) {
		myfree( address );
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	s_total =
			( hostsize + 1 ) * sizeof( char ) +
			sizeof( char* ) +
			result * in_addr_size * sizeof( char ) +
			( result + 1 ) * sizeof( char* );

	if( buflen < s_total ) {
		myfree( address );
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	memset( buffer, '\0', buflen );

	/* Hostname */
	p_name = buffer;
	memcpy( p_name, hostname, hostsize );
	p_idx = p_name + hostsize + 1;

	/* Alias */
	p_aliases = p_idx;
	*(char**) p_aliases = NULL;
	p_idx += sizeof( char* );

	/* Address data */
	p_addr = p_idx;
	memcpy( p_addr, address, result * in_addr_size );
	p_idx += result * in_addr_size;

	/* Address pointer */
	p_addr_list = p_idx;
	p_idx = p_addr;
	for( j=0; j<result; j++ ) {
		((char**) p_addr_list)[j] = p_idx;
		p_idx += in_addr_size;
	}
	((char**) p_addr_list)[result] = NULL;

	/* Finish object */
	host->h_name = p_name;
	host->h_aliases = (char**) p_aliases;
	host->h_addrtype = af;
	host->h_length = in_addr_size;
	host->h_addr_list = (char**) p_addr_list;

	if( ttlp != NULL ) {
		*ttlp = 0;
	}

	if( canonp != NULL ) {
		*canonp = p_name;
	}

	myfree( address );
	return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_tk_gaih_tuple( const char *hostname, int hostsize, struct
		gaih_addrtuple **pat, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp ) {

	UCHAR *address = NULL;
	char *p_name = NULL;
	char *p_idx = NULL;
	UCHAR *p_addr = NULL;
	struct gaih_addrtuple *p_tuple = NULL;
	struct gaih_addrtuple *p_start = NULL;
	size_t s_total = 0;
	int af = AF_INET6;
	int in_addr_size = 0;
	int port = 0;
	int mode = 0;
	int result = 0;
	int j = 0;

	/* Check */
	if( !_nss_tk_str_valid_hostname( hostname, hostsize ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	if( !_nss_tk_str_valid_tld( hostname, hostsize ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	/* Load config file */
	if( !_nss_tk_conf( &port, &mode ) ) {
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	af = ( mode == 6 ) ? AF_INET6 : AF_INET;

	in_addr_size = (mode == 6) ?
			sizeof( struct in6_addr ) : sizeof( struct in_addr );

	/* 8 addresses max */
	address = myalloc( 8 * in_addr_size * sizeof( char ) );

	/* Ask daemon */
	result = _nss_tk_lookup( hostname, hostsize, address, in_addr_size, port, mode );
	if( result == 0 ) {
		myfree( address );
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	s_total =
		( hostsize + 1 ) * sizeof( char ) +
		result * sizeof( struct gaih_addrtuple );

	if( buflen < s_total ) {
		myfree( address );
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	/* Hostname */
	memset( buffer, '\0', buflen );
	p_name = buffer;
	memcpy( p_name, hostname, hostsize );

	/* Object */
	p_idx = p_name + hostsize + 1;
	p_addr = address;
	p_start = ( struct gaih_addrtuple* ) p_idx;
	for( j=0; j<result; j++ ) {
		p_tuple = ( struct gaih_addrtuple* ) p_idx;
		p_tuple->name = p_name;
		p_tuple->family = af;
		memcpy( p_tuple->addr, p_addr, in_addr_size );
		p_tuple->scopeid = 0;

		/* Linked list */
		if( j == result - 1 ) {
			p_tuple->next = NULL;
		} else {
			p_tuple->next = ( struct gaih_addrtuple* )
				(p_idx + sizeof( struct gaih_addrtuple ));
		}

		p_idx += sizeof( struct gaih_addrtuple );
		p_addr += in_addr_size;
	}

	*pat = p_start;

	if( ttlp != NULL ) {
		*ttlp = 0;
	}

	myfree( address );
	return NSS_STATUS_SUCCESS;
}

int _nss_tk_lookup( const char *hostname, int hostsize, UCHAR *address,
		int address_size, int port, int mode ) {

    UCHAR bencode[BUF_SIZE];
    int bensize = 0;
    int sockfd = -1;
    struct sockaddr_in6 sa;
    socklen_t sa_size = sizeof( struct sockaddr_in6 );
    UCHAR tid[TID_SIZE];
    UCHAR nid[SHA1_SIZE];
    BEN *packet = NULL;
    BEN *values = NULL;
    BEN *ip_bin = NULL;
    ITEM *item = NULL;
	int pair_size = ( mode == 6 ) ? 18 : 6;
	int j = 0;
	UCHAR *p = address;

    /* Create transaction id */
    rand_urandom( tid, TID_SIZE );

    /* Create random node id */
    rand_urandom( nid, SHA1_SIZE );

    /* Prepare socket */
    if( !_nss_tk_socket( &sockfd, &sa, &sa_size, port, mode ) ) {
        return 0;
    }

    /* Send request */
    if( !_nss_tk_send_lookup( sockfd, &sa, &sa_size, nid, tid,
        (UCHAR *)hostname, strlen( hostname ) ) ) {
        return 0;
    }

    /* Read reply */
    bensize = _nss_tk_read_data( sockfd, &sa, &sa_size, bencode, BUF_SIZE );
    if( bensize <= 0 ) {
        return 0;
    }

    /* Create packet */
    if( ( packet = _nss_tk_packet( bencode, bensize ) ) == NULL ) {
        return 0;
    }

    /* Get values*/
    if( ( values = _nss_tk_values( packet, tid ) ) == NULL ) {
        ben_free( packet );
        return 0;
    }

	/* Get values */
	item = list_start( values->v.l );
	while( item != NULL && j < 8 ) {
		ip_bin = list_value( item );

		if( !ben_is_str( ip_bin ) || ben_str_i( ip_bin ) != pair_size ) {
			ben_free( packet );
			return 0;
		}

		/* Copy address */
		memcpy( p, ben_str_s( ip_bin ), address_size );

		item = list_next(item);
		p += address_size;
		j++;
	}

	ben_free( packet );
	return j;
}
