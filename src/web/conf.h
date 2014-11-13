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

#ifndef CONF_H
#define CONF_H

#include "../shr/config.h"
#include "../shr/malloc.h"
#include "../shr/fail.h"
#include "../shr/file.h"

#include "../shr/unix.h"
#include "../shr/str.h"

struct obj_conf {
	char home[BUF_SIZE];
	char file[BUF_SIZE];
	int cores;
	int verbosity;
	int mode;
	unsigned int port;
};

struct obj_conf *conf_init( int argc, char **argv );
void conf_free( void );

void conf_print( void );
void conf_write( void );

void conf_home_from_env( struct obj_conf *conf );
void conf_home_from_arg( struct obj_conf *conf, char *optarg );
int conf_verbosity( void );
int conf_mode( void );
void conf_usage( char *command );

#endif
