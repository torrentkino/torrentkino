/*
Copyright 2009 Aiko Barz

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

#ifndef THRD_H
#define THRD_H

sem_t *thrd_init( const char *semname );
void thrd_destroy( sem_t *mutex );
void thrd_block( sem_t *mutex );
void thrd_unblock( sem_t *mutex );

pthread_mutex_t *mutex_init( void );
void mutex_destroy( pthread_mutex_t *mutex );
void mutex_block( pthread_mutex_t *mutex );
void mutex_unblock( pthread_mutex_t *mutex );

pthread_cond_t *cond_init( void );
void cond_destroy( pthread_cond_t *cond );

#endif
