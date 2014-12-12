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
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#include "identity.h"

LIST *id_init(void)
{
	return list_init();
}

void id_free(LIST * l)
{
	list_clear(l);
	list_free(l);
}

void id_put(char *hostname, UCHAR * node_id, char *realm, int bool_realm)
{
	ID *h = myalloc(sizeof(ID));

	if (!str_valid_hostname(hostname, strlen(hostname))) {
		fail("Bad hostname");
	}

	snprintf(h->hostname, BUF_SIZE, "%s", hostname);
	id_hostid(h->host_id, hostname, realm, bool_realm);
	h->time_announce_host = 0;

	/* I don't want to be responsible for myself */
	if (memcmp(h->host_id, node_id, SHA1_SIZE) == 0) {
		fail("The host id and the node id must not be the same");
	}

	list_put(_main->identity, h);
}

void id_hostid(UCHAR * host_id, char *hostname, char *realm, int bool)
{
	UCHAR sha1_buf1[SHA1_SIZE];
	UCHAR sha1_buf2[SHA1_SIZE];
	int j = 0;

	/* The realm influences the way, the lookup hash gets computed */
	if (bool == TRUE) {
		sha1_hash(sha1_buf1, hostname, strlen(hostname));
		sha1_hash(sha1_buf2, realm, strlen(realm));

		for (j = 0; j < SHA1_SIZE; j++) {
			host_id[j] = sha1_buf1[j] ^ sha1_buf2[j];
		}
	} else {
		sha1_hash(host_id, hostname, strlen(hostname));
	}
}

void id_print(void)
{
	ITEM *i = list_start(_main->identity);
	char hex[HEX_LEN];
	ID *identity = NULL;

	while (i != NULL) {
		identity = list_value(i);

		hex_hash_encode(hex, identity->host_id);
		info(_log, NULL, "Hostname: %s / %s", hex, identity->hostname);

		i = list_next(i);
	}
}
