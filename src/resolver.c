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

#include "main.h"
#include "conf.h"
#include "log.h"
#include "resolver.h"
#include "p2p.h"
#include "send_udp.h"
#include "bucket.h"
#include "cache.h"
#include "value.h"

int r_get16bits( const UCHAR** buffer ) {
	size_t value = (*buffer)[0];
	value = value << 8;
	value += (*buffer)[1];
	(*buffer) += 2;
	return value;
}

void r_put16bits( UCHAR** buffer, unsigned int value ) {
	(*buffer)[0] = (value & 0xFF00) >> 8;
	(*buffer)[1] = value & 0xFF;
	(*buffer) += 2;
}

void r_put32bits( UCHAR** buffer, unsigned long long value ) {
	(*buffer)[0] = (value & 0xFF000000) >> 24;
	(*buffer)[1] = (value & 0xFF0000) >> 16;
	(*buffer)[2] = (value & 0xFF00) >> 16;
	(*buffer)[3] = (value & 0xFF) >> 16;
	(*buffer) += 4;
}

/* 3foo3bar3com0 => foo.bar.com */
int r_decode_domain( char *domain, const UCHAR** buffer, int size ) {
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
void r_encode_domain( UCHAR** buffer, const char *domain ) {
	char *buf = (char*) *buffer;
	const char *beg = domain;
	const char *pos;
	size_t len = 0;
	size_t i = 0;

	while( (pos = strchr(beg, '.')) != '\0' ) {
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

int r_decode_header( DNS_MSG *msg, const UCHAR** buffer, int size ) {
	unsigned int fields;

	if( size < 12 ) {
		return -1;
	}

	msg->id = r_get16bits( buffer );
	fields = r_get16bits( buffer );
	msg->qr = fields & QR_MASK;
	msg->opcode = fields & OPCODE_MASK;
	msg->aa = fields & AA_MASK;
	msg->tc = fields & TC_MASK;
	msg->rd = fields & RD_MASK;
	msg->ra = fields & RA_MASK;
	msg->rcode = fields & RCODE_MASK;

	msg->qdCount = r_get16bits( buffer );
	msg->anCount = r_get16bits( buffer );
	msg->nsCount = r_get16bits( buffer );
	msg->arCount = r_get16bits( buffer );

	return 12;
}

void r_encode_header( DNS_MSG *msg, UCHAR** buffer ) {
	r_put16bits( buffer, msg->id );

	/* Set response flag only */
	r_put16bits( buffer, (1 << 15) );

	r_put16bits( buffer, msg->qdCount );
	r_put16bits( buffer, msg->anCount );
	r_put16bits( buffer, msg->nsCount );
	r_put16bits( buffer, msg->arCount );
}

int dns_decode_query( DNS_MSG *msg, const UCHAR *buffer, int size ) {
	ssize_t n;

	if( (n = r_decode_header( msg, &buffer, size )) < 0 ) {
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
	n = r_decode_domain( msg->qName_buffer, &buffer, size );
	if( n < 0 ) {
		return -1;
	}

	size -= n;

	if( size < 4 ) {
		return -1;
	}

	int qType = r_get16bits( &buffer );
	int qClass = r_get16bits( &buffer );

	msg->question.qName = msg->qName_buffer;
	msg->question.qType = qType;
	msg->question.qClass = qClass;

	return 1;
}

void r_parse( UCHAR *buffer, size_t bufsize, IP *from ) {
	DNS_MSG msg;
	const char *hostname = NULL;

	/* UDP packet too small */
	if( bufsize < 1 ) {
		info( from, "DNS: Zero size packet from" );
		return;
	}

	/* Ignore link-local address */
	if( ip_is_linklocal( from ) ) {
		info( from, "DNS: Drop LINK-LOCAL message from" );
		return;
	}

	info( from, "DNS: Received query from" );

	if( dns_decode_query( &msg, buffer, bufsize ) < 0 ) {
		return;
	}

	hostname = msg.question.qName;

	if ( hostname == NULL ) {
		info( from, "DNS: Missing hostname" );
		return;
	}

	if( strlen( hostname ) > 255 ) {
		info( from, "DNS: Hostname too long from" );
		return;
	}

	/* Validate hostname */
	if ( !str_valid_hostname( hostname, strlen( hostname ) ) ) {
		info( from, "DNS: Invalid hostname for lookup: '%s'", hostname );
		return;
	}

#ifdef IPV6
	if( msg.question.qType != AAAA_Resource_RecordType ) {
		r_empty( from, &msg );
		return;
	}
#endif

#ifdef IPV4
	if( msg.question.qType != A_Resource_RecordType ) {
		r_empty( from, &msg );
		return;
	}
#endif

	/* Start lookup */
	r_lookup( (char *)hostname, from, &msg );
}

void r_lookup( char *hostname, IP *from, DNS_MSG *msg ) {
	UCHAR target[SHA1_SIZE];
	int result = FALSE;

	/* Compute lookup key */
	conf_hostid( target, hostname,
		_main->conf->realm, _main->conf->bool_realm );

	/* Check local cache */
	result = r_lookup_cache_db( target, from, msg );
	if( result == TRUE ) {
		info( from, "LOOKUP %s (cached)", hostname );
		return;
	}

	/* Check local database */
	result = r_lookup_local_db( target, from, msg );
	if( result == TRUE ) {
		info( from, "LOOKUP %s (local)", hostname );
		return;
	}

	/* Start remote search */
	r_lookup_remote( target, P2P_GET_PEERS, from, msg );
	info( from, "LOOKUP %s (remote)", hostname );
}

int r_lookup_cache_db( UCHAR *target, IP *from, DNS_MSG *msg ) {
	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	int nodes_compact_size = 0;

	/* Check cache for hostname */
	nodes_compact_size = cache_compact_list( nodes_compact_list, target );
	if( nodes_compact_size <= 0 ) {
		return FALSE;
	}

	r_success( from, msg, nodes_compact_list, nodes_compact_size );

	return TRUE;
}

/* Use local info_hash database for lookups too. This is nessecary if only 2
 * nodes are active: Node A announces its name to node B. But Node B cannot
 * talk to itself to lookup A. So, it must use its database directly. */
int r_lookup_local_db( UCHAR *target, IP *from, DNS_MSG *msg ) {
	UCHAR nodes_compact_list[IP_SIZE_META_PAIR8];
	int nodes_compact_size = 0;

	/* Check cache for hostname */
	nodes_compact_size = val_compact_list( nodes_compact_list, target );
	if( nodes_compact_size <= 0 ) {
		return FALSE;
	}

	r_success( from, msg, nodes_compact_list, nodes_compact_size );

	return TRUE;
}

void r_lookup_remote( UCHAR *target, int type, IP *from, DNS_MSG *msg ) {
	UCHAR nodes_compact_list[IP_SIZE_META_TRIPLE8];
	int nodes_compact_size = 0;
	LOOKUP *l = NULL;
	UCHAR *p = NULL;
	UCHAR *id = NULL;
	ITEM *ti = NULL;
	int j = 0;
	IP sin;

	/* Start the incremental remote search program */
	nodes_compact_size = bckt_compact_list( _main->nbhd->bucket,
		nodes_compact_list, target );

	/* Create tid and get the lookup table */
	ti = tdb_put( type );
	l = ldb_init( target, from, msg );
	tdb_link_ldb( ti, l );

	p = nodes_compact_list;
	for( j=0; j<nodes_compact_size; j+=IP_SIZE_META_TRIPLE ) {

		/* ID */
		id = p;	p += SHA1_SIZE;

		/* IP + Port */
		p = ip_bytes_to_sin( &sin, p );

		/* Remember queried node */
		ldb_put( l, id, &sin );

		/* Query node */
		send_get_peers_request( &sin, target, tdb_tid( ti ) );
	}
}

UCHAR *r_encode_response( DNS_MSG *msg, UCHAR *buffer ) {
	int i = 0;

	r_encode_header( msg, &buffer );

	r_encode_domain( &buffer, msg->question.qName );
	r_put16bits( &buffer, msg->question.qType );
	r_put16bits( &buffer, msg->question.qClass );

	if( msg->anCount == 0 ) {
		return buffer;
	}

	for( i=0; i<msg->anCount; i++ ) {
		r_encode_domain( &buffer, msg->answer[i].name );
		r_put16bits( &buffer, msg->answer[i].type );
		r_put16bits( &buffer, msg->answer[i].class );
		r_put32bits( &buffer, msg->answer[i].ttl );
		r_put16bits( &buffer, msg->answer[i].rd_length );

		memcpy( buffer, msg->answer[i].rd_data.x_record.addr, IP_SIZE );
		buffer += IP_SIZE;
	}

	return buffer;
}

void resolver_reply_msg( DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size ) {
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

void r_empty_msg( DNS_MSG *msg ) {
	DNS_RR *rr;
	DNS_Q *qu;

	qu = &msg->question;
	rr = msg->answer;

	msg->rcode = Ok_ResponseType;
	msg->qr = 1;
	msg->aa = 1;
	msg->ra = 0;
	msg->anCount = 0;
	msg->nsCount = 0;
	msg->arCount = 0;

	rr->name = qu->qName;
	rr->class = qu->qClass;
	rr->ttl = 0;
	rr->type = 0;
	rr->rd_length = 0;
}

void r_success( IP *from, DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size ) {
	UCHAR buffer[DNS_BUF];
	UCHAR *p = NULL;
	int buflen = 0;

	if( nodes_compact_size == 0 ) {
		return;
	}

	resolver_reply_msg( msg, nodes_compact_list, nodes_compact_size );
	p = r_encode_response( msg, buffer );

	buflen = p - buffer;
	info( from, "DNS: Packet has %d bytes.", buflen	);

	sendto( _main->dns->sockfd, buffer, buflen, 0,
		(struct sockaddr*) from, sizeof(IP) );
}

void r_empty( IP *from, DNS_MSG *msg ) {
	UCHAR buffer[DNS_BUF];
	UCHAR *p = buffer;
	int buflen = 0;

	r_empty_msg( msg );
	p = r_encode_response( msg, buffer );

	buflen = p - buffer;
	info( from, "DNS: Packet has %d bytes.", buflen	);

	sendto( _main->dns->sockfd, buffer, buflen, 0,
		(struct sockaddr*) from, sizeof(IP) );
}
