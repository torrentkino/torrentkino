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

#define HTTP_UNKNOWN 0
#define HTTP_GET 1
#define HTTP_HEAD 2

#define HTTP_1_0 10
#define HTTP_1_1 11

long int http_urlDecode( char *src, long int srclen, char *dst, long int dstlen );
HASH *http_hashHeader( char *head );
void http_deleteHeader( HASH *head );

void http_buf( TCP_NODE *n );
void http_read( TCP_NODE *n, char *p_cmd, char *p_url, char *p_proto, HASH *p_head );

int http_check( TCP_NODE *n, char *p_cmd, char *p_url, char *p_proto );
void http_code( TCP_NODE *n );
void http_keepalive( TCP_NODE *n, HASH *head );
void http_lastmodified( TCP_NODE *n, HASH *head );
void http_size( TCP_NODE *n, HASH *head );
void http_gate( TCP_NODE *n, HASH *head );
void http_send( TCP_NODE *n );

void http_404( TCP_NODE *n );
void http_304( TCP_NODE *n );
void http_200( TCP_NODE *n );
#ifdef RANGE
void http_206( TCP_NODE *n );
void http_416( TCP_NODE *n );
#endif
