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

void _nss_dns_cli( const char *hostname ) {
#ifdef IPV6
	ngethostbyname( hostname , AAAA_Resource_RecordType );
#else
	ngethostbyname( hostname , A_Resource_RecordType );
#endif
}

/*
 * Perform a DNS query by sending a packet
 * */
void ngethostbyname( const char *host , int query_type ) {
	UCHAR buf[65536],*qname,*reader;
	int i , j , stop , s;
	struct timeval tv;

	IP a;

	struct RES_RECORD answers[20],auth[20],addit[20];
	IP dest;

	struct DNS_HEADER *dns = NULL;
	struct QUESTION *qinfo = NULL;

	printf("Resolving %s" , host);


	s = socket(IP_INET , SOCK_DGRAM , IPPROTO_UDP);
	if( s < 0 ) {
		return;
	}

	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,
			sizeof( struct timeval ) );

#ifdef IPV6
	dest.sin6_family = IP_INET;
	dest.sin6_port = htons( PORT_DNS_DEFAULT );
	inet_pton(IP_INET, DNS_SERVER, &(dest.sin6_addr));
#elif IPV4
	dest.sin_family = IP_INET;
	dest.sin_port = htons( PORT_DNS_DEFAULT );
	dest.sin_addr.s_addr = inet_addr( DNS_SERVER );
#endif

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf;

	dns->id = (unsigned short) htons(getpid());
	dns->qr = 0; //This is a query
	dns->opcode = 0; //This is a standard query
	dns->aa = 0; //Not Authoritative
	dns->tc = 0; //This message is not truncated
	dns->rd = 1; //Recursion Desired
	dns->ra = 0; //Recursion not available! hey we dont have it (lol)
	dns->z = 0;
	dns->ad = 0;
	dns->cd = 0;
	dns->rcode = 0;
	dns->q_count = htons(1); //we have only 1 question
	dns->ans_count = 0;
	dns->auth_count = 0;
	dns->add_count = 0;

	//point to the query portion
	qname =(UCHAR*)&buf[sizeof(struct DNS_HEADER)];

	ChangetoDnsNameFormat(qname , host);
	qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

	qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
	qinfo->qclass = htons(1); //its internet (lol)

	printf("\nSending Packet...");
	if( sendto(s,(char*)buf,sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION),0,(struct sockaddr*)&dest,sizeof(dest)) < 0)
	{
		perror("sendto failed");
	}
	printf("Done");

	//Receive the answer
	i = sizeof dest;
	printf("\nReceiving answer...");
	if(recvfrom (s,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest , (socklen_t*)&i ) < 0)
	{
		perror("recvfrom failed");
	}
	printf("Done");

	dns = (struct DNS_HEADER*) buf;

	//move ahead of the dns header and the query field
	reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

	printf("\nThe response contains : ");
	printf("\n %d Questions.",ntohs(dns->q_count));
	printf("\n %d Answers.",ntohs(dns->ans_count));
	printf("\n %d Authoritative Servers.",ntohs(dns->auth_count));
	printf("\n %d Additional records.\n\n",ntohs(dns->add_count));

	//Start reading answers
	stop=0;

	for(i=0;i<ntohs(dns->ans_count);i++)
	{
		answers[i].name=ReadName(reader,buf,&stop);
		reader = reader + stop;

		answers[i].resource = (struct R_DATA*)(reader);
		reader = reader + sizeof(struct R_DATA);

		if( ntohs( answers[i].resource->type ) == A_Resource_RecordType ||
			ntohs( answers[i].resource->type ) == AAAA_Resource_RecordType )
		{
			answers[i].rdata = (UCHAR*)malloc(ntohs(answers[i].resource->data_len)+1);

printf("Size: %i\n", ntohs(answers[i].resource->data_len));
			for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
			{
				answers[i].rdata[j]=reader[j];
			}

			answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

			reader = reader + ntohs(answers[i].resource->data_len);
		}
		else
		{
printf("hm?\n");
			answers[i].rdata = ReadName(reader,buf,&stop);
			reader = reader + stop;
		}
	}

	//read authorities
