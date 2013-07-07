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
	HASH *hash;
};

struct obj_database_id {
	UCHAR target[SHA_DIGEST_LENGTH];
	LIST *list;
	HASH *hash;
};
typedef struct obj_database_id DB_ID;

struct obj_database_node {
	UCHAR id[SHA_DIGEST_LENGTH];
	IP c_addr;
	time_t time_anno;
};
typedef struct obj_database_node DB_NODE;

struct obj_database *db_init(void);
void db_free(void);

void db_put(UCHAR *target, int port, UCHAR *node_id, IP *sa);
void db_del_id(ITEM *i_id);
void db_del_node(DB_ID *db_id, ITEM *i_node);

void db_clean(void);
void db_expire(void);

void db_update(DB_NODE *db, IP *sa, int port);
ITEM *db_find_id(UCHAR *target);
ITEM *db_find_node(HASH *hash, UCHAR *node_id);
IP *db_address(UCHAR *target);
