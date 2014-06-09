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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#include "torrentkino.h"
#include "log.h"
#include "conf.h"

struct obj_conf *conf_init( int argc, char **argv ) {
	struct obj_conf *conf = myalloc( sizeof( struct obj_conf ) );
	int opt = 0;
	int i = 0;

	/* Defaults */
	conf->mode = CONF_CONSOLE;
	conf->p2p_port = PORT_DHT_DEFAULT;
	conf->dns_port = PORT_DNS_DEFAULT;
	conf->verbosity = CONF_VERBOSE;
	conf->bootstrap_port = PORT_DHT_DEFAULT;
	conf->announce_port = PORT_DHT_DEFAULT;
	conf->cores = unix_cpus();
	conf->bool_realm = FALSE;
#ifdef POLARSSL
	conf->bool_encryption = FALSE;
	memset( conf->key, '\0', BUF_SIZE );
#endif
	memset( conf->null_id, '\0', SHA1_SIZE );
	memset( conf->hostname, '\0', BUF_SIZE );
	strncpy( conf->realm, CONF_REALM, BUF_OFF1 );
	strncpy( conf->bootstrap_node, MULTICAST_DEFAULT, BUF_OFF1 );
	rand_urandom( conf->node_id, SHA1_SIZE );

	/* Arguments */
	while( ( opt = getopt( argc, argv, "a:d:fhk:ln:p:qr:x:y:" ) ) != -1 ) {
		switch( opt ) {
			case 'a':
				conf->announce_port = str_safe_port( optarg );
				break;
			case 'd':
				conf->dns_port = str_safe_port( optarg );
				break;
			case 'f':
				conf->mode = CONF_DAEMON;
				break;
			case 'h':
				conf_usage( argv[0] );
				break;
			case 'k':
#ifdef POLARSSL
				snprintf( conf->key, BUF_SIZE, "%s", optarg );
				conf->bool_encryption = TRUE;
#endif
				break;
			case 'l':
				snprintf( conf->bootstrap_node, BUF_SIZE,
						"%s", BOOTSTRAP_DEFAULT );
				break;
			case 'n':
				sha1_hash( conf->node_id, optarg, strlen( optarg ) );
				break;
			case 'p':
				conf->p2p_port = str_safe_port( optarg );
				break;
			case 'q':
				conf->verbosity = CONF_BEQUIET;
				break;
			case 'r':
				snprintf( conf->realm, BUF_SIZE, "%s", optarg );
				conf->bool_realm = TRUE;
				break;
			case 'x':
				snprintf( conf->bootstrap_node, BUF_SIZE, "%s", optarg );
				break;
			case 'y':
				conf->bootstrap_port = str_safe_port( optarg );
				break;
			default: /* '?' */
				conf_usage( argv[0] );
		}
	}

	/* Get non-option values. Make it possible to announce multiple
	   hostnames later. */
	for( i=optind; i<argc; i++ ) {
		snprintf( conf->hostname, BUF_SIZE, "%s", argv[i] );
	}
	if( strlen( conf->hostname ) <= 0 ) {
		conf_usage( argv[0] );
	}
	if( !str_valid_hostname( conf->hostname, strlen(conf->hostname) ) ) {
		conf_usage( argv[0] );
	}

	if( conf->p2p_port == 0 ) {
		fail( "Invalid P2P port number (-p)" );
	}

	if( conf->dns_port == 0 ) {
		fail( "Invalid DNS port number (-d)" );
	}

	if( conf->dns_port == conf->p2p_port ) {
		fail( "P2P port (-p) and DNS port (-d) must not be the same." );
	}

	if( conf->bootstrap_port == 0 ) {
		fail( "Invalid bootstrap port number (-y)" );
	}

	if( conf->announce_port == 0 ) {
		fail( "Invalid announce port number (-a)" );
	}

	if( conf->cores < 1 || conf->cores > 128 ) {
		fail( "Invalid number of CPU cores" );
	}

	/* Compute host_id. Respect the realm. */
	conf_hostid( conf->host_id, conf->hostname,
		conf->realm, conf->bool_realm );

	/* I don't want to be responsible for myself */
	if( memcmp( conf->host_id, conf->node_id, SHA1_SIZE ) == 0 ) {
		fail( "The host id and the node id must not be the same" );
	}

	return conf;
}

void conf_free( void ) {
	myfree( _main->conf );
}

void conf_usage( char *command ) {
	fail(
		"Usage: %s [-p port] [-r realm] [-d port] [-a port] "
		"[-x server] [-y port] [-n string] [-q] [-l] hostname", 
		command );
}

void conf_hostid( UCHAR *host_id, char *hostname, char *realm, int bool ) {
	UCHAR sha1_buf1[SHA1_SIZE];
	UCHAR sha1_buf2[SHA1_SIZE];
	int j = 0;

	/* The realm influences the way, the lookup hash gets computed */
	if( bool == TRUE ) {
		sha1_hash( sha1_buf1, hostname, strlen( hostname ) );
		sha1_hash( sha1_buf2, realm, strlen( realm ) );

		for( j = 0; j < SHA1_SIZE; j++ ) {
			host_id[j] = sha1_buf1[j] ^ sha1_buf2[j];
		}
	} else {
		sha1_hash( host_id, hostname, strlen( hostname ) );
	}
}

void conf_print( void ) {
	char hex[HEX_LEN];

	info( NULL, "Hostname: %s", _main->conf->hostname );

	if( _main->conf->bool_realm == 1 ) {
		info( NULL, "Realm: %s (-r)", _main->conf->realm );
	} else {
		info( NULL, "Realm: None (-r)" );
	}

#ifdef POLARSSL
	if( _main->conf->bool_encryption == 1 ) {
		info( NULL, "Encryption key: %s (-k)", _main->conf->key );
	} else {
		info( NULL, "Encryption key: None (-k)" );
	}
#endif

	hex_hash_encode( hex, _main->conf->node_id );
	info( NULL, "Node ID: %s", hex );

	hex_hash_encode( hex, _main->conf->host_id );
	info( NULL, "Host ID: %s", hex );

	info( NULL, "P2P daemon is listening to UDP/%i (-p)", _main->conf->p2p_port );
	info( NULL, "DNS daemon is listening to UDP/%i (-d)", _main->conf->dns_port );
	info( NULL, "Bootstrap node: %s (-x/-l)", _main->conf->bootstrap_node );
	info( NULL, "Bootstrap port: UDP/%i (-y)", _main->conf->bootstrap_port );
	info( NULL, "Announced port: %i (-y)", _main->conf->announce_port );

	if( _main->conf->mode == CONF_CONSOLE ) {
		info( NULL, "Mode: Console (-f)" );
	} else {
		info( NULL, "Mode: Daemon (-f)" );
	}

	if( _main->conf->verbosity == CONF_BEQUIET ) {
		info( NULL, "Verbosity: Quiet (-q)" );
	} else {
		info( NULL, "Verbosity: Verbose (-q)" );
	}

	info( NULL, "Cores: %i", _main->conf->cores );
}

int conf_verbosity( void ) {
	return _main->conf->verbosity;
}

int conf_mode( void ) {
	return _main->conf->mode;
}
