/*
Copyright 2011 Aiko Barz

This file is part of Torrentkino.

Torrentkino is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Torrentkino is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Torrentkino.  If not, see <http://www.gnu.org/licenses/>.
*/

struct obj_stnode {
	UCHAR node_id[SHA_DIGEST_LENGTH];
	IP c_addr;
	time_t time_anno;
};

struct obj_store {
	struct obj_list *list;
};

struct obj_store *stor_init(void);
void stor_free(void);

void stor_put(UCHAR *node_id, IP *sa);
void stor_del(struct obj_item *item_st);

void stor_expire(void);

struct obj_item *stor_find_node(UCHAR *node_id);
IP *stor_address(UCHAR *node_id);

void stor_send(IP *from, UCHAR *node_id, UCHAR *lkp_id, UCHAR *key_id);

