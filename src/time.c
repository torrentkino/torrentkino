/*
Copyright 2012 Aiko Barz

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
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>

#include "malloc.h"
#include "thrd.h"
#include "torrentkino.h"
#include "str.h"
#include "list.h"
#include "hash.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "unix.h"
#include "ben.h"
#include "token.h"
#include "neighbourhood.h"
#include "lookup.h"
#include "transaction.h"
#include "p2p.h"
#include "time.h"

void time_add_1_min( time_t *time ) {
	*time = _main->p2p->time_now.tv_sec + 60;
}

void time_add_30_min( time_t *time ) {
	*time = _main->p2p->time_now.tv_sec + 1800;
}

void time_add_5_sec_approx( time_t *time ) {
	*time = _main->p2p->time_now.tv_sec + 4 + random() % 3;
}

void time_add_1_min_approx( time_t *time ) {
	*time = _main->p2p->time_now.tv_sec + 50 + random() % 20;
}

void time_add_5_min_approx( time_t *time ) {
	*time = _main->p2p->time_now.tv_sec + 240 + random() % 120;
}
