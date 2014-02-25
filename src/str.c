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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "str.h"

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

int str_safe_port( char *string ) {
	LONG number = 0;
	char *end = NULL;

	if( str_isNumber( string ) ) {
		errno = 0;
		number = strtol( string, &end, 10 );

		if( errno != 0 ) {
			return 0;
		}

		if( end == string ) {
			return 0;
		}

		if( *end != '\0' ) {
			return 0;
		}

		if( number < CONF_PORTMIN || number > CONF_PORTMAX ) {
			return 0;
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

int str_valid_hostname( const char *hostname, int hostsize ) {

	int i = 0;

	for( i=0; i<hostsize; i++ ) {
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

int str_valid_tld( const char *hostname, int hostsize, const char *domain ) {

	const char *p0 = NULL;
	char *p1 = NULL;

	/* "x.p2p" */
	if( hostsize < 5 ) {
		return 0;
	}

	/* Jump to the last '.' */
	p0 = hostname;
	while( ( p1 = strchr( p0, '.')) != NULL ) {
		p0 = p1+1;
	}

	/* TLD must be something like ".p2p" */
	if( strcmp( p0, domain ) != 0 ) {
		return 0;
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
		fail( "str_count(): Integer overflow?" );
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
		if( strftime( buffer, BUF_SIZE, "%a, %d %b %Y %H:%M:%S %Z", &date) == 0 ) {
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

/*
char *str_append( char *buf1, long int size1, char *buf2, long int size2 ) {
	if( buf1 == NULL ) {
		fail( "str_append(): buf1 broken" );
	}
	if( buf2 == NULL ) {
		fail( "str_append(): buf2 broken" );
	}
	if( size1 < 0 ) {
		fail( "str_append(): size1 broken" );
	}
	if( size2 < 0 ) {
		fail( "str_append(): size2 broken" );
	}
	if( size2 == 0 ) {
		return buf1;
	}
	if( size1+size2 <= 0 ) {
		fail( "str_append(): Overflow" );
	}

	size1 += size2;
	buf1 = myrealloc( buf1,( size1+1) * sizeof(char), "str_append" );
	strncat( buf1, buf2, size2 );
	buf1[size1] = '\0';

	return buf1;
}
*/

int str_sha1_compare(UCHAR *id1, UCHAR *id2, UCHAR *target) {
	UCHAR xor1;
	UCHAR xor2;
	int i = 0;

	for( i=0; i<SHA1_SIZE; i++ ) {
		if( id1[i] == id2[i] ) {
			continue;
		}
		xor1 = id1[i] ^ target[i];
		xor2 = id2[i] ^ target[i];
		if( xor1 < xor2 ) {
			return -1;
		} else {
			return 1;
		}
	}

	return 0;
}
