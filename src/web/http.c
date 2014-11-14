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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>

#include "tumbleweed.h"
#include "../shr/malloc.h"
#include "conf.h"
#include "../shr/str.h"
#include "../shr/hash.h"
#include "../shr/list.h"
#include "../shr/thrd.h"
#include "node_tcp.h"
#include "../shr/log.h"
#include "../shr/file.h"
#include "mime.h"
#include "http.h"
#include "send_tcp.h"

LONG http_urlDecode( char *src, LONG srclen, char *dst, LONG dstlen ) {
	int i = 0, j = 0;
	char *p1 = NULL;
	char *p2 = NULL;
	unsigned int hex = 0;

	if( src == NULL ) {
		info( _log, NULL, "http_urlDecode(): URL is NULL" );
		return 0;
	} else if( dst == NULL ) {
		info( _log, NULL, "http_urlDecode(): Destination is broken" );
		return 0;
	} else if( dstlen <= 0 ) {
		info( _log, NULL, "http_urlDecode(): Destination length is <= 0" );
		return 0;
	} else if( srclen <= 0 ) {
		info( _log, NULL, "http_urlDecode(): URL length is <= 0" );
		return 0;
	}

	memset( dst, '\0', dstlen );

	p1 = src;
	p2 = dst;
	while( i < dstlen && j < srclen ) {
		if( *p1 == '+' ) {
			*p2 = ' ';
			p1++; p2++;
			j++;
		} else if( *p1 == '%' ) {
			/* Safety check */
			if( (LONG)(src+srclen-p1) <= 2 ) {
				/* Path is broken: There should have been two more characters */
				info( _log, NULL, "http_urlDecode(): Broken url" );
				return 0;
			}

			if( !sscanf( ++p1,"%2x",&hex) ) {
				/* Path is broken: Broken characters */
				info( _log, NULL, "http_urlDecode(): Broken characters" );
				return 0;
			}

			*p2 = hex;
			p1 += 2; p2++;
			j += 3;
		} else {
			*p2 = *p1;
			p1++; p2++;
			j++;
		}

		i++;
	}

	return i;
}

HASH *http_hashHeader( char *head ) {
	int head_counter = 0;
	char *p = NULL;
	char *var = NULL;
	char *val = NULL;
	char *end = NULL;
	HASH *hash = NULL;

	/* Compute hash size */
	head_counter = str_count( head, "\r\n" );
	if( head_counter >= 100 ) {
		info( _log, NULL, "More than 100 headers?!" );
		return NULL;
	} else if( head_counter <= 0 ) {
		/* HTTP/1.0 */
		return NULL;
	}

	/* Create hash */
	hash = hash_init( head_counter );

	/* Put header into hash */
	p = head;
	while( (end = strstr( p, "\r\n")) != NULL ) {
		memset( end, '\0', 2 );
		if( (val = strchr( p, ':')) != NULL ) {

			/* Variable & Value */
			var = p;
			*val = '\0';
			val++;

			/* More or less empty spaces */
			while( *val == ' ' ) {
				val++;
			}

			/* Store tuple */
			hash_put( hash, (UCHAR *)var, strlen( var), val );

		} else {
			info( _log, NULL, "Missing ':' in header?!" );
			hash_free( hash );
			return NULL;
		}
		p = end + 2;
	}

	return hash;
}

void http_deleteHeader( HASH *head ) {
	hash_free( head );
}