//	for(i=0;i<ntohs(dns->auth_count);i++) {
//		auth[i].name=ReadName(reader,buf,&stop);
//		reader+=stop;
//
//		auth[i].resource=(struct R_DATA*)(reader);
//		reader+=sizeof(struct R_DATA);
//
//		auth[i].rdata=ReadName(reader,buf,&stop);
//		reader+=stop;
//	}

	//read additional
//	for(i=0;i<ntohs(dns->add_count);i++) {
//		addit[i].name=ReadName(reader,buf,&stop);
//		reader+=stop;
//
//		addit[i].resource=(struct R_DATA*)(reader);
//		reader+=sizeof(struct R_DATA);
//
//		if(ntohs(addit[i].resource->type)==1)
//		{
//			addit[i].rdata = (UCHAR*)malloc(ntohs(addit[i].resource->data_len));
//			for(j=0;j<ntohs(addit[i].resource->data_len);j++)
//			addit[i].rdata[j]=reader[j];
//
//			addit[i].rdata[ntohs(addit[i].resource->data_len)]='\0';
//			reader+=ntohs(addit[i].resource->data_len);
//		}
//		else
//		{
//			addit[i].rdata=ReadName(reader,buf,&stop);
//			reader+=stop;
//		}
//	}

	//print answers
	printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
	for(i=0 ; i < ntohs(dns->ans_count) ; i++)
	{
		printf("Name : %s ",answers[i].name);

		if( ntohs( answers[i].resource->type ) == A_Resource_RecordType ||
			ntohs( answers[i].resource->type ) == AAAA_Resource_RecordType )
		{
			char ip_buf[IP_ADDRLEN+1];
			UCHAR *p = NULL;
			p=answers[i].rdata;
	printf("help\n");
			ip_bytes_to_sin( &a, p );
	printf("help\n");
			ip_sin_to_string( &a, ip_buf );
	printf("help\n");
			printf("has IP address : %s", ip_buf);
	printf("help\n");
		}

//		if(ntohs(answers[i].resource->type)==5)
//		{
//			//Canonical name for an alias
//			printf("has alias name : %s",answers[i].rdata);
//		}

		printf("\n");
	}

	//print authorities
//	printf("\nAuthoritive Records : %d \n" , ntohs(dns->auth_count) );
//	for( i=0 ; i < ntohs(dns->auth_count) ; i++) {
//		printf("Name : %s ",auth[i].name);
//		if(ntohs(auth[i].resource->type)==2)
//		{
//			printf("has nameserver : %s",auth[i].rdata);
//		}
//		printf("\n");
//	}

	//print additional resource records
//	printf("\nAdditional Records : %d \n" , ntohs(dns->add_count) );
//	for(i=0; i < ntohs(dns->add_count) ; i++)
//	{
//		printf("Name : %s ",addit[i].name);
//		if(ntohs(addit[i].resource->type)==1)
//		{
//			char ip_buf[IP_ADDRLEN+1];
//			UCHAR *p;
//			p=addit[i].rdata;
//			ip_bytes_to_sin( &a, p );
//			ip_sin_to_string( &a, ip_buf );
//			printf("has IP address : %s", ip_buf);
//		}
//		printf("\n");
//	}
	return;
}

/*
 * 
 * */
u_char* ReadName(UCHAR* reader,UCHAR* buffer,int* count)
{
	UCHAR *name;
	unsigned int p=0,jumped=0,offset;
	int i , j;

	*count = 1;
	name = (UCHAR*)malloc(256);

	name[0]='\0';

	//read the names in 3www6google3com format
	while(*reader!=0)
	{
		if(*reader>=192)
		{
			offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
			reader = buffer + offset - 1;
			jumped = 1; //we have jumped to another location so counting wont go up!
		}
		else
		{
			name[p++]=*reader;
		}

		reader = reader+1;

		if(jumped==0)
		{
			*count = *count + 1; //if we havent jumped to another location then we can count up
		}
	}

	name[p]='\0'; //string complete
	if(jumped==1)
	{
		*count = *count + 1; //number of steps we actually moved forward in the packet
	}

	//now convert 3www6google3com0 to www.google.com
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
	name[i-1]='\0'; //remove the last dot
	return name;
}

/*
 * This will convert www.google.com to 3www6google3com
 * got it :)
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
			lock++; //or lock=i+1;
		}
	}
	*dns++='\0';
}
