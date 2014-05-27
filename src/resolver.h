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

#ifndef RESOLVER_H
#define RESOLVER_H

#include "main.h"
#include "str.h"
#include "ip.h"
#include "conf.h"
#include "dns.h"

static const unsigned int QR_MASK = 0x8000;
static const unsigned int OPCODE_MASK = 0x7800;
static const unsigned int AA_MASK = 0x0400;
static const unsigned int TC_MASK = 0x0200;
static const unsigned int RD_MASK = 0x0100;
static const unsigned int RA_MASK = 0x8000;
static const unsigned int RCODE_MASK = 0x000F;

enum {
	Ok_ResponseType = 0,
	FormatError_ResponseType = 1,
	ServerFailure_ResponseType = 2,
	NameError_ResponseType = 3,
	NotImplemented_ResponseType = 4,
	Refused_ResponseType = 5
};

enum {
	A_Resource_RecordType = 1,
	NS_Resource_RecordType = 2,
	CNAME_Resource_RecordType = 5,
	SOA_Resource_RecordType = 6,
	PTR_Resource_RecordType = 12,
	MX_Resource_RecordType = 15,
	TXT_Resource_RecordType = 16,
	AAAA_Resource_RecordType = 28
};

enum {
	QUERY_OperationCode = 0,
	IQUERY_OperationCode = 1,
	STATUS_OperationCode = 2,
	NOTIFY_OperationCode = 4,
	UPDATE_OperationCode = 5
};

enum {
	NoError_ResponseCode = 0,
	FormatError_ResponseCode = 1,
	ServerFailure_ResponseCode = 2,
	NameError_ResponseCode = 3
};

enum {
	IXFR_QueryType = 251,
	AXFR_QueryType = 252,
	MAILB_QueryType = 253,
	MAILA_QueryType = 254,
	STAR_QueryType = 255
};

typedef struct {
	char *qName;
	unsigned int qType;
	unsigned int qClass;
} DNS_Q;

typedef union {
	struct {
		char *txt_data;
	} txt_record;
	struct {
		char *name;
	} name_server_record;
	struct {
		char *name;
	} cname_record;
	struct {
		char *name;
	} ptr_record;
	struct {
		unsigned int preference;
		char *exchange;
	} mx_record;
	struct {
		UCHAR addr[IP_SIZE];
	} x_record;
} DNS_RD;

typedef struct {
	char *name;
	unsigned int type;
	unsigned int class;
	unsigned int ttl;
	unsigned int rd_length;
	DNS_RD rd_data;
} DNS_RR;

typedef struct {
	unsigned int id;

	unsigned int qr;
	unsigned int opcode;
	unsigned int aa;
	unsigned int tc;
	unsigned int rd;
	unsigned int ra;
	unsigned int rcode;

	unsigned int qdCount;
	unsigned int anCount;
	unsigned int nsCount;
	unsigned int arCount;

	DNS_Q question;
	DNS_RR answer[8];

	char qName_buffer[256];
} DNS_MSG;

void r_parse( UCHAR *buffer, size_t bufsize, IP *from );
void r_lookup( char *hostname, IP *from, DNS_MSG *msg );
int r_lookup_cache_db( UCHAR *target, IP *from, DNS_MSG *msg );
int r_lookup_local_db( UCHAR *target, IP *from, DNS_MSG *msg );
void r_lookup_remote( UCHAR *target, int type, IP *from, DNS_MSG *msg );
void r_success( IP *from, DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size );
void r_empty( IP *from, DNS_MSG *msg );

#endif /* RESOLVER_H */
