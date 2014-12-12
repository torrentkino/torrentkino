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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>

#include "../shr/malloc.h"
#include "tumbleweed.h"
#include "../shr/thrd.h"
#include "../shr/str.h"
#include "conf.h"
#include "../shr/list.h"
#include "node_tcp.h"
#include "../shr/log.h"
#include "../shr/hash.h"
#include "../shr/file.h"
#include "send_tcp.h"
#include "http.h"

void send_tcp(TCP_NODE * n)
{
	switch (n->pipeline) {
	case NODE_SEND_INIT:
		send_cork_start(n);

	case NODE_SEND_DATA:
		send_data(n);

	case NODE_SEND_STOP:
		send_cork_stop(n);
	}
}

void send_cork_start(TCP_NODE * n)
{
	int on = 1;

	if (n->pipeline != NODE_SEND_INIT) {
		return;
	}

	if (setsockopt(n->connfd, IPPROTO_TCP, TCP_CORK, &on, sizeof(on)) != 0) {
		fail(strerror(errno));
	}

	node_status(n, NODE_SEND_DATA);
}

void send_data(TCP_NODE * n)
{
	ITEM *i = NULL;
	RESPONSE *r = NULL;

	if (n->pipeline != NODE_SEND_DATA) {
		return;
	}

	while (status == RUMBLE && list_size(n->response) > 0) {
		i = list_start(n->response);
		r = list_value(i);

		if (r->type == RESPONSE_FROM_MEMORY) {
			send_mem(n, i);
		} else {
			send_file(n, i);
		}

		if (n->pipeline == NODE_SHUTDOWN) {
			return;
		}
	}

	node_status(n, NODE_SEND_STOP);
}

void send_mem(TCP_NODE * n, ITEM * item_r)
{
	RESPONSE *r = list_value(item_r);
	ssize_t bytes_sent = 0;
	ssize_t bytes_todo = 0;
	char *p = NULL;

	while (status == RUMBLE) {
		p = r->data.memory.send_buf + r->data.memory.send_offset;
		bytes_todo =
		    r->data.memory.send_size - r->data.memory.send_offset;

		bytes_sent = send(n->connfd, p, bytes_todo, 0);

		if (bytes_sent < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return;
			}

			/* Client closed the connection, etc... */
			node_status(n, NODE_SHUTDOWN);
			return;
		}

		r->data.memory.send_offset += bytes_sent;

		/* Done */
		if (r->data.memory.send_offset >= r->data.memory.send_size) {
			resp_del(n->response, item_r);
			return;
		}
	}
}

void send_file(TCP_NODE * n, ITEM * item_r)
{
	RESPONSE *r = list_value(item_r);
	ssize_t bytes_sent = 0;
	ssize_t bytes_todo = 0;
	int fh = 0;

	while (status == RUMBLE) {
		fh = open(r->data.file.filename, O_RDONLY);
		if (fh < 0) {
			info(_log, NULL, "Failed to open %s",
			     r->data.file.filename);
			fail(strerror(errno));
		}

		bytes_todo = r->data.file.f_stop + 1 - r->data.file.f_offset;
		/* The SIGPIPE gets catched in sig.c */
		bytes_sent =
		    sendfile(n->connfd, fh, &r->data.file.f_offset, bytes_todo);

		if (close(fh) != 0) {
			info(_log, NULL, "Failed to close %s",
			     r->data.file.filename);
			fail(strerror(errno));
		}

		if (bytes_sent < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				/* connfd is blocking */
				return;
			}

			/* Client closed the connection, etc... */
			node_status(n, NODE_SHUTDOWN);
			return;
		}

		/* Done */
		if (r->data.file.f_offset > r->data.file.f_stop) {
			resp_del(n->response, item_r);
			return;
		}
	}
}

void send_cork_stop(TCP_NODE * n)
{
	int off = 0;

	if (n->pipeline != NODE_SEND_STOP) {
		return;
	}

	if (setsockopt(n->connfd, IPPROTO_TCP, TCP_CORK, &off, sizeof(off)) !=
	    0) {
		fail(strerror(errno));
	}

	/* Done, Ready or shutdown connection. */
	switch (n->keepalive) {
	case HTTP_KEEPALIVE:
		node_status(n, NODE_READY);
		break;
	default:
		node_status(n, NODE_SHUTDOWN);
	}
}
