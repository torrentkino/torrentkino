/*
Copyright 2006 Aiko Barz

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

#ifdef DEBUG
extern unsigned long int myalloc_counter;
extern unsigned long int myfree_counter;

void mem_init( void );
void mem_print( const char *caller );
#endif

void *myalloc( long int size, const char *caller );
void *myrealloc( void *arg, long int size, const char *caller );
void myfree( void *arg, const char *caller );
