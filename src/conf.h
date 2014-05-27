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

#ifndef CONF_H
#define CONF_H

#include "torrentkino.h"
#include "main.h"
#include "malloc.h"
#include "fail.h"
#include "file.h"

#include "sha1.h"
#include "random.h"
#include "hex.h"
#include "unix.h"
#include "str.h"
#include "ben.h"

struct obj_conf {
	char hostname[BUF_SIZE];
	char domain[BUF_SIZE];
	char realm[BUF_SIZE];
	char bootstrap_node[BUF_SIZE];
	UCHAR group_id[SHA1_SIZE];
	UCHAR host_id[SHA1_SIZE];
	UCHAR node_id[SHA1_SIZE];
	UCHAR null_id[SHA1_SIZE];
	int cores;
	int verbosity;
	int mode;
	int strict;
	int bool_realm;
	unsigned int port;
	unsigned int bootstrap_port;
	unsigned int announce_port;
#ifdef POLARSSL
	char key[BUF_SIZE];
	int bool_encryption;
#endif
};

struct obj_conf *conf_init( int argc, char **argv );
void conf_free( void );

void conf_usage( char *command );
void conf_print( void );

void conf_home_from_env( struct obj_conf *conf );
void conf_hostname_from_file( char *opt_hostname );
void conf_hostid( UCHAR *host_id, char *hostname, char *realm, int bool );

int conf_verbosity( void );
int conf_mode( void );

#endif
