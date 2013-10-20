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

BEN *_nss_tk_conf( void );
int _nss_tk_port( BEN *conf );
int _nss_tk_mode( BEN *conf );

int _nss_tk_connect( const char *hostname, int hostsize, 
	UCHAR *reply, int reply_size, int port, int mode );

UCHAR *_nss_tk_convert_to_sin6( struct sockaddr_in6 *sin, UCHAR *p );
UCHAR *_nss_tk_convert_to_sin( struct sockaddr_in *sin, UCHAR *p );

#endif /* TK_SHARE_H */
