/*
Copyright 2006 Aiko Barz

This file is part of torrentkino.

torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with torrentkino. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <stdarg.h>

#include "log.h"

void info( IP *from, const char *format, ... ) {
	char log_buf[BUF_SIZE];
	char va_buf[BUF_SIZE];
	va_list vlist;

#ifdef IPV6
	char ip_buf[INET6_ADDRSTRLEN+1];
	memset( ip_buf, '\0', INET6_ADDRSTRLEN+1 );
#elif IPV4
	char ip_buf[INET_ADDRSTRLEN+1];
	memset( ip_buf, '\0', INET_ADDRSTRLEN+1 );
#endif

	if( conf_verbosity() != CONF_VERBOSE ) {
		return;
	}

	va_start( vlist, format );
	vsnprintf( va_buf, BUF_SIZE, format, vlist );
	va_end( vlist );

	if( from != NULL ) {

#ifdef TUMBLEWEED
		snprintf( log_buf, BUF_SIZE, "%s %s",
#ifdef IPV6
			inet_ntop( AF_INET6, &from->sin6_addr, ip_buf, INET6_ADDRSTRLEN ),
#elif IPV4
			inet_ntop( AF_INET, &from->sin_addr, ip_buf, INET_ADDRSTRLEN ),
#endif
			va_buf);
#endif

#ifdef TORRENTKINO
		snprintf( log_buf, BUF_SIZE, "%s %s", va_buf,
#ifdef IPV6
			inet_ntop( AF_INET6, &from->sin6_addr, ip_buf, INET6_ADDRSTRLEN )
#elif IPV4
			inet_ntop( AF_INET, &from->sin_addr, ip_buf, INET_ADDRSTRLEN )
#endif
			);
#endif

	} else {
		strncpy( log_buf, va_buf, BUF_OFF1 );
	}

	/* Console or Syslog */
	if( conf_mode() == CONF_CONSOLE ) {
		printf( "%s\n", log_buf );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER );
		syslog( LOG_INFO, "%s", log_buf );
		closelog();
	}
}