void http_buf( TCP_NODE *n ) {
	char *p_cmd = NULL;
	char *p_url = NULL;
	char *p_var = NULL;
	char *p_proto = NULL;
	char *p_head = NULL;
	char *p_body = NULL;
	HASH *head = NULL;
	char recv_buf[BUF_SIZE];

	/* Do not start before at least this much arrived: "GET / HTTP/1.1" */
	if( n->recv_size <= 14 ) {
		return;
	}

	/* Copy HTTP request */
	memset( recv_buf, '\0', BUF_SIZE );
	memcpy( recv_buf, n->recv_buf, n->recv_size );

	/* Return until the header is complete. Gets killed if this grows beyond
	 * all imaginations. */
	if( (p_body = strstr( recv_buf, "\r\n\r\n")) == NULL ) {
		return;
	}

	/* Keep one \r\n for consistent header analysis. */
	*p_body = '\r'; p_body++;
	*p_body = '\n'; p_body++;
	*p_body = '\0'; p_body++;
	*p_body = '\0'; p_body++;

	/* HTTP Pipelining: There is at least one more request. */
	if( n->recv_size > (ssize_t)(p_body-recv_buf) ) {
		n->recv_size -= (ssize_t)(p_body-recv_buf );
		memset( n->recv_buf, '\0', BUF_SIZE );
		memcpy( n->recv_buf, p_body, n->recv_size );
	} else {
		/* The request has been copied. Reset the receive buffer. */
		node_clearRecvBuf( n );
	}

	/* Remember start point */
	p_cmd = recv_buf;

	/* Find url */
	if( (p_url = strchr( p_cmd, ' ')) == NULL ) {
		info( _log, &n->c_addr, "Requested URL was not found" );
		node_status( n, NODE_SHUTDOWN );
		return;
	}
	*p_url = '\0'; p_url++;

	/* Find protocol type */
	if( (p_proto = strchr( p_url, ' ')) == NULL ) {
		info( _log, &n->c_addr, "No protocol found in request" );
		node_status( n, NODE_SHUTDOWN );
		return;
	}
	*p_proto = '\0'; p_proto++;

	/* Find variables( Start from p_url again) */
	if( (p_var = strchr( p_url, '?')) != NULL ) {
		*p_var = '\0'; p_var++;
	}

	/* Find header lines */
	if( (p_head = strstr( p_proto, "\r\n")) == NULL ) {
		info( _log, &n->c_addr, "There must be a \\r\\n. I put it there..." );
		node_status( n, NODE_SHUTDOWN );
		return;
	}
	*p_head = '\0'; p_head++;
	*p_head = '\0'; p_head++;

	/* Hash header */
	head = http_hashHeader( p_head );

	/* Validate input */
	http_read( n, p_cmd, p_url, p_proto, head );

	/* Delete Hash */
	http_deleteHeader( head );
}

