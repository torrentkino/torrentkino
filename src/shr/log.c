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

LOG *log_init(void)
{
	LOG *log = myalloc(sizeof(LOG));
	log->verbosity = CONF_VERBOSE;
	log->mode = CONF_CONSOLE;
	return log;
}

void log_free(LOG * log)
{
	myfree(log);
}

void log_set_verbosity(LOG * log, int verbosity)
{
	log->verbosity = verbosity;
}

void log_set_mode(LOG * log, int mode)
{
	log->mode = mode;
}

int log_verbosely(LOG * log)
{
	return log->verbosity;
}

int log_console(LOG * log)
{
	return log->mode;
}

void info(LOG * log, IP * from, const char *format, ...)
{
	char log_buf[BUF_SIZE];
	char va_buf[BUF_SIZE];
	char ip_buf[IP_ADDRLEN + 1];
	va_list vlist;
	int result;

	if (!log_verbosely(log)) {
		return;
	}

	va_start(vlist, format);
	vsnprintf(va_buf, BUF_SIZE, format, vlist);
	va_end(vlist);

	if (from != NULL) {

#ifdef TUMBLEWEED
		ip_sin_to_string(from, ip_buf);
		result = snprintf(log_buf, BUF_SIZE, "%s %s", ip_buf, va_buf);
#elif TORRENTKINO
		ip_sin_to_string(from, ip_buf);
		result = snprintf(log_buf, BUF_SIZE, "%s %s", va_buf, ip_buf);
#endif

	} else {
		result = snprintf(log_buf, BUF_SIZE, "%s", va_buf);
	}

	/* -Wformat-truncation */
	if( result >= BUF_SIZE ) {
		return;
	}

	/* Console or Syslog */
	if (log_console(log)) {
		printf("%s\n", log_buf);
	} else {
		openlog(LOG_NAME, LOG_PID | LOG_CONS, LOG_USER);
		syslog(LOG_INFO, "%s", log_buf);
		closelog();
	}
}
