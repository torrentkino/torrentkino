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

enum nss_status _nss_tk_gethostbyname_r( const char *hostname, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop ) {

	return _nss_tk_gethostbyname3_r( hostname, AF_UNSPEC, host,
			buffer, buflen,
			errnop, h_errnop,
			NULL, NULL );
}

enum nss_status _nss_tk_gethostbyname2_r( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop ) {

	return _nss_tk_gethostbyname3_r( hostname, af, host,
			buffer, buflen,
			errnop, h_errnop,
			NULL, NULL );
}

enum nss_status _nss_tk_gethostbyname3_r( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp ) {

	return _nss_tk_hostent( hostname, strlen( hostname ), af, host, 
			buffer, buflen,
			errnop, h_errnop,
			ttlp, canonp );
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

enum nss_status _nss_tk_hostent( const char *hostname, int size, int af,
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
	BEN *conf = NULL;
	int in_addr_size = 0;

	/* Check */
	if( !_nss_tk_valid_hostname( hostname, size ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	if( !_nss_tk_valid_tld( hostname, size ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	/* Load port from config file */
	if( ( conf = _nss_tk_conf() ) == NULL ) {
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	port = _nss_tk_port( conf );
	if( port == -1 ) {
		ben_free( conf );
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	mode = _nss_tk_mode( conf );
	if( mode == -1 ) {
		ben_free( conf );
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	ben_free( conf );

	af = ( mode == 6 ) ? AF_INET6 : AF_INET;

	in_addr_size = (mode == 6) ?
			sizeof( struct in6_addr ) : sizeof( struct in_addr );

	s_total = size + 1 + sizeof(char*) + in_addr_size + 2 * sizeof(char*);

	if( buflen < s_total ) {
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	address = myalloc(in_addr_size);

	if( !_nss_tk_lookup( hostname, size, address, in_addr_size, port, mode ) ) {
		myfree( address );
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
	memcpy( p_addr, address, in_addr_size );
	p_idx += in_addr_size;

	p_addr_list = p_idx;
	((char**) p_addr_list)[0] = p_addr;
	((char**) p_addr_list)[1] = NULL;
	p_idx += 2*sizeof(char* );

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
	struct gaih_addrtuple *p_tuple;
	size_t s_total = 0;
	int af = AF_INET6;
	int in_addr_size = 0;
	BEN *conf = NULL;
	int port = 0;
	int mode = 0;

	/* Check */
	if( !_nss_tk_valid_hostname( hostname, hostsize ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	if( !_nss_tk_valid_tld( hostname, hostsize ) ) {
		*errnop = ENOENT;
		*h_errnop = HOST_NOT_FOUND;
		return NSS_STATUS_NOTFOUND;
	}

	/* Load port from config file */
	if( ( conf = _nss_tk_conf() ) == NULL ) {
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	port = _nss_tk_port( conf );
	if( port == -1 ) {
		ben_free( conf );
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	mode = _nss_tk_mode( conf );
	if( mode == -1 ) {
		ben_free( conf );
		*errnop = EAFNOSUPPORT;
		*h_errnop = NO_DATA;
		return NSS_STATUS_UNAVAIL;
	}

	ben_free( conf );

	af = ( mode == 6 ) ? AF_INET6 : AF_INET;

	in_addr_size = (mode == 6) ?
			sizeof( struct in6_addr ) : sizeof( struct in_addr );

	s_total = hostsize + 1 + sizeof(struct gaih_addrtuple );
	if( buflen < s_total ) {
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	address = myalloc(in_addr_size);

	if( !_nss_tk_lookup( hostname, hostsize, address, in_addr_size,
			port, mode ) ) {
		myfree( address );
		*errnop = ENOMEM;
		*h_errnop = NO_RECOVERY;
		return NSS_STATUS_TRYAGAIN;
	}

	memset( buffer, '\0', buflen );
	p_name = buffer;
	memcpy( p_name, hostname, hostsize );

	p_idx = p_name + hostsize + 1;
	p_tuple = (struct gaih_addrtuple*) p_idx;
	p_tuple->next = NULL;
	p_tuple->name = p_name;
	p_tuple->family = af;
	memcpy( p_tuple->addr, address, in_addr_size );
	p_tuple->scopeid = 0;

	*pat = p_tuple;

	if( ttlp != NULL ) {
		*ttlp = 0;
	}

	myfree( address );

	return NSS_STATUS_SUCCESS;
}

int _nss_tk_lookup( const char *hostname, int hostsize, UCHAR *address,
		int address_size, int port, int mode ) {

	UCHAR buffer[BUF_SIZE];
	int pair_size = ( mode == 6 ) ? 18 : 6;
	int n = 0;

	n = _nss_tk_connect( hostname, hostsize, buffer, BUF_SIZE, port, mode );
	if( n == 0 ) {
		return 0;
	}

	if( n % pair_size != 0 ) {
		return 0;
	}

	/* Copy IPv6 address */
	memcpy( address, buffer, address_size );

	return 1;
}