void http_read( TCP_NODE *n, char *p_cmd, char *p_url, char *p_proto,
		HASH *p_head ) {

	int code = 0;
	char lastmodified[DATE_SIZE];
	char resource[BUF_SIZE];
	char filename[BUF_SIZE];
	char range[BUF_SIZE];
	char *p_range = range;
	size_t filesize = 0;
	size_t content_length = 0;
	char keepalive[BUF_SIZE];
	const char *mimetype = NULL;

	/* Get protocol */
	if( !http_proto( n, p_proto ) ) {
		node_status( n, NODE_SHUTDOWN );
		goto END;
	}

	/* Get action */
	if( !http_action( n, p_cmd ) ) {
		node_status( n, NODE_SHUTDOWN );
		goto END;
	}

	/* Get resource */
	if( !http_resource( n, p_url, resource ) ) {
		node_status( n, NODE_SHUTDOWN );
		goto END;
	}

	/* Check Keep-Alive */
	n->keepalive = http_keepalive( p_head, keepalive );

	/* Compute filename */
	if( ! http_filename( resource, filename ) ) {
		http_404( n, keepalive );
		info( _log, &n->c_addr, "404 %s", resource );
		goto END;
	}

	/* Compute file size */
	filesize = http_size_simple( filename );

	/* Compute mime type */
	mimetype = mime_find( filename );

	/* Last-Modified. */
	if( ! http_resource_modified( filename, p_head, lastmodified ) ) {
		http_304( n, keepalive );
		info( _log, &n->c_addr, "304 %s", resource );
		goto END;
	}

	/* Range request? */
	if( http_range_detected( p_head, range ) ) {
		code = 206;
	} else {
		code = 200;
	}

	/* Normal 200-er request */
	if( code == 200 ) {
		info( _log, &n->c_addr, "200 %s", resource );
		http_200( n, lastmodified, filename, filesize, keepalive, mimetype );
		http_body( n, filename, filesize );
		goto END;
	}

	/* Check for 'bytes=' in range. Fallback to 200 if necessary. */
	if( !http_range_prepare( &p_range ) ) {
		info( _log, &n->c_addr, "200 %s", resource );
		http_200( n, lastmodified, filename, filesize, keepalive, mimetype );
		http_body( n, filename, filesize );
		goto END;
	}

	/* multipart/byteranges */
	if( !http_range_multipart( p_range ) ) {
		RESPONSE *r_head = NULL, *r_file = NULL;

		/* Header */
		if( ( r_head = resp_put( n->response, RESPONSE_FROM_MEMORY ) ) == NULL ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}

		/* File */
		if( ( r_file = resp_put( n->response, RESPONSE_FROM_FILE ) ) == NULL ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}

		/* Parse range. */
		if( !http_range_simple( n, r_file, filename, filesize, p_range,
				&content_length ) ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}

		/* Header with known content_length */
		http_206_simple( n, r_head, r_file, lastmodified, filename,
				filesize, content_length, keepalive, mimetype );

		info( _log, &n->c_addr, "206 %s [%s]", resource, range );
		goto END;
	} else {
		RESPONSE *r_head = NULL, *r_bottom = NULL, *r_zsyncbug = NULL;
		char boundary[12];

		/* Create boundary string */
		http_random( boundary, 12 );

		/* Header */
		if( ( r_head = resp_put( n->response, RESPONSE_FROM_MEMORY ) ) == NULL ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}

		/* zsync bug? One more \r\n between header and body. */
		if( ( r_zsyncbug = resp_put( n->response, RESPONSE_FROM_MEMORY ) ) == NULL ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}

		/* Parse range. */
		if( !http_range_complex( n, filename, filesize, mimetype, p_range, &content_length, boundary ) ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}

		/* Bottom */
		if( ( r_bottom = resp_put( n->response, RESPONSE_FROM_MEMORY ) ) == NULL ) {
			node_status( n, NODE_SHUTDOWN );
			goto END;
		}
		http_206_boundary_finish( r_bottom, &content_length, boundary );

		/* Header with known content_length */
		http_206_complex( n, r_head, lastmodified, filename,
				filesize, content_length, boundary, keepalive );


		/* zsync bug? One more \r\n between header and body. */
		http_newline( r_zsyncbug, &content_length );

		info( _log, &n->c_addr, "206 %s [%s]", resource, range );

		goto END;
	}

	info( _log, &n->c_addr, "FIXME: HTTP parser end reached without result" );
	node_status( n, NODE_SHUTDOWN );

	END:

	/* HTTP Pipeline: There is at least one more request to parse.
	 * Recursive request! Limited by the input buffer.
	 */
	if( n->pipeline != NODE_SHUTDOWN && n->recv_size > 0 ) {
		http_buf( n );
	}
}

void http_body( TCP_NODE *n, char *filename, size_t filesize ) {
	RESPONSE *r = NULL;

	if( filesize == 0 ) {
		return;
	}

	if( ( r = resp_put( n->response, RESPONSE_FROM_FILE ) ) == NULL ) {
		return;
	}

	strncpy( r->data.file.filename, filename, BUF_OFF1 );
	r->data.file.f_offset = 0;
	r->data.file.f_stop = filesize - 1;
}

int http_action( TCP_NODE *n, char *p_cmd ) {

	if( strncmp( p_cmd, "GET", 3) == 0 ) {
		return TRUE;
	}

	info( _log, &n->c_addr, "Unsupported request action: %s", p_cmd );
	return FALSE;
}

int http_proto( TCP_NODE *n, char *p_proto ) {

	/* Protocol */
	if( strncmp( p_proto, "HTTP/1.1", 8) == 0 ) {
		return TRUE;
	}

	if( strncmp( p_proto, "HTTP/1.0", 8) == 0 ) {
		return TRUE;
	}

	info( _log, &n->c_addr, "Unsupported protocol type: %s", p_proto );

	return FALSE;
}

int http_resource( TCP_NODE *n, char *p_url, char *resource ) {

	/* URL */
	if( !http_urlDecode( p_url, strlen( p_url ), resource, BUF_SIZE) ) {
		info( _log, &n->c_addr, "Decoding resource failed" );
		return FALSE;
	}

	/* Minimum path requirement */
	if( resource[0] != '/' ) {
		info( _log, &n->c_addr, "Resource must start with '/'" );
		return FALSE;
	}

	/* ".." in path. Do not like. */
	if( strstr( resource, "../") != NULL ) {
		info( _log, &n->c_addr, "Double dots in resource" );
		return FALSE;
	}

	/* URL */
	if( ! str_isValidUTF8( resource) ) {
		info( _log, &n->c_addr, "Invalid UTF8 in resource" );
		return FALSE;
	}

	return TRUE;
}


