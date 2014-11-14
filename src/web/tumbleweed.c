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

#include "tumbleweed.h"
#include "conf.h"
#include "node_tcp.h"
#include "send_tcp.h"
#include "tcp.h"
#include "mime.h"

#include "worker.h"

struct obj_main *_main = NULL;
struct obj_log *_log = NULL;
int status = RUMBLE;

struct obj_main *main_init( int argc, char **argv ) {
	struct obj_main *_main = (struct obj_main *) myalloc( sizeof(struct obj_main) );

	_main->argv = argv;
	_main->argc = argc;

	_main->conf = NULL;
	_main->work = NULL;

	_main->node = NULL;
	_main->mime = NULL;
	_main->tcp  = NULL;

	_log = NULL;

	return _main;
}

void main_free( void ) {
	myfree( _main );
}

int main( int argc, char **argv ) {
	struct sigaction sig_stop;
	struct sigaction sig_time;

	_main = main_init( argc, argv );
	_log = log_init();
	_main->conf = conf_init( argc, argv );
	_main->work = work_init();

	_main->tcp = tcp_init();
	_main->node = node_init();
	_main->mime = mime_init();

	/* Check configuration */
	conf_print();

	/* Catch SIG INT */
	unix_signal( &sig_stop, &sig_time );

	/* Fork daemon */
	unix_fork( log_console( _log ) );

	/* Increase limits */
	unix_limits( _main->conf->cores, CONF_EPOLL_MAX_EVENTS );

	/* Load mime types */
	mime_load();
	mime_hash();

	/* Prepare TCP daemon */
	tcp_start();

	/* Drop privileges */
	unix_dropuid0();

	/* Start worker threads */
	work_start();

	/* Stop worker threads */
	work_stop();

	/* Stop TCP daemon */
	tcp_stop();

	mime_free();
	node_free();
	tcp_free();

	work_free();
	conf_free();
	log_free( _log );
	main_free();

	return 0;
}
