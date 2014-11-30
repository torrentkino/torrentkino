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
	*buffer += 2;
	return value;
}

void p_put16bits( UCHAR** buffer, USHORT value ) {
	(*buffer)[0] = (value & 0xFF00) >> 8;
	(*buffer)[1] = value & 0xFF;
	*buffer += 2;
}

void p_put32bits( UCHAR** buffer, unsigned long long value ) {
	(*buffer)[0] = (value & 0xFF000000) >> 24;
	(*buffer)[1] = (value & 0xFF0000) >> 16;
	(*buffer)[2] = (value & 0xFF00) >> 16;
	(*buffer)[3] = (value & 0xFF) >> 16;
	*buffer += 4;
}

/* 3foo3bar3com0 => foo.bar.com */
int p_decode_domain( char *domain, const UCHAR** buffer, size_t size ) {
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

	*buffer = p + 1;

	return (*buffer) - beg;
}

/* foo.bar.com => 3foo3bar3com0 */
void p_encode_domain( UCHAR** buffer, const char *domain ) {
	char *buf = (char*) *buffer;
	const char *beg = domain;
	const char *pos = NULL;
	size_t len = strlen( domain );
	size_t plen = 0;
	size_t i = 0;

	while( (pos = strchr(beg, '.')) != NULL ) {
		plen = pos - beg;
		buf[i] = plen;
		i += 1;
		memcpy( buf + i, beg, plen );
		i += plen;

		beg = pos + 1;
	}

	plen = len - (beg - domain);

	buf[i] = plen;
	i += 1;

	memcpy( buf + i, beg, plen );
	i += plen;

	buf[i] = '\0';
	i += 1;

	*buffer += i;
}

int p_decode_header( DNS_MSG *msg, const UCHAR** buffer ) {
	size_t fields;

	msg->id = p_get16bits( buffer );
	fields = p_get16bits( buffer );

	msg->qr = (fields & QR_MASK) >> 15;
	msg->opcode = (fields & OPCODE_MASK) >> 11;
	msg->aa = (fields & AA_MASK) >> 10;
	msg->tc = (fields & TC_MASK) >> 9;
	msg->rd = (fields & RD_MASK) >> 8;
	msg->ra = (fields & RA_MASK) >> 7;
	msg->rcode = (fields & RCODE_MASK) >> 0;

	msg->qdCount = p_get16bits( buffer );
	msg->anCount = p_get16bits( buffer );
	msg->nsCount = p_get16bits( buffer );
	msg->arCount = p_get16bits( buffer );

	return 12;
}

void p_encode_header( DNS_MSG *msg, UCHAR** buffer ) {
	size_t fields = 0;

	p_put16bits( buffer, msg->id );

	fields |= (msg->qr << 15) & QR_MASK;
	fields |= (msg->rcode << 0) & RCODE_MASK;
	p_put16bits( buffer, fields );

	p_put16bits( buffer, msg->qdCount );
	p_put16bits( buffer, msg->anCount );
	p_put16bits( buffer, msg->nsCount );
	p_put16bits( buffer, msg->arCount );
}

int p_decode_query( DNS_MSG *msg, const UCHAR *buffer, int size ) {
	ssize_t n;

	if( (n = p_decode_header( msg, &buffer )) < 0 ) {
		return 0;
	}
	size -= n;

	if( msg->anCount != 0 ) {
		return -1;
	}
	if( msg->nsCount != 0 ) {
		return -2;
	}
	if( msg->qdCount != 1 ) {
		return -3;
	}
#if 0
	if( msg->arCount != 0 ) {
		return -1;
	}
#endif
	/* parse questions */
	n = p_decode_domain( msg->qName_buffer, &buffer, size );
	if( n < 0 ) {
		return -4;
	}

	size -= n;

	if( size < 4 ) {
		return -5;
	}

	int qType = p_get16bits( &buffer );
	int qClass = p_get16bits( &buffer );

	msg->question.qName = msg->qName_buffer;
	msg->question.qType = qType;
	msg->question.qClass = qClass;

	return 1;
}

