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

#ifndef MALLOC_H
#define MALLOC_H

#include "main.h"

#ifdef NSS
#define myalloc _nss_tk_myalloc
#define myfree _nss_tk_myfree
#define myrealloc _nss_tk_myrealloc
#endif

void *myalloc( long int size );
void *myrealloc( void *arg, long int size );
void myfree( void *arg );

#endif /* MALLOC_H */
