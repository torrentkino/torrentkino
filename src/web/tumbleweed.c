/*
Copyright 2010 Aiko Barz

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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/un.h>

#ifdef TORRENTKINO
#include "torrentkino.h"
#include "info_hash.h"
#include "udp.h"
#include "cache.h"
#endif

#ifdef TUMBLEWEED
#include "tumbleweed.h"
#include "conf.h"
#include "node_tcp.h"
#include "send_tcp.h"
#include "tcp.h"
#include "mime.h"
#endif

#include "worker.h"

struct obj_main *_main = NULL;
int status = RUMBLE;

struct obj_main *main_init( int argc, char **argv ) {
	struct obj_main *_main = (struct obj_main *) myalloc( sizeof(struct obj_main) );

	_main->argv = argv;
	_main->argc = argc;

	_main->conf = NULL;
	_main->work = NULL;

#ifdef TORRENTKINO
	_main->transaction = NULL;
	_main->cache = NULL;
	_main->token = NULL;
	_main->nbhd = NULL;
	_main->value = NULL;
	_main->p2p = NULL;
	_main->udp = NULL;
#endif

#ifdef TUMBLEWEED
	_main->node = NULL;
	_main->mime = NULL;
	_main->tcp  = NULL;
#endif

	return _main;
}

void main_free( void ) {
	myfree( _main );
}

int main( int argc, char **argv ) {
	struct sigaction sig_stop;
	struct sigaction sig_time;

	_main = main_init( argc, argv );
	_main->conf = conf_init( argc, argv );
	_main->work = work_init();

#ifdef TORRENTKINO
	_main->nbhd = nbhd_init();
	_main->value = idb_init();
	_main->transaction = tdb_init();
	_main->token = tkn_init();
	_main->p2p = p2p_init();
	_main->udp = udp_init();
	_main->cache = cache_init();
#endif

#ifdef TUMBLEWEED
	_main->tcp = tcp_init();
	_main->node = node_init();
	_main->mime = mime_init();
#endif

	/* Check configuration */
	conf_print();

	/* Catch SIG INT */
	unix_signal( &sig_stop, &sig_time );

	/* Fork daemon */
	unix_fork( _main->conf->mode );

#ifdef TORRENTKINO
	/* Write configuration */
	conf_write();
#endif


#ifdef TORRENTKINO
	/* Create kademlia token */
	tkn_put();
#endif

	/* Increase limits */
	unix_limits( _main->conf->cores, CONF_EPOLL_MAX_EVENTS );

#ifdef TUMBLEWEED
	/* Load mime types */
	mime_load();
	mime_hash();
#endif

#ifdef TUMBLEWEED
	/* Prepare TCP daemon */
	tcp_start();
#endif

#ifdef TORRENTKINO
	/* Prepare UDP daemon */
	udp_start();
#endif

	/* Drop privileges */
	unix_dropuid0();

	/* Start worker threads */
	work_start();

	/* Stop worker threads */
	work_stop();

	/* Stop TCP daemon */
#ifdef TUMBLEWEED
	tcp_stop();
#endif

#ifdef TORRENTKINO
	/* Stop UDP daemon */
	udp_stop();
#endif

#ifdef TUMBLEWEED
	mime_free();
	node_free();
	tcp_free();
#endif

#ifdef TORRENTKINO
	cache_free();
	idb_free();
	nbhd_free();
	tdb_free();
	tkn_free();
	p2p_free();
	udp_free();
#endif

	work_free();
	conf_free();
	main_free();

	return 0;
}