int http_filename( char *resource, char *filename ) {
	snprintf( filename, BUF_SIZE, "%s%s", _main->conf->home, resource );

	if( file_isreg( filename ) ) {
		return TRUE;
	} else if( file_isdir( filename ) ) {
		/* There is a index.html file within that directory? */
		snprintf( filename, BUF_SIZE, "%s%s/%s",
				_main->conf->home, resource, _main->conf->file );

		if( file_isreg( filename ) ) {
			return TRUE;
		}
	}

	return FALSE;
}

int http_keepalive( HASH *head, char *keepalive ) {

	memset( keepalive, '\0', BUF_SIZE );

	if( ! hash_exists( head, (UCHAR *)"Connection",  10 ) ) {
		return HTTP_UNDEF;
	}

	if( strcasecmp( (char *)hash_get( head, (UCHAR *)"Connection", 10),
			"keep-alive") == 0 ) {
		strncpy( keepalive, "Connection: keep-alive\r\n", BUF_OFF1 );
		return HTTP_KEEPALIVE;
	}

	if( strcasecmp( (char *)hash_get( head, (UCHAR *)"Connection", 10),
			"close") == 0 ) {
		strncpy( keepalive, "Connection: close\r\n", BUF_OFF1 );
		return HTTP_CLOSE;
	}

	return HTTP_UNDEF;
}

int http_resource_modified( char *filename, HASH *head, char *lastmodified ) {
	str_gmttime( lastmodified, DATE_SIZE, file_mod( filename ) );

	if( !hash_exists( head, (UCHAR *)"If-Modified-Since", 17 ) ) {
		return TRUE;
	}

	if( strcmp( (char *)hash_get( head, (UCHAR *)"If-Modified-Since", 17 ),
			lastmodified ) != 0 ) {
		return TRUE;
	}

	return FALSE;
}

void http_send( TCP_NODE *n ) {
	if( n->pipeline == NODE_SHUTDOWN ) {
		return;
	}

	/* Prepare event system for sending data */
	node_status( n, NODE_SEND_INIT );

	/* Start sending */
	send_tcp( n );
}

int http_range_detected( HASH *head, char *range ) {
	if ( hash_exists(head, (UCHAR *)"Range", 5) ) {
		memset( range, '\0', BUF_SIZE );
		strncpy( range, (char *)hash_get( head, (UCHAR *)"Range", 5 ),
				BUF_OFF1 );
		return TRUE;
	}

	return FALSE;
}

int http_range_prepare( char **p_range ) {
	if ( strncmp( *p_range, "bytes=", 6 ) != 0 ) {
		return FALSE;
	}
	*p_range += 6;
	return TRUE;
}

int http_range_multipart( char *p_range ) {
	if ( strchr( p_range, ',' ) != NULL ) {
		return TRUE;
	}
	return FALSE;
}

size_t http_size_simple( char *filename ) {
	return file_size(filename);
}

int http_range_simple( TCP_NODE *n, RESPONSE *r,
		char *filename, size_t filesize,
		char *p_range, size_t *content_length ) {
	char *p_start = NULL;
	char *p_stop = NULL;
	size_t range_start = 0;
	size_t range_stop = 0;
	char range[BUF_SIZE];

	if( filesize == 0 ) {
		return FALSE;
	}

	memset( range, '\0', BUF_SIZE);
	strncpy( range, p_range, BUF_OFF1);

	strncpy( r->data.file.filename, filename, BUF_OFF1 );

	p_start = range;

	if ( (p_stop = strchr(p_start, '-')) == NULL ) {
		return FALSE;
	}
	*p_stop = '\0'; p_stop++;

	if ( *p_start == '\0' ) {
		range_start = 0;
	} else if ( str_isNumber(p_start) ) {
		range_start = atol(p_start);
	} else {
		return FALSE;
	}

	if ( *p_stop == '\0' ) {
		range_stop = filesize - 1;
	} else if ( str_isNumber(p_stop) ) {
		range_stop = atol(p_stop);
	} else {
		return FALSE;
	}

	if ( range_start < 0 || range_start >= filesize ) {
		return FALSE;
	}

	if ( range_stop < range_start ) {
	   	return FALSE;
	}

	/* Be liberal: Patch the range stop if the end is beyond the end of
	 * the file. Other webservers also behave like this. zsync uses a stop
	 * marker, that is beyond the file limits for example... */
	if ( range_stop >= filesize ) {
		range_stop = filesize - 1;
	}

	r->data.file.f_offset = range_start;
	r->data.file.f_stop = range_stop;
	*content_length += r->data.file.f_stop + 1 - r->data.file.f_offset;

	/* Success */
	return TRUE;
}

