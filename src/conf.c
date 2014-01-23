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

#ifdef TUMBLEWEED
#include "tumbleweed.h"
#include "str.h"
#include "malloc.h"
#include "list.h"
#include "node_tcp.h"
#include "log.h"
#include "file.h"
#include "unix.h"
#endif

#ifdef TORRENTKINO
#include "torrentkino.h"
#include "log.h"
#endif

#include "conf.h"

struct obj_conf *conf_init( int argc, char **argv ) {
	struct obj_conf *conf = (struct obj_conf *) myalloc( sizeof(struct obj_conf) );
	BEN *opts = opts_init();
	BEN *value = NULL;

	/* Parse command line */
	opts_load( opts, argc, argv );

	/* Mode */
	if( ben_dict_search_str( opts, "-d" ) != NULL ) {
		conf->mode = CONF_DAEMON;
	} else {
		conf->mode = CONF_CONSOLE;
	}

	/* Verbosity */
	if( ben_dict_search_str( opts, "-v" ) != NULL ) {
		conf->verbosity = CONF_VERBOSE;
	} else if ( ben_dict_search_str( opts, "-q" ) != NULL ) {
		conf->verbosity = CONF_BEQUIET;
	} else {
		/* Be verbose in the console and quiet while running as a daemon. */
		conf->verbosity = ( conf->mode == CONF_CONSOLE ) ?
			CONF_VERBOSE : CONF_BEQUIET;
	}

	/* Port */
	value = ben_dict_search_str( opts, "-p" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		conf->port = atoi( (char *)ben_str_s( value ) );
	} else {
#ifdef TUMBLEWEED
		if( getuid() == 0 ) {
			conf->port = PORT_WWW_PRIV;
		} else {
			conf->port = PORT_WWW_USER;
		}
#elif TORRENTKINO
		conf->port = PORT_DHT_DEFAULT;
#endif
	}
	if( conf->port < CONF_PORTMIN || conf->port > CONF_PORTMAX ) {
		fail( "Invalid port number (-p)" );
	}

	/* Cores */
	conf->cores = unix_cpus();
	if( conf->cores < 1 || conf->cores > 128 ) {
		fail( "Invalid number of CPU cores" );
	}

	/* HOME */
	conf_home( conf, opts );

#ifdef TUMBLEWEED
	/* HTML index */
	value = ben_dict_search_str( opts, "-i" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		snprintf( conf->file, BUF_SIZE, "%s", (char *)ben_str_s( value ) );
	} else {
		snprintf( conf->file, BUF_SIZE, "%s", CONF_INDEX_NAME );
	}
	if( !str_isValidFilename( conf->file ) ) {
		fail( "Index %s looks suspicious", conf->file );
	}
#endif

#ifdef TORRENTKINO
	/* Hostname */
	conf_hostname( conf, opts );

	/* Realm */
	value = ben_dict_search_str( opts, "-r" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		snprintf( conf->realm, BUF_SIZE, "%s", (char *)ben_str_s( value ) );
		conf->bool_realm = TRUE;
	} else {
		snprintf( conf->realm, BUF_SIZE, "%s", CONF_REALM );
		conf->bool_realm = FALSE;
	}

	/* Compute host_id. Respect the realm. */
	conf_hostid( conf->host_id, conf->hostname,
		conf->realm, conf->bool_realm );

	/* Announce this port */
	value = ben_dict_search_str( opts, "-b" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		conf->announce_port = atoi( (char *)ben_str_s( value ) );
	} else {
		if( getuid() == 0 ) {
			conf->announce_port = PORT_WWW_PRIV;
		} else {
			conf->announce_port = PORT_WWW_USER;
		}
	}
	if( conf->announce_port < CONF_PORTMIN || conf->announce_port > CONF_PORTMAX ) {
		fail( "Invalid announce port number. (-a)" );
	}

