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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include "main.h"
#include "malloc.h"
#include "str.h"
#include "list.h"
#include "file.h"

int file_isdir( const char *dirname ) {
	struct stat statbuf;

	if( lstat( dirname, &statbuf) == 0 && S_ISDIR( statbuf.st_mode) )
		return 1;

	return 0;
}

int file_isreg( const char *filename ) {
	struct stat statbuf;

	if( lstat( filename, &statbuf) == 0 && S_ISREG( statbuf.st_mode) )
		return 1;
	
	return 0;
}

int file_islink( const char *filename ) {
	struct stat statbuf;

	if( lstat( filename, &statbuf) == 0 && S_ISLNK( statbuf.st_mode) )
		return 1;
	
	return 0;
}

int file_mkdir( const char *dirname ) {
	if( !file_isdir( dirname) ) {
		if( mkdir( dirname, S_IRUSR|S_IWUSR|S_IXUSR) == 0 ) {
			return 1;
		}
	} else {
		return 1;
	}
			
	return 0;
}

size_t file_size( const char *filename ) {
	struct stat statbuf;

	if( lstat( filename, &statbuf) == 0 ) {
		return statbuf.st_size;
	}

	return 0;
}

time_t file_mod( const char *filename ) {
	struct stat statbuf;

	if( lstat( filename, &statbuf) == 0 ) {
		return statbuf.st_mtime;
	}

	return -1;
}

char *file_load( const char *filename, long int offset, size_t size ) {
	FILE *fh = fopen( filename, "rb" );
	char *buffer = NULL;

	if( fh == NULL ) {
		return NULL;
	}

	if( fseek( fh, offset, SEEK_SET) != 0 ) {
		return 0;
	}

	buffer = (char *) myalloc( (size+1) * sizeof(char), "file_load" );

	if( size != fread( buffer, sizeof(char), size, fh) ) {
		myfree( buffer, "file_load" );
		return NULL;
	}

	if( fclose( fh) != 0 ) {
		myfree( buffer, "file_load" );
		return NULL;
	}
	
	return buffer;
}

int file_write( const char *filename, char *buffer, size_t size ) {
	FILE *fh = fopen( filename, "wb" );

	if( fh == NULL) 
		return -1;

	if( size != fwrite( buffer, sizeof(char), size, fh))
		return -2;

	if( fclose( fh) != 0 )
		return -3;
	
	return size;
}

size_t file_append( const char *filename, char *buffer, size_t size ) {
	FILE *fh = fopen( filename, "ab" );

	if( fh == NULL) 
		return -1;

	if( size != fwrite( buffer, sizeof(char), size, fh))
		return -2;

	if( fclose( fh) != 0 )
		return -3;
	
	return size;
}

int file_rm( const char *filename ) {
	if( unlink( filename) != 0 )
		return 0;
	
	return 1;
}

int file_rmdir( const char *dirname ) {
	if( rmdir( dirname) == 0 ) {
		return 1;
	}
	
	return 0;
}

int file_rmrf( char *fileordir ) {
	DIR *dh	= NULL;
	struct dirent *entry = NULL;
	char filename[MAIN_BUF+1];

	if( file_isreg( fileordir) ) {
		if( !file_rm( fileordir) ) {
			return 0;
		}
	} else if( file_isdir( fileordir) ) {
		if( ( dh = opendir( fileordir)) == NULL ) {
			return 0;
		}
	  
		while( ( entry = readdir( dh)) != NULL ) {
			if( strcmp( entry->d_name,".") != 0 && strcmp( entry->d_name,"..") != 0 ) {
				snprintf( filename, MAIN_BUF+1, "%s/%s", fileordir, entry->d_name );
				if( !file_rmrf( filename) ) {
					return 0;
				}
			}
		}
		
		if( closedir( dh) != 0 )
			return 0;

		if( !file_rmdir( fileordir) )
			return 0;
	} else {
		return 0;
	}

	return 1;
}
