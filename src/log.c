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
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <syslog.h>
#include <stdarg.h>

#ifdef TUMBLEWEED
#include "tumbleweed.h"
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "thrd.h"
#include "list.h"
#include "node_web.h"
#endif

#ifdef MASALA
#include "malloc.h"
#include "masala-srv.h"
#include "conf.h"
#include "str.h"
#include "thrd.h"
#include "list.h"
#endif

#include "log.h"

void info( IP *c_addr, int code, const char *format, ... ) {
	char ip_buf[INET6_ADDRSTRLEN+1];
	char log_buf[BUF_SIZE];
	char va_buf[BUF_SIZE];
	va_list vlist;

	if( _main->conf->verbosity != CONF_VERBOSE ) {
		return;
	}

	va_start(vlist, format);
	vsnprintf(va_buf, BUF_SIZE, format, vlist);
	va_end(vlist);

	if( c_addr != NULL ) {
		memset( ip_buf, '\0', INET6_ADDRSTRLEN+1 );
#ifdef TUMBLEWEED
		snprintf(log_buf, BUF_SIZE, "%.3li %.3u %s %s",
			list_size( _main->nodes->list ), code,
			inet_ntop( AF_INET6, &c_addr->sin6_addr, ip_buf, INET6_ADDRSTRLEN), 
			va_buf);
#elif MASALA
		snprintf(log_buf, BUF_SIZE, "%s %s", va_buf,
			inet_ntop( AF_INET6, &c_addr->sin6_addr, ip_buf, INET6_ADDRSTRLEN) );
#endif
	} else {
#ifdef TUMBLEWEED
		snprintf(log_buf, BUF_SIZE, "%.3li %.3u ::1 %s",
			list_size( _main->nodes->list ), code, va_buf);
#elif MASALA
		strncpy(log_buf, va_buf, BUF_OFF1);
#endif
	}

	if( _main->conf->mode == CONF_CONSOLE ) {
		printf( "%s\n", log_buf );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER );
		syslog( LOG_INFO, "%s", log_buf );
		closelog();
	}
}
