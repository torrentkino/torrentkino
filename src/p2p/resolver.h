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

#include "../shr/config.h"
#include "../shr/str.h"
#include "../shr/ip.h"
#include "../p2p/conf.h"
#include "../p2p/bucket.h"
#include "../p2p/transaction.h"
#include "../p2p/neighbourhood.h"
#include "../dns/dns.h"
#include "../p2p/lookup.h"

void r_parse( UCHAR *buffer, size_t bufsize, IP *from );
void r_lookup( char *hostname, IP *from, DNS_MSG *msg );
int r_lookup_cache_db( UCHAR *target, IP *from, DNS_MSG *msg );
int r_lookup_local_db( UCHAR *target, IP *from, DNS_MSG *msg );
void r_lookup_remote( UCHAR *target, int type, IP *from, DNS_MSG *msg );

void r_success( IP *from, DNS_MSG *msg, UCHAR *nodes_compact_list, int nodes_compact_size );
void r_failure( IP *from, DNS_MSG *msg );

#endif /* RESOLVER_H */
