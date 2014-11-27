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

#ifndef NSS_H
#define NSS_H

#include "../shr/config.h"
#include "../shr/malloc.h"
#include "../shr/str.h"
#include "../dns/client.h"

#ifdef IPV6
#define _nss_tk_gethostbyname_r _nss_tk6_gethostbyname_r
#define _nss_tk_gethostbyname2_r _nss_tk6_gethostbyname2_r
#define _nss_tk_gethostbyname3_r _nss_tk6_gethostbyname3_r
#define _nss_tk_gethostbyname4_r _nss_tk6_gethostbyname4_r
#define _nss_tk_hostent _nss_tk6_hostent
#define _nss_tk_gaih_tuple _nss_tk6_gaih_tuple
#else
#define _nss_tk_gethostbyname_r _nss_tk4_gethostbyname_r
#define _nss_tk_gethostbyname2_r _nss_tk4_gethostbyname2_r
#define _nss_tk_gethostbyname3_r _nss_tk4_gethostbyname3_r
#define _nss_tk_gethostbyname4_r _nss_tk4_gethostbyname4_r
#define _nss_tk_hostent _nss_tk4_hostent
#define _nss_tk_gaih_tuple _nss_tk4_gaih_tuple
#endif

#define _public_ __attribute__( ( visibility( "default")))
#define _hidden_ __attribute__( ( visibility( "hidden")))

enum nss_status _nss_tk_gethostbyname_r( const char *hostname,
		struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop) _public_;

enum nss_status _nss_tk_gethostbyname2_r( const char *hostname, int af,
		struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop) _public_;

enum nss_status _nss_tk_gethostbyname3_r( const char *hostname, int af,
		struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp) _public_;

enum nss_status _nss_tk_gethostbyname4_r( const char *hostname,
		struct gaih_addrtuple **pat,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp) _public_;

enum nss_status _nss_tk_hostent( const char *hostname, int hostsize, int af,
		struct hostent *host, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp );

enum nss_status _nss_tk_gaih_tuple( const char *hostname, int hostsize, struct
		gaih_addrtuple **pat, char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp );


int _nss_tk_lookup( const char *hostname, int hostsize, UCHAR *address,
		int address_size, unsigned int port, int mode );

#endif /* NSS_H */
