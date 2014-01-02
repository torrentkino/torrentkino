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
#include "malloc.h"
#include "conf.h"
#include "str.h"
#include "hash.h"
#include "list.h"
#include "thrd.h"
#include "node_tcp.h"
#include "log.h"
#include "file.h"
#include "mime.h"
#include "http.h"
#include "send_tcp.h"

long int http_urlDecode( char *src, long int srclen, char *dst, long int dstlen ) {
	int i = 0, j = 0;
	char *p1 = NULL;
	char *p2 = NULL;
	unsigned int hex = 0;

	if( src == NULL ) {
		info( NULL, 400, "http_urlDecode(): URL is NULL" );
		return 0;
	} else if( dst == NULL ) {
		info( NULL, 400, "http_urlDecode(): Destination is broken" );
		return 0;
	} else if( dstlen <= 0 ) {
		info( NULL, 400, "http_urlDecode(): Destination length is <= 0" );
		return 0;
	} else if( srclen <= 0 ) {
		info( NULL, 400, "http_urlDecode(): URL length is <= 0" );
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
			if( (long int)(src+srclen-p1) <= 2 ) {
				/* Path is broken: There should have been two more characters */
				info( NULL, 500, "http_urlDecode(): Broken url" );
				return 0;
			}

			if( !sscanf( ++p1,"%2x",&hex) ) {
				/* Path is broken: Broken characters */
				info( NULL, 500, "http_urlDecode(): Broken characters" );
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
		info( NULL, 400, "More than 100 headers?!" );
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
			info( NULL, 400, "Missing ':' in header?!" );
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

	/* Return until the header is complete. Gets killed if this grows beyond all imaginations. */
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
		info( &n->c_addr, 400, "Requested URL was not found" );
		node_status( n, NODE_MODE_SHUTDOWN );
		return;
	}
	*p_url = '\0'; p_url++;

	/* Find protocol type */
	if( (p_proto = strchr( p_url, ' ')) == NULL ) {
		info( &n->c_addr, 400, "No protocol found in request" );
		node_status( n, NODE_MODE_SHUTDOWN );
		return;
	}
	*p_proto = '\0'; p_proto++;

	/* Find variables( Start from p_url again) */
	if( (p_var = strchr( p_url, '?')) != NULL ) {
		*p_var = '\0'; p_var++;
	}

	/* Find header lines */
	if( (p_head = strstr( p_proto, "\r\n")) == NULL ) {
		info( &n->c_addr, 400, "Impossible. There must be a \\r\\n. I put it there..." );
		node_status( n, NODE_MODE_SHUTDOWN );
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

void http_read( TCP_NODE *n, char *p_cmd, char *p_url, char *p_proto, HASH *p_head ) {
	/* Check header */
	if( !http_check( n, p_cmd, p_url, p_proto) ) {
		return;
	}

	/* Guess HTTP reply code. */
	/* 200, 404 */
	http_code( n );

	/* Add Keep-Alive if necessary. */
	http_keepalive( n, p_head );

	/* Add Last-Modified. */
	/* 200, 404, 304 */
	http_lastmodified( n, p_head );

	/* HTTP Reply Size. */
	/* 200, 404, 304, 206, 416 */
	http_size( n, p_head );

	/* Prepare reply according to the HTTP code. */
	http_gate( n, p_head );

	/* Send reply */
	http_send( n );
}

int http_check( TCP_NODE *n, char *p_cmd, char *p_url, char *p_proto ) {

	/* Command */
	if( strncmp( p_cmd, "GET", 3) == 0 ) {
		n->type = HTTP_GET;
	} else if( strncmp( p_cmd, "HEAD", 4) == 0 ) {
		n->type = HTTP_HEAD;
	} else {
		info( &n->c_addr, 400, "Invalid request type: %s", p_cmd );
		node_status( n, NODE_MODE_SHUTDOWN );
		return 0;
	}

	/* Protocol */
	if( strncmp( p_proto, "HTTP/1.1", 8) != 0 && strncmp( p_proto, "HTTP/1.0", 8) != 0 ) {
		info( &n->c_addr, 400, "Invalid protocol: %s", p_proto );
		node_status( n, NODE_MODE_SHUTDOWN );
		return 0;
	}
	if( strncmp( p_proto, "HTTP/1.1", 8) == 0 ) {
		n->proto = HTTP_1_1;
	} else {
		n->proto = HTTP_1_0;
	}

	/* URL */
	if( !http_urlDecode( p_url, strlen( p_url), n->entity_url, BUF_SIZE) ) {
		info( &n->c_addr, 400, "Decoding URL failed" );
		node_status( n, NODE_MODE_SHUTDOWN );
		return 0;
	}

	/* Minimum path requirement */
	if( n->entity_url[0] != '/' ) {
		info( &n->c_addr, 400, "URL must start with '/': %s",
			n->entity_url );
		node_status( n, NODE_MODE_SHUTDOWN );
		return 0;
	}

	/* ".." in path. Do not like. */
	if( strstr( n->entity_url, "../") != NULL ) {
		info( &n->c_addr, 400, "Double dots: %s",
			n->entity_url );
		node_status( n, NODE_MODE_SHUTDOWN );
		return 0;
	}
 
	/* URL */
	if( ! str_isValidUTF8( n->entity_url) ) {
		info( &n->c_addr, 400, "Invalid UTF8 in URL" );
		node_status( n, NODE_MODE_SHUTDOWN );
		return 0;
	}

	return 1;
}

void http_code( TCP_NODE *n ) {
	snprintf( n->filename, BUF_SIZE, "%s%s", _main->conf->home, n->entity_url );

	if( file_isreg( n->filename ) ) {
		
		/* The requested entity is a file */
		n->code = 200;
	
	} else if( file_isdir( n->filename ) ) {

		/* There is a index.html file within that directory? */
		snprintf( n->filename, BUF_SIZE, "%s%s/%s",
				_main->conf->home, n->entity_url, _main->conf->file );
		
		if( file_isreg( n->filename ) ) {
			n->code = 200;
		} else {
			n->code = 404;
		}
	
	} else {

		n->code = 404;
	}
}

void http_keepalive( TCP_NODE *n, HASH *head ) {

	if( ! hash_exists( head, (UCHAR *)"Connection",  10 ) ) {
		return;
	}

	if( strcasecmp( (char *)hash_get( head, (UCHAR *)"Connection", 10),	"Keep-Alive") != 0 ) {
		return;
	}

	n->keepalive = TRUE;
}

void http_lastmodified( TCP_NODE *n, HASH *head ) {
	if( n->code == 404 ) {
		return;
	}

	str_gmttime( n->lastmodified, DATE_SIZE, file_mod( n->filename ) );

	if( hash_exists( head, (UCHAR *)"If-Modified-Since", 17 ) ) {
		if( strcmp( n->lastmodified, (char *)hash_get( head, (UCHAR *)"If-Modified-Since", 17 ) ) == 0 ) {
			n->code = 304;
		}
	}
}

void http_gate( TCP_NODE *n, HASH *head ) {
	/* Compute full reply */
	switch( n->code ) {
		case 200:
			http_200( n );
			info( &n->c_addr, n->code, n->entity_url );
			break;
#ifdef RANGE
		case 206:
			http_206( n );
			info( &n->c_addr, n->code,
				"%s [%s]", n->entity_url,
				(char *)hash_get(head, (UCHAR *)"Range", 5 ) );
			break;
		case 416:
			http_416( n );
			info( &n->c_addr, n->code,
				"%s [%s]", n->entity_url,
				(char *)hash_get(head, (UCHAR *)"Range", 5 ) );
			break;
#endif
		case 304:
			http_304( n );
			info( &n->c_addr, n->code, n->entity_url );
			break;
		case 404:
			http_404( n );
			info( &n->c_addr, n->code, n->entity_url );
			break;
	}
}

void http_send( TCP_NODE *n ) {
	/* Compute header size */
	n->send_size = strlen( n->send_buf );

	/* Prepare event system for sending data */
	node_status( n, NODE_MODE_SEND_INIT );

	/* Start sending */
	send_tcp( n );
}

void http_size( TCP_NODE *n, HASH *head ) {
#ifdef RANGE
	char *p_start = NULL;
	char *p_stop = NULL;
	char range[BUF_SIZE];
#endif

 	/* Not a valid file */
	if ( n->code == 404 ) {
 		return;
 	}

 	/* Normal case: content_length == filesize == f_stop */
	n->filesize = file_size(n->filename);

#ifdef RANGE
	n->f_stop = n->filesize;
	n->content_length = n->filesize;

	/* No Range header found: Nothing to do */
	if ( !hash_exists(head, (UCHAR *)"Range", 5) ) {
		return;
	}

	strncpy(range, (char *)hash_get(head, (UCHAR *)"Range", 5), BUF_OFF1);
	range[BUF_OFF1] = '\0';
	p_start = range;

	if ( strncmp(p_start, "bytes=", 6) != 0 ) {
		n->code = 416;
		return;
	}
	p_start += 6;

	if ( (p_stop = strchr(p_start, '-')) == NULL ) {
		n->code = 416;
		return;
	}
	*p_stop = '\0'; p_stop++;

	if ( *p_start == '\0' ) {
		n->range_start = 0;
	} else if ( str_isNumber(p_start) ) {
		n->range_start = atol(p_start);
	} else {
		n->code = 416;
		return;
	}

	if ( *p_stop == '\0' ) {
		n->range_stop = n->filesize-1;
	} else if ( str_isNumber(p_stop) ) {
		n->range_stop = atol(p_stop);
	} else {
		n->code = 416;
		return;
	}
	
	if ( n->range_start < 0 || n->range_start >= n->filesize ) {
		n->code = 416;
		return;
	}
	
	if ( n->range_stop < n->range_start || n->range_stop >= n->filesize ) {
		n->code = 416;
	   	return;
	}

	n->f_offset = n->range_start;
	n->f_stop = n->range_stop + 1;
	n->content_length = n->range_stop - n->f_offset + 1;

	/* Success */
	n->code = 206;
#endif
}

void http_404( TCP_NODE *n ) {
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
	char protocol = ( n->proto == HTTP_1_1 ) ? '1' : '0';
	char keepalive[] = "Connection: keep-alive\r\n";

	if( n->keepalive == FALSE ) {
		keepalive[0] = '\0';
	}

	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	snprintf( n->send_buf, BUF_SIZE,
	"HTTP/1.%c 404 Not found\r\n"
	"Date: %s\r\n"
	"Server: %s\r\n"
	"Content-Length: %i\r\n"
	"Content-Type: text/html; charset=utf-8\r\n"
	"%s"
	"\r\n"
	"%s",
	protocol, datebuf, CONF_SRVNAME, size, keepalive, buffer );
}

void http_304( TCP_NODE *n ) {
	char datebuf[DATE_SIZE];
	char protocol = ( n->proto == HTTP_1_1 ) ? '1' : '0';
	char keepalive[] = "Connection: keep-alive\r\n";

	if( n->keepalive == FALSE ) {
		keepalive[0] = '\0';
	}
	
	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	snprintf( n->send_buf, BUF_SIZE,
	"HTTP/1.%c 304 Not Modified\r\n"
	"Date: %s\r\n"
	"Server: %s\r\n"
	"%s"
	"\r\n",
	protocol, datebuf, CONF_SRVNAME, keepalive );
}

void http_200( TCP_NODE *n ) {
	char datebuf[DATE_SIZE];
	const char *mimetype = NULL;
	char protocol = ( n->proto == HTTP_1_1 ) ? '1' : '0';
	char keepalive[] = "Connection: keep-alive\r\n";

	if( n->keepalive == FALSE ) {
		keepalive[0] = '\0';
	}
	
	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	/* Compute mime type */
	mimetype = mime_find( n->filename );

	/* Compute answer */
	snprintf( n->send_buf, BUF_SIZE,
	"HTTP/1.%c 200 OK\r\n"
	"Date: %s\r\n"
	"Server: %s\r\n"
#ifdef __amd64__
	"Content-Length: %lu\r\n"
#else /* __i386__ __arm__ */
	"Content-Length: %u\r\n"
#endif
	"Content-Type: %s\r\n"
	"Last-Modified: %s\r\n"
	"%s"
	"\r\n",
	protocol, datebuf, CONF_SRVNAME, n->filesize, mimetype, n->lastmodified, keepalive );
}

#ifdef RANGE
void http_206( TCP_NODE *n ) {
	char datebuf[DATE_SIZE];
	const char *mimetype = NULL;
	char protocol = ( n->proto == HTTP_1_1 ) ? '1' : '0';
	char keepalive[] = "Connection: keep-alive\r\n";

	if( n->keepalive == FALSE ) {
		keepalive[0] = '\0';
	}

	str_GMTtime(datebuf, DATE_SIZE);
	mimetype = mime_find(n->filename);

	snprintf(n->send_buf, BUF_SIZE,
	"HTTP/1.%c 206 OK\r\n"
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
	"Accept-Ranges: bytes\r\n"
	"Last-Modified: %s\r\n"
	"%s"
	"\r\n",
	protocol, datebuf, CONF_SRVNAME,
#ifdef __amd64__
	n->content_length,
	n->range_start, n->range_stop, n->filesize,
#else /* __i386__ __arm__ */
	n->content_length,
	(unsigned int)n->range_start, (unsigned int)n->range_stop, n->filesize,
#endif
	mimetype, n->lastmodified, keepalive);
}

void http_416( TCP_NODE *n ) {
	char datebuf[DATE_SIZE];
	char buffer[BUF_SIZE] = 
	"<!DOCTYPE html>"
	"<html lang=\"en\" xml:lang=\"en\">"
	"<head>"
	"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"
	"<title>Broken range</title>"
	"</head>"
	"<body>"
	"<h1>Broken range</h1>"
	"</body>"
	"</html>";
	int size = strlen( buffer );
	char protocol = ( n->proto == HTTP_1_1 ) ? '1' : '0';
	char keepalive[] = "Connection: keep-alive\r\n";

	if( n->keepalive == FALSE ) {
		keepalive[0] = '\0';
	}

	/* Compute GMT time */
	str_GMTtime( datebuf, DATE_SIZE );

	snprintf( n->send_buf, BUF_SIZE,
	"HTTP/1.%c 416 Requested Range Not Satisfiable\r\n"
	"Date: %s\r\n"
	"Server: %s\r\n"
	"Content-Length: %i\r\n"
	"Content-Type: text/html; charset=utf-8\r\n"
	"%s"
	"\r\n"
	"%s",
	protocol, datebuf, CONF_SRVNAME, size, keepalive, buffer );
}


#endif
