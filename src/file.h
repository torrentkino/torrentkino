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

#ifndef FILE_H
#define FILE_H

#include "main.h"
#include "malloc.h"

#ifdef NSS
#define file_append _nss_tk_file_append
#define file_isdir _nss_tk_file_isdir
#define file_islink _nss_tk_file_islink
#define file_isreg _nss_tk_file_isreg
#define file_load _nss_tk_file_load
#define file_mkdir _nss_tk_file_mkdir
#define file_mod _nss_tk_file_mod
#define file_size _nss_tk_file_size
#define file_write _nss_tk_file_write
#endif

int file_isdir( const char *dirname );
int file_isreg( const char *filename );
int file_islink( const char *filename );
int file_mkdir( const char *dirname );

size_t file_size( const char *filename );
time_t file_mod( const char *filename );

char *file_load( const char *filename, long int offset, size_t size );
int file_write( const char *filename, char *buffer, size_t size );
size_t file_append( const char *filename, char *buffer, size_t size );

#if 0
int file_rm( const char *filename );
int file_rmdir( const char *dirname );
int file_rmrf( char *filename );
#endif

#endif
