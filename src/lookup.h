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

struct obj_lookup {

	/* What are we looking for */
    UCHAR target[SHA_DIGEST_LENGTH];

	/* Organize replies like our own neighbourhood */
	NBHD *nbhd;

	/* Caller */
	IP c_addr;
};
typedef struct obj_lookup LOOKUP;

LOOKUP *ldb_init( UCHAR *target, IP *from );
void ldb_free( LOOKUP *ldb );

int ldb_contacted_node(LOOKUP *ldb, UCHAR *node_id);
void ldb_update_token( LOOKUP *ldb, UCHAR *node_id, struct obj_ben *token, IP *from );
