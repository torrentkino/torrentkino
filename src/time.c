/*
Copyright 2012 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
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
#include <semaphore.h>
#include <signal.h>
#include <netdb.h>
#include <openssl/ssl.h>

#include "malloc.h"
#include "thrd.h"
#include "main.h"
#include "str.h"
#include "list.h"
#include "hash.h"
#include "log.h"
#include "conf.h"
#include "file.h"
#include "unix.h"
#include "ben.h"
#include "p2p.h"
#include "time.h"

time_t time_add_1_min(void ) {
	return _main->p2p->time_now.tv_sec + TIME_1_MINUTE;
}

time_t time_add_15_min(void ) {
	return _main->p2p->time_now.tv_sec + TIME_15_MINUTES;
}

time_t time_add_2_min_approx(void ) {
	return _main->p2p->time_now.tv_sec + TIME_1_MINUTE + random() % TIME_2_MINUTES;
}

time_t time_add_5_min_approx(void ) {
	return _main->p2p->time_now.tv_sec + TIME_4_MINUTES + random() % TIME_2_MINUTES;
}