	/* Lookup replies may enter the cache if the announced port matches mine */
	if( ben_dict_search_str( opts, "-l" ) != NULL ) {
		conf->cache_port_policy = TRUE;
	} else {
		conf->cache_port_policy = FALSE;
	}

	if( getuid() == 0 ) {
		snprintf( conf->file, BUF_SIZE, "%s/%s", conf->home, CONF_FILE );
	} else {
		snprintf( conf->file, BUF_SIZE, "%s/.%s", conf->home, CONF_FILE );
	}

	/* Node ID */
	value = ben_dict_search_str( opts, "-n" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		sha1_hash( conf->node_id, (char *)ben_str_s( value ), ben_str_i( value ) );
	} else {
		rand_urandom( conf->node_id, SHA1_SIZE );
	}

	memset( conf->null_id, '\0', SHA1_SIZE );

	/* Bootstrap node */
	value = ben_dict_search_str( opts, "-x" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		snprintf( conf->bootstrap_node, BUF_SIZE, "%s", (char *)ben_str_s( value ) );
	} else {
		snprintf( conf->bootstrap_node, BUF_SIZE, "%s", CONF_MULTICAST );
	}

	/* Bootstrap port */
	value = ben_dict_search_str( opts, "-y" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		if( str_isSafePort( (char *)ben_str_s( value ) ) != -1 ) {
			conf->bootstrap_port = atoi( (char *)ben_str_s( value ) );
		} else {
			conf->bootstrap_port = PORT_DHT_DEFAULT;
		}
	} else {
		conf->bootstrap_port = PORT_DHT_DEFAULT;
	}

#ifdef POLARSSL
	/* Secret key */
	value = ben_dict_search_str( opts, "-k" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		snprintf( conf->key, BUF_SIZE, "%s", (char *)ben_str_s( value ) );
		conf->bool_encryption = TRUE;
	} else {
		memset( conf->key, '\0', BUF_SIZE );
		conf->bool_encryption = FALSE;
	}
#endif
#endif

	opts_free( opts );

	return conf;
}

void conf_free( void ) {
	myfree( _main->conf );
}

void conf_home( struct obj_conf *conf, BEN *opts ) {
#ifdef TUMBLEWEED
	BEN *value = NULL;
#endif

#ifdef TORRENTKINO
	if( getenv( "HOME" ) == NULL || getuid() == 0 ) {
		strncpy( conf->home, "/etc", BUF_OFF1 );
	} else {
		snprintf( conf->home,  BUF_SIZE, "%s", getenv( "HOME") );
	}
#endif

#ifdef TUMBLEWEED
	value = ben_dict_search_str( opts, "-w" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		char *p = NULL;

		p = (char *)ben_str_s( value );

		/* Absolute path or relative path */
		if( *p == '/' ) {
			snprintf( conf->home, BUF_SIZE, "%s", p );
		} else if ( getenv( "PWD" ) != NULL ) {
			snprintf( conf->home, BUF_SIZE, "%s/%s", getenv( "PWD" ), p );
		} else {
			strncpy( conf->home, "/var/www", BUF_OFF1 );
		}
	} else {
		if( getenv( "HOME" ) == NULL || getuid() == 0 ) {
			strncpy( conf->home, "/var/www", BUF_OFF1 );
		} else {
			snprintf( conf->home, BUF_SIZE, "%s/%s", getenv( "HOME"), "Public" );
		}
	}
#endif

	if( !file_isdir( conf->home ) ) {
		fail( "%s does not exist", conf->home );
	}
}

