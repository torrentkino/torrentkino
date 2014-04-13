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
	char opt_hostname[BUF_SIZE];
	char opt_group[BUF_SIZE];

	/* Defaults */
	conf->mode = CONF_CONSOLE;
	conf->port = PORT_DHT_DEFAULT;
	conf->verbosity = CONF_VERBOSE;
	conf->bootstrap_port = PORT_DHT_DEFAULT;
	conf->cores = unix_cpus();
	conf->strict = FALSE;
	conf->bool_group = FALSE;
	conf->bool_realm = FALSE;
#ifdef POLARSSL
	conf->bool_encryption = FALSE;
	memset( conf->key, '\0', BUF_SIZE );
#endif
	memset( conf->null_id, '\0', SHA1_SIZE );
	strncpy( conf->domain, TLD_DEFAULT, BUF_OFF1 );
	strncpy( conf->realm, CONF_REALM, BUF_OFF1 );
	strncpy( conf->bootstrap_node, MULTICAST_DEFAULT, BUF_OFF1 );
	strncpy( opt_hostname, CONF_SRVNAME, BUF_OFF1 );
	strncpy( opt_group, "None", BUF_OFF1 );
	conf_hostname_from_file( opt_hostname );
	conf_home_from_env( conf );
	rand_urandom( conf->node_id, SHA1_SIZE );

	/* Arguments */
	while( ( opt = getopt( argc, argv, "a:d:fg:hk:ln:p:qr:sx:y:" ) ) != -1 ) {
		switch( opt ) {
			case 'a':
				snprintf( opt_hostname, BUF_SIZE, "%s", optarg );
				break;
			case 'd':
				snprintf( conf->domain, BUF_SIZE, "%s", optarg );
				break;
			case 'f':
				conf->mode = CONF_DAEMON;
				break;
			case 'g':
				snprintf( opt_group, BUF_SIZE, "%s", optarg );
				conf->bool_group = TRUE;
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
				conf->port = str_safe_port( optarg );
				break;
			case 'q':
				conf->verbosity = CONF_BEQUIET;
				break;
			case 'r':
				snprintf( conf->realm, BUF_SIZE, "%s", optarg );
				conf->bool_realm = TRUE;
				break;
			case 's':
				conf->strict = TRUE;
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

	/* Put domain and hostname together */
	snprintf( conf->hostname, BUF_SIZE, "%s.%s",
			opt_hostname, conf->domain );

	/* Put domain and group together */
	if( conf->bool_group ) {
		snprintf( conf->groupname, BUF_SIZE, "%s.%s",
				opt_group, conf->domain );
	} else {
		snprintf( conf->groupname, BUF_SIZE, "%s",
				opt_group );
	}

	if( conf->port == 0 ) {
		fail( "Invalid port number (-p)" );
	}

	if( conf->bootstrap_port == 0 ) {
		fail( "Invalid bootstrap port number (-y)" );
	}

	if( conf->cores < 1 || conf->cores > 128 ) {
		fail( "Invalid number of CPU cores" );
	}

	/* Compute host_id. Respect the realm. */
	conf_hostid( conf->host_id, conf->hostname,
		conf->realm, conf->bool_realm );

	/* Compute group_id. Respect the realm. */
	conf_hostid( conf->group_id, conf->groupname,
		conf->realm, conf->bool_realm );

	/* I don't want to be responsible for myself */
	if( memcmp( conf->host_id, conf->node_id, SHA1_SIZE ) == 0 ) {
		fail( "The host id and the node id must not be the same" );
	}

	/* UID dependent configuration file */
	if( getuid() == 0 ) {
		snprintf( conf->file, BUF_SIZE, "%s/%s", conf->home, CONF_FILE );
	} else {
		snprintf( conf->file, BUF_SIZE, "%s/.%s", conf->home, CONF_FILE );
	}

	return conf;
}

void conf_free( void ) {
	myfree( _main->conf );
}

void conf_usage( char *command ) {
	fail(
		"Usage: %s [-q] [-p port] [-a hostname] [-g group] "
		"[-d domain] [-r realm] [-s] [-l] [-x server] [-y port]",
		command );
}

void conf_home_from_env( struct obj_conf *conf ) {

	if( getenv( "HOME" ) == NULL || getuid() == 0 ) {
		strncpy( conf->home, "/etc", BUF_OFF1 );
	} else {
		snprintf( conf->home,  BUF_SIZE, "%s", getenv( "HOME") );
	}

	if( !file_isdir( conf->home ) ) {
		fail( "%s does not exist", conf->home );
	}
}

void conf_hostname_from_file( char *opt_hostname ) {
	char *f = NULL;
	char *p = NULL;

	snprintf( opt_hostname, BUF_SIZE, "%s", CONF_SRVNAME );

	if( ! file_isreg( CONF_HOSTFILE ) ) {
		return;
	}

	f = (char *) file_load( CONF_HOSTFILE, 0, file_size( CONF_HOSTFILE ) );

	if( f == NULL ) {
		return;
	}

	if( ( p = strchr( f, '\n')) != NULL ) {
		*p = '\0';
	}

	snprintf( opt_hostname, BUF_SIZE, "%s", f );

	myfree( f );
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

	if ( getenv( "PWD" ) == NULL || getenv( "HOME" ) == NULL ) {
		info( NULL, "# Hint: Reading environment variables failed. sudo?");
		info( NULL, "# This is not a problem. But in some cases it might be useful" );
		info( NULL, "# to use 'sudo -E' to export some variables like $HOME or $PWD." );
	}

	info( NULL, "Hostname: %s (-a)", _main->conf->hostname );
	info( NULL, "Domain: %s (-d)", _main->conf->domain );
	info( NULL, "Group: %s (-g)", _main->conf->groupname );

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

	if( _main->conf->bool_group ) {
		hex_hash_encode( hex, _main->conf->group_id );
		info( NULL, "Group ID: %s", hex );
	}

	info( NULL, "Listen to UDP/%i (-p)", _main->conf->port );
	info( NULL, "Bootstrap node: %s (-x/-l)", _main->conf->bootstrap_node );
	info( NULL, "Bootstrap port: UDP/%i (-y)", _main->conf->bootstrap_port );
	if( _main->conf->strict ) {
		info( NULL, "Strict mode: Yes (-s)" );
	} else {
		info( NULL, "Strict mode: No (-s)" );
	}

	info( NULL, "Workdir: %s", _main->conf->home );
	info( NULL, "Config file: %s", _main->conf->file );

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

void conf_write( void ) {
	BEN *dict = ben_init( BEN_DICT );
	BEN *key = NULL;
	BEN *val = NULL;
	RAW *raw = NULL;

	/* Port */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_INT );
	ben_str( key, (UCHAR *)"port", 4 );
	ben_int( val, _main->conf->port );
	ben_dict( dict, key, val );

	/* IP mode */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_INT );
	ben_str( key, (UCHAR *)"ip_version", 10 );
#ifdef IPV6
	ben_int( val, 6 );
#elif IPV4
	ben_int( val, 4 );
#endif
	ben_dict( dict, key, val );

	/* Domain */
	key = ben_init( BEN_STR );
	val = ben_init( BEN_STR );
	ben_str( key, (UCHAR *)"domain", 6 );
	ben_str( val, (UCHAR *)_main->conf->domain, strlen( _main->conf->domain ) );
	ben_dict( dict, key, val );

	/* Encode */
	raw = ben_enc( dict );

	/* Write */
	if( !file_write( _main->conf->file, (char *)raw->code, raw->size ) ) {
		fail( "Writing %s failed", _main->conf->file );
	}

	raw_free( raw );
	ben_free( dict );
}

int conf_verbosity( void ) {
	return _main->conf->verbosity;
}

int conf_mode( void ) {
	return _main->conf->mode;
}
