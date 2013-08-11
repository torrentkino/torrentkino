/*
Copyright 2006 Aiko Barz

This file is part of masala/tumbleweed.

masala/tumbleweed is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/tumbleweed is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/tumbleweed.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONF_H
#define CONF_H

#include "main.h"
#include "malloc.h"
#include "file.h"
#ifdef MASALA
#include "sha1.h"
#include "random.h"
#include "hex.h"
#endif
#include "unix.h"
#include "str.h"

#define CONF_CORES 2
#define CONF_PORTMIN 1
#define CONF_PORTMAX 65535

#define CONF_BEQUIET 0
#define CONF_VERBOSE 1

#define CONF_DAEMON 0
#define CONF_FOREGROUND 1

#define CONF_HOSTFILE "/etc/hostname"

#ifdef TUMBLEWEED
#define CONF_USERNAME "tumbleweed"
#define CONF_EPOLL_WAIT 1000
#define CONF_SRVNAME "tumbleweed"
#define CONF_PORT 8080
#define CONF_INDEX_NAME "index.html"
#define CONF_KEEPALIVE 5
#elif MASALA
#define CONF_USERNAME "masala"
#define CONF_EPOLL_WAIT 2000
#define CONF_SRVNAME "masala"
#define CONF_PORT 6881
#define CONF_BOOTSTRAP_NODE "ff0e::1"
#define CONF_BOOTSTRAP_PORT "6881"
#define CONF_PORT_SIZE 5
#define CONF_KEY "open.p2p"
#define CONF_REALM "open.p2p"
#endif

struct obj_conf {
	char username[MAIN_BUF+1];
	char home[MAIN_BUF+1];

#ifdef MASALA
	char hostname[MAIN_BUF+1];
	UCHAR node_id[SHA_DIGEST_LENGTH];
	UCHAR host_id[SHA_DIGEST_LENGTH];
	UCHAR null_id[SHA_DIGEST_LENGTH];
	char bootstrap_node[MAIN_BUF+1];
	char bootstrap_port[CONF_PORT_SIZE+1];
	int announce_port;

	char key[MAIN_BUF+1];
	int bool_encryption;

	char realm[MAIN_BUF+1];
	int bool_realm;
	
	char file[MAIN_BUF+1];
#endif

#ifdef TUMBLEWEED
	char index_name[MAIN_BUF+1];
	int ipv6_only;
#endif

	/* Number of cores */
	int cores;

	/* Verbosity */
	int quiet;

	/* Verbosity mode */
	int mode;

	/* TCP/UDP Port */
	int port;
};

struct obj_conf *conf_init( void );
void conf_free( void );

void conf_check( void );
void conf_write( void );

#endif
