/*
Copyright 2006 Aiko Barz

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

#ifndef HOSTNAME_H
#define HOSTNAME_H

#include "torrentkino.h"
#include "main.h"
#include "sha1.h"
#include "str.h"
#include "hex.h"
#include "log.h"

typedef struct {
	char hostname[BUF_SIZE];
	UCHAR host_id[SHA1_SIZE];
	time_t time_announce_host;
} HOSTNAME;

LIST *hostname_init( void );
void hostname_free( LIST *l );

void hostname_put( char *hostname, UCHAR *node_id, char *realm, int bool_realm );
void hostname_hostid( UCHAR *host_id, char *hostname, char *realm, int bool );
void hostname_print( void );

#endif /* HOSTNAME_H */