UCHAR *p_encode_response( DNS_MSG *msg, UCHAR *buffer ) {
	DNS_RR *rr = NULL;
	int i = 0;

	p_encode_header( msg, &buffer );

	p_encode_domain( &buffer, msg->question.qName );
	p_put16bits( &buffer, msg->question.qType );
	p_put16bits( &buffer, msg->question.qClass );

	if( msg->anCount == 0 ) {
		return buffer;
	}

	for( i=0; i<msg->anCount+msg->arCount; i++ ) {
		rr = &msg->answer[i];
		p_encode_domain( &buffer, rr->name );
		p_put16bits( &buffer, rr->type );
		p_put16bits( &buffer, rr->class );
		p_put32bits( &buffer, rr->ttl );
		p_put16bits( &buffer, rr->rd_length );

		switch( rr->type ) {
			case SRV_Resource_RecordType:
				p_put16bits( &buffer, rr->rd_data.srv_record.priority );
				p_put16bits( &buffer, rr->rd_data.srv_record.weight );
				p_put16bits( &buffer, rr->rd_data.srv_record.port );
				p_encode_domain( &buffer, rr->rd_data.srv_record.target );
				break;
			default:
				memcpy( buffer, rr->rd_data.x_record.addr, IP_SIZE );
				buffer += IP_SIZE;
		}
	}

	return buffer;
}

void p_prepare_msg( DNS_MSG *msg, USHORT anCount ) {
	msg->rcode = Ok_ResponseType;
	msg->qr = 1;
	msg->aa = 1;
	msg->ra = 0;

	msg->qdCount = 1;
	msg->anCount = anCount;
	msg->nsCount = 0;
	msg->arCount = 0;
}

void p_reply_msg( DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size ) {
	DNS_Q *qu = &msg->question;
	UCHAR *p = nodes_compact_list;
	char *domain = NULL;
	int i = 0, j = 0;

	p_prepare_msg( msg, nodes_compact_size / IP_SIZE_META_PAIR );

	switch( msg->question.qType ) {
		case SRV_Resource_RecordType:
			msg->arCount = msg->anCount;
			domain = p_get_domain_from_srv_record( qu->qName );

			p = nodes_compact_list;
			for( i = 0; i < msg->anCount; i++ ) {
				p_put_srv( &msg->answer[i], qu, p, domain );
				p += IP_SIZE_META_PAIR;
			}

			p = nodes_compact_list;
			for( j = i; j < msg->anCount + msg->arCount; j++ ) {
				p_put_addr( &msg->answer[j], qu, p, domain );
				p += IP_SIZE_META_PAIR;
			}
			break;
		default:
			for( i = 0; i < msg->anCount; i++ ) {
				p_put_addr( &msg->answer[i], qu, p, qu->qName );
				p += IP_SIZE_META_PAIR;
			}
	}
}

char *p_get_domain_from_srv_record( char *name ) {
	char *domain = NULL;

	/* _http._tcp.owncloud.p2p -> owncloud.p2p */
	if( ( domain = strstr( name, "._tcp." ) ) != NULL ) {
		domain += 6;
		return domain;
	} else if( ( domain = strstr( name, "._udp." ) ) != NULL ) {
		domain += 6;
		return domain;
	} /* FIXME */

	/* owncloud.p2p -> owncloud.p2p */
	return name;
}

void p_put_srv( DNS_RR *rr, DNS_Q *qu, UCHAR *p, char *name ) {
	const UCHAR *p_port = p+IP_SIZE;

	rr->name = qu->qName;
	rr->type = SRV_Resource_RecordType;
	rr->class = 1;
	rr->ttl = DNS_TTL;
	rr->rd_length = 6 + strlen( name ) + 2;

	rr->rd_data.srv_record.priority = 0;
	rr->rd_data.srv_record.weight = 0;
	rr->rd_data.srv_record.port = p_get16bits( &p_port );
	rr->rd_data.srv_record.target = name;
}

void p_put_addr( DNS_RR *rr, DNS_Q *qu, UCHAR *p, char *name ) {
	rr->name = name;
	rr->class = qu->qClass;
	rr->ttl = DNS_TTL;
	rr->rd_length = IP_SIZE;

#ifdef IPV6
	rr->type = AAAA_Resource_RecordType;
#elif IPV4
	rr->type = A_Resource_RecordType;
#endif

	memcpy( rr->rd_data.x_record.addr, p, IP_SIZE );
}

void p_reset_msg( DNS_MSG *msg ) {
	DNS_RR *rr = msg->answer;
	DNS_Q *qu = &msg->question;

	p_prepare_msg( msg, 0 );

	rr = msg->answer;
	rr->name = qu->qName;
	rr->class = qu->qClass;
	rr->ttl = 0;
	rr->type = 0;
	rr->rd_length = 0;
}