int http_range_complex( TCP_NODE *n, char *filename, size_t filesize, const char *mimetype,
		char *p_range, size_t *content_length, char *boundary ) {
	char *p_start = NULL;
	char *p_stop = NULL;
	char range[BUF_SIZE];
	RESPONSE *r_head = NULL, *r_file = NULL, *r_bottom = NULL;

	/* Boundary head */
	if( ( r_head = resp_put( n->response, RESPONSE_FROM_MEMORY ) ) == NULL ) {
		return FALSE;
	}

	/* File */
	if( ( r_file = resp_put( n->response, RESPONSE_FROM_FILE ) ) == NULL ) {
		return FALSE;
	}

	/* Boundary bottom */
	if( ( r_bottom = resp_put( n->response, RESPONSE_FROM_MEMORY ) ) == NULL ) {
		return FALSE;
	}

	/* Copy range */
	memset( range, '\0', BUF_SIZE );
	strncpy( range, p_range, BUF_OFF1 );

	p_start = range;

	/* Find separator */
	if ( (p_stop = strchr(p_start, ',')) != NULL ) {
		*p_stop = '\0';
	}

	/* Parse range. Create file meta data. */
	if( !http_range_simple( n, r_file, filename, filesize, p_start, content_length ) ) {
		return FALSE;
	}

	/* Create boundary header */
	http_206_boundary_head( r_head, r_file, filename, filesize, mimetype, boundary, content_length );

	/* Create boundary bottom */
	http_newline( r_bottom, content_length );

	/* There is more to check */
	if( p_stop != NULL ) {
		p_start = p_stop + 1;
		return http_range_complex( n, filename, filesize, mimetype, p_start, content_length, boundary );
	}

	return TRUE;
}

void http_404( TCP_NODE *n, char *keepalive ) {
	RESPONSE *r = resp_put( n->response, RESPONSE_FROM_MEMORY );
	char datebuf[DATE_SIZE];
	char buffer[BUF_SIZE] =
		"<!DOCTYPE html>"
		"<html lang=\"en\" xml:lang=\"en\">"
		"<head>"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
		"<title>Not found</title>"
		"</head>"
		"<body>"
		"<h1>Not found</h1>"
		"</body>"
		"</html>";
	int size = strlen( buffer );

	if( r == NULL ){
		return;
	}

	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	resp_set_memory( r,
		"HTTP/1.1 404 Not found\r\n"
		"Date: %s\r\n"
		"Server: %s\r\n"
		"Content-Length: %i\r\n"
		"Content-Type: text/html; charset=utf-8\r\n"
		"%s"
		"\r\n"
		"%s",
		datebuf, LOG_NAME, size, keepalive, buffer );
}

void http_304( TCP_NODE *n, char *keepalive ) {
	RESPONSE *r = resp_put( n->response, RESPONSE_FROM_MEMORY );
	char datebuf[DATE_SIZE];

	if( r == NULL ){
		return;
	}

	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	resp_set_memory( r,
		"HTTP/1.1 304 Not Modified\r\n"
		"Date: %s\r\n"
		"Server: %s\r\n"
		"%s"
		"\r\n",
		datebuf, LOG_NAME, keepalive );
}

