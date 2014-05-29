/*
Copyright 2014 Aiko Barz

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
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include "dns.h"

int p_get16bits( const UCHAR** buffer ) {
	size_t value = (*buffer)[0];
	value = value << 8;
	value += (*buffer)[1];
	(*buffer) += 2;
	return value;
}

void p_put16bits( UCHAR** buffer, unsigned int value ) {
	(*buffer)[0] = (value & 0xFF00) >> 8;
	(*buffer)[1] = value & 0xFF;
	(*buffer) += 2;
}

void p_put32bits( UCHAR** buffer, unsigned long long value ) {
	(*buffer)[0] = (value & 0xFF000000) >> 24;
	(*buffer)[1] = (value & 0xFF0000) >> 16;
	(*buffer)[2] = (value & 0xFF00) >> 16;
	(*buffer)[3] = (value & 0xFF) >> 16;
	(*buffer) += 4;
}

/* 3foo3bar3com0 => foo.bar.com */
int p_decode_domain( char *domain, const UCHAR** buffer, int size ) {
	const UCHAR *p = *buffer;
	const UCHAR *beg = p;
	size_t i = 0;
	size_t len = 0;

	while( *p != '\0' ) {

		if( i != 0 ) {
			domain[i] = '.';
			i += 1;
		}

		len = *p;
		p += 1;

		if( i+len >=  256 || i+len >= size ) {
			return -1;
		}

		memcpy( domain+i, p, len );
		p += len;
		i += len;
	}

	domain[i] = '\0';

	/* also jump over the last 0 */
	*buffer = p + 1;

	return (*buffer) - beg;
}

/* foo.bar.com => 3foo3bar3com0 */
void p_encode_domain( UCHAR** buffer, const char *domain ) {
	char *buf = (char*) *buffer;
	const char *beg = domain;
	const char *pos;
	size_t len = 0;
	size_t i = 0;

	while( (pos = strchr(beg, '.')) != NULL ) {
		len = pos - beg;
		buf[i] = len;
		i += 1;
		memcpy( buf+i, beg, len );
		i += len;

		beg = pos + 1;
	}

	len = strlen( domain ) - (beg - domain);

	buf[i] = len;
	i += 1;

	memcpy( buf + i, beg, len );
	i += len;

	buf[i] = 0;
	i += 1;

	*buffer += i;
}

int p_decode_header( DNS_MSG *msg, const UCHAR** buffer, int size ) {
	unsigned int fields;

	if( size < 12 ) {
		return -1;
	}

	msg->id = p_get16bits( buffer );
	fields = p_get16bits( buffer );
	msg->qr = fields & QR_MASK;
	msg->opcode = fields & OPCODE_MASK;
	msg->aa = fields & AA_MASK;
	msg->tc = fields & TC_MASK;
	msg->rd = fields & RD_MASK;
	msg->ra = fields & RA_MASK;
	msg->rcode = fields & RCODE_MASK;

	msg->qdCount = p_get16bits( buffer );
	msg->anCount = p_get16bits( buffer );
	msg->nsCount = p_get16bits( buffer );
	msg->arCount = p_get16bits( buffer );

	return 12;
}

void p_encode_header( DNS_MSG *msg, UCHAR** buffer ) {
	p_put16bits( buffer, msg->id );

	/* Set response flag only */
	p_put16bits( buffer, (1 << 15) );

	p_put16bits( buffer, msg->qdCount );
	p_put16bits( buffer, msg->anCount );
	p_put16bits( buffer, msg->nsCount );
	p_put16bits( buffer, msg->arCount );
}

int p_decode_query( DNS_MSG *msg, const UCHAR *buffer, int size ) {
	ssize_t n;

	if( (n = p_decode_header( msg, &buffer, size )) < 0 ) {
		return -1;
	}
	size -= n;

	if( msg->anCount != 0 ) {
		info( NULL, "DNS: Only questions expected." );
		return -1;
	}
	if( msg->nsCount != 0 ) {
		info( NULL, "DNS: Only questions expected." );
		return -1;
	}
	if( msg->qdCount != 1 ) {
		info( NULL, "DNS: Only only 1 question expected." );
		return -1;
	}
#if 0
	if( msg->arCount != 0 ) {
		info( NULL, "DNS: Only questions expected." );
		return -1;
	}
#endif
	/* parse questions */
	n = p_decode_domain( msg->qName_buffer, &buffer, size );
	if( n < 0 ) {
		return -1;
	}

	size -= n;

	if( size < 4 ) {
		return -1;
	}

	int qType = p_get16bits( &buffer );
	int qClass = p_get16bits( &buffer );

	msg->question.qName = msg->qName_buffer;
	msg->question.qType = qType;
	msg->question.qClass = qClass;

	return 1;
}

UCHAR *p_encode_response( DNS_MSG *msg, UCHAR *buffer ) {
	int i = 0;

	p_encode_header( msg, &buffer );

	p_encode_domain( &buffer, msg->question.qName );
	p_put16bits( &buffer, msg->question.qType );
	p_put16bits( &buffer, msg->question.qClass );

	if( msg->anCount == 0 ) {
		return buffer;
	}

	for( i=0; i<msg->anCount; i++ ) {
		p_encode_domain( &buffer, msg->answer[i].name );
		p_put16bits( &buffer, msg->answer[i].type );
		p_put16bits( &buffer, msg->answer[i].class );
		p_put32bits( &buffer, msg->answer[i].ttl );
		p_put16bits( &buffer, msg->answer[i].rd_length );

		memcpy( buffer, msg->answer[i].rd_data.x_record.addr, IP_SIZE );
		buffer += IP_SIZE;
	}

	return buffer;
}

void p_reply_msg( DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size ) {
	DNS_RR *rr;
	DNS_Q *qu;
	UCHAR *p = NULL;
	int i = 0;

	qu = &msg->question;
	rr = &msg->answer[0];

	msg->rcode = Ok_ResponseType;
	msg->qr = 1;
	msg->aa = 1;
	msg->ra = 0;
	msg->anCount = nodes_compact_size / IP_SIZE_META_PAIR;
	msg->nsCount = 0;
	msg->arCount = 0;

	p = nodes_compact_list;
	for( i = 0; i < msg->anCount; i++ ) {

		rr[i].name = qu->qName;
		rr[i].class = qu->qClass;
		rr[i].ttl = 0;
		rr[i].rd_length = IP_SIZE;

#ifdef IPV4
		rr[i].type = A_Resource_RecordType;
#endif

#ifdef IPV6
		rr[i].type = AAAA_Resource_RecordType;
#endif

		memcpy( rr[i].rd_data.x_record.addr, p, IP_SIZE );
		p += IP_SIZE_META_PAIR;
	}
}
