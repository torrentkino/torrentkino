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
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "thrd.h"
#include "list.h"
#include "node_web.h"
#include "log.h"
#else
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "thrd.h"
#include "list.h"
#include "log.h"
#include "hex.h"
#endif

void log_info( IP *c_addr, int code, const char *format, ... ) {
#ifdef TUMBLEWEED
	int verbosity = (_main->conf->quiet == CONF_BEQUIET && code == 200) ? CONF_BEQUIET : CONF_VERBOSE;
#elif MASALA
	int verbosity = (_main->conf->quiet == CONF_BEQUIET) ? CONF_BEQUIET : CONF_VERBOSE;
#endif
	char ip_buf[INET6_ADDRSTRLEN+1];
	char log_buf[MAIN_BUF+1];
	char va_buf[MAIN_BUF+1];
	va_list vlist;

	if( verbosity != CONF_VERBOSE ) {
		return;
	}

	va_start(vlist, format);
	vsnprintf(va_buf, MAIN_BUF+1, format, vlist);
	va_end(vlist);

	if( c_addr != NULL ) {
		memset( ip_buf, '\0', INET6_ADDRSTRLEN+1 );
#ifdef TUMBLEWEED
		snprintf(log_buf, MAIN_BUF+1, "%.3li %.3u %s %s",
			list_size( _main->nodes->list ), code,
			inet_ntop( AF_INET6, &c_addr->sin6_addr, ip_buf, INET6_ADDRSTRLEN), 
			va_buf);
#elif MASALA
		snprintf(log_buf, MAIN_BUF+1, "%s %s", va_buf,
			inet_ntop( AF_INET6, &c_addr->sin6_addr, ip_buf, INET6_ADDRSTRLEN) );
#endif
	} else {
#ifdef TUMBLEWEED
		snprintf(log_buf, MAIN_BUF+1, "%.3li %.3u ::1 %s",
			list_size( _main->nodes->list ), code, va_buf);
#elif MASALA
		strncpy(log_buf, va_buf, MAIN_BUF);
#endif
	}

	if( _main->conf->mode == CONF_FOREGROUND ) {
		printf( "%s\n", log_buf );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER );
		syslog( LOG_INFO, "%s", log_buf );
		closelog();
	}
}

void log_fail( const char *format, ... ) {
	char va_buf[MAIN_BUF+1];
	va_list vlist;

	va_start(vlist, format);
	vsnprintf(va_buf, MAIN_BUF+1, format, vlist);
	va_end(vlist);

	if( _main->conf->mode == CONF_FOREGROUND ) {
		fprintf( stderr, "%s\n", va_buf );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER|LOG_PERROR );
		syslog( LOG_INFO, "%s", va_buf );
		closelog();
	}
	exit( 1 );
}
