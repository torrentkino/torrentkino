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

#include "masala-srv.h"
#include "info_hash.h"
#include "opts.h"
#include "udp.h"
#include "cache.h"

struct obj_main *_main = NULL;

struct obj_main *main_init( int argc, char **argv ) {
	struct obj_main *_main = (struct obj_main *) myalloc( sizeof(struct obj_main), "main_init" );

	_main->argv = argv;
	_main->argc = argc;

	_main->transaction = NULL;
	_main->cache = NULL;
	_main->token = NULL;
	_main->conf = NULL;
	_main->nbhd = NULL;
	_main->idb = NULL;
	_main->p2p = NULL;
	_main->udp = NULL;

	_main->status = RUMBLE;

	return _main;
}

void main_free( void ) {
	myfree( _main, "main_free" );
}

int main( int argc, char **argv ) {
	_main = main_init( argc, argv );
	_main->conf = conf_init();
	_main->nbhd = nbhd_init();
	_main->idb = idb_init();
	_main->transaction = tdb_init();
	_main->token = tkn_init();
	_main->p2p = p2p_init();
	_main->udp = udp_init();
	_main->cache = cache_init();

	/* Load options */
	opts_load( argc, argv );

	/* Catch SIG INT */
	unix_signal();

	/* Fork daemon */
	unix_fork();

	/* Check configuration */
	conf_check();

	/* Write configuration */
	conf_write();

	/* Create first token */
	tkn_put();

	/* Increase limits */
	unix_limits();

	/* Start server */
	udp_start();
	udp_stop();

	cache_free();
	idb_free();
	nbhd_free();
	tdb_free();
	tkn_free();
	p2p_free();
	udp_free();
	conf_free();
	main_free();

	return 0;
}
