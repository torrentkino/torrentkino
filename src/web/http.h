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

#ifndef HTTP_H
#define HTTP_H

#include "../shr/list.h"

#define HTTP_UNDEF 0
#define HTTP_CLOSE 1
#define HTTP_KEEPALIVE 2

#define HTTP_1_0 10
#define HTTP_1_1 11

LONG http_urlDecode( char *src, LONG srclen, char *dst,	LONG dstlen );
HASH *http_hashHeader( char *head );
void http_deleteHeader( HASH *head );

void http_buf( TCP_NODE *n );
void http_read( TCP_NODE *n, char *p_cmd, char *p_url, char *p_proto,
		HASH *p_head );

int http_action( TCP_NODE *n, char *p_cmd );
int http_proto( TCP_NODE *n, char *p_proto );
int http_resource( TCP_NODE *n, char *p_url, char *resource );

int http_filename( char *resource, char *filename );
int http_keepalive( HASH *head, char *keepalive );
int http_resource_modified( char *filename, HASH *head,
		char *lastmodified );
int http_range_detected( HASH *head, char *range );
int http_range_prepare( char **p_range );
int http_range_multipart( char *p_range );

int http_size( TCP_NODE *n, HASH *head, char *filename );
size_t http_size_simple( char *filename );
int http_range_simple( TCP_NODE *n, RESPONSE *r,
		char *filename, size_t filesize,
		char *p_range, size_t *content_length );
int http_range_complex( TCP_NODE *n, char *filename, size_t filesize,
		const char *mimetype,
		char *p_range, size_t *content_length, char *boundary );

void http_404( TCP_NODE *n, char *keepalive );
void http_304( TCP_NODE *n, char *keepalive );
void http_200( TCP_NODE *n, char *lastmodified,
		char *filename, size_t filesize, char *keepalive, const char *mimetype );
void http_206_simple( TCP_NODE *n, RESPONSE *r_head, RESPONSE *r_file,
		char *lastmodified, char *filename, size_t filesize,
		size_t content_length, char *keepalive, const char *mimetype );
void http_206_complex( TCP_NODE *n, RESPONSE *r_head,
		char *lastmodified, char *filename, size_t filesize,
		size_t content_length, char *boundary, char *keepalive );
void http_206_boundary_head( RESPONSE *r_head, RESPONSE *r_file,
		char *filename, size_t filesize, const char *mimetype,
		char *boundary, size_t *content_size );
void http_newline( RESPONSE *r, size_t *content_size );
void http_206_boundary_finish( RESPONSE *r, size_t *content_length,
	char *boundary );

void http_body( TCP_NODE *n, char *filename, size_t filesize );
void http_send( TCP_NODE *n );
void http_random( char *boundary, int size );

#endif /* HTTP_H */
