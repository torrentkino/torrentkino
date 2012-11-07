/*
Copyright 2006 Aiko Barz

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <signal.h>

#ifdef VINEGAR
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
#endif

struct obj_conf *conf_init( void ) {
	struct obj_conf *conf = (struct obj_conf *) myalloc( sizeof( struct obj_conf), "conf_init" );
#ifndef VINEGAR
	char *fbuf = NULL;
	char *p = NULL;
#endif

	conf->mode = CONF_FOREGROUND;

	conf->port = CONF_PORT;

#ifdef VINEGAR
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

	/* SHA1 Hash of hostname */
#ifdef MASALA
	sha1_hash( (unsigned char *)conf->host_id, conf->hostname, strlen( conf->hostname) );
#endif

#ifdef MASALA
	memset( conf->null_id, '\0', SHA_DIGEST_LENGTH );
#endif

#ifdef VINEGAR
	conf->cores = (unix_cpus() > 2) ? unix_cpus() : CONF_CORES;
#elif MASALA
	conf->cores = (unix_cpus() > 2) ? unix_cpus() : CONF_CORES;
#endif

#ifdef VINEGAR
	conf->quiet = CONF_VERBOSE;
#elif MASALA
	conf->quiet = CONF_VERBOSE;
#endif

#ifdef VINEGAR
	strncpy( conf->username, CONF_USERNAME, MAIN_BUF );
#elif MASALA
	strncpy( conf->username, CONF_USERNAME, MAIN_BUF );
#endif

#ifdef MASALA
	snprintf( conf->bootstrap_node, MAIN_BUF+1, "%s", CONF_BOOTSTRAP_NODE );
	snprintf( conf->bootstrap_port, CONF_BOOTSTRAP_PORT_BUF+1, "%s", CONF_BOOTSTRAP_PORT );
	snprintf( conf->key, MAIN_BUF+1, "%s", CONF_KEY );
	conf->encryption = 0;
	
	rand_urandom( conf->risk_id, SHA_DIGEST_LENGTH );
#endif

#ifdef VINEGAR
	snprintf( conf->index_name, MAIN_BUF+1, "%s", CONF_INDEX_NAME );
#endif

	return conf;
}

void conf_free( void ) {
	if( _main->conf != NULL ) {
		myfree( _main->conf, "conf_free" );
	}
}

void conf_check( void ) {
	char buf[MAIN_BUF+1];
#ifdef MASALA
	char hex[HEX_LEN+1];
#endif

#ifdef MASALA
	snprintf( buf, MAIN_BUF+1, "Hostname: %s( -h)", _main->conf->hostname );
	log_info( buf );
#endif

#ifdef MASALA
	hex_encode( hex, _main->conf->host_id );
	snprintf( buf, MAIN_BUF+1, "Host ID: %s", hex );
	log_info( buf );

	hex_encode( hex, _main->conf->risk_id );
	snprintf( buf, MAIN_BUF+1, "Collision ID: %s", hex );
	log_info( buf );
#endif

#ifdef MASALA
	snprintf( buf, MAIN_BUF+1, "Bootstrap Node: %s( -x)", _main->conf->bootstrap_node );
	log_info( buf );
	snprintf( buf, MAIN_BUF+1, "Bootstrap Port: UDP/%s( -y)", _main->conf->bootstrap_port );
	log_info( buf );
#endif

#ifdef VINEGAR
	snprintf( buf, MAIN_BUF+1, "Shared: %s( -s)", _main->conf->home );
	log_info( 0, buf );
	
	if( !file_isdir( _main->conf->home) ) {
		log_fail( "The shared directory does not exist" );
	}
#endif

#ifdef VINEGAR
	snprintf( buf, MAIN_BUF+1, "Index file: %s( -i)", _main->conf->index_name );
	log_info( 0, buf );
	if( !str_isValidFilename( _main->conf->index_name) ) {
		snprintf( buf, MAIN_BUF+1, "%s looks suspicious", _main->conf->index_name );
		log_fail( buf );
	}
#endif

	if( _main->conf->mode == CONF_FOREGROUND ) {
		snprintf( buf, MAIN_BUF+1, "Mode: Foreground( -d)" );
	} else {
		snprintf( buf, MAIN_BUF+1, "Mode: Daemon( -d)" );
	}
#ifdef VINEGAR
	log_info( 0, buf );
#elif MASALA
	log_info( buf );
#endif

	if( _main->conf->quiet == CONF_BEQUIET ) {
		snprintf( buf, MAIN_BUF+1, "Verbosity: Quiet( -q)" );
	} else {
		snprintf( buf, MAIN_BUF+1, "Verbosity: Verbose( -q)" );
	}
#ifdef VINEGAR
	log_info( 0, buf );
#elif MASALA
	log_info( buf );
#endif

#ifdef VINEGAR
	snprintf( buf, MAIN_BUF+1, "Listen to TCP/%i( -p)", _main->conf->port );
	log_info( 0, buf );
#elif MASALA
	snprintf( buf, MAIN_BUF+1, "Listen to UDP/%i( -p)", _main->conf->port );
	log_info( buf );
#endif

	/* Port == 0 => Random source port */
	if( _main->conf->port < CONF_PORTMIN || _main->conf->port > CONF_PORTMAX ) {
		log_fail( "Invalid www port number.( -p)" );
	}

	/* Check bootstrap server port */
#ifndef VINEGAR
	if( str_isSafePort( _main->conf->bootstrap_port) < 0 ) {
		log_fail( "Invalid bootstrap port number.( -y)" );
	}
#endif

	/* Encryption */
#ifdef MASALA
	if( _main->conf->encryption == 1 ) {
		snprintf( buf, MAIN_BUF+1, "Encryption key: %s( -k)", _main->conf->key );
		log_info( buf );
	} else {
		snprintf( buf, MAIN_BUF+1, "Encryption key: None( -k)" );
		log_info( buf );
	}
#endif

	snprintf( buf, MAIN_BUF+1, "Worker threads: %i", _main->conf->cores );
#ifdef VINEGAR
	log_info( 0, buf );
#else
	log_info( buf );
#endif
	if( _main->conf->cores < 1 || _main->conf->cores > 128 ) {
		log_fail( "Invalid core number." );
	}
}
