/*
Copyright 2010 Aiko Barz

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

#ifndef RESPONSE_H
#define RESPONSE_H

#include "../shr/list.h"

#define RESPONSE_FROM_MEMORY 0
#define RESPONSE_FROM_FILE 1

typedef struct {
	char send_buf[BUF_SIZE];
	int send_size;
	int send_offset;
} R_MEMORY;

typedef struct {
	char filename[BUF_SIZE];
	off_t f_offset;
	off_t f_stop;
} R_FILE;

typedef struct {
	int type;

	union  {
		R_MEMORY memory;
		R_FILE file;
	} data;
} RESPONSE;

LIST *resp_init( void );
void resp_free( LIST *list );

RESPONSE *resp_put( LIST *list, int TYPE );
void resp_del( LIST *list, ITEM *item );

int resp_set_memory( RESPONSE *r, const char *format, ... );

#endif /* RESPONSE_H */
