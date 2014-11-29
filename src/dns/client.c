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
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "client.h"

int _nss_dns_cli( const char *hostname, int hostsize, UCHAR *address,
	int address_size ) {
#ifdef IPV6
	return ngethostbyname( hostname, hostsize, address, address_size,
		AAAA_Resource_RecordType );
#else
	return ngethostbyname( hostname, hostsize, address, address_size,
		A_Resource_RecordType );
#endif
}

int ngethostbyname( const char *host, int hostsize, UCHAR *address,
	int address_size, int query_type ) {
	UCHAR buf[65536], *qname, *reader;
	int i , stop , s;
	struct timeval tv;
	UCHAR *p = address;

	IP dest;

	struct RES_RECORD answers[ DNS_ANSWERS_MAX ];

	struct DNS_HEADER *dns = NULL;
	struct QUESTION *qinfo = NULL;

	s = socket( IP_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( s < 0 ) {
		return -1;
	}

	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
			sizeof( struct timeval ) );

#ifdef IPV6
	dest.sin6_family = IP_INET;
	dest.sin6_port = htons( PORT_DNS_DEFAULT );
	inet_pton(IP_INET, DNS_SERVER, &(dest.sin6_addr));
#else
	dest.sin_family = IP_INET;
	dest.sin_port = htons( PORT_DNS_DEFAULT );
	dest.sin_addr.s_addr = inet_addr( DNS_SERVER );
#endif

	dns = (struct DNS_HEADER *)&buf;
	dns->id = (USHORT) htons(getpid());
	dns->qr = 0; /* This is a query */
	dns->opcode = 0; /* This is a standard query */
	dns->aa = 0; /* Not Authoritative */
	dns->tc = 0; /* This message is not truncated */
	dns->rd = 1; /* Recursion Desired */
	dns->ra = 0; /* Recursion not available! */
	dns->z = 0;
	dns->ad = 0;
	dns->cd = 0;
	dns->rcode = 0;
	dns->q_count = htons(1); /* we have only 1 question */
	dns->ans_count = 0;
	dns->auth_count = 0;
	dns->add_count = 0;

	qname =(UCHAR*)&buf[sizeof(struct DNS_HEADER)];

	ChangetoDnsNameFormat(qname , host);
	qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)];

	qinfo->qtype = htons( query_type ); /* type of the query , A , MX , CNAME , NS etc */
	qinfo->qclass = htons(1); /* internet */

	if( sendto( s, (char*)buf, 
			sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION),
			0, (struct sockaddr*)&dest, sizeof(dest) ) < 0 ) {
		return -1;
	}

	i = sizeof dest;
	if(recvfrom (s,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest , (socklen_t*)&i ) < 0) {
		return -1;
	}

	dns = (struct DNS_HEADER*) buf;

	/* Questions */
	if( ntohs( dns->q_count ) != 1 ) {
		return -1;
	}

	/* Answers */
	if( ntohs( dns->ans_count ) < 1 || ntohs( dns->ans_count ) > 8 ) {
		return -1;
	}

	/* Authoritative servers */
	if( ntohs( dns->auth_count ) != 0 ) {
		return -1;
	}

	/* Additional records */
	if( ntohs( dns->add_count ) != 0 ) {
		return -1;
	}

	/* move ahead of the dns header and the query field */
	reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

	/* Start reading answers */
	stop=0;

	for( i=0; i<ntohs(dns->ans_count) && i<DNS_ANSWERS_MAX; i++ ) {
		int ip_size = 0;
		answers[i].name=ReadName(reader,buf,&stop);
		reader = reader + stop;

		answers[i].resource = (struct R_DATA*)(reader);
		reader = reader + sizeof(struct R_DATA);

		ip_size = ntohs( answers[i].resource->data_len );
		if( ip_size != IP_SIZE ) {
			return -1;
		}

#ifdef IPV6
		if( ntohs( answers[i].resource->type ) == AAAA_Resource_RecordType )
#else
		if( ntohs( answers[i].resource->type ) == A_Resource_RecordType )
#endif
		{
			memcpy( p, reader, IP_SIZE );
			p += IP_SIZE;

#if 0
			memcpy( answers[i].rdata, reader, IP_SIZE );
#endif
			reader = reader + ip_size;
		}

		myfree( answers[i].name );
	}

#if 0
	printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
	for(i=0 ; i < ntohs(dns->ans_count) ; i++)
	{
		printf("Name : %s ",answers[i].name);

#ifdef IPV6
		if( ntohs( answers[i].resource->type ) == AAAA_Resource_RecordType )
#else
		if( ntohs( answers[i].resource->type ) == A_Resource_RecordType )
#endif
		{
			UCHAR *p1 = NULL;
			p1 = answers[i].rdata;
			ip_bytes_to_sin( &a, p1 );
			ip_sin_to_string( &a, ip_buf );
			printf("has IP address : %s", ip_buf);
		}

		printf("\n");
	}
#endif

	return 1;
}

u_char* ReadName(UCHAR* reader,UCHAR* buffer,int* count)
{
	UCHAR *name;
	unsigned int p=0,jumped=0,offset;
	int i , j;

	*count = 1;
	name = (UCHAR*) myalloc( 256 * sizeof( UCHAR ) );

	name[0]='\0';

	/* read the names in 3www6google3com format */
	while(*reader!=0)
	{
		if(*reader>=192)
		{
			offset = (*reader)*256 + *(reader+1) - 49152; /* 49152 = 11000000 00000000 */
			reader = buffer + offset - 1;
			jumped = 1; /* we have jumped to another location so counting wont go up! */
		}
		else
		{
			name[p++]=*reader;
		}

		reader = reader+1;

		if(jumped==0)
		{
			*count = *count + 1; /* if we havent jumped to another location then we can count up */
		}
	}

	name[p]='\0'; /* string complete */
	if( jumped == 1 ) {
		*count = *count + 1; /* number of steps we actually moved forward in the packet */
	}

	/* now convert 3www6google3com0 to www.google.com */
	for(i=0;i<(int)strlen((const char*)name);i++) 
	{
		p=name[i];
		for(j=0;j<(int)p;j++) 
		{
			name[i]=name[i+1];
			i=i+1;
		}
		name[i]='.';
	}
	name[i-1]='\0'; /* remove the last dot */
	return name;
}

/*
 * This will convert www.google.com to 3www6google3com
 * got it
 * */
void
ChangetoDnsNameFormat( UCHAR *dns, const char* host )
{
	int lock = 0 , i;
	strcat((char*)host,".");

	for(i = 0 ; i < strlen((char*)host) ; i++)
	{
		if(host[i]=='.')
		{
			*dns++ = i-lock;
			for(;lock<i;lock++)
			{
				*dns++=host[lock];
			}
			lock++; /* or lock=i+1; */
		}
	}
	*dns++='\0';
}
