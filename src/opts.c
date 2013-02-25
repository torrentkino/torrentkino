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
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef TUMBLEWEED
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "hash.h"
#include "list.h"
#include "node_web.h"
#include "log.h"
#include "file.h"
#include "opts.h"
#else
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "hash.h"
#include "list.h"
#include "log.h"
#include "file.h"
#include "opts.h"
#include "sha1.h"
#include "node_p2p.h"
#include "search.h"
#endif

void opts_load( int argc, char **argv ) {
	unsigned int i;

	/* ./program blabla */
	if( argc < 2 ) {
		return;
	}
	if( argv == NULL ) {
		return;
	}

	for( i=0; i<argc; i++ ) {
		if( argv[i] != NULL && argv[i][0] == '-' ) {
			
			/* -x */
			if( strlen( argv[i]) == 2 ) {
				if( i+1 < argc && argv[i+1] != NULL && argv[i+1][0] != '-' ) {
					
					/* -x abc */
					opts_interpreter( argv[i], argv[i+1] );
					i++;
				} else {
					
					/* -x -y => -x */
					opts_interpreter( argv[i], NULL );
				}
			}
		}
	}
}

void opts_interpreter( char *var, char *val ) {
#ifdef TUMBLEWEED
	char *p0 = NULL, *p1 = NULL;

	/* WWW Directory */
	if( strcmp( var, "-s") == 0 && val != NULL && strlen( val ) > 1 ) {
		p0 = val;

		if( *p0 == '/' ) {
			/* Absolute path? */
			snprintf( _main->conf->home, MAIN_BUF+1, "%s", p0 );
		} else {
			/* Relative path? */
			if( ( p1 = getenv( "PWD")) != NULL ) {
				snprintf( _main->conf->home, MAIN_BUF+1, "%s/%s", p1, p0 );
			} else {
				snprintf( _main->conf->home, MAIN_BUF+1, "/notexistant" );
			}
		}
	}

	/* Create HTML index */
	if( strcmp( var, "-i") == 0 && val != NULL && strlen( val ) > 1 ) {
		snprintf( _main->conf->index_name, MAIN_BUF+1, "%s", val );
	}

	/* IPv6 only */
	if( strcmp( var, "-6") == 0 && val == NULL ) {
		_main->conf->ipv6_only = TRUE;
	}

#endif

#ifdef MASALA
	if( strcmp( var, "-x") == 0 && val != NULL && strlen( val ) > 1 ) {
		strncpy( _main->conf->bootstrap_node, val, MAIN_BUF );
	} else if( strcmp( var, "-y") == 0 && val != NULL && strlen( val ) > 1 ) {
		snprintf( _main->conf->bootstrap_port, CONF_BOOTSTRAP_PORT_BUF+1, "%s", val );
	} else if( strcmp( var, "-k") == 0 && val != NULL && strlen( val ) > 1 ) {
		snprintf( _main->conf->key, MAIN_BUF+1, "%s", val );
		_main->conf->encryption = 1;

	} else if( strcmp( var, "-h") == 0 && val != NULL && strlen( val ) > 1 ) {
		snprintf( _main->conf->hostname, MAIN_BUF+1, "%s", val );
		sha1_hash( (UCHAR *)_main->conf->host_id, val, strlen( val ) );

	} else if( strcmp( var, "-q") == 0 && val == NULL ) {
		_main->conf->quiet = CONF_BEQUIET;
	}
#endif

	/* Port number */
	if( strcmp( var, "-p") == 0 && val != NULL ) {
		_main->conf->port = atoi( val );
	} else if( strcmp( var, "-u") == 0 && val != NULL ) {
		snprintf( _main->conf->username, MAIN_BUF+1, "%s", val );
	} else if( strcmp( var, "-d") == 0 && val == NULL ) {
		_main->conf->mode = CONF_DAEMON;
	}
}
