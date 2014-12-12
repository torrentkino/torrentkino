/*
Copyright 2006 Aiko Barz

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

#ifndef LOG_H
#define LOG_H

extern struct obj_log *_log;

#include "malloc.h"
#include "ip.h"

struct obj_log {
	int verbosity;
	int mode;
};
typedef struct obj_log LOG;

LOG *log_init(void);
void log_free(LOG * log);

void log_set_verbosity(LOG * log, int verbosity);
void log_set_mode(LOG * log, int mode);

int log_verbosely(LOG * log);
int log_console(LOG * log);

void info(LOG * log, IP * from, const char *format, ...);

#endif