void http_200( TCP_NODE *n, char *lastmodified, char *filename, size_t filesize,
		char *keepalive, const char *mimetype ) {
	RESPONSE *r = resp_put( n->response, RESPONSE_FROM_MEMORY );
	char datebuf[DATE_SIZE];

	if( r == NULL ){
		return;
	}

	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	/* Compute answer */
	resp_set_memory( r,
		"HTTP/1.1 200 OK\r\n"
		"Date: %s\r\n"
		"Server: %s\r\n"
#ifdef __amd64__
		"Content-Length: %lu\r\n"
#else /* __i386__ __arm__ */
		"Content-Length: %u\r\n"
#endif
		"Content-Type: %s\r\n"
		"Last-Modified: %s\r\n"
		"Accept-Ranges: bytes\r\n"
		"%s"
		"\r\n",
		datebuf, LOG_NAME, filesize, mimetype, lastmodified, keepalive );
}

void http_206_simple( TCP_NODE *n, RESPONSE *r_head, RESPONSE *r_file,
		char *lastmodified, char *filename, size_t filesize, size_t content_length, char *keepalive, const char *mimetype ) {
	char datebuf[DATE_SIZE];
	size_t range_start = r_file->data.file.f_offset;
	size_t range_stop = r_file->data.file.f_stop;

	str_GMTtime( datebuf, DATE_SIZE );

	resp_set_memory( r_head,
		"HTTP/1.1 206 OK\r\n"
		"Date: %s\r\n"
		"Server: %s\r\n"
#ifdef __amd64__
		"Content-Length: %lu\r\n"
		"Content-Range: bytes %lu-%lu/%lu\r\n"
#else /* __i386__ __arm__ */
		"Content-Length: %u\r\n"
		"Content-Range: bytes %u-%u/%u\r\n"
#endif
		"Content-Type: %s\r\n"
		"Last-Modified: %s\r\n"
		"%s"
		"\r\n",
		datebuf, LOG_NAME,
#ifdef __amd64__
		content_length,
		range_start, range_stop, filesize,
#else /* __i386__ __arm__ */
		content_length,
		(unsigned int)range_start, (unsigned int)range_stop, filesize,
#endif
		mimetype, lastmodified, keepalive);
}

void http_206_complex( TCP_NODE *n, RESPONSE *r_head,
		char *lastmodified, char *filename, size_t filesize,
		size_t content_length, char *boundary, char *keepalive ) {
	char datebuf[DATE_SIZE];

	str_GMTtime( datebuf, DATE_SIZE );

	resp_set_memory( r_head,
		"HTTP/1.1 206 Partial Content\r\n"
		"Server: %s\r\n"
		"Date: %s\r\n"
		"Content-Type: multipart/byteranges; boundary=%s\r\n"
#ifdef __amd64__
		"Content-Length: %lu\r\n"
#else /* __i386__ __arm__ */
		"Content-Length: %u\r\n"
#endif
		"Last-Modified: %s\r\n"
		"%s"
		"\r\n",
		LOG_NAME, datebuf, boundary,
#ifdef __amd64__
		content_length,
#else /* __i386__ __arm__ */
		content_length,
#endif
		lastmodified, keepalive );
}

void http_206_boundary_head( RESPONSE *r_head, RESPONSE *r_file,
		char *filename, size_t filesize, const char *mimetype,
		char *boundary, size_t *content_size ) {
	size_t range_start = r_file->data.file.f_offset;
	size_t range_stop = r_file->data.file.f_stop;

	*content_size += resp_set_memory( r_head,
		"--%s\r\n"
		"Content-Type: %s\r\n"
#ifdef __amd64__
		"Content-Range: bytes %lu-%lu/%lu\r\n"
#else /* __i386__ __arm__ */
		"Content-Range: bytes %u-%u/%u\r\n"
#endif
		"\r\n",
		boundary, mimetype,
#ifdef __amd64__
		range_start, range_stop, filesize
#else /* __i386__ __arm__ */
		(unsigned int)range_start, (unsigned int)range_stop, filesize
#endif
	);
}

void http_newline( RESPONSE *r, size_t *content_size ) {
	*content_size += resp_set_memory( r, "\r\n" );
}

void http_206_boundary_finish( RESPONSE *r, size_t *content_length,
	char *boundary ) {
	*content_length += resp_set_memory( r, "--%s--\r\n", boundary );
}

void http_random( char *boundary, int size ) {
	static const char alphanum[] =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz";

    for(int i=0; i<size; i++) {
        boundary[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    boundary[size-1] = '\0';
}
