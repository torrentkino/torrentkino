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

#include "main.h"
#include "malloc.h"
#include "fail.h"
#include "file.h"
#include "opts.h"


#ifdef TORRENTKINO
#include "sha1.h"
#include "random.h"
#include "hex.h"
#endif
#include "unix.h"
#include "str.h"
#include "ben.h"

#define CONF_CORES 2
#define CONF_PORTMIN 1
#define CONF_PORTMAX 65535

#define CONF_BEQUIET 1
#define CONF_VERBOSE 2

#define CONF_DAEMON 0
#define CONF_CONSOLE 1

#define CONF_HOSTFILE "/etc/hostname"

#ifdef TUMBLEWEED
#define CONF_USERNAME "tumbleweed"
#define CONF_EPOLL_WAIT 1000
#define CONF_SRVNAME "tumbleweed"
#define CONF_PORT 8080
#define CONF_INDEX_NAME "index.html"
#define CONF_KEEPALIVE 5
#endif

#if TORRENTKINO
#define CONF_USERNAME "torrentkino"
#define CONF_EPOLL_WAIT 2000
#define CONF_SRVNAME "torrentkino"
#define CONF_PORT 6881
#define CONF_ANNOUNCED_PORT 8080
#define CONF_BOOTSTRAP_NODE "ff0e::1"
#define CONF_PORT_SIZE 5
#define CONF_REALM "open.p2p"
#endif

#define CONF_EPOLL_MAX_EVENTS 32

struct obj_conf {
	char username[BUF_SIZE];
	char home[BUF_SIZE];

#ifdef TORRENTKINO
	char hostname[BUF_SIZE];
	UCHAR node_id[SHA1_SIZE];
	UCHAR host_id[SHA1_SIZE];
	UCHAR null_id[SHA1_SIZE];
	char bootstrap_node[BUF_SIZE];
	char bootstrap_port[CONF_PORT_SIZE+1];
	int announce_port;

#ifdef POLARSSL
	char key[BUF_SIZE];
	int bool_encryption;
#endif

	char realm[BUF_SIZE];
	int bool_realm;
	
	char file[BUF_SIZE];
#endif

#ifdef TUMBLEWEED
	char index_name[BUF_SIZE];
	int ipv6_only;
#endif

	int cores;
	int verbosity;
	int mode;
	int port;
};

struct obj_conf *conf_init( int argc, char **argv );
void conf_free( void );

void conf_print( void );
void conf_write( void );

void conf_home( struct obj_conf *conf, BEN *opts );
#ifdef TORRENTKINO
void conf_hostname( struct obj_conf *conf, BEN *opts );
void conf_hostid( UCHAR *host_id, char *hostname, char *realm, int bool );
#endif

#endif
