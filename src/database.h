/*
Copyright 2011 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

struct obj_database {
	LIST *list;
};

struct obj_database_node {
	UCHAR host_id[SHA_DIGEST_LENGTH];
	IP c_addr;
	time_t time_anno;
};
typedef struct obj_database_node DB;

struct obj_database *db_init(void);
void db_free(void);

void db_put(UCHAR *host_id, IP *sa);
void db_del(ITEM *item_st);

void db_expire(void);

ITEM *db_find_node(UCHAR *host_id);
IP *db_address(UCHAR *host_id);

void db_send(IP *from, UCHAR *host_id, UCHAR *lkp_id, UCHAR *key_id);
