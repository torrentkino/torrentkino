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

#ifndef MAKRO_H
#define MAKRO_H

#define BUF_SIZE 1024
#define BUF_OFF1 1023
#define DATE_SIZE 32
#define SHA1_SIZE 20

#define TIMEOUT 10

#define TRUE 1
#define FALSE 0

#define GAMEOVER 0
#define RUMBLE 1

#define CONF_PORTMIN 1
#define CONF_PORTMAX 65535

#define CONF_BEQUIET 1
#define CONF_VERBOSE 2

#define CONF_DAEMON 0
#define CONF_CONSOLE 1

#define CONF_HOSTFILE "/etc/hostname"

#define CONF_FILE "torrentkino.conf"

#define CONF_EPOLL_MAX_EVENTS 32

#define CONF_USERNAME "nobody"

#define PORT_WWW_USER 8080
#define PORT_WWW_PRIV 80
#define PORT_DHT_DEFAULT 6881

#ifdef TUMBLEWEED
#define CONF_EPOLL_WAIT 1000
#define CONF_SRVNAME "tumbleweed"
#define CONF_INDEX_NAME "index.html"
#define CONF_KEEPALIVE 5
#endif

#if TORRENTKINO
#define CONF_EPOLL_WAIT 2000
#define CONF_SRVNAME "torrentkino"
#define CONF_REALM "open.p2p"
#ifdef IPV6
#define CONF_MULTICAST "ff0e::1"
#elif IPV4
#define CONF_MULTICAST "224.0.0.252"
#endif
#endif

#define SEND_LIMIT_PER_SECOND 5

typedef unsigned long int ULONG;
typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef long int LONG;

#ifdef IPV6
#define IP_SIZE 16
#define IP_SIZE_META_PAIR 18
#define IP_SIZE_META_PAIR8 144
#define IP_SIZE_META_TRIPLE 38
#define IP_SIZE_META_TRIPLE8 304
#define IP_ADDRLEN INET6_ADDRSTRLEN
typedef struct sockaddr_in6 IP;
typedef struct in6_addr IN_ADDR;
#elif IPV4
#define IP_SIZE 4
#define IP_SIZE_META_PAIR 6
#define IP_SIZE_META_PAIR8 48
#define IP_SIZE_META_TRIPLE 26
#define IP_SIZE_META_TRIPLE8 208
#define IP_ADDRLEN INET_ADDRSTRLEN
typedef struct sockaddr_in IP;
typedef struct in_addr IN_ADDR;
#endif

extern int status;

#endif
