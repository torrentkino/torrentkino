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

#include "tumbleweed.h"
#include "../shr/str.h"
#include "../shr/malloc.h"
#include "../shr/list.h"
#include "node_tcp.h"
#include "../shr/log.h"
#include "../shr/file.h"
#include "../shr/unix.h"
#include "conf.h"

struct obj_conf *conf_init( int argc, char **argv ) {
	struct obj_conf *conf = myalloc( sizeof(struct obj_conf) );
	int opt = 0;
	int i = 0;

	/* Defaults */
	conf->mode = CONF_CONSOLE;
	conf->verbosity = CONF_VERBOSE;
	conf->port = ( getuid() == 0 ) ? PORT_WWW_PRIV : PORT_WWW_USER;
	conf->cores = unix_cpus();
	strncpy( conf->file, CONF_INDEX_NAME, BUF_OFF1 );
	conf_home_from_env( conf );

	/* Arguments */
	while( ( opt = getopt( argc, argv, "dhi:p:q" ) ) != -1 ) {
		switch( opt ) {
			case 'd':
				conf->mode = CONF_DAEMON;
				break;
			case 'h':
				conf_usage( argv[0] );
				break;
			case 'i':
				snprintf( conf->file, BUF_SIZE, "%s", optarg );
				break;
			case 'p':
				conf->port = str_safe_port( optarg );
				break;
			case 'q':
				conf->verbosity = CONF_BEQUIET;
				break;
			default: /* '?' */
				conf_usage( argv[0] );
		}
	}

	/* Get non-option values. */
	for( i=optind; i<argc; i++ ) {
		conf_home_from_arg( conf, argv[i] );
	}

	if( conf->port == 0 ) {
		fail( "Invalid port number (-p)" );
	}

	if( conf->cores < 1 || conf->cores > 128 ) {
		fail( "Invalid number of CPU cores" );
	}

	if( !file_isdir( conf->home ) ) {
		fail( "%s does not exist", conf->home );
	}

	if( !str_isValidFilename( conf->file ) ) {
		fail( "Index %s looks suspicious", conf->file );
	}

	return conf;
}

void conf_free( void ) {
	myfree( _main->conf );
}

void conf_home_from_env( struct obj_conf *conf ) {
	if( getenv( "HOME" ) == NULL || getuid() == 0 ) {
		strncpy( conf->home, "/var/www", BUF_OFF1 );
	} else {
		snprintf( conf->home, BUF_SIZE, "%s/%s", getenv( "HOME"), "Public" );
	}
}

void conf_home_from_arg( struct obj_conf *conf, char *optarg ) {
	char *p = optarg;

	/* Absolute path or relative path */
	if( *p == '/' ) {
		snprintf( conf->home, BUF_SIZE, "%s", p );
	} else if ( getenv( "PWD" ) != NULL ) {
		snprintf( conf->home, BUF_SIZE, "%s/%s", getenv( "PWD" ), p );
	} else {
		strncpy( conf->home, "/var/www", BUF_OFF1 );
	}
}

void conf_print( void ) {
	if ( getenv( "PWD" ) == NULL || getenv( "HOME" ) == NULL ) {
		info( NULL, "# Hint: Reading environment variables failed. sudo?");
		info( NULL, "# This is not a problem. But in some cases it might be useful" );
		info( NULL, "# to use 'sudo -E' to export some variables like $HOME or $PWD." );
	}

	info( NULL, "Workdir: %s", _main->conf->home );
	info( NULL, "Index file: %s (-i)", _main->conf->file );
	info( NULL, "Listen to TCP/%i (-p)", _main->conf->port );

	if( _main->conf->mode == CONF_CONSOLE ) {
		info( NULL, "Mode: Console (-d)" );
	} else {
		info( NULL, "Mode: Daemon (-d)" );
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

void conf_usage( char *command ) {
	fail( "Usage: %s [-d] [-q] [-p port] [-i index] workdir", command );
}
