/*
Copyright 2011 Aiko Barz

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

#ifndef WORK_H
#define WORK_H

#include "malloc.h"
#include "thrd.h"
#include "log.h"

struct obj_work {
	int number_of_threads;
	int active;
	int id;

	/* Global lock */
	pthread_t **threads;
	pthread_attr_t attr;
	pthread_mutex_t *mutex;

#ifdef TUMBLEWEED
	/* TCP nodes */
	pthread_mutex_t *tcp_node;
#endif
};

struct obj_work *work_init( void );
void work_free( void );

void work_start( void );
void work_stop( void );

#endif
