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
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#ifdef TUMBLEWEED
#include "malloc.h"
#include "main.h"
#include "list.h"
#include "node_web.h"
#include "log.h"
#include "str.h"
#include "conf.h"
#else
#include "malloc.h"
#include "main.h"
#include "list.h"
#include "log.h"
#include "str.h"
#include "conf.h"
#endif

struct obj_str *str_init( UCHAR *buf, long int len ) {
	struct obj_str *str = (struct obj_str *) myalloc( sizeof(struct obj_str), "str_init" );
	
	str->s = (UCHAR *) myalloc( (len+1) * sizeof(UCHAR), "str_init" );
	if( len > 0 ) {
		memcpy( str->s, buf, len );
	}
	str->i = len;
	
	return str;
}

void str_free( struct obj_str *str ) {
	if( str->s != NULL ) {
		myfree( str->s, "str_free" );
	}
	myfree( str, "str_free" );
}

int str_isValidUTF8( char *string ) {
	unsigned int i = 0, j = 0, n = 0;
	unsigned int length = strlen( string );

	for( i=0; i<length; i++ ) {
		if( string[i] < 0x80)			 n=0; /* 0bbbbbbb */
		else if( ( string[i] & 0xE0) == 0xC0) n=1; /* 110bbbbb */
		else if( ( string[i] & 0xF0) == 0xE0) n=2; /* 1110bbbb */
		else if( ( string[i] & 0xF8) == 0xF0) n=3; /* 11110bbb */
		else if( ( string[i] & 0xFC) == 0xF8) n=4; /* 111110bb */
		else if( ( string[i] & 0xFE) == 0xFC) n=5; /* 1111110b */
		else return 0;
		
		for( j=0; j<n; j++ ) {
			if( ( ++i == length) ||(( string[i] & 0xC0) != 0x80) ) {
				return 0;
			}
		}
	}

	/* String is valid UTF8 */
	return 1;
}

int str_isNumber( char *string ) {
	char *p = string;

	while( *p != '\0' ) {
		if( *p < '0' || *p > '9' ) {
			return 0;
		}
		p++;
	}

	return 1;
}

int str_isSafePort( char *string ) {
	/* Variables */
	int number = -1;

	if( str_isNumber( string) ) {
		number = atoi( string );

		if( number < CONF_PORTMIN || number > CONF_PORTMAX ) {
			number = -1;
		}
	}

	return number;
}

/*int str_isHex( char *string ) {
	unsigned int i = 0;

	for( i=0; i<strlen( string ); i++ ) {
		if( string[i] >= '0' && string[i] <= '9' ) {
			continue;
		} else if( string[i] >= 'A' && string[i] <= 'F' ) {
			continue;
		} else if( string[i] >= 'a' && string[i] <= 'f' ) {
			continue;
		} else {
			return 0;
		}
	}

	return 1;
}*/

int str_isValidFilename( char *string ) {
	unsigned int i = 0;
	
	for( i=0; i<strlen( string ); i++ ) {
		if( string[i] >= '0' && string[i] <= '9' ) {
			continue;
		} else if( string[i] >= 'A' && string[i] <= 'Z' ) {
			continue;
		} else if( string[i] >= 'a' && string[i] <= 'z' ) {
			continue;
		} else if( string[i] == '-' ) {
			continue;
		} else if( string[i] == '_' ) {
			continue;
		} else if( string[i] == '.' ) {
			continue;
		} else {
			return 0;
		}
	}

	return 1;
}

int str_isValidHostname( const char *hostname, int size ) {

	int i = 0;
	
	for( i=0; i<size; i++ ) {
		if( hostname[i] >= '0' && hostname[i] <= '9' ) {
			continue;
		} else if( hostname[i] >= 'A' && hostname[i] <= 'Z' ) {
			continue;
		} else if( hostname[i] >= 'a' && hostname[i] <= 'z' ) {
			continue;
		} else if( hostname[i] == '-' ) {
			continue;
		} else if( hostname[i] == '_' ) {
			continue;
		} else if( hostname[i] == '.' ) {
			continue;
		} else {
			return 0;
		}
	}

	return 1;
}

/*
void str_htmlDisarmInput( char *string ) {
	char *p = string;
	
	while( *p != '\0' ) {
		switch( *p ) {
			case '<':
			case '>':
			case '"':
			case '\'':
			case '%':
			case ';':
			case '(':
			case ')':
			case '&':
			case '+':
			case '/':
			case '?':
			case '\\':
				*p = '-';
				break;
		}

		p++;
	}
} */

int str_count( char *buffer, const char *search ) {
	int counter = 0;
	int size = strlen( search );
	char *p = buffer;
	
	while( ( p = strstr( p,search)) != NULL ) {
		p += size;
		counter++;
	}
	
	/* Something is broken here */
	if( counter < 0 ) {
		log_fail( "str_count(): Integer overflow?" );
	}

	return counter;
}

void str_GMTtime( char *buffer, int size ) {
	char fallback[] = "Thu, 01 Jan 1970 00:00:00 GMT";
	time_t timestamp;

	if( time( &timestamp) > 0 ) {
		str_gmttime( buffer, size, timestamp );
	} else {
		snprintf( buffer, size, "%s", fallback );
	}
}

void str_gmttime( char *buffer, int size, time_t timestamp ) {
	char fallback[] = "Thu, 01 Jan 1970 00:00:00 GMT";
	struct tm date;
	
	memset( buffer, '\0', size );

	/* Last-Modified: Thu, 10 Feb 2011 20:25:39 GMT */
	if( gmtime_r( &timestamp, &date) != NULL ) {
		if( strftime( buffer, MAIN_BUF+1, "%a, %d %b %Y %H:%M:%S %Z", &date) == 0 ) {
			strncpy( buffer, fallback, size-1 );
		}
	} else {
		strncpy( buffer, fallback, size-1 );
	}
}

void str_prettySize( char *buffer, int size, unsigned long filesize ) {
	long int thissize = 0;

	memset( buffer, '\0', size );

	if( filesize > 1073741824 ) {
		thissize = filesize / 1073741824;
		snprintf( buffer, size, "%lu GiB", thissize );
	} else if( filesize > 1048576 ) {
		thissize = filesize / 1048576;
		snprintf( buffer, size, "%lu MiB", thissize );
	} else if( filesize > 1024 ) {
		thissize = filesize / 1024;
		snprintf( buffer, size, "%lu KiB", thissize );
	} else {
		snprintf( buffer, size, "%lu B", filesize );
	}
}

char *str_append( char *buf1, long int size1, char *buf2, long int size2 ) {
	if( buf1 == NULL ) {
		log_fail( "str_append(): buf1 broken" );
	}
	if( buf2 == NULL ) {
		log_fail( "str_append(): buf2 broken" );
	}
	if( size1 < 0 ) {
		log_fail( "str_append(): size1 broken" );
	}
	if( size2 < 0 ) {
		log_fail( "str_append(): size2 broken" );
	}
	if( size2 == 0 ) {
		return buf1;
	}
	if( size1+size2 <= 0 ) {
		log_fail( "str_append(): Overflow" );
	}

	size1 += size2;
	buf1 = myrealloc( buf1,( size1+1) * sizeof(char), "str_append" );
	strncat( buf1, buf2, size2 );
	buf1[size1] = '\0';

	return buf1;
}
