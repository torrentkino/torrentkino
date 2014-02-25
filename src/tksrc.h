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

#ifndef TK_SHARE_H
#define TK_SHARE_H

#include "ben.h"
#include "malloc.h"
#include "file.h"
#include "str.h"
#include "random.h"

BEN *_nss_tk_load( void );
int _nss_tk_conf( unsigned int *port, int *mode, char *domain );
unsigned int _nss_tk_port( BEN *conf );
int _nss_tk_mode( BEN *conf );
int _nss_tk_domain( BEN *conf, char *domain );

int _nss_tk_socket( int *sockfd, struct sockaddr_in6 *sa, socklen_t *sa_size,
	unsigned int port, int mode );

int _nss_tk_send_lookup( int sockfd,
	struct sockaddr_in6 *sa, socklen_t *sa_size,
	UCHAR *nid, UCHAR *tid, UCHAR *hostname, int hostsize );

ssize_t _nss_tk_read_data( int sockfd,
	struct sockaddr_in6 *sa, socklen_t *sa_size,
	UCHAR *buffer, int bufsize );

BEN *_nss_tk_packet( UCHAR *bencode, int bensize );
BEN *_nss_tk_values( BEN *packet, UCHAR *tid );

UCHAR *_nss_tk_bytes_to_sin6( struct sockaddr_in6 *sin, UCHAR *p );
UCHAR *_nss_tk_bytes_to_sin( struct sockaddr_in *sin, UCHAR *p );

#endif /* TK_SHARE_H */
