/*
Copyright 2010 Aiko Barz

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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/un.h>

#include "malloc.h"
#include "main.h"
#include "str.h"
#include "conf.h"
#include "list.h"
#include "hash.h"
#include "file.h"
#include "opts.h"
#include "unix.h"
#ifdef TUMBLEWEED
#include "node_web.h"
#include "send_web.h"
#include "tcp.h"
#include "mime.h"
#elif MASALA
#include "udp.h"
#include "ben.h"
#include "token.h"
#include "neighbourhood.h"
#include "bucket.h"
#include "search.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "info_hash.h"
#endif
#include "log.h"

/* Global object variables */
struct obj_main *_main = NULL;

/* Init main object */
struct obj_main *main_init( int argc, char **argv ) {
	struct obj_main *_main = (struct obj_main *) myalloc( sizeof(struct obj_main), "main_init" );

	_main->argc = argc;
	_main->argv = argv;

	_main->conf = NULL;
#ifdef TUMBLEWEED
	_main->nodes = NULL;
	_main->mime = NULL;
	_main->tcp  = NULL;
#elif MASALA
	_main->transaction = NULL;
	_main->infohash = NULL;
	_main->token = NULL;
	_main->nbhd = NULL;
	_main->p2p = NULL;
	_main->udp = NULL;
#endif

	/* Server is doing a shutdown if this value changes */
	_main->status = RUMBLE;

	return _main;
}

void main_free( void ) {
	myfree( _main, "main_free" );
}

int main( int argc, char **argv ) {
	/* Init */
	_main = main_init( argc,argv );
#ifdef TUMBLEWEED
	_main->conf = conf_init();
	_main->tcp = tcp_init();
	_main->nodes = nodes_init();
	_main->mime = mime_init();
#else
	_main->conf = conf_init();
#ifdef MASALA
	_main->nbhd = nbhd_init();
	_main->infohash = idb_init();
#endif
	_main->transaction = tdb_init();
	_main->token = tkn_init();
	_main->p2p = p2p_init();
	_main->udp = udp_init();
#endif

	/* Load options */
	opts_load( argc, argv );

	/* Catch SIG INT */
	unix_signal();

	/* Fork daemon */
	unix_fork();

	/* Check configuration */
	conf_check();

#ifdef MASALA
	/* Create first token */
	tkn_put();
#endif

	/* Increase limits */
	unix_limits();

#ifdef TUMBLEWEED
	/* Load mime types */
	mime_load();
	mime_hash();
#endif

	/* Start server */
#ifdef TUMBLEWEED
	tcp_start();
	tcp_stop();
#else
	udp_start();
	udp_stop();
#endif

#ifdef TUMBLEWEED
	mime_free();
	nodes_free();
	tcp_free();
#elif MASALA
	idb_free();
	nbhd_free(_main->nbhd);
	tdb_free();
	tkn_free();
	p2p_free();
	udp_free();
#endif
	conf_free();
	main_free();

	return 0;
}
