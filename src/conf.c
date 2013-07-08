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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <signal.h>

#ifdef TUMBLEWEED
#include "main.h"
#include "str.h"
#include "malloc.h"
#include "list.h"
#include "node_web.h"
#include "log.h"
#include "file.h"
#include "conf.h"
#include "unix.h"
#else
#include "main.h"
#include "str.h"
#include "malloc.h"
#include "list.h"
#include "log.h"
#include "file.h"
#include "conf.h"
#include "sha1.h"
#include "unix.h"
#include "hex.h"
#include "random.h"
#include "ben.h"
#include "hash.h"
#include "token.h"
#include "neighbourhood.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#endif

struct obj_conf *conf_init( void ) {
	struct obj_conf *conf = (struct obj_conf *) myalloc( sizeof(struct obj_conf), "conf_init" );
#ifndef TUMBLEWEED
	char *fbuf = NULL;
	char *p = NULL;
#endif

	conf->mode = CONF_FOREGROUND;

	conf->port = CONF_PORT;

#ifdef TUMBLEWEED
	if( ( getenv( "HOME")) == NULL ) {
		strncpy( conf->home, "/var/www", MAIN_BUF );
	} else {
		snprintf( conf->home, MAIN_BUF+1, "%s/%s", getenv( "HOME"), "Public" );
	}
#endif

#ifdef MASALA
	/* /etc/hostname */
	strncpy( conf->hostname, "bulk.p2p", MAIN_BUF );
	if( file_isreg( CONF_HOSTFILE) ) {
		if( ( fbuf = (char *) file_load( CONF_HOSTFILE, 0, file_size( CONF_HOSTFILE))) != NULL ) {
			if( ( p = strchr( fbuf, '\n')) != NULL ) {
				*p = '\0';
			}
			snprintf( conf->hostname, MAIN_BUF+1, "%s.p2p", fbuf );
			myfree( fbuf, "conf_init" );
		}
	}
#endif

#ifdef MASALA
	snprintf( conf->key, MAIN_BUF+1, "%s", CONF_KEY );
	conf->bool_encryption = FALSE;

	snprintf( conf->realm, MAIN_BUF+1, "%s", CONF_REALM );
	conf->bool_realm = FALSE;
#endif

	/* SHA1 Hash of hostname */
#ifdef MASALA
	sha1_hash( conf->host_id, conf->hostname, strlen( conf->hostname ) );
#endif

#ifdef MASALA
	rand_urandom( conf->node_id, SHA_DIGEST_LENGTH );
#endif

#ifdef MASALA
	memset( conf->null_id, '\0', SHA_DIGEST_LENGTH );
#endif

#ifdef TUMBLEWEED
	conf->cores = (unix_cpus() > 2) ? unix_cpus() : CONF_CORES;
#elif MASALA
	conf->cores = (unix_cpus() > 2) ? unix_cpus() : CONF_CORES;
#endif

#ifdef TUMBLEWEED
	conf->quiet = CONF_BEQUIET;
#elif MASALA
	conf->quiet = CONF_BEQUIET;
#endif

#ifdef TUMBLEWEED
	strncpy( conf->username, CONF_USERNAME, MAIN_BUF );
#elif MASALA
	strncpy( conf->username, CONF_USERNAME, MAIN_BUF );
#endif

#ifdef MASALA
	snprintf( conf->bootstrap_node, MAIN_BUF+1, "%s", CONF_BOOTSTRAP_NODE );
	snprintf( conf->bootstrap_port, CONF_BOOTSTRAP_PORT_BUF+1, "%s", CONF_BOOTSTRAP_PORT );
#endif

#ifdef TUMBLEWEED
	snprintf( conf->index_name, MAIN_BUF+1, "%s", CONF_INDEX_NAME );
#endif

#ifdef TUMBLEWEED
	conf->ipv6_only = FALSE;
#endif

	return conf;
}

void conf_free( void ) {
	if( _main->conf != NULL ) {
		myfree( _main->conf, "conf_free" );
	}
}

void conf_check( void ) {
#ifdef MASALA
	char hex[HEX_LEN];
#endif

#ifdef MASALA
	log_info( NULL, 0, "Hostname: %s (-h)", _main->conf->hostname );
#endif

#ifdef MASALA
	hex_hash_encode( hex, _main->conf->node_id );
	log_info( NULL, 0, "Node ID: %s", hex );

	hex_hash_encode( hex, _main->conf->host_id );
	log_info( NULL, 0, "Host ID: %s", hex );
#endif

#ifdef MASALA
	log_info( NULL, 0, "Bootstrap Node: %s (-x)", _main->conf->bootstrap_node );
	log_info( NULL, 0, "Bootstrap Port: UDP/%s (-y)",
		_main->conf->bootstrap_port );
#endif

#ifdef TUMBLEWEED
	log_info( NULL, 0, "Shared: %s (-s)", _main->conf->home );
	
	if( !file_isdir( _main->conf->home) ) {
		log_fail( "The shared directory does not exist" );
	}
#endif

#ifdef TUMBLEWEED
	log_info( NULL, 0, "Index file: %s (-i)", _main->conf->index_name );
	if( !str_isValidFilename( _main->conf->index_name ) ) {
		log_fail( "%s looks suspicious", _main->conf->index_name );
	}
#endif

	if( _main->conf->mode == CONF_FOREGROUND ) {
		log_info( NULL, 0, "Mode: Foreground (-d)" );
	} else {
		log_info( NULL, 0, "Mode: Daemon (-d)" );
	}

	if( _main->conf->quiet == CONF_BEQUIET ) {
		log_info( NULL, 0, "Verbosity: Quiet (-v)" );
	} else {
		log_info( NULL, 0, "Verbosity: Verbose (-v)" );
	}

#ifdef TUMBLEWEED
	log_info( NULL, 0, "Listen to TCP/%i (-p)", _main->conf->port );
#elif MASALA
	log_info( NULL, 0, "Listen to UDP/%i (-p)", _main->conf->port );
#endif

	/* Port == 0 => Random source port */
	if( _main->conf->port < CONF_PORTMIN || _main->conf->port > CONF_PORTMAX ) {
		log_fail( "Invalid www port number. (-p)" );
	}

	/* Check bootstrap server port */
#ifndef TUMBLEWEED
	if( str_isSafePort( _main->conf->bootstrap_port) < 0 ) {
		log_fail( "Invalid bootstrap port number. (-y)" );
	}
#endif

	/* Encryption */
#ifdef MASALA
	if( _main->conf->bool_encryption == 1 ) {
		log_info( NULL, 0, "Encryption key: %s (-k)", _main->conf->key );
	} else {
		log_info( NULL, 0, "Encryption key: None (-k)" );
	}
#endif

	/* Realm */
#ifdef MASALA
	if( _main->conf->bool_realm == 1 ) {
		log_info( NULL, 0, "Realm: %s (-r)", _main->conf->realm );
	} else {
		log_info( NULL, 0, "Realm: None (-r)" );
	}
#endif



	log_info( NULL, 0, "Worker threads: %i", _main->conf->cores );
	if( _main->conf->cores < 1 || _main->conf->cores > 128 ) {
		log_fail( "Invalid core number." );
	}
}
