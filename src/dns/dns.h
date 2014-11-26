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

#ifndef PACKER_H
#define PACKER_H

#include "../shr/config.h"

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
	AAAA_Resource_RecordType = 28,
	SRV_Resource_RecordType = 33
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
	struct {
		USHORT priority;
		USHORT weight;
		USHORT port;
		const char *target;
	} srv_record;
} DNS_RD;

typedef struct {
	char *name;
	USHORT type;
	USHORT class;
	USHORT ttl;
	USHORT rd_length;
	DNS_RD rd_data;
} DNS_RR;

typedef struct {
	USHORT id;

	USHORT qr;
	USHORT opcode;
	USHORT aa;
	USHORT tc;
	USHORT rd;
	USHORT ra;
	USHORT rcode;

	USHORT qdCount;
	USHORT anCount;
	USHORT nsCount;
	USHORT arCount;

	DNS_Q question;
	DNS_RR answer[16];

	char qName_buffer[256];
} DNS_MSG;

int p_get16bits( const UCHAR** buffer );
void p_put16bits( UCHAR** buffer, USHORT value );
void p_put32bits( UCHAR** buffer, unsigned long long value );

int p_decode_domain( char *domain, const UCHAR** buffer, size_t size );
void p_encode_domain( UCHAR** buffer, const char *domain );

int p_decode_header( DNS_MSG *msg, const UCHAR** buffer );
void p_encode_header( DNS_MSG *msg, UCHAR** buffer );

int p_decode_query( DNS_MSG *msg, const UCHAR *buffer, int size );
UCHAR *p_encode_response( DNS_MSG *msg, UCHAR *buffer );

void p_prepare_msg( DNS_MSG *msg, USHORT arCount );
void p_reply_msg( DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size );
void p_reset_msg( DNS_MSG *msg );

void p_put_srv( DNS_RR *rr, DNS_Q *qu, UCHAR *p, char *name );
void p_put_addr( DNS_RR *rr, DNS_Q *qu, UCHAR *p, char *name );

char *p_get_domain_from_srv_record( char *name );

#endif /* PACKER_H */
