/*
Copyright 2011 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _public_ __attribute__( ( visibility( "default")))
#define _hidden_ __attribute__( ( visibility( "hidden")))

#define MAIN_BUF 1024
#define BOOTSTRAP_PORT 8337

typedef unsigned char UCHAR;

enum nss_status _nss_masala_gethostbyname_r( const char *hostname, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop) _public_;

enum nss_status _nss_masala_gethostbyname2_r( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop) _public_;

enum nss_status _nss_masala_gethostbyname3_r( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp) _public_;

enum nss_status _nss_masala_gethostbyname4_r( const char *hostname, struct gaih_addrtuple **pat,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp) _public_;

enum nss_status _nss_masala_hostent( const char *hostname, int af, struct hostent *host,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp, char **canonp );

enum nss_status _nss_masala_gaih_tuple( const char *hostname, struct gaih_addrtuple **pat,
		char *buffer, size_t buflen, int *errnop,
		int *h_errnop, int32_t *ttlp );

int _nss_masala_valid_tld( const char *hostname );
int _nss_masala_valid_hostname( const char *hostname );

int _nss_masala_lookup( const char *hostname, unsigned char *address );