#ifdef TORRENTKINO
void conf_hostname( struct obj_conf *conf, BEN *opts ) {
	BEN *value = NULL;
	char *f = NULL;
	char *p = NULL;
	
	/* Hostname from args */
	value = ben_dict_search_str( opts, "-a" );
	if( ben_is_str( value ) && ben_str_i( value ) >= 1 ) {
		snprintf( conf->hostname, BUF_SIZE, "%s", (char *)ben_str_s( value ) );
		return;
	} else {
		strncpy( conf->hostname, "bulk.p2p", BUF_OFF1 );
	}

	/* Hostname from file */
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

	snprintf( conf->hostname, BUF_SIZE, "%s.p2p", f );

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
#endif

void conf_print( void ) {
#ifdef TORRENTKINO
	char hex[HEX_LEN];
#endif

	if ( getenv( "PWD" ) == NULL || getenv( "HOME" ) == NULL ) {
		info( NULL, 0, "# Hint: Reading environment variables failed. sudo?");
		info( NULL, 0, "# This is not a problem. But in some cases it might be useful" );
		info( NULL, 0, "# to use 'sudo -E' to export some variables like $HOME or $PWD." );
	}

	info( NULL, 0, "Cores: %i", _main->conf->cores );
	
	if( _main->conf->mode == CONF_CONSOLE ) {
		info( NULL, 0, "Mode: Console (-d)" );
	} else {
		info( NULL, 0, "Mode: Daemon (-d)" );
	}

	if( _main->conf->verbosity == CONF_BEQUIET ) {
		info( NULL, 0, "Verbosity: Quiet (-q/-v)" );
	} else {
		info( NULL, 0, "Verbosity: Verbose (-q/-v)" );
	}

#ifdef TUMBLEWEED
	info( NULL, 0, "Workdir: %s (-w)", _main->conf->home );
#elif TORRENTKINO
	info( NULL, 0, "Workdir: %s", _main->conf->home );
#endif

#ifdef TUMBLEWEED
	info( NULL, 0, "Index file: %s (-i)", _main->conf->file );
#elif TORRENTKINO
	info( NULL, 0, "Config file: %s", _main->conf->file );
#endif

#ifdef TUMBLEWEED
	info( NULL, 0, "Listen to TCP/%i (-p)", _main->conf->port );
#elif TORRENTKINO
	info( NULL, 0, "Listen to UDP/%i (-p)", _main->conf->port );
#endif

#ifdef TORRENTKINO
	info( NULL, 0, "Hostname: %s (-a)", _main->conf->hostname );

	hex_hash_encode( hex, _main->conf->node_id );
	info( NULL, 0, "Node ID: %s", hex );

	hex_hash_encode( hex, _main->conf->host_id );
	info( NULL, 0, "Host ID: %s", hex );

	info( NULL, 0, "Bootstrap node: %s (-x)", _main->conf->bootstrap_node );
	info( NULL, 0, "Bootstrap port: UDP/%i (-y)", _main->conf->bootstrap_port );
	info( NULL, 0, "Announce port: %i (-a)", _main->conf->announce_port );
	if( _main->conf->cache_port_policy ) {
		info( NULL, 0, "Cache port policy: Yes (-l)" );
	} else {
		info( NULL, 0, "Cache port policy: No (-l)" );
	}

	/* Realm */
	if( _main->conf->bool_realm == 1 ) {
		info( NULL, 0, "Realm: %s (-r)", _main->conf->realm );
	} else {
		info( NULL, 0, "Realm: None (-r)" );
	}

	/* Encryption */
#ifdef POLARSSL
	if( _main->conf->bool_encryption == 1 ) {
		info( NULL, 0, "Encryption key: %s (-k)", _main->conf->key );
	} else {
		info( NULL, 0, "Encryption key: None (-k)" );
	}
#endif
#endif
}

#ifdef TORRENTKINO
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

	raw = ben_enc( dict );

	if( !file_write( _main->conf->file, (char *)raw->code, raw->size ) ) {
		fail( "Writing %s failed", _main->conf->file );
	}

	raw_free( raw );
	ben_free( dict );
}
#endif

int conf_verbosity( void ) {
	return _main->conf->verbosity;
}

int conf_mode( void ) {
	return _main->conf->mode;
}
