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
	char ip_buf[IP_ADDRLEN+1];
	va_list vlist;

	if( conf_verbosity() != CONF_VERBOSE ) {
		return;
	}

	va_start( vlist, format );
	vsnprintf( va_buf, BUF_SIZE, format, vlist );
	va_end( vlist );

	if( from != NULL ) {

#ifdef TUMBLEWEED
		ip_sin_to_string( from, ip_buf );
		snprintf( log_buf, BUF_SIZE, "%s %s", ip_buf, va_buf);
#elif TORRENTKINO
		ip_sin_to_string( from, ip_buf );
		snprintf( log_buf, BUF_SIZE, "%s %s", va_buf, ip_buf );
#endif

	} else {
		strncpy( log_buf, va_buf, BUF_OFF1 );
	}

	/* Console or Syslog */
	if( conf_mode() == CONF_CONSOLE ) {
		printf( "%s\n", log_buf );
	} else {
		openlog( LOG_NAME, LOG_PID|LOG_CONS,LOG_USER );
		syslog( LOG_INFO, "%s", log_buf );
		closelog();
	}
}
