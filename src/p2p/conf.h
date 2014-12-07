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

#include "torrentkino.h"
#include "../shr/config.h"
#include "../shr/malloc.h"
#include "../shr/fail.h"
#include "../shr/file.h"

#include "sha1.h"
#include "../shr/random.h"
#include "hex.h"
#include "../shr/unix.h"
#include "../shr/str.h"
#include "ben.h"
#include "time.h"
#include "identity.h"

#define BOOTSTRAP_LOCAL 0
#define BOOTSTRAP_LAZY 1
#define BOOTSTRAP_HOST 2
#define BOOTSTRAP_SIZE 3

struct obj_conf {
	char realm[BUF_SIZE];

	char bootstrap_node[BUF_SIZE];
	int bootstrap_mode;
	char bootstrap_lazy[BOOTSTRAP_SIZE][BUF_SIZE];

	UCHAR node_id[SHA1_SIZE];
	UCHAR null_id[SHA1_SIZE];
	int cores;
	int bool_realm;
	unsigned int p2p_port;
	unsigned int dns_port;
	unsigned int announce_port;
	unsigned int bootstrap_port;
#ifdef POLARSSL
	char key[BUF_SIZE];
	int bool_encryption;
#endif
};
typedef struct obj_conf CONF;

struct obj_conf *conf_init( int argc, char **argv );
void conf_free( void );

void conf_usage( char *command );
void conf_print( void );

#endif
